/*
 * Copyright (c) 2021 Acme CPU
 *
 * host_cmds.h
 * Created on: 20-Apr-2021
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef COMPONENTS_HOST_CMDS_INCLUDE_HOST_CMDS_H_
#define COMPONENTS_HOST_CMDS_INCLUDE_HOST_CMDS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "comm_wifi.h"

typedef enum {
	HOST_CMD_STAT_OK=0,
	HOST_CMD_STAT_ERR,
	HOST_CMD_STAT_CONT,

	HOST_CMD_STAT_MAX
} HOST_CMD_STAT;

/* */
#define CMD_END_CHARS		"\n"
#define CMD_RESP_DELIM		"\r\n"

/* Command responses */
#define CMD_RESP_OK			"OK"
#define CMD_RESP_ERR		"ERR"

/* General commands */
#define CMD_ACPU			"acpu"				/* Test command to check connectivity */

/* Device Info commands */
#define CMD_VERSION			"version"			/* Get host processor's kernel version*/

/* Blower commands */
#define BLOWER_ON	0x01
#define BLOWER_OFF	0x00
#define CMD_BLOWER_STATUS_GET		"blower status_get"
#define CMD_BLOWER_START			"blower start"
#define CMD_BLOWER_STOP				"blower stop"
#define CMD_BLOWER_SET_VOLTAGE		"blower set_voltage"
#define CMD_BLOWER_SET_DUTY			"blower set_duty"
#define CMD_BLOWER_GET_VOLTS_MV		"blower get_volts_mv"
#define CMD_BLOWER_GET_SPEED_HZ		"blower get_speed_hz"
#define CMD_BLOWER_GET_SPEED_RPM	"blower get_speed_rpm"

/* Battery commands */
#define CMD_BATTERY_LEVEL_GET		"battery level_get"	/* Get the battery level in percentage */

/* Device Firmware Upgrade (DFU) commands */
#define DFU_START		0x01
#define DFU_STOP		0x00
#define CMD_DFU_START				"dfu start"			/* Start the DFU process */
#define CMD_DFU_STOP				"dfu stop"			/* Stop the DFU process */

/* Stepper commands */
#define STEPPER_DIR_CLOCKWISE		0
#define STEPPER_DIR_ANTICLOCKWISE	1
#define CMD_STEPPER_DIR_SET			"stepper dir_set"
#define CMD_STEPPER_DIR_GET			"stepper dir_get"
#define CMD_STEPPER_SPEED_HZ_SET	"stepper speed_hz_set"
#define CMD_STEPPER_SPEED_HZ_GET	"stepper speed_hz_get"
#define CMD_STEPPER_NUM_ROT_SET		"stepper num_rot_set"
#define CMD_STEPPER_NUM_ROT_GET		"stepper num_rot_get"
#define CMD_STEPPER_POS_REL_SET		"stepper pos_rel_set"
#define CMD_STEPPER_POS_ABS_SET		"stepper pos_abs_set"
#define CMD_STEPPER_POS_CUR_GET		"stepper pos_cur_get"
#define CMD_STEPPER_ZERO_SET		"stepper zero_set"


/* Function declarations */
int host_cmds_init_verify_start();
int host_cmds_verify();
int host_cmds_send_only(const char *data, size_t len);
int host_cmds_recv_bytes(void* buf, uint32_t length, uint32_t ms_to_wait);
void host_cmds_delay(uint32_t ms_delay);

/* Device info commands */
int host_cmds_devinf_mfr_name_read(char *read_buf, size_t buf_size);
int host_cmds_devinf_model_num_read(char *read_buf, size_t buf_size);
int host_cmds_devinf_serial_num_read(char *read_buf, size_t buf_size);
int host_cmds_devinf_fw_version_read(char *read_buf, size_t buf_size);
int host_cmds_devinf_hw_version_read(char *read_buf, size_t buf_size);
int host_cmds_devinf_sw_version_read(char *read_buf, size_t buf_size);

/* Blower commands */
int host_cmds_blower_status_write(uint8_t status);
int host_cmds_blower_status_read();
int host_cmds_blower_voltage_write(uint32_t milli_volts);
int host_cmds_blower_duty_write(uint32_t duty_percent);
int host_cmds_blower_voltage_read();
int host_cmds_blower_speed_hz_read();
int host_cmds_blower_speed_rpm_read();

/* Battery commands */
int host_cmds_battery_level_read(char *read_buf, size_t buf_size);

/* Device Firmware Upgrade (DFU) commands */
int host_cmds_dfu_status_read();
int host_cmds_dfu_status_write(uint8_t status);

/* Stepper commands */
int host_cmds_stepper_dir_write(uint8_t dir);
int host_cmds_stepper_dir_read();
int host_cmds_stepper_speed_hz_write(uint32_t speed_hz);
int host_cmds_stepper_speed_hz_read();
int host_cmds_stepper_num_rot_write(uint32_t num_rot);
int host_cmds_stepper_num_rot_read();
int host_cmds_stepper_pos_rel_write(uint16_t pos_rel);
int host_cmds_stepper_pos_abs_write(uint16_t pos_abs);
int host_cmds_stepper_pos_cur_read();
int host_cmds_stepper_zeroset_write(uint8_t zero_set);

/* Settings */
int host_cmds_settings_write(void *data, uint32_t len);
int host_cmds_settings_val_set(const char *path, const char *val);

/* Sensor */
int host_cmds_sensor_list_get();
int host_cmds_sensor_value_getone(uint8_t *sens_info /* channel no, sensor id (2 bytes fixed length) */);
int host_cmds_sensor_value_getall();

/* html server */
int host_cmds_html_server_page_get(const char *path);

/* common */
int host_cmds_send_settings_val_set_to_host(char *settings_path, char *settings_val);
int host_cmds_settings_init();

/* wifi */
int host_cmds_send_send_ssid_to_host(wifi_ap_record_t *ap_info, uint16_t ap_count);
int host_cmds_wifi_init();

/* net */
/**
 * @brief	Sends a stream response to the host processor.
 * 			If the data buffer to be sent is larger than the payload buffer,
 * 			then this function loops through the buffer and sends the stream
 * 			of data sequentially.
 * @param frame_type	UART_M2M_FRAME_STREAM_RESP or UART_M2M_FRAME_STREAM_RESP_ENDSTR
 * @param cmd			The command ID
 * @param data_buf		data buffer
 * @param data_len		data buffer length
 * @return
 * 		0 		Success
 *		-ENOMEM	Out of memory
 *		-EINVAL	Invalid input parameter
 */
int host_cmds_net_send_stream_resp(int frame_type, int cmd, char *data_buf, int data_len);

/**
 * @brief	Function to check received acknowledgement
 * @param frame	incomming acknowledgement frame
 */
void host_cmds_net_data_ack_handler(void *frame);

void host_cmds_net_total_payload_sent_reset();
int host_cmds_net_total_payload_sent_get();
int host_cmds_net_init();

/* fw_update */
/**
 * @brief	This function must be called after the firmware is fetched and
 * 			written to the correct partition by calling host_cmds_fw_update_netapp_get().
 * 			This function simply reboots the esp32 to boot the new firmware.
 */
void host_cmds_fw_update_do_update();
/**
 * @brief	Sends command to the app processor to send an updated firmware app.
 * @return
 * 	0		Success
 * 	-ve		Failure
 */
int host_cmds_fw_update_netapp_get();

/**
 * @brief	Initializes callbacks and semaphores for required for getting new
 * 			firmware from the app processor via m2m_comm protocol
 * @return
 * 	0		Success
 * 	-ve		Failure
 */
int host_cmds_fw_update_init();

#ifdef __cplusplus
}
#endif

#endif /* COMPONENTS_HOST_CMDS_INCLUDE_HOST_CMDS_H_ */
