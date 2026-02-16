/*
 * Copyright (c) 2022 Acme CPU
 *
 * host_cmds_req.c
 * Created on: 02-Mar-2023
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <inttypes.h>
#include <stdlib.h>            /* calloc, free */
#include <stdio.h>             /* sprintf */
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_mac.h"

#include "host_cmds_uart_config.h"
#include "host_cmds.h"
#include "host_cmds_priv.h"
#include "host_cmds_send_recv.h"
#include "host_cmds_callback.h"
#include "c20x_m2m_cmds.h"

#include "app_http_server.h"
//#include "m2m_frame.h"
#include "lib_m2m_frame.h"
#include "comm_wifi.h"
#include "app_net_file_download.h"

#define TAG "host_cmds_req"

#define CMD_REQ_Q_LEN 10

static TaskHandle_t m_th_req_svc = NULL;
static SemaphoreHandle_t m_mutex; /* mutex for transmission of data */
static QueueHandle_t m_cmd_req_q;

struct cmd_req_data {
    uint32_t cmd;
    struct m2m_frame_t *frame;
};

/* wifi cmd callbacks */
static struct host_cmd_callback cb_wifi_onoff;
static struct host_cmd_callback cb_wifi_connect;
static struct host_cmd_callback cb_hotspot;
static struct host_cmd_callback cb_wifi_scan;
static struct host_cmd_callback cb_wifi_mac;

/* settings file download callback */
static struct host_cmd_callback cb_filednl_url_set;
static struct host_cmd_callback cb_filednl_cert_set;
static struct host_cmd_callback cb_filednl;
static struct host_cmd_callback cb_data_ack;

/* firmware update callback */
static struct host_cmd_callback cb_fw_avail;
static struct host_cmd_callback cb_fw_update;

static void cb_cmd_svc_handler(struct host_cmd_callback *cb, uint32_t cmd, void *frame)
{
    ESP_LOGD(TAG, "cb_cmd_svc_handler");
    struct cmd_req_data *pcrd = (struct cmd_req_data *)calloc(1, sizeof(struct cmd_req_data));
    if (pcrd == NULL) {
        ESP_LOGE(TAG, "calloc failed at %s", __func__);
        return;
    }

    pcrd->cmd = cmd;
    pcrd->frame = frame;

    /* send pointer to queue; wait up to 10 ms */
    if (xQueueSend(m_cmd_req_q, (void *)&pcrd, pdMS_TO_TICKS(10)) != pdPASS) {
        ESP_LOGE(TAG, "xQueueSend failed for %lu cmd", (unsigned long)cmd);
        free(pcrd); /* avoid leak if queue full */
        return;
    }
}

static int wifi_ssid_arr_make(int idx, wifi_ap_record_t *ap_info, uint16_t ap_count, uint8_t *out_arr)
{
    for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++) {
        int len = strlen((char *)ap_info[i].ssid);
        if (len < 10)
            idx += sprintf((char *)(out_arr + idx), "0%d%s", len, ap_info[i].ssid);
        else
            idx += sprintf((char *)(out_arr + idx), "%d%s", len, ap_info[i].ssid);
    }

    return idx;
}

static void out_frame_ok_err_set(struct m2m_frame_t *out_frame, int cmd, const char *msg)
{
    sprintf((char *)out_frame->payload, "%d%s%s%s", cmd, M2M_CMD_PAYLOAD_DELIM, msg, M2M_CMD_PAYLOAD_TERM);
    out_frame->payload_len = strlen((char *)out_frame->payload);
}

static void cmd_req_svc_task(void *pvParameters)
{
    ESP_LOGI(TAG, "cmd_req_svc_task started");
    struct cmd_req_data *pcrd = NULL;

    while (1) {
        /* wait forever for a request pointer on our queue */
        if (xQueueReceive(m_cmd_req_q, &pcrd, portMAX_DELAY) != pdPASS) {
            ESP_LOGE(TAG, "xQueueReceive failed on m_cmd_req_q");
            continue;
        }

        if (pcrd == NULL) {
            ESP_LOGE(TAG, "received NULL pcrd");
            continue;
        }

        struct m2m_frame_t *in_frame = pcrd->frame;
        if (in_frame == NULL) {
            ESP_LOGE(TAG, "pcrd->frame is NULL");
            free(pcrd);
            pcrd = NULL;
            continue;
        }

        if (in_frame->payload_len <= 0) {
            ESP_LOGE(TAG, "Invalid payload length");
            goto cleanup;
        }

        /* extract the command id */
        char *tok = strtok((char *)in_frame->payload, ",\n");
        if (tok == NULL) {
            ESP_LOGE(TAG, "payload parse failed");
            goto cleanup;
        }
        uint16_t cmd_id = (uint16_t)atoi(tok);
        ESP_LOGW(TAG, "servicing cmd %d", cmd_id);

        struct m2m_frame_t out_frame;
        memset(&out_frame, 0, sizeof(out_frame));

        /* default values used across cases */
        uint8_t wifi_state = 0;
        int ret = 0;

        if (in_frame->type == UART_M2M_FRAME_SINGLE_REQ) {
            /* make a single response frame */
            lib_m2m_frame_header_single_resp_make(&out_frame);

            switch (cmd_id) {
            case C20X_M2M_CMD_NET_WIFI_START_STOP: {
                tok = strtok(NULL, ",\n");
                if (tok == NULL) {
                    ESP_LOGE(TAG, "missing param for WIFI_START_STOP");
                    goto cleanup;
                }
                wifi_state = (uint8_t)atoi(tok);

                /* get current wifi mode */
                int wifi_mode = comm_wifi_mode_get();

                if (wifi_state == 1) { /* start wifi station */
                    if (wifi_mode == WIFI_MODE_AP) {
                        ESP_LOGI(TAG, "Wifi mode = %d (WIFI_MODE_AP)", wifi_mode);
                        /* stop AP and start STA - TODO if required */
                    } else if (wifi_mode == WIFI_MODE_APSTA) {
                        ESP_LOGI(TAG, "Wifi mode = %d (WIFI_MODE_APSTA)", wifi_mode);
                    } else if (wifi_mode == WIFI_MODE_STA) {
                        ESP_LOGI(TAG, "Wifi mode = %d (WIFI_MODE_STA)", wifi_mode);
                    } else {
                        ESP_LOGI(TAG, "starting wifi sta");
                        ret = comm_wifi_softap_and_sta_init(); /* soft ap works in AP+STA mode so keeping this now */
                    }
                    if (ret == 0) {
#if 0
                        /* scan and get results */
                        wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
                        uint16_t ap_count = comm_wifi_scan(ap_info);

                        /* make response payload */
                        int idx = 0;
                        idx += sprintf((char *)(out_frame.payload + idx), "%d%s%s%s",
                                       C20X_M2M_CMD_NET_WIFI_START_STOP,
                                       M2M_CMD_PAYLOAD_DELIM, M2M_CMD_RESP_OK, M2M_CMD_PAYLOAD_DELIM);

                        idx = wifi_ssid_arr_make(idx, ap_info, ap_count, out_frame.payload);
                        idx += sprintf((char *)(out_frame.payload + idx), "%s", M2M_CMD_PAYLOAD_TERM);
#else
                        sprintf((char *)out_frame.payload, "%d%s%s%s", C20X_M2M_CMD_NET_WIFI_START_STOP,
                                M2M_CMD_PAYLOAD_DELIM, M2M_CMD_RESP_OK, M2M_CMD_PAYLOAD_TERM);
#endif
                    } else {
                        sprintf((char *)out_frame.payload, "%d%s%s%s", C20X_M2M_CMD_NET_WIFI_START_STOP,
                                M2M_CMD_PAYLOAD_DELIM, M2M_CMD_RESP_ERR, M2M_CMD_PAYLOAD_TERM);
                    }
                } else if (wifi_state == 0) { /* stop wifi station */
                    ESP_LOGI(TAG, "stopping wifi sta");
                    /* TODO: deinit only Wi-Fi STA */
                    ret = 0; /* comm_wifi_softap_deinit(); */
                    if (ret == 0) {
                        sprintf((char *)out_frame.payload, "%d%s%s%s", C20X_M2M_CMD_NET_WIFI_START_STOP,
                                M2M_CMD_PAYLOAD_DELIM, M2M_CMD_RESP_OK, M2M_CMD_PAYLOAD_TERM);
                    } else {
                        sprintf((char *)out_frame.payload, "%d%s%s%s", C20X_M2M_CMD_NET_WIFI_START_STOP,
                                M2M_CMD_PAYLOAD_DELIM, M2M_CMD_RESP_ERR, M2M_CMD_PAYLOAD_TERM);
                    }
                } else if (wifi_state == 2) {
                    sprintf((char *)out_frame.payload, "%d%s%s%s", C20X_M2M_CMD_NET_WIFI_START_STOP,
                            M2M_CMD_PAYLOAD_DELIM, M2M_CMD_RESP_OK, M2M_CMD_PAYLOAD_TERM);
                }
                out_frame.payload_len = (uint16_t)strlen((char *)out_frame.payload);
            } break;

            case C20X_M2M_CMD_NET_WIFI_CONNECT: {
                comm_wifi_sta_disconnect(); /* disconnect from previously connected AP */

                char ssid[32] = {0x00};
                char pwd[64] = {0x00};
                tok = strtok(NULL, "\n"); /* ssid & password */
                if (tok != NULL) {
                    char *tmp = tok;
                    char ssidlen[3] = {0x00};
                    ssidlen[0] = tmp[0];
                    ssidlen[1] = tmp[1];
                    tmp += 2;

                    int ssidlen_i = atoi(ssidlen);
                    strncpy(ssid, tmp, ssidlen_i);
                    tmp += ssidlen_i;

                    char pwdlen[3] = {0x00};
                    pwdlen[0] = tmp[0];
                    pwdlen[1] = tmp[1];
                    tmp += 2;

                    int pwdlen_i = atoi(pwdlen);
                    strncpy(pwd, tmp, pwdlen_i);
                    tmp += pwdlen_i;

                    /* connect to wifi */
                    char ip[20] = {0x00};
                    int len = 0;
                    ret = comm_wifi_sta_connect(ssid, pwd);
                    ret |= comm_wifi_sta_ip_get(ip, &len);
                    if (ret == 0) {
                        sprintf((char *)out_frame.payload, "%d%s%s%s%s%s", C20X_M2M_CMD_NET_WIFI_CONNECT,
                                M2M_CMD_PAYLOAD_DELIM, ip, M2M_CMD_PAYLOAD_DELIM, M2M_CMD_RESP_OK,
                                M2M_CMD_PAYLOAD_TERM);
                    } else {
                        sprintf((char *)out_frame.payload, "%d%s%s%s", C20X_M2M_CMD_NET_WIFI_CONNECT,
                                M2M_CMD_PAYLOAD_DELIM, M2M_CMD_RESP_ERR, M2M_CMD_PAYLOAD_TERM);
                    }
                    out_frame.payload_len = (uint16_t)strlen((char *)out_frame.payload);
                }
            } break;

            case C20X_M2M_CMD_NET_HOTSPOT_START_STOP: {
                tok = strtok(NULL, ",\n");
                if (tok == NULL) {
                    ESP_LOGE(TAG, "missing param for HOTSPOT_START_STOP");
                    goto cleanup;
                }
                uint8_t hp_state = (uint8_t)atoi(tok);

                int wifi_mode = comm_wifi_mode_get();
                char ip[20] = {0x00};
                int len = 0;

                if (hp_state == 1) { /* start hotspot */
                    if (wifi_mode == WIFI_MODE_AP) {
                        ESP_LOGI(TAG, "Wifi mode = %d (WIFI_MODE_AP)", wifi_mode);
                        comm_wifi_softap_ip_get(ip, &len);
                        ret = 0;
                    } else if (wifi_mode == WIFI_MODE_APSTA) {
                        ESP_LOGI(TAG, "Wifi mode = %d (WIFI_MODE_APSTA)", wifi_mode);
                        comm_wifi_softap_ip_get(ip, &len);
                        ret = 0;
                    } else if (wifi_mode == WIFI_MODE_STA) {
                        ESP_LOGI(TAG, "Wifi mode = %d (WIFI_MODE_STA)", wifi_mode);
                        /* TODO stop STA start AP */
                    } else {
                        ESP_LOGI(TAG, "starting wifi soft ap");
                        ret = comm_wifi_softap_and_sta_init();
                        comm_wifi_softap_ip_get(ip, &len);
                    }
                } else { /* stop hotspot */
                    ESP_LOGI(TAG, "stopping wifi soft ap");
                    ret = comm_wifi_softap_deinit();
                    strcpy(ip, "0.0.0.0");
                }

                if (ret == 0) {
                    sprintf((char *)out_frame.payload, "%d%s%s%s%s%s", C20X_M2M_CMD_NET_HOTSPOT_START_STOP,
                            M2M_CMD_PAYLOAD_DELIM, ip, M2M_CMD_PAYLOAD_DELIM, M2M_CMD_RESP_OK,
                            M2M_CMD_PAYLOAD_TERM);
                } else {
                    sprintf((char *)out_frame.payload, "%d%s%s%s", C20X_M2M_CMD_NET_HOTSPOT_START_STOP,
                            M2M_CMD_PAYLOAD_DELIM, M2M_CMD_RESP_ERR, M2M_CMD_PAYLOAD_TERM);
                }
                out_frame.payload_len = (uint16_t)strlen((char *)out_frame.payload);
            } break;

            case C20X_M2M_CMD_NET_BT_START_STOP:
                break;

            case C20X_M2M_CMD_NET_WIFI_SCAN_REQ: {
                int conn_stat = comm_wifi_connection_status_get();
                if (conn_stat == COMM_WIFI_STA_CONNECTED) {
                    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
                    uint16_t ap_count = comm_wifi_scan(ap_info);

                    int idx = 0;
                    idx += sprintf((char *)(out_frame.payload + idx), "%d%s%s%s",
                                   C20X_M2M_CMD_NET_WIFI_SCAN_REQ, M2M_CMD_PAYLOAD_DELIM, M2M_CMD_RESP_OK,
                                   M2M_CMD_PAYLOAD_DELIM);

                    idx = wifi_ssid_arr_make(idx, ap_info, ap_count, out_frame.payload);
                    idx += sprintf((char *)(out_frame.payload + idx), "%s", M2M_CMD_PAYLOAD_TERM);
                } else {
                    sprintf((char *)out_frame.payload, "%d%s%s%s", C20X_M2M_CMD_NET_WIFI_SCAN_REQ,
                            M2M_CMD_PAYLOAD_DELIM, M2M_CMD_RESP_ERR, M2M_CMD_PAYLOAD_TERM);
                }
                out_frame.payload_len = (uint16_t)strlen((char *)out_frame.payload);
            } break;

            case C20X_M2M_CMD_NET_WIFI_MAC_GET: {
                char sta_mac[20] = {0x00};
                int mac_ret = comm_wifi_mac_get(sta_mac, ESP_MAC_WIFI_STA, true);
                if (mac_ret == 0) {
                    sprintf((char *)out_frame.payload, "%d%s%s%s%s%s", C20X_M2M_CMD_NET_WIFI_MAC_GET,
                            M2M_CMD_PAYLOAD_DELIM, M2M_CMD_RESP_OK, M2M_CMD_PAYLOAD_DELIM, sta_mac,
                            M2M_CMD_PAYLOAD_TERM);
                } else {
                    sprintf((char *)out_frame.payload, "%d%s%s%s", C20X_M2M_CMD_NET_WIFI_MAC_GET,
                            M2M_CMD_PAYLOAD_DELIM, M2M_CMD_RESP_ERR, M2M_CMD_PAYLOAD_TERM);
                }
                out_frame.payload_len = (uint16_t)strlen((char *)out_frame.payload);
            } break;

            case C20X_M2M_CMD_FILE_DOWNLOAD_URL_SET: {
                tok = strtok(NULL, ",");
                if (tok == NULL) goto cleanup;

                int url_len = atoi(tok);
                int tok_len = strlen(tok);
                ret = app_net_file_download_url_set(tok + tok_len + 1, url_len);
                if (!ret)
                    out_frame_ok_err_set(&out_frame, C20X_M2M_CMD_FILE_DOWNLOAD_URL_SET, M2M_CMD_RESP_OK);
                else
                    out_frame_ok_err_set(&out_frame, C20X_M2M_CMD_FILE_DOWNLOAD_URL_SET, M2M_CMD_RESP_ERR);
            } break;

            case C20X_M2M_CMD_FILE_DOWNLOAD_CERT_SET: {
                tok = strtok(NULL, ",");
                if (tok == NULL) goto cleanup;

                int cert_len = atoi(tok);
                int tok_len = strlen(tok);
                ret = app_net_file_download_cert_set(tok + tok_len + 1, cert_len);
                if (!ret)
                    out_frame_ok_err_set(&out_frame, C20X_M2M_CMD_FILE_DOWNLOAD_CERT_SET, M2M_CMD_RESP_OK);
                else
                    out_frame_ok_err_set(&out_frame, C20X_M2M_CMD_FILE_DOWNLOAD_CERT_SET, M2M_CMD_RESP_ERR);
            } break;

            case C20X_M2M_CMD_NET_FWAPP_AVAIL: {
                out_frame_ok_err_set(&out_frame, C20X_M2M_CMD_NET_FWAPP_AVAIL, M2M_CMD_RESP_OK);
            } break;

            case C20X_M2M_CMD_NET_FWAPP_UPDATE: {
                host_cmds_fw_update_do_update();
                /* should not return */
            } break;

            default:
                break;
            } /* switch cmd_id */

            /* compute checksum & serialize & send response */
            (void)lib_m2m_frame_checksum_compute(&out_frame);

            size_t sdata_len = 0;
            uint8_t *serialized_buffer = lib_m2m_frame_alloc_serialize(&out_frame, &sdata_len);
            if (serialized_buffer != NULL) {
                host_cmds_send_only((const char *)serialized_buffer, sdata_len);
                ESP_LOG_BUFFER_HEXDUMP(TAG, serialized_buffer, sdata_len, ESP_LOG_WARN);
                free(serialized_buffer);
            } else {
                ESP_LOGE(TAG, "failed to serialize out_frame");
            }

            /* check for special case, like trigger get request for ssid list on http server */
            if ((cmd_id == C20X_M2M_CMD_NET_WIFI_START_STOP) && (wifi_state == 1)) {
                char redirect_url[100] = {0x00};
                strcat(redirect_url, HTTP_REDIRECT_URL_WSSID);
                ESP_LOGW(TAG, "URL = %s", redirect_url);
                app_http_server_continue_url_update(redirect_url);
            } else if ((cmd_id == C20X_M2M_CMD_NET_WIFI_START_STOP) && (wifi_state == 2)) {
                char redirect_url[100] = {0x00};
                strcat(redirect_url, HTTP_REDIRECT_URL_WPWD);
                ESP_LOGW(TAG, "URL = %s", redirect_url);
                app_http_server_continue_url_update(redirect_url);
            } else if (cmd_id == C20X_M2M_CMD_NET_WIFI_CONNECT) {
                char redirect_text[100] = {0x00};
                if (ret == 0)
                    strcat(redirect_text, HTTP_REDIRECT_PAGE_SUCCESS);
                else
                    strcat(redirect_text, HTTP_REDIRECT_PAGE_FAIL);
                ESP_LOGW(TAG, "URL = %s", redirect_text);
                app_http_server_continue_text_update(redirect_text);
            } else if (cmd_id == C20X_M2M_CMD_NET_FWAPP_AVAIL) {
                /* New firmware app available: send command to host to get the new fw app */
                host_cmds_fw_update_netapp_get();
            }

        } else if (in_frame->type == UART_M2M_FRAME_STREAM_REQ) {
            /* handle stream request if needed */

        } else if (in_frame->type == UART_M2M_FRAME_DATA_REQ) {
            /* data request handling */
            char *tok2 = strtok(NULL, ",\n");
            if (tok2 != NULL) {
                uint16_t cmd_id2 = (uint16_t)atoi(tok2);
                switch (cmd_id2) {
                case C20X_M2M_CMD_FILE_DOWNLOAD: {
                    app_net_http_file_download_and_stream_start();
                } break;
                default:
                    break;
                }
            }

        } else if (in_frame->type == UART_M2M_FRAME_DATA_ACK) {
            /* data ack handling */
            char *tok3 = strtok(NULL, ",\n");
            if (tok3 != NULL) {
                uint16_t cmd_id3 = (uint16_t)atoi(tok3);
                switch (cmd_id3) {
                case C20X_M2M_CMD_FILE_DOWNLOAD: {
                    host_cmds_net_data_ack_handler(in_frame);
                } break;
                default:
                    break;
                }
            }
        }

    cleanup:
        ESP_LOGD(TAG, "in_frame: %p", in_frame);
        free(in_frame);
        in_frame = NULL;

        ESP_LOGD(TAG, "pcrd: %p", pcrd);
        free(pcrd);
        pcrd = NULL;
    } /* while (1) */
}

int host_cmds_req_init()
{
    int ret = 0;

    m_mutex = xSemaphoreCreateMutex();
    m_cmd_req_q = xQueueCreate(CMD_REQ_Q_LEN, sizeof(struct cmd_req_data *));

    if ((m_mutex == NULL) || (m_cmd_req_q == NULL)) {
        ESP_LOGE(TAG, "could not allocate resources!");
        return -1;
    }

    /* create and start request command service thread */
    BaseType_t xret = xTaskCreate(cmd_req_svc_task, "cmd_req_svc", HOST_CMDS_REQ_SERVICE_TASK_STACK,
                                  NULL, HOST_CMDS_REQ_SERVICE_TASK_PRIO, &m_th_req_svc);
    if (xret == pdPASS) {
        ESP_LOGI(TAG, "commands request service task created successfully");
    } else {
        ESP_LOGE(TAG, "commands request service task creation failed");
        return -1;
    }

    /* add callbacks to all request commands */

    // wifi
    host_cmds_add_callback(&cb_wifi_onoff, cb_cmd_svc_handler, C20X_M2M_CMD_NET_WIFI_START_STOP);
    host_cmds_add_callback(&cb_wifi_connect, cb_cmd_svc_handler, C20X_M2M_CMD_NET_WIFI_CONNECT);
    host_cmds_add_callback(&cb_hotspot, cb_cmd_svc_handler, C20X_M2M_CMD_NET_HOTSPOT_START_STOP);
    host_cmds_add_callback(&cb_wifi_scan, cb_cmd_svc_handler, C20X_M2M_CMD_NET_WIFI_SCAN_REQ);
    host_cmds_add_callback(&cb_wifi_mac, cb_cmd_svc_handler, C20X_M2M_CMD_NET_WIFI_MAC_GET);

    // settings file download
    host_cmds_add_callback(&cb_filednl_url_set, cb_cmd_svc_handler, C20X_M2M_CMD_FILE_DOWNLOAD_URL_SET);
    host_cmds_add_callback(&cb_filednl_cert_set, cb_cmd_svc_handler, C20X_M2M_CMD_FILE_DOWNLOAD_CERT_SET);
    host_cmds_add_callback(&cb_filednl, cb_cmd_svc_handler, C20X_M2M_CMD_FILE_DOWNLOAD);
    host_cmds_add_callback(&cb_data_ack, cb_cmd_svc_handler, C20X_M2M_CMD_FILE_DOWNLOAD);

    // firmware update
    host_cmds_add_callback(&cb_fw_avail, cb_cmd_svc_handler, C20X_M2M_CMD_NET_FWAPP_AVAIL);
    host_cmds_add_callback(&cb_fw_update, cb_cmd_svc_handler, C20X_M2M_CMD_NET_FWAPP_UPDATE);

    return ret;
}
