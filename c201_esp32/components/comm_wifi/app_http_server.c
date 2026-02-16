/*
 * Copyright (c) 2022 Acme CPU
 *
 * http_server.c
 * Created on: 26-Mar-2023
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <inttypes.h>
#include <ctype.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#ifndef portTICK_PERIOD_MS
#define portTICK_PERIOD_MS portTICK_PERIOD_MS
#endif

#include <esp_http_server.h>

#include "host_cmds.h"
#include "host_cmds_priv.h"
#include "host_cmds_send_recv.h"
#include "host_cmds_callback.h"
#include "c20x_m2m_cmds.h"
//#include "m2m_frame.h"
#include "lib_m2m_frame.h"

#include "app_http_server.h"
#include "comm_wifi.h"

static const char *TAG = "http_server";

#define ROOT_PATH	SETTINGS_KEY_ROOT 	//"r"
#define NET_PATH	SETTINGS_KET_NET	// "r/ds/net"

typedef enum {
	STREAM_FRAME_START=0,
	STREAM_FRAME_IN_PROCESS,
	STREAM_FRAME_END
} STREAM_FRAME_STAT;

struct resp_cb_data {
	bool resp_available;
	SemaphoreHandle_t lock;
};
static struct resp_cb_data m_rcd;
static struct host_cmd_callback cb_html_page;
static httpd_handle_t m_server = NULL;

char m_buf[SCRATCH_BUFSIZE];
char m_html_buf[HTML_PAGE_BUFSIZE];
int m_hbufidx = 0;
int m_stream_stat = STREAM_FRAME_START;

static char m_curr_path[SETTINGS_FULLPATH_LEN_MAX] = ROOT_PATH;

/* variables to redirect from a page, used with CONT type of command stats */
static int m_redirect = APP_HTTP_SERVER_REDIRECT_NONE;
static char m_continue_url[100] = {0x00};
static char m_continue_text[100] = {0x00};
static int m_wifi_scan_stat = 0;

static int update_data_availability(SemaphoreHandle_t mutex, bool *pvar, bool val) {
	if (mutex != NULL) {
		if (xSemaphoreTake(mutex, (TickType_t) 10/portTICK_PERIOD_MS ) == pdTRUE) {
			*pvar = val;
			xSemaphoreGive(mutex);
			return 0;
		} else {
			return -1;
		}
	}
	return -1;
}
#define RESP_AVAILABLE_LOOP_DELAY	(10)
#define RESP_AVAILABLE_TIMEOUT		(30*1000)	// 30 secs
static int wait_until_timeout(bool *pvar) {
	uint32_t delay = 0;
	while (!(*pvar)) {
		vTaskDelay(RESP_AVAILABLE_LOOP_DELAY / portTICK_PERIOD_MS);
		delay += RESP_AVAILABLE_LOOP_DELAY;
		if (delay > RESP_AVAILABLE_TIMEOUT) {
			return -1;
		}
	}
	return 0;
}

void app_http_server_continue_url_update(const char *redirect_url) {
	if (redirect_url != NULL) {
		memset(m_continue_url, 0x00, sizeof(m_continue_url));
		strcpy(m_continue_url, redirect_url);
	}
	m_redirect = APP_HTTP_SERVER_REDIRECT_TYPE_URL;
}

void app_http_server_continue_text_update(const char *text) {
	if (text != NULL) {
		memset(m_continue_text, 0x00, sizeof(m_continue_text));
		strcpy(m_continue_text, text);
	}
	m_redirect = APP_HTTP_SERVER_REDIRECT_TYPE_TEXT;
}

void cb_html_page_handler(struct host_cmd_callback *cb, uint32_t cmd, void *frame)
{
	struct m2m_frame_t *fr = (struct m2m_frame_t *) frame;

	if (cmd == C20X_M2M_CMD_NET_WS_HTML_PAGE_GET) {
		ESP_LOGW(TAG, "cb_html_page_handler: cmd %d", C20X_M2M_CMD_NET_WS_HTML_PAGE_GET);

//		if ((fr->type != UART_M2M_FRAME_STREAM_RESP) || (fr->type != UART_M2M_FRAME_SINGLE_RESP)) {
//			ESP_LOGE(TAG, "Invalid Frame type");
//			return;
//		}

		if (fr->payload_len <= 0) {
			ESP_LOGE(TAG, "Invalid payload length");
			goto err;
		}

		char *tok = strtok((char*)fr->payload, ",");	// cmd id

		if (fr->type == UART_M2M_FRAME_DATA_RESP) {
			if (m_stream_stat == STREAM_FRAME_START) {		// check start of transmission and zero all buffers
				memset(m_html_buf, 0x00, sizeof(m_html_buf));
				m_hbufidx = 0;
				m_stream_stat = STREAM_FRAME_IN_PROCESS;
			}

			/* copy the html data */
			int cmd_len = (strlen(tok) + strlen(M2M_CMD_PAYLOAD_DELIM));
			uint32_t dlen = fr->payload_len - cmd_len;	// data length = payload len - (cmd id + delim) len
			memcpy(m_html_buf+m_hbufidx, fr->payload+cmd_len, dlen);
			m_hbufidx += dlen;
//			ESP_LOGW(TAG, "%s", m_html_buf);
		} else if (fr->type == UART_M2M_FRAME_DATA_RESP_ENDSTR) {
			tok = strtok(NULL, "\n");				// cmd_stat, we are expecting STREND (end of stream)
			if (tok != NULL) {
				ESP_LOGW(TAG, "cb_html_page_handler: Parsing cmd_stat");
				if (strcmp(tok, M2M_CMD_RESP_STREND) == 0) {
					ESP_LOGW(TAG, "cmd_stat received %s", M2M_CMD_RESP_STREND);
					if (m_rcd.lock != NULL) {
						if (xSemaphoreTake(m_rcd.lock, (TickType_t) 10/portTICK_PERIOD_MS ) == pdTRUE) {
							m_rcd.resp_available = true;
							xSemaphoreGive(m_rcd.lock);
						}
					}
				} else if (strcmp(tok, M2M_CMD_RESP_ERR) == 0) {
					ESP_LOGW(TAG, "cb_html_page_handler: cmd_stat received %s", M2M_CMD_RESP_ERR);
					ESP_LOGE(TAG, "cmd %ld response error", cmd);
				}
			}
			m_stream_stat = STREAM_FRAME_END;
		}

#if 0
		if (tok != NULL) {
			ESP_LOGW(TAG, "cb_html_page_handler: Parsing cmd_stat");
			if (strcmp(tok, M2M_CMD_RESP_OK) == 0) {
				ESP_LOGW(TAG, "cmd_stat received %s", M2M_CMD_RESP_OK);
				tok = strtok(NULL, "\n");		// html page
				if (tok != NULL) {
					memset(m_html_buf, 0x00, sizeof(m_html_buf));
					strcpy(m_html_buf, tok);			// copy the page
					ESP_LOGW(TAG, "%s", m_html_buf);
					if (m_rcd.lock != NULL) {
						if (xSemaphoreTake(m_rcd.lock, (TickType_t) 10/portTICK_PERIOD_MS ) == pdTRUE) {
							m_rcd.resp_available = true;
							xSemaphoreGive(m_rcd.lock);
						}
					}
				}
			} else if (strcmp(tok, M2M_CMD_RESP_ERR) == 0) {
				ESP_LOGW(TAG, "cb_html_page_handler: cmd_stat received %s", M2M_CMD_RESP_ERR);
				ESP_LOGE(TAG, "cmd %d response error", cmd);
			}
		}
#endif
	}

err:
	free(fr);
}

static int send_get_cmd_to_host(char *path)
{
	int ret = 0;
	if (update_data_availability(m_rcd.lock, &m_rcd.resp_available, false) < 0) {
		ESP_LOGE(TAG, "send_get_cmd_to_host: update_data_availability failed");
		return -1;
	}
	ret = host_cmds_html_server_page_get(path);
	if (ret < 0) {
		ESP_LOGE(TAG, "send_get_cmd_to_host: : host_cmds_html_server_page_get failed");
		return -1;
	}
	ret = 0;
	if (wait_until_timeout(&m_rcd.resp_available) < 0) {
		ESP_LOGE(TAG, "send_get_cmd_to_host: : wait_until_timeout failed");
		return -1;
	}
	return ret;
}

/*************************************************************************
 * HTTP Server functions and handlers
 *************************************************************************/
void urldecode2(char *dst, const char *src)
{
        char a, b;
        while (*src) {
                if ((*src == '%') &&
                    ((a = src[1]) && (b = src[2])) &&
                    (isxdigit(a) && isxdigit(b))) {
                        if (a >= 'a')
                                a -= 'a'-'A';
                        if (a >= 'A')
                                a -= ('A' - 10);
                        else
                                a -= '0';
                        if (b >= 'a')
                                b -= 'a'-'A';
                        if (b >= 'A')
                                b -= ('A' - 10);
                        else
                                b -= '0';
                        *dst++ = 16*a+b;
                        src+=3;
                } else if (*src == '+') {
                        *dst++ = ' ';
                        src++;
                } else {
                        *dst++ = *src++;
                }
        }
        *dst++ = '\0';
}

static esp_err_t post_uri_handler(httpd_req_t *req)
{
	if (m_wifi_scan_stat == 1) {
		ESP_LOGI(TAG, "scan in progress, exiting");
		return ESP_OK;
	}

    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = m_buf; //((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    urldecode2(buf, buf);
    ESP_LOGI(TAG, "Received = %s", buf);
    int ret = 0;
    char path[32] = {0x00};
    char *tok = strtok(buf, "=");
    if (tok != NULL) {
    	strcpy(path, tok);
    	tok = strtok(NULL, "\n");
    	if (tok != NULL) {
    		ESP_LOGI(TAG, "sending, path = %s, val = %s", path, tok);
    		ret = host_cmds_send_settings_val_set_to_host(path, tok);
    	}
    }
//    if (ret == 0)
//    	httpd_resp_sendstr(req, "Post control value successfully");
//    else
//    	httpd_resp_sendstr(req, "Post control value failed");

    /* reload home page */
    if (ret == HOST_CMD_STAT_OK) {
//    	httpd_resp_sendstr(req, "<h3>SUCCESS</h3>"BACK);
//    	ret = send_get_cmd_to_host(m_curr_path);
//    	httpd_resp_send(req, m_html_buf, HTTPD_RESP_USE_STRLEN);
    	char html_cmd[256] = {0x00};
    	strcat(html_cmd, HTML_META_REDIRECT_START);
    	strcat(html_cmd, HTTP_REDIRECT_URL_HOME);
    	strcat(html_cmd, HTML_META_REDIRECT_END);
    	ESP_LOGW(TAG, "HTML = %s", html_cmd);
    	httpd_resp_sendstr(req, html_cmd);
    } else if (ret == HOST_CMD_STAT_CONT) {
//    	httpd_resp_sendstr(req, "<h3>Loading ...</h3>");
    	while (!m_redirect) {
    		vTaskDelay(250 / portTICK_PERIOD_MS);
    	}

    	if (m_redirect == APP_HTTP_SERVER_REDIRECT_TYPE_URL) {
			char html_cmd[256] = {0x00};
			strcat(html_cmd, HTML_META_REDIRECT_START);
			strcat(html_cmd, m_continue_url);
			strcat(html_cmd, HTML_META_REDIRECT_END);
			ESP_LOGW(TAG, "HTML = %s", html_cmd);
			httpd_resp_sendstr(req, html_cmd);
    	} else if (m_redirect == APP_HTTP_SERVER_REDIRECT_TYPE_TEXT) {
    		httpd_resp_sendstr(req, m_continue_text);
    	}

    	m_redirect = APP_HTTP_SERVER_REDIRECT_NONE;
    }
    else {
//    	httpd_resp_sendstr(req, "<h3>FAILED</h3>"BACK);
    	httpd_resp_sendstr(req, HTTP_REDIRECT_PAGE_FAIL);
    }

    return ESP_OK;
}

httpd_uri_t post_uri = {
    .uri = "/p",
    .method = HTTP_POST,
    .handler = post_uri_handler,
    .user_ctx = NULL
};

static esp_err_t scan_uri_handler(httpd_req_t *req)
{
	/* set status of scan url being processed */
	m_wifi_scan_stat = 1;
	/* when a scan request is received take the following steps */

	/* 1. do a wifi scan */
	wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
	uint16_t ap_count = comm_wifi_scan(ap_info);

	/* 2. send a wifi SSID command to the host */
	int ret = host_cmds_send_send_ssid_to_host(ap_info, ap_count);

	/* 2. wait for redirect url cmd from */
	if (ret == HOST_CMD_STAT_OK) {
		//    	httpd_resp_sendstr(req, "<h3>Loading ...</h3>");
		while (!m_redirect) {
			vTaskDelay(250 / portTICK_PERIOD_MS);
		}
		m_redirect = APP_HTTP_SERVER_REDIRECT_NONE;

		char html_cmd[256] = { 0x00 };
		strcat(html_cmd, HTML_META_REDIRECT_START);
		strcat(html_cmd, m_continue_url);
		strcat(html_cmd, HTML_META_REDIRECT_END);
		ESP_LOGW(TAG, "HTML = %s", html_cmd);
		httpd_resp_sendstr(req, html_cmd);
	}

	m_wifi_scan_stat = 0;
	return ESP_OK;
}

httpd_uri_t scan_uri = {
    .uri = "/scan",
    .method = HTTP_POST,
    .handler = scan_uri_handler,
    .user_ctx = NULL
};

static esp_err_t get_uri_handler(httpd_req_t *req)
{
	memset(m_buf, 0x00, SCRATCH_BUFSIZE);

	int qlen = httpd_req_get_url_query_len(req);
	ESP_LOGI(TAG, "url len = %d", qlen);
	esp_err_t ret = httpd_req_get_url_query_str(req, m_buf, SCRATCH_BUFSIZE);
	if (ret == ESP_OK) {
		ESP_LOGI(TAG, "url query = %s", m_buf);
		char *tok = strtok(m_buf, "=");	// tok should contain the string "pa"
		if (strcmp(tok, "pa") == 0) {
			tok = strtok(NULL, "=");	// tok should contain the path
			m_stream_stat = STREAM_FRAME_START;		// reset status

			if (strcmp(tok, SETTINGS_PATH_SSID) == 0) {
				/* scan for ssids and send to host */
				wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
				uint16_t ap_count = comm_wifi_scan(ap_info);
				ret = host_cmds_send_send_ssid_to_host(ap_info, ap_count);
			}

			ret = send_get_cmd_to_host(tok);
//			httpd_resp_send(req, m_html_buf, HTTPD_RESP_USE_STRLEN);
			ret = httpd_resp_send(req, m_html_buf, m_hbufidx+1);
			if (ret != ESP_OK) {
				ESP_LOGE(TAG, "httpd_resp_send failed, err = %d", ret);
			}
		}
	} else {
		ESP_LOGE(TAG, "err = %d", ret);
		return ESP_FAIL;
	}

//    httpd_resp_sendstr(req, "Post control value successfully");
    return ret;
}

httpd_uri_t get_uri = {
    .uri = "/g",
    .method = HTTP_GET,
    .handler = get_uri_handler,
    .user_ctx = NULL
};

//#define HOME_HTML "<!DOCTYPE html><html><body><form action=\"/post_uri\" method=\"post\">  <label for=\"fname\">First name:</label>  <input type=\"text\" id=\"fname\" name=\"fname\"><br><br>  <label for=\"lname\">Last name:</label>  <input type=\"text\" id=\"lname\" name=\"lname\"><br><br>  <button type=\"submit\">Submit</button></form></body></html>"
static esp_err_t net_get_handler(httpd_req_t *req)
{
	int ret = 0;
	const char *type = "text/html";
	httpd_resp_set_type(req, type);

	m_stream_stat = STREAM_FRAME_START;		// reset status
	memset(m_curr_path, 0x00, sizeof(m_curr_path));
	strcpy(m_curr_path, NET_PATH);
	ret = send_get_cmd_to_host(m_curr_path);

//	const char *home_page_str = HOME_HTML;
	httpd_resp_send(req, m_html_buf, HTTPD_RESP_USE_STRLEN);

	return ret;
}

httpd_uri_t net = {
    .uri = "/net",
    .method = HTTP_GET,
    .handler = net_get_handler,
    .user_ctx = NULL
};

static esp_err_t home_get_handler(httpd_req_t *req)
{
	int ret = 0;
	const char *type = "text/html";
	httpd_resp_set_type(req, type);

	m_stream_stat = STREAM_FRAME_START;		// reset status
	memset(m_curr_path, 0x00, sizeof(m_curr_path));
	strcpy(m_curr_path, ROOT_PATH);
	ret = send_get_cmd_to_host(m_curr_path);

//	const char *home_page_str = HOME_HTML;
	httpd_resp_send(req, m_html_buf, HTTPD_RESP_USE_STRLEN);

	return ret;
}

httpd_uri_t home = {
    .uri = "/home",
    .method = HTTP_GET,
    .handler = home_get_handler,
    .user_ctx = NULL
};

int http_server_start(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &home);
        httpd_register_uri_handler(server, &net);
        httpd_register_uri_handler(server, &get_uri);
        httpd_register_uri_handler(server, &post_uri);
        m_server = server;
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Error starting server!");
    return ESP_FAIL;
}

void http_server_stop(void)
{
    // Stop the httpd server
    httpd_stop(m_server);
}

int http_server_init()
{
	int ret = 0;

	m_rcd.lock = xSemaphoreCreateMutex();
	m_rcd.resp_available = false;

	/* add callbacks to expected response of various commands */
	host_cmds_add_callback(&cb_html_page, cb_html_page_handler, C20X_M2M_CMD_NET_WS_HTML_PAGE_GET);

	return ret;
}
