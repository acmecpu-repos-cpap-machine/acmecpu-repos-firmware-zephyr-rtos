/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 27-Mar-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_NET_APP_NET_HTML_GEN_H_
#define SRC_INCLUDE_APP_NET_APP_NET_HTML_GEN_H_

#include <stdint.h>
#include "app_settings/app_settings_data.h"

#define HTML_PAGE_SIZE_MAX			(4*1024)

/* ********************************************************
 * HTML tags and constants
 * ********************************************************/
#define HTTP_GET_URI				"/g"
#define HTTP_POST_URI				"/p"

#define HTML_PAGE_PREFIX 			"<!DOCTYPE html><html><body>"
#define HTML_PAGE_POSTFIX 			"</body></html>"

/* h tag */
#define HTML_HTAG_OPEN_1			"<h1>"
#define HTML_HTAG_CLOSE_1			"</h1>"
#define HTML_HTAG_OPEN_2			"<h2>"
#define HTML_HTAG_CLOSE_2			"</h2>"
#define HTML_HTAG_OPEN_3			"<h3>"
#define HTML_HTAG_CLOSE_3			"</h3>"
#define HTML_HTAG_OPEN_4			"<h4>"
#define HTML_HTAG_CLOSE_4			"</h4>"

#define HTML_LINE_BREAK				"<br>"

#define HTML_INPUT_START			"<input type="
#define HTML_INPUT_VALUE			"\" value=\""
#define HTML_INPUT_END				"\">"

#define HTML_READONLY				" readonly"

/* space */
#define HTML_REGULAR_SPACE			"&nbsp;"
#define HTML_TWO_SPACE				"&ensp;"
#define HTML_FOUR_SPACE				"&emsp;"

/* links */
#define HTML_LIST_ITEM_PREFIX		"<a href=\""HTTP_GET_URI"?pa="
#define HTML_LIST_ITEM_MIDFIX		"\">"
#define HTML_LIST_ITEM_POSTFIX		"</a>"

/* select list */
/* select name - <select name="r/ds/frs">	*/
#define HTML_SELECT_NAME_START		"<select name=\""
//#define HTML_SELECT_ID				"\" id=\""
#define HTML_SELECT_NAME_END		"\">"
#define HTML_SELECT_END				"</select>"

/* select option:
 * 		send value and display name = app_settings_value.key
 * 		e.g. <option value="Medium">Medium</option>
 */
#define HTML_SELECT_OPTION_START	"<option value=\""
#define HTML_SELECT_OPTION_SEP		"\">"
#define HTML_SELECT_OPTION_SELECTED	"<option selected value=\""
#define HTML_SELECT_OPTION_END		"</option>"

/* form and action */
#define HTML_FORM_ACTION_START		"<form action=\""HTTP_POST_URI"\" method=\"post\">"
#define HTML_FORM_BUTTON_END		HTML_INPUT_START"\"submit\" value=\"Submit\"></form>"

/* text input field */
#define HTML_TEXT_INPUT_FIELD_START		HTML_INPUT_START"\"text\" name=\""
#define HTML_PWD_INPUT_FIELD_START		HTML_INPUT_START"\"password\" name=\""
#define HTML_TEXT_INPUT_FIELD_VALUE		HTML_INPUT_VALUE
#define HTML_TEXT_INPUT_FIELD_END		HTML_INPUT_END

/* date and time input field */
#define HTML_DATE_INPUT_START		HTML_INPUT_START"\"date\" name=\""
#define HTML_DATE_INPUT_VALUE		HTML_INPUT_VALUE
#define HTML_DATE_INPUT_END			HTML_INPUT_END

#define HTML_TIME_INPUT_START		HTML_INPUT_START"\"time\" name=\""
#define HTML_TIME_INPUT_VALUE		HTML_INPUT_VALUE
#define HTML_TIME_INPUT_END			HTML_INPUT_END

/* end */


struct html_menu_data {
	struct app_settings_data const *settings_data;
	int16_t selected_idx;		// this gets populated once the user selects an option
//	menu_extra_func extra_func;
};

extern struct html_menu_data g_html_menu[SETTINGS_COUNT_MAX];

char* app_net_html_get(const char *path, uint32_t *html_len);

/**
 * @brief	This function should be called when a UART_M2M_FRAME_DATA_REQ is
 * 			received with command ID = C20X_M2M_CMD_NET_WS_HTML_PAGE_GET for
 * 			getting HTML page. This function starts a thread which generates
 * 			the HTML page and transmits it via the uart_m2m_comm interface to
 * 			the network processor. It uses the UART_M2M_FRAME_DATA_RESP to
 * 			transmit the html page and waits for UART_M2M_FRAME_DATA_ACK frames
 * 			to be received from the receiver.
 *
 * @param path[in]	This path whose html page is requested
 * @param cmd[in]	Incoming command id received along with the path
 * @return
 */
int app_net_html_gen_and_send(const char *path, uint32_t cmd);

#endif /* SRC_INCLUDE_APP_NET_APP_NET_HTML_GEN_H_ */
