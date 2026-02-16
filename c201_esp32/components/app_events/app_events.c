/*
 * Copyright (c) 2021 Acme CPU
 *
 * app_events.c
 * Created on: 24-Jun-2021
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "string.h"

#include "gll.h"
#include "app_events.h"

#define TAG	"app_events"

#define EVENT_Q_LEN		CONFIG_APP_EVENT_QUEUE_LEN

/* static variables */
static gll_t *m_callbacks;	/* a list of callback objects */
static int m_pos = 0;		/* node position in the list */
static SemaphoreHandle_t m_mutex;	/* mutex for transmission of data */
//static QueueHandle_t m_event_q;
QueueHandle_t m_event_q;


static void app_event_task( void * pvParameters ) {
	APP_EVENT_TYPE event;
	while (1) {
		if ((xQueueReceive(m_event_q, &(event), portMAX_DELAY) == pdPASS)) {

			ESP_LOGW(TAG, "Firing callback for event = %d", event);
			/* fire callbacks to registered modules */
			app_events_fire_callback(m_callbacks, event);
		}
//		vTaskDelay(50 / portTICK_PERIOD_MS);
	}
}

int app_events_report_event(APP_EVENT_TYPE reported_event) {
	/* Put data into the queue */
	if (m_event_q != NULL) {
		if ( xQueueSend( m_event_q, (void *) &reported_event, ( TickType_t ) 10 ) != pdPASS) {
			ESP_LOGE(TAG, "app_events_report_event failed = %d", reported_event);
			return -1;
		}
	}
	ESP_LOGI(TAG, "Reported event = %d", reported_event);
	return 0;
}

int app_events_add_callback(struct app_events_callback *cb_data,
		app_events_callback_handler_t handler, APP_EVENT_TYPE event) {

	if (cb_data == NULL) {
		ESP_LOGE(TAG, "Incorrect data!");
		return ESP_FAIL;
	}

	if (handler == NULL) {
		ESP_LOGE(TAG, "Incorrect handler!");
		return ESP_FAIL;
	}

	cb_data->pos = m_pos++;
	cb_data->handler = handler;
	cb_data->event = event;

	app_events_manage_callback(m_callbacks, cb_data, true);

	return 0;
}

int app_events_remove_callback(struct app_events_callback *cb_data,
		app_events_callback_handler_t handler, APP_EVENT_TYPE event) {

	if (cb_data == NULL) {
		ESP_LOGE(TAG, "Incorrect data!");
		return ESP_FAIL;
	}

	if (handler == NULL) {
		ESP_LOGE(TAG, "Incorrect handler!");
		return ESP_FAIL;
	}

	cb_data->pos = m_pos++;
	cb_data->handler = handler;
	cb_data->event = event;

	app_events_manage_callback(m_callbacks, cb_data, false);

	return 0;
}

int app_events_init() {
	m_callbacks = gll_init();
	m_mutex = xSemaphoreCreateMutex();
	m_event_q = xQueueCreate(EVENT_Q_LEN, sizeof(APP_EVENT_TYPE));

	if ((m_callbacks == NULL) || (m_mutex == NULL) || (m_event_q == NULL)) {
		ESP_LOGE(TAG, "could not allocate resources!");
		return -1;
	}

	TaskHandle_t recv_task;
	BaseType_t xret = xTaskCreate(app_event_task, "app_event_task", APP_EVENTS_TASK_STACK, NULL, APP_EVENTS_TASK_PRIO, &recv_task);
	if (xret == pdPASS) {
		ESP_LOGI(TAG, "commands receive task created successfully");
		return 0;
	} else {
		ESP_LOGE(TAG, "commands receive task creation failed");
		return -1;
	}
}
