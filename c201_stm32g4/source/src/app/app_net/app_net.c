/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 1-Mar-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_net);

#include <zephyr/drivers/gpio.h>

#include "app_thread_configs.h"

#include "app_settings/app_settings.h"
#include "app_settings/app_settings_paths.h"
#include "app_settings/app_settings_value.h"
#include "app_uart_m2m_com/c20x_m2m_cmds.h"
//#include "app_uart_m2m_com/app_uart_m2m_com_frame.h"
#if (CONFIG_LIB_M2M_FRAME)
#include "lib_m2m_frame/lib_m2m_frame.h"
#endif
#include "app_uart_m2m_com/app_uart_m2m_callback.h"
#include "app_uart_m2m_com/app_wifi_bt/app_wifi_bt.h"
#include "app_uart_m2m_com/app_wifi_bt/app_wifi_bt_cmds.h"

#include "lib_events/lib_events.h"

#include "app_net/app_net.h"

/* Wifi BT app thread variables */
K_THREAD_STACK_DEFINE(m_net_th_stack, APP_THREAD_STACK_SIZE_NET);
static struct k_thread m_net_th_data;
static k_tid_t m_net_tid;
struct k_fifo m_net_fifo;

struct k_work m_net_work;
static struct lib_events_callback m_cb_settings_changed;
static sys_slist_t m_ssid_list;

static int m_run_thread = 0; // 0 = NO, 1 = YES, used to block thread
static APP_NET_CONN_STATUS m_app_net_wifi_status = APP_NET_CONN_STAT_UNKNOWN;
static APP_NET_CONN_STATUS m_app_net_softap_status = APP_NET_CONN_STAT_UNKNOWN;

struct resp_cb_data {
	bool resp_available;
	struct k_sem lock;
};

struct net_comm_ctrl_data {
	struct app_uart_m2m_callback m2m_cb;
	struct resp_cb_data rcb_data;
	uint8_t resp_stat;
	char ip[20];
	char mac[20];
};

/* static functions */
static inline int ssid_list_add(sys_slist_t *list, struct app_net_wifi_ssid *ssid) {
	sys_slist_append(list, &ssid->node);
	return 0;
}

static int update_resp_availability(struct k_sem *mutex, bool *pvar, bool val) {
	if (mutex != NULL) {
		if (k_sem_take(mutex, K_MSEC(10)) == 0) {
			*pvar = val;
			k_sem_give(mutex);
			return 0;
		} else {
			return -1;
		}
	}
	return -1;
}

#define RESP_AVAILABLE_LOOP_DELAY	(10)
#define RESP_AVAILABLE_TIMEOUT		(10000)
static int wait_until_timeout(bool *pvar) {
	uint32_t delay = 0;
	while (!(*pvar)) {	/* TODO: IMPORTANT implement some IPC mechanism to check this variable */
		k_sleep(K_MSEC(RESP_AVAILABLE_LOOP_DELAY));
		delay += RESP_AVAILABLE_LOOP_DELAY;
		if (delay > RESP_AVAILABLE_TIMEOUT) {
			return -1;
		}
	}
	return 0;
}

static struct net_comm_ctrl_data * nccd_alloc_and_init()
{
	struct net_comm_ctrl_data *nccd = (struct net_comm_ctrl_data*) calloc(1, sizeof(struct net_comm_ctrl_data));
	if (nccd == NULL) {
		LOG_ERR("%s calloc failed!", __func__);
		return NULL;
	}
	k_sem_init(&nccd->rcb_data.lock, 1, 1);
	nccd->rcb_data.resp_available = false;
	memset(nccd->ip, 0x00, strlen(nccd->ip));
	return nccd;
}

static void m2m_cb_wifi_connect_handler(struct app_uart_m2m_callback *cb, uint16_t cmd, void *data)
{
	struct net_comm_ctrl_data *nccd = (struct net_comm_ctrl_data *) cb->user_data;

	if (cmd == C20X_M2M_CMD_NET_WIFI_CONNECT) {
		struct m2m_frame_t *frame = (struct m2m_frame_t*) data;

		if (frame->type != UART_M2M_FRAME_SINGLE_RESP) {
			LOG_ERR("Invalid frame type!");
			return;
		}

		if (frame->payload_len <= 0) {
			LOG_ERR("Invalid payload length");
			return;
		}
		char *tok = strtok(frame->payload, ",");	// cmd id

		tok = strtok(NULL, ",\n");					// ip address
		if (tok != NULL) {
			if (strcmp(tok, M2M_CMD_RESP_ERR) == 0) {
				k_sem_take(&nccd->rcb_data.lock, K_FOREVER);
				nccd->resp_stat = APP_NET_ERR;
				k_sem_give(&nccd->rcb_data.lock);
				return;
			}

			strcpy(nccd->ip, tok);
			LOG_INF("soft AP IP = %s", nccd->ip);

			tok = strtok(NULL, "\n");					// status OK / ERR
			if (tok != NULL) {
				k_sem_take(&nccd->rcb_data.lock, K_FOREVER);
				if (strcmp(tok, M2M_CMD_RESP_OK) == 0)
					nccd->resp_stat = APP_NET_OK;
				else if (strcmp(tok, M2M_CMD_RESP_ERR) == 0)
					nccd->resp_stat = APP_NET_ERR;
				nccd->rcb_data.resp_available = true;
				k_sem_give(&nccd->rcb_data.lock);
			}
		}
	}
}

static void m2m_cb_hotspot_handler(struct app_uart_m2m_callback *cb, uint16_t cmd, void *data)
{
	struct net_comm_ctrl_data *nccd = (struct net_comm_ctrl_data *) cb->user_data;

	if (cmd == C20X_M2M_CMD_NET_HOTSPOT_START_STOP) {
		struct m2m_frame_t *frame = (struct m2m_frame_t*) data;

		if (frame->type != UART_M2M_FRAME_SINGLE_RESP) {
			LOG_ERR("Invalid frame type!");
			return;
		}

		if (frame->payload_len <= 0) {
			LOG_ERR("Invalid payload length");
			return;
		}
		char *tok = strtok(frame->payload, ",");	// cmd id

		tok = strtok(NULL, ",\n");					// ip address
		if (tok != NULL) {
			if (strcmp(tok, M2M_CMD_RESP_ERR) == 0) {
				k_sem_take(&nccd->rcb_data.lock, K_FOREVER);
				nccd->resp_stat = APP_NET_ERR;
				k_sem_give(&nccd->rcb_data.lock);
				return;
			}

			strcpy(nccd->ip, tok);
			LOG_INF("soft AP IP = %s", nccd->ip);

			tok = strtok(NULL, "\n");					// status OK / ERR
			if (tok != NULL) {
				k_sem_take(&nccd->rcb_data.lock, K_FOREVER);
				if (strcmp(tok, M2M_CMD_RESP_OK) == 0)
					nccd->resp_stat = APP_NET_OK;
				else if (strcmp(tok, M2M_CMD_RESP_ERR) == 0)
					nccd->resp_stat = APP_NET_ERR;
				nccd->rcb_data.resp_available = true;
				k_sem_give(&nccd->rcb_data.lock);
			}
		}
	}
}

static void m2m_cb_wifi_ss_handler(struct app_uart_m2m_callback *cb, uint16_t cmd, void *data)
{
	struct net_comm_ctrl_data *nccd = (struct net_comm_ctrl_data *) cb->user_data;
	if (cmd == C20X_M2M_CMD_NET_WIFI_START_STOP) {
		struct m2m_frame_t *frame = (struct m2m_frame_t*) data;

		if (frame->type != UART_M2M_FRAME_SINGLE_RESP) {
			LOG_ERR("Invalid frame type!");
			return;
		}

		if (frame->payload_len <= 0) {
			LOG_ERR("Invalid payload length");
			return;
		}
		char *tok = strtok(frame->payload, ",");	// cmd id

		tok = strtok(NULL, ",\n");					// cmd stat
		if (tok != NULL) {
			if (strcmp(tok, M2M_CMD_RESP_ERR) == 0) {
				k_sem_take(&nccd->rcb_data.lock, K_FOREVER);
				nccd->resp_stat = APP_NET_ERR;
				k_sem_give(&nccd->rcb_data.lock);
				return;
			} else if (strcmp(tok, M2M_CMD_RESP_OK) == 0) {
				k_sem_take(&nccd->rcb_data.lock, K_FOREVER);
				nccd->resp_stat = APP_NET_OK;
				k_sem_give(&nccd->rcb_data.lock);
#if 0
				tok = strtok(NULL, "\n");					// ssid list
				if (tok != NULL) {
					app_net_parse_ssid_list(tok);
//					char *ssid = tok;
//					while (*ssid != '\0') {
//						char ssidlen[3] = { 0x00 };
//						ssidlen[0] = ssid[0];		// 2 bytes of length
//						ssidlen[1] = ssid[1];
//						ssid += 2;
//
//						int len = atoi(ssidlen);
//						struct app_net_wifi_ssid *anws =
//								(struct app_net_wifi_ssid*) calloc(1,
//										sizeof(struct app_net_wifi_ssid));
//						if (anws == NULL) {
//							LOG_ERR("calloc failed %s", __func__);
//							return;
//						}
//						strncpy(anws->ssid, ssid, len);
//						ssid_list_add(&m_ssid_list, anws);
//
//						ssid += len;
//					}
				}
#endif
				k_sem_take(&nccd->rcb_data.lock, K_FOREVER);
				nccd->rcb_data.resp_available = true;
				k_sem_give(&nccd->rcb_data.lock);
			}
		}
	}
}

static void m2m_cb_wifi_mac_handler(struct app_uart_m2m_callback *cb, uint16_t cmd, void *data)
{
	struct net_comm_ctrl_data *nccd = (struct net_comm_ctrl_data *) cb->user_data;

	if (cmd == C20X_M2M_CMD_NET_WIFI_MAC_GET) {
		struct m2m_frame_t *frame = (struct m2m_frame_t*) data;

		if (frame->type != UART_M2M_FRAME_SINGLE_RESP) {
			LOG_ERR("Invalid frame type!");
			return;
		}

		if (frame->payload_len <= 0) {
			LOG_ERR("Invalid payload length");
			return;
		}
		char *tok = strtok(frame->payload, ",");	// cmd id

		tok = strtok(NULL, ",");					// cmd stat
		if (tok != NULL) {
			if (strcmp(tok, M2M_CMD_RESP_OK) == 0) {
				tok = strtok(NULL, "\n");					// mac addr
				if (tok != NULL) {
					strcpy(nccd->mac, tok);
					LOG_INF("MAC = %s", nccd->mac);

					k_sem_take(&nccd->rcb_data.lock, K_FOREVER);
					nccd->resp_stat = APP_NET_OK;
					nccd->rcb_data.resp_available = true;
					k_sem_give(&nccd->rcb_data.lock);
				}
			} else if (strcmp(tok, M2M_CMD_RESP_ERR) == 0) {
				k_sem_take(&nccd->rcb_data.lock, K_FOREVER);
				nccd->resp_stat = APP_NET_ERR;
				nccd->rcb_data.resp_available = true;
				k_sem_give(&nccd->rcb_data.lock);
			}
		}
	}
}

sys_slist_t* app_net_parse_ssid_list(char *ssid_list)
{
	if (ssid_list != NULL) {
		char *ssid = ssid_list;
		while (*ssid != '\0') {
			char ssidlen[3] = { 0x00 };
			ssidlen[0] = ssid[0];		// 2 bytes of length
			ssidlen[1] = ssid[1];
			ssid += 2;

			int len = atoi(ssidlen);
			struct app_net_wifi_ssid *anws =
					(struct app_net_wifi_ssid*) calloc(1,
							sizeof(struct app_net_wifi_ssid));
			if (anws == NULL) {
				LOG_ERR("calloc failed %s", __func__);
				return NULL;
			}
			strncpy(anws->ssid, ssid, len);
			ssid_list_add(&m_ssid_list, anws);

			ssid += len;
		}
	}
	return &m_ssid_list;
}

int app_net_wifi_hotspot_start_stop(uint8_t wifi_hp, uint8_t start_stop, char *ip,
		sys_slist_t **ssid_list)
{
	int ret = 0;

	/* allocate control data */
	struct net_comm_ctrl_data *nccd = nccd_alloc_and_init();
	if (nccd == NULL)	return -ENOMEM;
	nccd->m2m_cb.user_data = nccd;

	switch (wifi_hp) {
	case APP_NET_WIFI:
		nccd->m2m_cb.cmd = C20X_M2M_CMD_NET_WIFI_START_STOP;
		app_uart_m2m_callback_add(&nccd->m2m_cb, m2m_cb_wifi_ss_handler, nccd->m2m_cb.cmd);
		break;
	case APP_NET_HOTSPOT:
		nccd->m2m_cb.cmd = C20X_M2M_CMD_NET_HOTSPOT_START_STOP;
		app_uart_m2m_callback_add(&nccd->m2m_cb, m2m_cb_hotspot_handler, nccd->m2m_cb.cmd);
		break;
	case APP_NET_WIFI_DUMMY:
		nccd->m2m_cb.cmd = C20X_M2M_CMD_NET_WIFI_START_STOP;
//		app_uart_m2m_callback_add(&nccd->m2m_cb, m2m_cb_wifi_ss_handler, nccd->m2m_cb.cmd);
		break;
	}

	/* make and send command to network processor */
	struct m2m_frame_t frame;
	memset(&frame, 0x00, sizeof(frame));

	lib_m2m_frame_header_single_req_make(&frame);

	frame.payload_len = sprintf((char*)frame.payload, "%d%s%d%s",	/* 40,1\n --> start | 41,0\n --> stop*/
								nccd->m2m_cb.cmd,
								M2M_CMD_PAYLOAD_DELIM,
								start_stop,
								M2M_CMD_PAYLOAD_TERM);

#if 0
	/* serialize the frame and send */
	/* buffer size = frame header size + pay load size + 1 NULL char */
	uint32_t sbuf_len = UART_M2M_HEADER_SIZE_MAX+frame.payload_len+1;
	uint32_t sdata_len=0;
	uint8_t *serialized_buffer = (uint8_t *) calloc(1, sbuf_len);
	if (serialized_buffer == NULL) {
		LOG_ERR("%s calloc failed!", __func__);
		goto err;
	}
	lib_m2m_frame_serialize(serialized_buffer, sbuf_len, &frame, &sdata_len);
#endif
	/* compute checksum */
	ret = lib_m2m_frame_checksum_compute(&frame);

	/* serialize the frame and send */
	size_t sdata_len=0;
	char *serialized_buffer = lib_m2m_frame_alloc_serialize(&frame, &sdata_len);
	if (serialized_buffer == NULL) {
		goto err;
	}

	if (update_resp_availability(&nccd->rcb_data.lock, &nccd->rcb_data.resp_available, false) < 0) {
		free(serialized_buffer);
		goto err;
	}
	LOG_HEXDUMP_DBG(serialized_buffer, sdata_len, "APP NET TX");
	ret = app_wifi_bt_cmd_send(serialized_buffer, sdata_len);
	free(serialized_buffer);

	if (wifi_hp != APP_NET_WIFI_DUMMY) {
		/* wait for response until timeout */
		if (wait_until_timeout(&nccd->rcb_data.resp_available) < 0) {
			ret = -1;
			goto err;
		}

		/* copy response, deallocate memory and return */
		ret = nccd->resp_stat;
		if (wifi_hp == APP_NET_WIFI) {
			if (ssid_list != NULL)
				*ssid_list = &m_ssid_list;
		} else if (wifi_hp == APP_NET_HOTSPOT) {
			if (ip != NULL) {
				strcpy(ip, nccd->ip);
				m_app_net_softap_status = APP_NET_CONN_STAT_WIFI_SOFTAP_CONNECTED;
			} else if (!ip && !start_stop) {
				m_app_net_softap_status = APP_NET_CONN_STAT_WIFI_SOFTAP_DISCONNECTED;
			}
		}
	}
err:
	if (wifi_hp != APP_NET_WIFI_DUMMY) {
		app_uart_m2m_callback_remove(&nccd->m2m_cb, m2m_cb_hotspot_handler, nccd->m2m_cb.cmd);
	}
	free(nccd);

	return ret;
}

int app_net_wifi_sta_connect(const char *ssid, const char *pwd, char *ip)
{
	int ret = 0;

	/* allocate control data */
	struct net_comm_ctrl_data *nccd = nccd_alloc_and_init();
	if (nccd == NULL)	return -ENOMEM;
	nccd->m2m_cb.user_data = nccd;
	nccd->m2m_cb.cmd = C20X_M2M_CMD_NET_WIFI_CONNECT;
	app_uart_m2m_callback_add(&nccd->m2m_cb, m2m_cb_wifi_connect_handler, nccd->m2m_cb.cmd);

	/* make and send command to network processor */
	struct m2m_frame_t frame;
	memset(&frame, 0x00, sizeof(frame));

	lib_m2m_frame_header_single_req_make(&frame);

	int idx = sprintf((char*)frame.payload, "%d%s",	/* 41,ssid_lenssidpwd_lenpwd\n*/
								nccd->m2m_cb.cmd,
								M2M_CMD_PAYLOAD_DELIM);
	int len = strlen(ssid);
	if (len < 10)
		idx += sprintf((char*) (frame.payload+idx), "0%d%s", len, ssid);
	else
		idx += sprintf((char*) (frame.payload+idx), "%d%s", len, ssid);

	len = strlen(pwd);
	if (len < 10)
		idx += sprintf((char*) (frame.payload+idx), "0%d%s", len, pwd);
	else
		idx += sprintf((char*) (frame.payload+idx), "%d%s", len, pwd);

	idx += sprintf((char*) (frame.payload+idx), "%s", M2M_CMD_PAYLOAD_TERM);

	frame.payload_len = strlen((char*)frame.payload);
#if 0
	/* serialize the frame and send */
	/* buffer size = frame header size + pay load size + 1 NULL char */
	uint32_t sbuf_len = UART_M2M_HEADER_SIZE_MAX+frame.payload_len+1;
	uint32_t sdata_len=0;
	uint8_t *serialized_buffer = (uint8_t *) calloc(1, sbuf_len);
	if (serialized_buffer == NULL) {
		LOG_ERR("%s calloc failed!", __func__);
		goto err;
	}
	lib_m2m_frame_serialize(serialized_buffer, sbuf_len, &frame, &sdata_len);
#endif
	/* compute checksum */
	ret = lib_m2m_frame_checksum_compute(&frame);

	/* serialize the frame and send */
	size_t sdata_len=0;
	char *serialized_buffer = lib_m2m_frame_alloc_serialize(&frame, &sdata_len);
	if (serialized_buffer == NULL) {
		goto err;
	}

	if (update_resp_availability(&nccd->rcb_data.lock, &nccd->rcb_data.resp_available, false) < 0) {
		free(serialized_buffer);
		goto err;
	}
	LOG_HEXDUMP_DBG(serialized_buffer, sdata_len, "APP NET TX");
	ret = app_wifi_bt_cmd_send(serialized_buffer, sdata_len);
	free(serialized_buffer);

	/* wait for response until timeout */
	if (wait_until_timeout(&nccd->rcb_data.resp_available) < 0) {
		ret = -1;
		goto err;
	}

	/* copy response, deallocate memory and return */
	ret = nccd->resp_stat;
	if (ip != NULL)
		strcpy(ip, nccd->ip);

err:
	app_uart_m2m_callback_remove(&nccd->m2m_cb, m2m_cb_hotspot_handler, nccd->m2m_cb.cmd);
	free(nccd);

	return ret;
}

static int app_net_wifi_sta_mac_get(char *sta_mac)
{
	int ret = 0;

	/* allocate control data */
	struct net_comm_ctrl_data *nccd = nccd_alloc_and_init();
	if (nccd == NULL)	return -ENOMEM;
	nccd->m2m_cb.user_data = nccd;
	nccd->m2m_cb.cmd = C20X_M2M_CMD_NET_WIFI_MAC_GET;
	app_uart_m2m_callback_add(&nccd->m2m_cb, m2m_cb_wifi_mac_handler, nccd->m2m_cb.cmd);

	/* make and send command to network processor */
	struct m2m_frame_t frame;
	memset(&frame, 0x00, sizeof(frame));

	lib_m2m_frame_header_single_req_make(&frame);

	sprintf((char*)frame.payload, "%d%s",	/* 48\n */
						nccd->m2m_cb.cmd, M2M_CMD_PAYLOAD_TERM);
	frame.payload_len = strlen((char*)frame.payload);
#if 0
	/* serialize the frame and send */
	/* buffer size = frame header size + pay load size + 1 NULL char */
	uint32_t sbuf_len = UART_M2M_HEADER_SIZE_MAX+frame.payload_len+1;
	uint32_t sdata_len=0;
	uint8_t *serialized_buffer = (uint8_t *) calloc(1, sbuf_len);
	if (serialized_buffer == NULL) {
		LOG_ERR("%s calloc failed!", __func__);
		goto err;
	}
	lib_m2m_frame_serialize(serialized_buffer, sbuf_len, &frame, &sdata_len);
#endif
	/* compute checksum */
	ret = lib_m2m_frame_checksum_compute(&frame);

	/* serialize the frame and send */
	size_t sdata_len=0;
	char *serialized_buffer = lib_m2m_frame_alloc_serialize(&frame, &sdata_len);
	if (serialized_buffer == NULL) {
		goto err;
	}

	if (update_resp_availability(&nccd->rcb_data.lock, &nccd->rcb_data.resp_available, false) < 0) {
		free(serialized_buffer);
		goto err;
	}
	LOG_HEXDUMP_DBG(serialized_buffer, sdata_len, "APP NET TX");
	ret = app_wifi_bt_cmd_send(serialized_buffer, sdata_len);
	free(serialized_buffer);

	/* wait for response until timeout */
	if (wait_until_timeout(&nccd->rcb_data.resp_available) < 0) {
		ret = -1;
		goto err;
	}

	/* copy response, deallocate memory and return */
	ret = nccd->resp_stat;
	if (sta_mac != NULL)
		strcpy(sta_mac, nccd->mac);

err:
	app_uart_m2m_callback_remove(&nccd->m2m_cb, m2m_cb_hotspot_handler, nccd->m2m_cb.cmd);
	free(nccd);

	return ret;
}

int app_net_connect()
{
	int ret = 0;
	struct wifi_sta_config wifi_sta_cfg[SETTING_VAL_WIFI_CRED_SAVED_NUM_MAX];
	ret = app_settings_load_single(SETTINGS_KEY_FULL_DS_NET_WSTACFG, wifi_sta_cfg, sizeof(wifi_sta_cfg));

	for (int i=0; i<SETTING_VAL_WIFI_CRED_SAVED_NUM_MAX; i++) {
		char *ssid = wifi_sta_cfg[i].ssid;
		char *pwd = wifi_sta_cfg[i].pwd;
		if ((strlen(ssid) > 1) && (strlen(pwd) > 7)) {
			/* we have saved wifi ssid and password */
			char ip[20];
			ret = app_net_wifi_sta_connect(ssid, pwd, ip);
			if (ret == 0) {
				LOG_INF("connected to %s", ssid);
				ret = app_settings_save_single_with_retry(SETTINGS_KEY_FULL_DS_NET_WSSIDCON, ssid, SETTING_VAL_WIFI_SSID_LEN_MAX, 10, true);
				ret = app_settings_save_single_with_retry(SETTINGS_KEY_FULL_DS_NET_WIP, ip, sizeof(ip), 10, true);
				lib_events_report_event(LIB_EVENT_NET_WIFI_CONNECTED);
				break;
			}
		} else {
			ret = -1;
		}
	}

	return ret;
}

//static void net_work_handler(struct k_work *work)
static void app_net_thread(void *p1, void *p2, void *p3)
{
	int ret = 0;
	int *run = NULL;
	LOG_INF("app_net_thread started");
	while (1) {
		/* dequeue a rx data packet from the fifo */
		run = k_fifo_get(&m_net_fifo, K_FOREVER);
		if (run == NULL) {
			continue;
		}
		if (*run == 0) {
			continue;
		}

		*run = 0;

		char changed_setting[SETTINGS_FULLPATH_LEN_MAX] = { 0x00 };
		app_settings_changed_latest_get(changed_setting);
		LOG_DBG("changed setting = %s", changed_setting);
		if (strcmp(changed_setting, SETTINGS_KEY_FULL_DS_NET_WI) == 0) {
			struct setting_value val;
			sys_slist_t *ssid_list = NULL;
			ret = app_settings_load_single(SETTINGS_KEY_FULL_DS_NET_WI, &val,
					sizeof(struct setting_value));

			/* wait until the cmd processor is busy */
			while (app_wifi_bt_com_state_get() != APP_WIFI_BT_COM_STATE_IDLE) {
				k_sleep(K_MSEC(10));
			}

			if (val.val1 == 1) { /* start wifi and get list of SSIDs*/
				LOG_INF("Starting WiFI ...");
				ret = app_net_wifi_hotspot_start_stop(APP_NET_WIFI, val.val1,
						NULL, &ssid_list);
				if (ret == 0) {
					m_app_net_wifi_status = APP_NET_CONN_STAT_WIFI_STA_CONNECTED;

					/* populate the ssid option list */
					int idx = app_settings_array_idx_get(SETTINGS_KEY_FULL_DS_NET_WSSID);
					struct app_settings_data const *asd = app_settings_data_obj_get(idx);
					struct setting_value_options *options = asd->options;

					if (options != NULL) {
						/* get number of ssid entries */
						options->num_options = ssid_count_get(ssid_list);

						/* loop and populate the options */
						struct app_net_wifi_ssid *ssid, *tmp;
						int i=0;
						SYS_SLIST_FOR_EACH_CONTAINER_SAFE(ssid_list, ssid, tmp, node)
						{
							char *name = (char *)options->op_val[i].key;
							if (ssid) {
								strcpy(name, ssid->ssid);
								i++;
							}
						}
					}

					/* remove the list from memory */
					ssid_list_remove(ssid_list);

					/* get Wi-Fi station MAC */
					char sta_mac[20] = {0x00};
					ret = app_net_wifi_sta_mac_get(sta_mac);
					if (ret == 0)
						app_settings_save_single_with_retry(SETTINGS_KEY_FULL_DS_NET_WMAC,
												sta_mac, SETTING_VAL_WIFI_MAC_LEN_MAX, 10, true);

					/* report event */
					lib_events_report_event(LIB_EVENT_NET_WIFI_STARTED);
				}
			} else if (val.val1 == 0) {
				ret = app_net_wifi_hotspot_start_stop(APP_NET_WIFI, val.val1,
						NULL, NULL);
				if (ret == 0) {
					m_app_net_wifi_status = APP_NET_CONN_STAT_WIFI_STA_DISCONNECTED;
					lib_events_report_event(LIB_EVENT_NET_WIFI_STOPPED);
				}
			}
		} else if (strcmp(changed_setting, SETTINGS_KEY_FULL_DS_NET_WSSID) == 0) {
			ret = app_net_wifi_hotspot_start_stop(APP_NET_WIFI_DUMMY, 2, NULL, NULL);

		} else if (strcmp(changed_setting, SETTINGS_KEY_FULL_DS_NET_WPWD) == 0) {
			struct setting_value val;
			char wifiSsid[SETTING_VAL_WIFI_SSID_LEN_MAX] = {0x00};
			char wifiPwd[SETTING_VAL_WIFI_PWD_LEN_MAX] = {0x00};

			/* get selected ssid */
			ret = app_settings_load_single(SETTINGS_KEY_FULL_DS_NET_WSSID, &val, sizeof(struct setting_value));
			int idx = app_settings_array_idx_get(SETTINGS_KEY_FULL_DS_NET_WSSID);
			struct app_settings_data const *asd = app_settings_data_obj_get(idx);
			app_settings_option_val_to_key(asd->options, &val, wifiSsid);

			/* get password */
			ret = app_settings_load_single(SETTINGS_KEY_FULL_DS_NET_WPWD, wifiPwd, SETTING_VAL_WIFI_PWD_LEN_MAX);

			/* connect */
			LOG_INF("connecting to wifi station with ssid = %s, and pwd = %s", wifiSsid, wifiPwd);
			char ip[20] = {0x00};
			ret = app_net_wifi_sta_connect(wifiSsid, wifiPwd, ip);
			if (ret == 0) {
				LOG_INF("connected to %s", wifiSsid);
				ret = app_settings_save_single_with_retry(SETTINGS_KEY_FULL_DS_NET_WSSIDCON, wifiSsid, SETTING_VAL_WIFI_SSID_LEN_MAX, 10, true);

				struct wifi_sta_config wifi_sta_cfg[SETTING_VAL_WIFI_CRED_SAVED_NUM_MAX];
				ret = app_settings_load_single(SETTINGS_KEY_FULL_DS_NET_WSTACFG, wifi_sta_cfg, sizeof(wifi_sta_cfg));
				int i;
				for (i=0; i<SETTING_VAL_WIFI_CRED_SAVED_NUM_MAX; i++) {
					if (strlen(wifi_sta_cfg[i].ssid) == 0) {
						strcpy(wifi_sta_cfg[i].ssid, wifiSsid);
						strcpy(wifi_sta_cfg[i].pwd, wifiPwd);
						break;
					}
				}
				if (i == SETTING_VAL_WIFI_CRED_SAVED_NUM_MAX) {
					// roll over
					memset(wifi_sta_cfg[0].ssid, 0x00, sizeof(wifi_sta_cfg[0].ssid));
					memset(wifi_sta_cfg[0].pwd, 0x00, sizeof(wifi_sta_cfg[0].pwd));
					strcpy(wifi_sta_cfg[0].ssid, wifiSsid);
					strcpy(wifi_sta_cfg[0].pwd, wifiPwd);
				}
				/* save ssid and pwd */
//				int count=0;
//				do {
//					ret = app_settings_save_single(SETTINGS_KEY_FULL_DS_NET_WIP, ip, sizeof(ip), true);
//					k_sleep(K_MSEC(10));
//				} while ((ret != 0) && (++count < 10));
				ret = app_settings_save_single_with_retry(SETTINGS_KEY_FULL_DS_NET_WIP, ip, sizeof(ip), 10, true);
//				count=0;
//				do {
//					ret = app_settings_save_single(SETTINGS_KEY_FULL_DS_NET_WSTACFG, wifi_sta_cfg, sizeof(wifi_sta_cfg), true);
//					k_sleep(K_MSEC(10));
//				} while ((ret != 0) && (++count < 10));
				ret = app_settings_save_single_with_retry(SETTINGS_KEY_FULL_DS_NET_WSTACFG, wifi_sta_cfg, sizeof(wifi_sta_cfg), 10, true);

				lib_events_report_event(LIB_EVENT_NET_WIFI_CONNECTED);
			} else {
				LOG_ERR("connection to %s, with password %s failed", wifiSsid, wifiPwd);
				lib_events_report_event(LIB_EVENT_NET_WIFI_DISCONNECTED);
				val.val1 = 1234; val.val2 = 4321;	// this value will not fetch any ssid to be displayed
//				int count=0;
//				do {
//					ret = app_settings_save_single(SETTINGS_KEY_FULL_DS_NET_WSSID, &val, sizeof(struct setting_value), true);
//					k_sleep(K_MSEC(10));
//				} while ((ret != 0) && (++count < 10));
				ret = app_settings_save_single_with_retry(SETTINGS_KEY_FULL_DS_NET_WSSID, &val, sizeof(struct setting_value), 10, true);
			}

		} else if (strcmp(changed_setting, SETTINGS_KEY_FULL_DS_NET_WIAP) == 0) {

		}
	}
}

static void app_event_handler(struct lib_events_callback *cb, LIB_EVENT_TYPE event) {
	switch (event) {
	case LIB_EVENT_POWER_OFF:
	{
		break;
	}
	case LIB_EVENT_SETTINGS_CHANGED:
	{
//		k_work_submit(&m_net_work);
		/* unblock the thread */
		m_run_thread = 1;
		k_fifo_put(&m_net_fifo, &m_run_thread);

		break;
	}
	default:
		break;
	}
}

static int wifi_bt_processor_reset()
{
	int ret=0;
	const struct device *dev;
	gpio_pin_t pin;
	gpio_flags_t flags;
#if (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205 || CONFIG_BOARD_E206 || CONFIG_BOARD_C208T)
	dev = DEVICE_DT_GET(DT_GPIO_CTLR(DT_NODELABEL(wifibt_rst), gpios));
	if (!dev) {
		LOG_ERR("Device %s not found", dev->name);
		return -ENXIO;
	}
	pin = DT_GPIO_PIN(DT_NODELABEL(wifibt_rst), gpios);
	flags = DT_GPIO_FLAGS(DT_NODELABEL(wifibt_rst), gpios);
//#elif (CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
//	const struct device *dev = DEVICE_DT_GET(DT_GPIO_CTLR(DT_NODELABEL(periph_reset), gpios));
//	if (!dev) {
//		LOG_ERR("Device %s not found", dev->name);
//		return -ENXIO;
//	}
//	gpio_pin_t pin = DT_GPIO_PIN(DT_NODELABEL(periph_reset), gpios);
//	gpio_flags_t flags = DT_GPIO_FLAGS(DT_NODELABEL(periph_reset), gpios);
#endif
	ret = gpio_pin_configure(dev, pin, (flags | GPIO_OUTPUT));
	if (ret != 0) {
		LOG_ERR("Failed to configure enable pin %d (%d)", pin, ret);
		return -1;
	}

	/* send reset pulse */
	ret = gpio_pin_set(dev, pin, 0);
	k_sleep(K_MSEC(10));
	ret = gpio_pin_set(dev, pin, 1);
	k_sleep(K_MSEC(10));

	ret = gpio_pin_configure(dev, pin, (GPIO_DISCONNECTED));

	return ret;
}

APP_NET_CONN_STATUS app_net_conn_wifi_status_get()
{
	return m_app_net_wifi_status;
}

APP_NET_CONN_STATUS app_net_conn_softap_status_get()
{
	return m_app_net_softap_status;
}

int app_net_init()
{
	int ret = 0;
#if !(CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)	// c201 boards have common reset pins with i2c_mux, io_exp(s) and esp32, so don't reset
	ret = wifi_bt_processor_reset();
#endif
	/* initial status */
	m_app_net_wifi_status = APP_NET_CONN_STAT_WIFI_STA_DISCONNECTED;
	m_app_net_softap_status = APP_NET_CONN_STAT_WIFI_SOFTAP_DISCONNECTED;

	/* keep the wifi& hotstop setting in OFF mode after boot */
	struct setting_value val; val.val1=0; val.val2=0;
//	int count = 0;
//	do {
//		ret = app_settings_save_single(SETTINGS_KEY_FULL_DS_NET_WIAP, &val, sizeof(struct setting_value), true);
//		k_sleep(K_MSEC(10));
//	} while ((ret != 0) && (++count < 10));
//	count = 0;
//	do {
//		ret = app_settings_save_single(SETTINGS_KEY_FULL_DS_NET_WI, &val, sizeof(struct setting_value), true);
//		k_sleep(K_MSEC(10));
//	} while ((ret != 0) && (++count < 10));

	ret = app_settings_save_single_with_retry(SETTINGS_KEY_FULL_DS_NET_WIAP, &val, sizeof(struct setting_value), 10, true);
	ret = app_settings_save_single_with_retry(SETTINGS_KEY_FULL_DS_NET_WI, &val, sizeof(struct setting_value), 10, true);

	/* Initialize  fifo */
	k_fifo_init(&m_net_fifo);

	/* Start net application thread */
	m_net_tid = k_thread_create(&m_net_th_data, m_net_th_stack,
					K_THREAD_STACK_SIZEOF(m_net_th_stack), app_net_thread,
					NULL, NULL, NULL, APP_THREAD_PRIO_NET, 0, K_NO_WAIT);
#if (CONFIG_THREAD_NAME)
	ret = k_thread_name_set(m_net_tid, APP_THREAD_NAME_NET);
#endif
	/* initialize the net work queue and register events */
//	k_work_init(&m_net_work, net_work_handler);
	ret = lib_events_callback_add(&m_cb_settings_changed, app_event_handler, LIB_EVENT_SETTINGS_CHANGED);

	return ret;
}
