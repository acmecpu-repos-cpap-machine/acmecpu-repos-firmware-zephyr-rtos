/*
 * Copyright (c) 2023 Acme CPU
 *
 * comm_wifi.c
 * Created on: 03-Mar-2023
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */


#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
//#include "esp_netif_net_stack.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "esp_wifi.h"

#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#if IP_NAPT
#include "lwip/lwip_napt.h"
#endif
#include "lwip/err.h"
#include "lwip/sys.h"

#include "app_http_server.h"

#include "comm_wifi.h"

#if (CONFIG_BOARD_C201 | CONFIG_BOARD_C204)
#define APP_WIFI_SSID      "C20X"
#elif (CONFIG_BOARD_E206)
#define APP_WIFI_SSID      "E20X"
#elif (CONFIG_BOARD_H205C)
#define APP_WIFI_SSID      "H20X"
#else
#define APP_WIFI_SSID      "myssid"
#endif

#define APP_WIFI_PASS      "abc123456"
#define APP_WIFI_CHANNEL   1
#define APP_MAX_STA_CONN   1

static const char *TAG = "comm_wifi";

static esp_netif_t *m_netif_softAP = NULL;
static esp_netif_t *m_netif_STA = NULL;

static int m_got_ip = 0;
static int m_wifi_sta_connection_stat = COMM_WIFI_STA_DISCONNECTED;

void print_mac(const unsigned char *mac) {
	ESP_LOGI(TAG, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}

static void print_auth_mode(int authmode)
{
    switch (authmode) {
    case WIFI_AUTH_OPEN:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_OPEN");
        break;
    case WIFI_AUTH_WEP:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WEP");
        break;
    case WIFI_AUTH_WPA_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA_PSK");
        break;
    case WIFI_AUTH_WPA2_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_PSK");
        break;
    case WIFI_AUTH_WPA_WPA2_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA_WPA2_PSK");
        break;
    case WIFI_AUTH_WPA2_ENTERPRISE:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_ENTERPRISE");
        break;
    case WIFI_AUTH_WPA3_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA3_PSK");
        break;
    case WIFI_AUTH_WPA2_WPA3_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_WPA3_PSK");
        break;
    default:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_UNKNOWN");
        break;
    }
}

static void print_cipher_type(int pairwise_cipher, int group_cipher)
{
    switch (pairwise_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    default:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }

    switch (group_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    default:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }
}

static void on_wifi_disconnect(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
	ESP_LOGI(TAG, "Wi-Fi disconnected");
	m_wifi_sta_connection_stat = COMM_WIFI_STA_DISCONNECTED;
//    ESP_LOGI(TAG, "Wi-Fi disconnected, trying to reconnect...");
//    esp_err_t err = esp_wifi_connect();
//    if (err == ESP_ERR_WIFI_NOT_STARTED) {
//        return;
//    }
//    ESP_ERROR_CHECK(err);
}

static esp_ip4_addr_t s_ip_addr;
static void on_got_ip(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "Got IPv4 event: Interface \"%s\" address: " IPSTR, esp_netif_get_desc(event->esp_netif), IP2STR(&event->ip_info.ip));
    memcpy(&s_ip_addr, &event->ip_info.ip, sizeof(s_ip_addr));
    m_got_ip = 1;
    m_wifi_sta_connection_stat = COMM_WIFI_STA_CONNECTED;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

int comm_wifi_connection_status_get()
{
	return m_wifi_sta_connection_stat;
}

/* Scan for access points, wifi should be in STA or AP+STA mode */
uint16_t comm_wifi_scan(wifi_ap_record_t *ap_info)
{
    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
//    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(wifi_ap_record_t)*number);

//    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
//    ESP_ERROR_CHECK(esp_wifi_start());
//    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
    int ret = esp_wifi_scan_start(NULL, true);
    if (ret != 0) {
    	ESP_LOGE(TAG, "esp_wifi_scan_start failed with code = %d", ret);
    }

    ESP_LOGI(TAG, "Max AP number ap_info can hold = %u", number);

//    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ret = esp_wifi_scan_get_ap_records(&number, ap_info);
    if (ret != 0) {
    	ESP_LOGE(TAG, "esp_wifi_scan_get_ap_records failed with code = %d", ret);
    }

//    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    /**
     * TODO: This API stopped working after updating to esp-idf-v5.2.1
     * This always returns ap_count as 0 (zero)
     * Modified the below line to work with both v4.3 and v5.2.1
     */
    ret = esp_wifi_scan_get_ap_num(&ap_count);
    if (ret != 0) {
    	ESP_LOGE(TAG, "esp_wifi_scan_get_ap_num failed with code = %d", ret);
    }
#if (CONFIG_IDF_V4_3)
    ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);
    for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++) {
        ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
        ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);
        print_auth_mode(ap_info[i].authmode);
        if (ap_info[i].authmode != WIFI_AUTH_WEP) {
            print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
        }
        ESP_LOGI(TAG, "Channel \t\t%d\n", ap_info[i].primary);
    }
#elif (CONFIG_IDF_V5_2_1)
    for (int i = 0; i < number; i++) {
        ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
        ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);
        print_auth_mode(ap_info[i].authmode);
        if (ap_info[i].authmode != WIFI_AUTH_WEP) {
            print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
        }
        ESP_LOGI(TAG, "Channel \t\t%d", ap_info[i].primary);
    }
    ap_count = number;
#endif
    return ap_count;
}

int comm_wifi_mode_get()
{
	wifi_mode_t wifi_mode = WIFI_MODE_NULL;
	esp_wifi_get_mode(&wifi_mode);
	return wifi_mode;
}

int comm_wifi_sta_ip_get(char *sta_ip, int *len)
{
#define WAIT_DELAY_MS		(250)
#define MAX_WAIT_FOR_IP		(1000 * 10)
	int delay=0;
	while (!m_got_ip) {
		vTaskDelay(WAIT_DELAY_MS / portTICK_PERIOD_MS);
		delay += WAIT_DELAY_MS;
		if (delay > MAX_WAIT_FOR_IP) {
			m_got_ip = 0;
			return -1;
			break;
		}
	}

	m_got_ip = 0;

    *len = sprintf(sta_ip, IPSTR, IP2STR(&s_ip_addr));
    return 0;
}

void comm_wifi_softap_ip_get(char *softap_ip, int *len)
{
	esp_netif_ip_info_t ipInfo;
	esp_err_t ret = esp_netif_get_ip_info(m_netif_softAP, &ipInfo);
//    esp_err_t ret = tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &ipInfo);
//    sprintf(softap_ip, "%x", ipInfo.ip.addr);
    *len = sprintf(softap_ip, IPSTR, IP2STR(&ipInfo.ip));
}

int comm_wifi_mac_get(char *mac, int if_type, bool pretty)
{
	uint8_t base_mac_addr[6] = {0};
    //Get base MAC address from EFUSE BLK0(default option)
    int ret = esp_efuse_mac_get_default(base_mac_addr);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get base MAC address from EFUSE BLK0. (%s)", esp_err_to_name(ret));
//        ESP_LOGE(TAG, "Aborting");
//        abort();
        return ret;
    } else {
        ESP_LOGI(TAG, "Base MAC Address read from EFUSE BLK0");
    }
    esp_read_mac(base_mac_addr, if_type/*ESP_MAC_WIFI_SOFTAP*/);
    print_mac(base_mac_addr);

//    uint8_t ap_mac[12] = {0x00};
    int i, j=0;
    for (i=0; i<6; i++) {
    	if (pretty) {
    		sprintf(mac+(i+j), "%02x-", base_mac_addr[i]);
    		j += 2;
    	} else {
        	sprintf(mac+(i+j), "%02x", base_mac_addr[i]);
        	j++;
    	}
    }
    if (pretty) {
    	int len = strlen(mac);
    	mac[len-1] = '\0';	// remove last : char
    }

    for (i=0; i<strlen(mac); i++) {
    	mac[i] = toupper(mac[i]);
    }

    return ret;
}

static esp_netif_t *wifi_init_softap(uint8_t* ssid_to_set)
{
	esp_netif_t *esp_netif_ap = esp_netif_create_default_wifi_ap();

    wifi_config_t wifi_ap_config = {
        .ap = {
//            .ssid = {ssid_to_set},
            .ssid_len = strlen((char*)ssid_to_set),
            .channel = APP_WIFI_CHANNEL,
            .password = APP_WIFI_PASS,
            .max_connection = APP_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = false,
            },
        },
    };
    memcpy(wifi_ap_config.ap.ssid, ssid_to_set, wifi_ap_config.ap.ssid_len);

    if (strlen(APP_WIFI_PASS) == 0) {
        wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));

//    ESP_LOGI(TAG_AP, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
//    		ssid, APP_WIFI_PASS, APP_WIFI_CHANNEL);

    return esp_netif_ap;
}

int comm_wifi_softap_and_sta_init(void)
{
	esp_err_t ret = ESP_OK;

	/* Register Event handler */
    ret = esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL);
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnect, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip, NULL));

    /*Initialize WiFi */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
    	ESP_LOGE(TAG, "esp_wifi_init failed");
    	return ret;
    }

    ret |= esp_wifi_set_mode(WIFI_MODE_APSTA/*WIFI_MODE_AP*/);


//    m_netif_softAP = esp_netif_create_default_wifi_ap();	// wifi access point net interface
	m_netif_STA = esp_netif_create_default_wifi_sta();		// wifi station mode net interface

    /* set the ssid by concatenating the softAP mac */
    char ap_mac[13] = {0x00};
    ret = comm_wifi_mac_get(ap_mac, ESP_MAC_WIFI_SOFTAP, false);

    uint8_t ssid[32] = {0x00};
    strcpy((char*)ssid, APP_WIFI_SSID);
    strncat((char*) ssid, "_", 2);
    strncat((char*) ssid, (const char*)ap_mac, 12);

    /* Initialize AP */
    m_netif_softAP = wifi_init_softap(ssid);

    /* initialize STA */
//    m_netif_STA = esp_netif_create_default_wifi_sta();
#if 0
    wifi_config_t wifi_config = {
        .ap = {
//            .ssid = ssid,
//            .ssid_len = strlen(ssid),
            .channel = APP_WIFI_CHANNEL,
            .password = APP_WIFI_PASS,
            .max_connection = APP_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    wifi_config.ap.ssid_len = strlen((char*)ssid);
    memcpy(wifi_config.ap.ssid, ssid, wifi_config.ap.ssid_len);

    if (strlen(APP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ret |= esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config);
#endif

    ret |= esp_wifi_start();

    if (ret == ESP_OK)
    	ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
    			ssid, APP_WIFI_PASS, APP_WIFI_CHANNEL);

    /* start the http server */
    ret |= http_server_start();
    if (ret == ESP_OK)
    	ESP_LOGI(TAG, "http server started successfully");
    else
    	ESP_LOGE(TAG, "could not start http server");

    /* TODO: done temporarily, remove this */
//    comm_wifi_scan();

    return ret;
}

int comm_wifi_softap_deinit(void)
{
	esp_err_t ret = ESP_OK;
	ret = esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler);
	ret |= esp_wifi_stop();
	ret |= esp_wifi_deinit();
	esp_netif_destroy(m_netif_softAP);
	esp_netif_destroy(m_netif_STA);

	/* stop the http server */
	http_server_stop();

	return ret;
}

int comm_wifi_sta_connect(char *ssid, char *password)
{
	int ret = 0;
	wifi_config_t wifi_config = {
			.sta = {
					.ssid = "",
					.password = "",
					.scan_method = WIFI_ALL_CHANNEL_SCAN,
			},
	};
	memcpy((char*) wifi_config.sta.ssid, ssid, strnlen(ssid, sizeof(wifi_config.sta.ssid)));
	memcpy((char*) wifi_config.sta.password, password, strnlen(password, sizeof(wifi_config.sta.password)));

	ESP_LOGW(TAG, "Connecting to %s..., Channel %d, interval %d",
			wifi_config.sta.ssid, wifi_config.sta.channel,
			wifi_config.sta.listen_interval);
//	ESP_LOGI(TAG, "Connecting to %s...", wifi_config.sta.ssid);
	ESP_LOGI(TAG, "Connecting to ssid \"%s\" with pwd \"%s\"",
			wifi_config.sta.ssid, wifi_config.sta.password);

	ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
	ret |= esp_wifi_start();
	ret |= esp_wifi_connect();
	if (ret != 0) {
		ESP_LOGE(TAG, "comm_wifi_sta_connect failed!");
	}
	return ret;
}

int comm_wifi_sta_disconnect()
{
	int ret = esp_wifi_disconnect();
	if (ret == ESP_OK) {
		while (m_wifi_sta_connection_stat != COMM_WIFI_STA_DISCONNECTED) {
			vTaskDelay(100 / portTICK_PERIOD_MS);
		}
	}
	return ret;
}

int comm_wifi_init()
{
	esp_err_t ret = ESP_OK;

	ret = esp_netif_init();
	ret |= esp_event_loop_create_default();

	ret |= http_server_init();

	return ret;
}


