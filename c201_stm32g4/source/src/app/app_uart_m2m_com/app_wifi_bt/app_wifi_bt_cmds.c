/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 23-May-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <zephyr/kernel.h>
#include <version.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_wifibt);

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "app_uart_m2m_com/app_uart_m2m_com.h"
//#include "app_uart_m2m_com/app_uart_m2m_com_frame.h"
#if (CONFIG_LIB_M2M_FRAME)
#include "lib_m2m_frame/lib_m2m_frame.h"
#endif
#include "app_uart_m2m_com/app_wifi_bt/app_wifi_bt_cmds.h"
#include "app_uart_m2m_com/c20x_m2m_cmds.h"
#include "app_uart_m2m_com/app_uart_m2m_callback.h"

#include "app_settings/app_settings.h"
#include "app_settings/app_settings_paths.h"
#include "app_settings/app_settings_value.h"

#if CONFIG_APP_BATTERY
#include "app_battery/app_battery.h"
#endif

#include "app_blower/app_blower.h"
#include "app_sensor/app_sensor.h"

#include "app_net/app_net.h"
#if (CONFIG_APP_HAS_HTML_GEN)
	#include "app_net/app_net_html_gen.h"
#endif
#include "app_dfu/app_dfu.h"

static struct k_mutex m_tx_mutex;
static volatile uint32_t m_last_seq = 0;	/* the last incomming packet sequence received */

static int data_response_ack_make_send(uint16_t cmd_id, struct m2m_frame_t *in_frame, struct m2m_frame_t *out_frame);

/*
static void frame_header_single_resp_make(struct m2m_frame_t *frame) {
	frame->sof = UART_M2M_START_OF_FRAME;
	frame->type = UART_M2M_FRAME_SINGLE_RESP;
	frame->sequence = 0;
}
*/

int app_wifi_bt_cmd_process(struct m2m_frame_t *in_frame, struct m2m_frame_t *out_frame)
{
	int ret = 0;

	if ((in_frame == NULL) || (out_frame == NULL)) {
		return -EINVAL;
	}

	if (in_frame->sof != UART_M2M_START_OF_FRAME) {
		LOG_ERR("Invalid SOF");
		return -EPROTO;
	}



	/* extract the command id */
	uint8_t *buf = (uint8_t *) calloc(1, in_frame->payload_len);
	if (buf == NULL) {
		return -ENOMEM;
	}
	memcpy(buf, in_frame->payload, in_frame->payload_len);

	char *tok = strtok(buf, ",\n");
	uint16_t cmd_id = atoi(tok);

//	free(buf);

	if (in_frame->type == UART_M2M_FRAME_SINGLE_REQ) {
		/* make a single response frame */
		lib_m2m_frame_header_single_resp_make(out_frame);

		switch (cmd_id) {
		case C20X_M2M_CMD_ID_COMM_CHK:
			tok = strtok(NULL, ",\n");
			if (strcmp(tok, M2M_CMD_PAYLOAD_GET_CHAR) == 0) {
//				frame_header_single_resp_make(out_frame);
				sprintf(out_frame->payload, "%d%s%s%s", C20X_M2M_CMD_ID_COMM_CHK,
						M2M_CMD_PAYLOAD_DELIM, M2M_CMD_RESP_OK,
						M2M_CMD_PAYLOAD_TERM);
				out_frame->payload_len = strlen(out_frame->payload);
			}
			break;
		case C20X_M2M_CMD_ID_BATT_LEVEL:
			tok = strtok(NULL, ",\n");
			if (strcmp(tok, M2M_CMD_PAYLOAD_GET_CHAR) == 0) {
				/* get the battery level */
				uint8_t batt_level=0;
#if CONFIG_APP_BATTERY
				ret = app_battery_level_get(&batt_level);
#endif
//				frame_header_single_resp_make(out_frame);
				sprintf(out_frame->payload, "%d%s%d%s",
						C20X_M2M_CMD_ID_BATT_LEVEL, M2M_CMD_PAYLOAD_DELIM,
						batt_level, M2M_CMD_PAYLOAD_TERM);
				out_frame->payload_len = strlen(out_frame->payload);

				ret = 0;
			}
			break;
		case C20X_M2M_CMD_DEVINFO_FWVER:
			tok = strtok(NULL, ",\n");
			if (strcmp(tok, M2M_CMD_PAYLOAD_GET_CHAR) == 0) {
//				frame_header_single_resp_make(out_frame);
				sprintf(out_frame->payload, "%d%s%s%s", C20X_M2M_CMD_DEVINFO_FWVER,
						M2M_CMD_PAYLOAD_DELIM, KERNEL_VERSION_STRING,
						M2M_CMD_PAYLOAD_TERM);
				out_frame->payload_len = strlen(out_frame->payload);
			}
			break;
		case C20X_M2M_CMD_DEVINFO_SWVER:
			break;
		case C20X_M2M_CMD_DEVINFO_APPVER:
			break;
		case C20X_M2M_CMD_ID_SETTINGS:
		{
			/* settings command will have sub-commands */
			tok = strtok(NULL, ",\n");
			cmd_id = atoi(tok);

			switch (cmd_id) {
			case C20X_M2M_CMD_BLOWER_STATE:
				tok = strtok(NULL, ",\n");
				if (strcmp(tok, M2M_CMD_PAYLOAD_GET_CHAR) == 0) {
					uint8_t blower_state=0;
//					app_blower_state_get(&blower_state);
#if CONFIG_APP_BLOWER
					blower_state = app_blower_run_state_get();
#endif
					sprintf(out_frame->payload, "%d%s%d%s%d%s",
							C20X_M2M_CMD_ID_SETTINGS, M2M_CMD_PAYLOAD_DELIM,
							C20X_M2M_CMD_BLOWER_STATE, M2M_CMD_PAYLOAD_DELIM,
							blower_state,
							M2M_CMD_PAYLOAD_TERM);
				} else {
					uint8_t state = atoi(tok);
					int ret = 0;
#if CONFIG_APP_BLOWER
					ret = app_blower_settings_change_state(state);
#endif
					sprintf(out_frame->payload, "%d%s%d%s%s%s",
							C20X_M2M_CMD_ID_SETTINGS, M2M_CMD_PAYLOAD_DELIM,
							C20X_M2M_CMD_BLOWER_STATE, M2M_CMD_PAYLOAD_DELIM,
							(!ret ? M2M_CMD_RESP_OK : M2M_CMD_RESP_ERR),
							M2M_CMD_PAYLOAD_TERM);
				}
				out_frame->payload_len = strlen(out_frame->payload);
				break;

			case C20X_M2M_CMD_BLOWER_SPEED_RPM:
				break;
			case C20X_M2M_CMD_STEPPER_RESET_POS:
				break;
			case C20X_M2M_CMD_STEPPER_STEP_SPEED_HZ:
				break;
			case C20X_M2M_CMD_PERS_BRGT:
				break;
			case C20X_M2M_CMD_PERS_SCN:
				break;
			case C20X_M2M_CMD_PERS_KPS:
				break;
			}
			break;
		}
		case C20X_M2M_CMD_ID_SENSOR_LIST:
		{
			tok = strtok(NULL, ",\n");
			if (strcmp(tok, M2M_CMD_PAYLOAD_GET_CHAR) == 0) {
				int sensor_count = 0;
				sys_slist_t *sens_list = app_sensor_info_get(&sensor_count);

				uint16_t val_sz = 1 + sensor_count*2;	/* sensor_count (1byte) + [sensor_count * {channel(1byte) id(1byte)}] */
				uint8_t *val = (uint8_t*)calloc(1, val_sz);
				if (val == NULL) {
					LOG_ERR("%s calloc failed", __func__);
					ret = -ENOMEM;
					goto err;
				}

				int i=0;

				/* copy sensor count to the 1st byte */
				val[i] = (uint8_t)sensor_count;
				i++;

				/* copy channel no. and id of all the sensors */
				struct sinfo *sen, *tmp;
				SYS_SLIST_FOR_EACH_CONTAINER_SAFE(sens_list, sen, tmp, node)
				{
					val[i] = sen->chan;
					i++;
					val[i] = sen->id;
					i++;
				}

				int wr = sprintf(out_frame->payload, "%d%s", C20X_M2M_CMD_ID_SENSOR_LIST, M2M_CMD_PAYLOAD_DELIM);
				memcpy(out_frame->payload + wr, val, val_sz);
				memcpy(out_frame->payload + wr + val_sz, M2M_CMD_PAYLOAD_TERM, 1);

				free(val);

				out_frame->payload_len = wr + val_sz + 1;
			}

			break;
		}
		case C20X_M2M_CMD_ID_SENSOR_GET:
		{
			tok = strtok(NULL, ",\n");
			if (tok == NULL) {
				ret = -EINVAL;
				break;
			}

			uint8_t channel=0, id=0;
			memcpy(&channel, tok, 1);
			memcpy(&id, tok+1, 1);

			struct sensor_value sens_val;
			ret = app_sensor_value_get(id, &sens_val);
			if (ret < 0) {
				ret = -EINVAL;
				break;
			}

			int wr = sprintf(out_frame->payload, "%d%s", C20X_M2M_CMD_ID_SENSOR_GET, M2M_CMD_PAYLOAD_DELIM);
			memcpy(out_frame->payload + wr, &sens_val, sizeof(struct sensor_value));
			memcpy(out_frame->payload + wr + sizeof(struct sensor_value), M2M_CMD_PAYLOAD_TERM, 1);

			out_frame->payload_len = wr + sizeof(struct sensor_value) + 1;

			break;
		}
		case C20X_M2M_CMD_ID_SENSOR_GETALL:
		{
			tok = strtok(NULL, ",\n");
			if (strcmp(tok, M2M_CMD_PAYLOAD_GET_CHAR) == 0) {
				int sensor_count = 0;
				sys_slist_t *sens_list = app_sensor_info_get(&sensor_count);

				/* Size of response calculation:
				 * 	sensor_count = 1 byte +
				 * 	channel & id = sensor_count * 2 --> channel(1byte) id(1byte)
				 * 	sensor value = sensor_count * 8 --> sizeof(struct sensor_value) is 8 bytes
				 * 	 */
				uint16_t val_sz = 1 + (sensor_count*2) + (sensor_count*8);
				uint8_t *val = (uint8_t*)calloc(1, val_sz);
				if (val == NULL) {
					LOG_ERR("%s calloc failed", __func__);
					ret = -ENOMEM;
					break;
				}

				int i=0;

				/* copy sensor count to the 1st byte */
				val[i] = (uint8_t)sensor_count;
				i++;

				/* iterate and copy channel no., id and sensor_value of all the sensors */
				struct sinfo *sen, *tmp;
				SYS_SLIST_FOR_EACH_CONTAINER_SAFE(sens_list, sen, tmp, node)
				{
					/* channel number, 1 byte */
					val[i] = sen->chan;
					i = i + 1;

					/* sensor id, 1 byte */
					val[i] = sen->id;
					i = i + 1;

					/* sensor value, 8 bytes */
					struct sensor_value sens_val;
					ret = app_sensor_value_get(sen->id, &sens_val);
					if (ret < 0) {
						free(val);
						ret = -EINVAL;
						goto err;
					}
					memcpy(val+i, &sens_val, sizeof(struct sensor_value));
					i = i + sizeof(struct sensor_value);
				}

				/* create the response buffer */
				int wr = sprintf(out_frame->payload, "%d%s", C20X_M2M_CMD_ID_SENSOR_GETALL, M2M_CMD_PAYLOAD_DELIM);
				memcpy(out_frame->payload + wr, val, val_sz);
				memcpy(out_frame->payload + wr + val_sz, M2M_CMD_PAYLOAD_TERM, 1);

				free(val);

				out_frame->payload_len = wr + val_sz + 1;
			}
			break;
		}
		case C20X_M2M_CMD_SETTINGS_VAL_SET:
		{
//			int payload_len = in_frame->payload_len;
			char path[SETTINGS_FULLPATH_LEN_MAX] = {0x00};
			tok = strtok(NULL, ",");	// settings path
			if (tok != NULL) {
				strcpy(path, tok);
//				int path_len = strlen(path);
				tok = strtok(NULL, "\n");	// settings val
				if (tok != NULL) {
//					int val_len = strlen(tok);

					/* get settings data idx */
					int idx = app_settings_array_idx_get(path);
					if (idx < 0) {
						ret = -1;
						break;
					}
					/* check datatype */
					int datatype = app_settings_datatype_get(idx);
					int displayable = app_settings_displayable_get(idx);
					struct app_settings_data const *asd = app_settings_data_obj_get(idx);

					if (datatype == SETTING_DATATYPE_SETTING_VALUE) {
						/* if setting val - get option idx from settings val
						 * and then store appropriate val */
						struct setting_value val;
						int opidx = app_settings_option_key_to_val(asd->options, tok, &val);
						if (opidx >= 0) {
							ret = app_settings_save_single(path, &val, sizeof(struct setting_value), true);
							if (!ret &&
									( (displayable == SETTING_DISP_MULTILEVEL) 		||
									  /*(displayable == SETTING_DISP_COND_WIFI_MULTI) ||*/
									  (displayable == SETTING_DISP_NO_MULTILEVEL)	)
									  ) {
								if ((strcmp(path, SETTINGS_KEY_FULL_DS_NET_WSSID) == 0)	||
										(strcmp(path, SETTINGS_KEY_FULL_DS_NET_WPWD) == 0)) {	// special case for wifi ssid, the display must continue to next screen, i.e. password
									ret = 1001;
								} else if (val.val1 != 0) {	// 0 indicates Off/No. If user is trying to turn something on, them the multilevel display should come into effect
									ret = 1001;
								}
							}
						}
						else
							ret = -1;
					} else if (datatype == SETTING_DATATYPE_STRING) {
						/* if string - store val */
						if (strlen(tok) <= asd->size) {
							char *str_val = (char*) calloc(1, asd->size);
							strcpy(str_val, tok);
							ret = app_settings_save_single(path, str_val, asd->size, true);
							free(str_val);
							if (!ret &&
									( (displayable == SETTING_DISP_MULTILEVEL) 		||
									  /*(displayable == SETTING_DISP_COND_WIFI_MULTI) ||*/
									  (displayable == SETTING_DISP_NO_MULTILEVEL)	)
									  ) {
//								if (strcmp(path, SETTINGS_KEY_FULL_DS_NET_WPWD) == 0) {	// special case for wifi ssid, the display must continue to next screen, i.e. password
									ret = 1001;
//								}
							}
						} else {
							ret = -1;
						}
					} else if (datatype == SETTING_DATATYPE_DATE) {
						/* expecting date fromat yyyy-mm-dd */
						char date[12] = {0x00};
						strcpy(date, tok);

						LOG_INF("DATE = %s", date);
						struct setting_value val;

						char *tok = strtok(date, "-");	// year
						val.val1 = atoi(tok);
						ret = app_settings_save_single(SETTINGS_KEY_FULL_DS_DAT_YR, &val, sizeof (struct setting_value), true);

						tok = strtok(NULL, "-");	// mon
						val.val1 = atoi(tok);
						ret |= app_settings_save_single(SETTINGS_KEY_FULL_DS_DAT_MON, &val, sizeof (struct setting_value), true);

						tok = strtok(NULL, "-");	// day
						val.val1 = atoi(tok);
						ret |= app_settings_save_single(SETTINGS_KEY_FULL_DS_DAT_DAY, &val, sizeof (struct setting_value), true);

					} else if (datatype == SETTING_DATATYPE_TIME) {
						/* expecting date fromat hr:mn */
						char time[6] = {0x00};
						strcpy(time, tok);

						LOG_INF("TIME = %s", tok);
						struct setting_value val;

						char *tok = strtok(time, ":");	// hr
						val.val1 = atoi(tok);
						ret = app_settings_save_single(SETTINGS_KEY_FULL_DS_TIM_HR, &val, sizeof (struct setting_value), true);

						tok = strtok(NULL, "-");	// min
						val.val1 = atoi(tok);
						ret |= app_settings_save_single(SETTINGS_KEY_FULL_DS_TIM_MIN, &val, sizeof (struct setting_value), true);
					} else if (datatype == SETTING_DATATYPE_UINT32) {
						/* if uint32 - convert to integer */
						if (tok != NULL) {
							uint32_t val_i = atoi(tok);

							/* validate the value */
							if (strcmp(path, SETTINGS_KEY_FULL_DEV_BRPM) == 0) {
#if CONFIG_APP_BLOWER
								ret = app_blower_speed_rpm_change(val_i);
#endif
							} else if (strcmp(path, SETTINGS_KEY_FULL_DEV_BRAMP) == 0) {
#if CONFIG_APP_BLOWER
								ret = app_blower_ramp_ms_set(val_i);
#endif
							}

							if (!ret)
								ret = app_settings_save_single_with_retry(path, &val_i, asd->size, 10, true);
						}
					}
				} else {
					ret = -1;
				}
			} else {
				ret = -1;
			}

			/* create a response buffer */
			if (ret <= 0) {
				sprintf(out_frame->payload, "%d%s%s%s", C20X_M2M_CMD_SETTINGS_VAL_SET, M2M_CMD_PAYLOAD_DELIM,
						(ret ? M2M_CMD_RESP_ERR : M2M_CMD_RESP_OK), M2M_CMD_PAYLOAD_TERM);
			} else if (ret == 1001) {	/* special case required for multi level display */
				sprintf(out_frame->payload, "%d%s%s%s", C20X_M2M_CMD_SETTINGS_VAL_SET, M2M_CMD_PAYLOAD_DELIM,
						M2M_CMD_RESP_CONT, M2M_CMD_PAYLOAD_TERM);
				ret = 0;
			}
			out_frame->payload_len = strlen(out_frame->payload);
		}
			break;
		case C20X_M2M_CMD_NET_WIFI_SCANNED_LIST:
		{
			tok = strtok(NULL, "\n");	// extract the ssid list char array
			if (tok != NULL) {
				sys_slist_t* ssid_list = app_net_parse_ssid_list(tok);
				/* populate the ssid option list */
				int idx = app_settings_array_idx_get(SETTINGS_KEY_FULL_DS_NET_WSSID);
				struct app_settings_data const *asd = app_settings_data_obj_get(idx);
				struct setting_value_options *options = asd->options;

				if (options != NULL) {
					if (options->num_options > 0) {	// delete the old ssid options
						for (int i=0; i<options->num_options; i++) {
							char *name = (char *)options->op_val[i].key;
							memset(name, 0x00, sizeof(options->op_val[i].key));
						}
					}

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

				ret = 0;
			} else {
				ret = -1;
			}
			sprintf(out_frame->payload, "%d%s%s%s", C20X_M2M_CMD_NET_WIFI_SCANNED_LIST, M2M_CMD_PAYLOAD_DELIM,
					(ret ? M2M_CMD_RESP_ERR : M2M_CMD_RESP_OK), M2M_CMD_PAYLOAD_TERM);
			out_frame->payload_len = strlen(out_frame->payload);
		}
			break;
		default:
			break;
		}

		/* response payload length */
//		out_frame->payload_len = strlen(out_frame->payload);

	} else if (in_frame->type == UART_M2M_FRAME_SINGLE_RESP) {

		/* fire callback */
		app_uart_m2m_fire_callbacks(app_uart_m2m_callback_get(), cmd_id, in_frame);
		ret = UART_M2M_FRAME_SINGLE_RESP;

	} else if (in_frame->type == UART_M2M_FRAME_STREAM_REQ) {

		switch (cmd_id) {
		case C20X_M2M_CMD_NET_WS_HTML_PAGE_GET: {
			tok = strtok(NULL, ",\n");
			if (tok == NULL) {
				ret = -1;
				break;
			}
			char path[SETTINGS_FULLPATH_LEN_MAX] = { 0x00 };
			strcpy(path, tok);
			LOG_INF("fetch html = %s", path);

			/* make a stream response frame header */
			lib_m2m_frame_header_stream_resp_make(out_frame);

			uint32_t html_len;
			char *html = NULL;
				/* make html data */
#if (CONFIG_APP_HAS_HTML_GEN)
			html = app_net_html_get(path, &html_len);
#endif
			char cmd_buf[10] = {0x00};
			int cmd_len = sprintf(cmd_buf, "%d%s", C20X_M2M_CMD_NET_WS_HTML_PAGE_GET, M2M_CMD_PAYLOAD_DELIM);

			int cpy_len = UART_M2M_PAYLOAD_SIZE_MAX-cmd_len;
			int idx=0, seq=1, send_stat=0;
			while (1) {

				if ((html_len - idx) < cpy_len) {
					cpy_len = (html_len - idx);
				}

				/* make frame */
				memset(out_frame->payload, 0x00, UART_M2M_PAYLOAD_SIZE_MAX);
				sprintf(out_frame->payload, "%s", cmd_buf);
				memcpy(out_frame->payload+cmd_len, html+idx, cpy_len);
				out_frame->payload_len = cmd_len + cpy_len;
				out_frame->sequence = seq++;
#if 0
				/* serialize and send frame */
				/* buffer size = frame header size + pay load size + 1 NULL char */
				uint32_t sbuf_len = UART_M2M_HEADER_SIZE_MAX + out_frame->payload_len + 1;
				uint32_t sdata_len = 0;
				char *ser_buf = (char*) calloc(1, sbuf_len);
				if (ser_buf == NULL) {
					LOG_ERR("%s calloc failed!", (__func__));
					free(ser_buf);
					break;
				}
				lib_m2m_frame_serialize(ser_buf, sbuf_len, out_frame, &sdata_len);
#endif
				/* compute checksum */
				ret = lib_m2m_frame_checksum_compute(out_frame);

				/* serialize the frame and send */
				size_t sdata_len=0;
				char *ser_buf = lib_m2m_frame_alloc_serialize(out_frame, &sdata_len);
				if (ser_buf == NULL) {
					LOG_ERR("%s calloc failed!", (__func__));
					free(ser_buf);
					break;;
				}

//				int64_t start, delta = 0;
//				start = k_uptime_get();
//				app_uart_m2m_send(UART_M2M_APP_ID_WIFI_BT, ser_buf, sdata_len);
				app_wifi_bt_cmd_send(ser_buf, sdata_len);
//				delta = k_uptime_delta(&start);
//				LOG_INF("app_uart_m2m_send time: %lld", delta);

				LOG_HEXDUMP_DBG(ser_buf, sdata_len, "ser_buf");

				/* free serial buffer */
				free(ser_buf);

				/* increment i and check break condition */
				idx = idx + cpy_len;
				if (idx >= html_len) {
					send_stat = 1;
					break;
				}
			}
			free(html);

			if (send_stat == 1) {
				/* make a single response frame */
				lib_m2m_frame_header_single_resp_make(out_frame);

				/* end of data, send stream end frame */
//				sprintf(out_frame->payload, "%d%s%s%s",
//				C20X_M2M_CMD_NET_WS_HTML_PAGE_GET, M2M_CMD_PAYLOAD_DELIM,
//				M2M_CMD_RESP_ERR, M2M_CMD_PAYLOAD_TERM);
				memset(out_frame->payload, 0x00, UART_M2M_PAYLOAD_SIZE_MAX);
				sprintf(out_frame->payload, "%d%s%s%s",
						C20X_M2M_CMD_NET_WS_HTML_PAGE_GET, M2M_CMD_PAYLOAD_DELIM,
						M2M_CMD_RESP_STREND, M2M_CMD_PAYLOAD_TERM);
				out_frame->payload_len = strlen(out_frame->payload);
			}
		}
			break;
		}

		ret = UART_M2M_FRAME_STREAM_REQ;

	} else if (in_frame->type == UART_M2M_FRAME_STREAM_RESP) {

		app_uart_m2m_fire_callbacks(app_uart_m2m_callback_get(), cmd_id, in_frame);
		ret = UART_M2M_FRAME_STREAM_RESP;

	} else if (in_frame->type == UART_M2M_FRAME_DATA_REQ) {		// received data request frame

		switch (cmd_id) {
		case C20X_M2M_CMD_NET_WS_HTML_PAGE_GET:
		{
			tok = strtok(NULL, ",\n");
			if (tok == NULL) {
				ret = -1;
				break;
			}
			char path[SETTINGS_FULLPATH_LEN_MAX] = { 0x00 };
			strcpy(path, tok);
			LOG_INF("fetch html = %s", path);

			/* generate and send the html page */
			app_net_html_gen_and_send(path, cmd_id);
		}
			break;
		case C20X_M2M_CMD_NET_FWAPP_GET:
		{
			/* send the network processor's firmware app binary in chunks */
#if (CONFIG_BOARD_E206 || CONFIG_BOARD_C208T)
			app_dfu_netproc_fw_send(cmd_id);
#endif
			break;
		}
		}

		ret = UART_M2M_FRAME_DATA_REQ;

	} else if (in_frame->type == UART_M2M_FRAME_DATA_RESP) {	// received data response frame

		/* make and send the acknowledgment frame */
		ret = data_response_ack_make_send(cmd_id, in_frame, out_frame);
		if (ret == 0) {	// ok
			/* call the application callback to process the in_frame */
			app_uart_m2m_fire_callbacks(app_uart_m2m_callback_get(), cmd_id, in_frame);
			ret = UART_M2M_FRAME_DATA_RESP;
		} else if (ret == -EBADF) {	// duplicate frame
			ret = UART_M2M_FRAME_DATA_RESP;
		} else if (ret < 0) {		// error
			LOG_ERR("data_response_ack_make_send failed!");
			goto err;
		}

	} else if (in_frame->type == UART_M2M_FRAME_DATA_ACK) {		// received data acknowledge frame

		app_uart_m2m_fire_callbacks(app_uart_m2m_callback_get(), cmd_id, in_frame);
		ret = UART_M2M_FRAME_DATA_ACK;

	} else if (in_frame->type == UART_M2M_FRAME_DATA_RESP_ENDSTR) {		// received data response end frame

		/* make and send the acknowledgment frame */
		ret = data_response_ack_make_send(cmd_id, in_frame, out_frame);
		if (ret == 0) {	// ok
			/* call the application callback to process the in_frame */
			app_uart_m2m_fire_callbacks(app_uart_m2m_callback_get(), cmd_id, in_frame);
			ret = UART_M2M_FRAME_DATA_RESP_ENDSTR;
			m_last_seq = 0;		// reset the sequence variable
		} else if (ret == -EBADF) {	// duplicate frame
			ret = UART_M2M_FRAME_DATA_RESP_ENDSTR;
		} else if (ret < 0) {		// error
			LOG_ERR("data_response_ack_make_send failed!");
			goto err;
		}
	} else {
		ret = -1;
	}

err:
	free(buf);

	return ret;
}

static int data_response_ack_make_send(uint16_t cmd_id, struct m2m_frame_t *in_frame, struct m2m_frame_t *out_frame)
{
	int ret = 0;
	/* make the acknowledgment frame */
	lib_m2m_frame_header_data_ack_make(out_frame, in_frame->sequence);

	memset(out_frame->payload, 0x00, UART_M2M_PAYLOAD_SIZE_MAX);
	sprintf(out_frame->payload, "%d%s%s%s", cmd_id, M2M_CMD_PAYLOAD_DELIM,
			M2M_CMD_RESP_OK, M2M_CMD_PAYLOAD_TERM);
	out_frame->payload_len = strlen(out_frame->payload);

	/* compute checksum */
	ret = lib_m2m_frame_checksum_compute(out_frame);
	if (ret < 0) goto err;

	/* send acknowledgment frame */
	size_t sdata_len=0;
	char *serialized_buffer = lib_m2m_frame_alloc_serialize(out_frame, &sdata_len);
	if (serialized_buffer == NULL) {
		ret = -1;
		goto err;
	}
	app_wifi_bt_cmd_send(serialized_buffer, sdata_len);

	/* free memory */
	free(serialized_buffer);

	/* check if this was a duplicate packet */
	if (m_last_seq == in_frame->sequence) {
		LOG_WRN("duplicate packet, ignoring");
		/* we should discard the duplicate packet's data but must acknowledge
		 * the packet so that the sender does not re-transmit the same packet again */
		ret = -EBADF;
	}
	m_last_seq = in_frame->sequence;
	LOG_DBG("m_last_seq = %d", m_last_seq);

err:
	return ret;
}

int app_wifi_bt_cmd_send(void *data, size_t len)
{
	int ret = 0;
	LOG_HEXDUMP_DBG(data, len, "WIFI_BT_TX");

	k_mutex_lock(&m_tx_mutex, K_FOREVER);
	ret = app_uart_m2m_send(UART_M2M_APP_ID_WIFI_BT, data, len);
	k_mutex_unlock(&m_tx_mutex);

	return ret;
}

void app_wifi_bt_cmd_init()
{
	/* initialize the Tx mutex */
	k_mutex_init(&m_tx_mutex);
}
