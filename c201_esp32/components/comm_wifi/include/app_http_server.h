/*
 * Copyright (c) 2022 Acme CPU
 *
 * http_server.h
 * Created on: 26-Mar-2023
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef COMPONENTS_COMM_WIFI_HTTP_SERVER_H_
#define COMPONENTS_COMM_WIFI_HTTP_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SETTINGS_KEY_ROOT			"r"
#define SETTINGS_KET_NET			"r/ds/net"

#define SETTINGS_FULLPATH_LEN_MAX		32
#define SCRATCH_BUFSIZE (2*1024)
#define HTML_PAGE_BUFSIZE (8*1024)

#define HTML_META_REDIRECT_START	"<meta http-equiv=\"refresh\" content=\"1;url="
#define HTML_META_REDIRECT_END		"\" />"
#define HTML_BACK_TO_HOME			"<h3><a href=\"/home\">Back to settings >></a></h3>"

#define HTTP_REDIRECT_URL_HOME		"home"
#define HTTP_REDIRECT_URL_WSSID		"g?pa=r/ds/net/wssid"
#define HTTP_REDIRECT_URL_WPWD		"g?pa=r/ds/net/wpwd"
#define HTTP_REDIRECT_PAGE_FAIL		"<h3>FAILED</h3>"HTML_BACK_TO_HOME
#define HTTP_REDIRECT_PAGE_SUCCESS	"<h3>SUCCESS</h3>"HTML_BACK_TO_HOME

#define SETTINGS_PATH_WIFI        "r/ds/net/wi"
#define SETTINGS_PATH_SSID        "r/ds/net/wssid"

typedef enum {
	APP_HTTP_SERVER_REDIRECT_NONE = 0,
	APP_HTTP_SERVER_REDIRECT_TYPE_URL,
	APP_HTTP_SERVER_REDIRECT_TYPE_TEXT,
} APP_HTTP_SERVER_REDIRECT_TYPE;

void app_http_server_continue_url_update(const char *redirect_url);
void app_http_server_continue_text_update(const char *text);

int http_server_start(void);

void http_server_stop(void);

int http_server_init();

#ifdef __cplusplus
}
#endif

#endif /* COMPONENTS_COMM_WIFI_HTTP_SERVER_H_ */
