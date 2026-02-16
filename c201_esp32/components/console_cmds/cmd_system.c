/*
 * Console example — various system commands
 * Compatible with ESP-IDF v5.x
 *
 * This example code is in the Public Domain (or CC0 licensed, at your option.)
 * Distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

#include "esp_log.h"
#include "esp_console.h"
#include "esp_system.h"
#include "esp_sleep.h"
#include "esp_spi_flash.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "driver/uart.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "cmd_system.h"

static const char *TAG = "cmd_system";

#ifdef CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS
#define WITH_TASKS_INFO 1
#endif

/* ---------- Forward Declarations ---------- */
static void register_free(void);
static void register_heap(void);
static void register_version(void);
static void register_restart(void);
static void register_deep_sleep(void);
static void register_light_sleep(void);
#if WITH_TASKS_INFO
static void register_tasks(void);
#endif

/* ---------- Command Registration ---------- */

void register_system_common(void)
{
    register_free();
    register_heap();
    register_version();
    register_restart();
#if WITH_TASKS_INFO
    register_tasks();
#endif
}

void register_system_sleep(void)
{
    register_deep_sleep();
    register_light_sleep();
}

void register_system(void)
{
    register_system_common();
    register_system_sleep();
}

/* ---------- Version Command ---------- */

static int get_version(int argc, char **argv)
{
    esp_chip_info_t info;
    esp_chip_info(&info);

    uint32_t size_flash_chip;
    esp_flash_get_size(NULL, &size_flash_chip);

    printf("IDF Version: %s\n", esp_get_idf_version());
    printf("Chip info:\n");
    printf("\tmodel: %s\n", info.model == CHIP_ESP32 ? "ESP32" : "Unknown");
    printf("\tcores: %d\n", info.cores);
    printf("\tfeatures: %s%s%s%s%lu MB\n",
           info.features & CHIP_FEATURE_WIFI_BGN ? "/802.11bgn" : "",
           info.features & CHIP_FEATURE_BLE ? "/BLE" : "",
           info.features & CHIP_FEATURE_BT ? "/BT" : "",
           info.features & CHIP_FEATURE_EMB_FLASH ? "/Embedded-Flash:" : "/External-Flash:",
           (unsigned long)(size_flash_chip / (1024 * 1024)));
    printf("\trevision number: %d\n", info.revision);
    return 0;
}

static void register_version(void)
{
    const esp_console_cmd_t cmd = {
        .command = "version",
        .help = "Get version of chip and SDK",
        .func = &get_version,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/* ---------- Restart Command ---------- */

static int restart(int argc, char **argv)
{
    ESP_LOGI(TAG, "Restarting system...");
    esp_restart();
    return 0;
}

static void register_restart(void)
{
    const esp_console_cmd_t cmd = {
        .command = "restart",
        .help = "Software reset of the chip",
        .func = &restart,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/* ---------- Free Heap Command ---------- */

static int free_mem(int argc, char **argv)
{
    printf("%lu\n", (unsigned long)esp_get_free_heap_size());
    return 0;
}

static void register_free(void)
{
    const esp_console_cmd_t cmd = {
        .command = "free",
        .help = "Get current size of free heap memory",
        .func = &free_mem,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/* ---------- Heap Min Command ---------- */

static int heap_size(int argc, char **argv)
{
    uint32_t heap_size = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
    printf("Min heap size: %lu\n", (unsigned long)heap_size);
    return 0;
}

static void register_heap(void)
{
    const esp_console_cmd_t cmd = {
        .command = "heap",
        .help = "Get minimum free heap memory available during program execution",
        .func = &heap_size,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/* ---------- Tasks Command ---------- */
#if WITH_TASKS_INFO
static int tasks_info(int argc, char **argv)
{
    const size_t bytes_per_task = 40;
    char *task_list_buffer = malloc(uxTaskGetNumberOfTasks() * bytes_per_task);
    if (task_list_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate buffer for vTaskList");
        return 1;
    }

    printf("Task Name\tStatus\tPrio\tHWM\tTask#\n");
#ifdef CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID
    printf("\tAffinity\n");
#endif
    vTaskList(task_list_buffer);
    fputs(task_list_buffer, stdout);
    free(task_list_buffer);
    return 0;
}

static void register_tasks(void)
{
    const esp_console_cmd_t cmd = {
        .command = "tasks",
        .help = "Show information about running tasks",
        .func = &tasks_info,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
#endif // WITH_TASKS_INFO

/* ---------- Deep Sleep Command ---------- */

static struct {
    struct arg_int *wakeup_time;
    struct arg_int *wakeup_gpio_num;
    struct arg_int *wakeup_gpio_level;
    struct arg_end *end;
} deep_sleep_args;

static int deep_sleep(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&deep_sleep_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, deep_sleep_args.end, argv[0]);
        return 1;
    }

    if (deep_sleep_args.wakeup_time->count) {
        uint64_t timeout = 1000ULL * (uint64_t)deep_sleep_args.wakeup_time->ival[0];
        ESP_LOGI(TAG, "Timer wakeup enabled: %llu us", (unsigned long long)timeout);
        ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(timeout));
    }

    if (deep_sleep_args.wakeup_gpio_num->count) {
        /* Build mask for ext1 (supports multiple pins). Validate pins first. */
        uint64_t mask = 0;
        int io_count = deep_sleep_args.wakeup_gpio_num->count;
        for (int i = 0; i < io_count; ++i) {
            int io = deep_sleep_args.wakeup_gpio_num->ival[i];
            if (!esp_sleep_is_valid_wakeup_gpio(io)) {
                ESP_LOGE(TAG, "GPIO %d is not RTC-capable and cannot be used for deep-sleep wakeup", io);
                return 1;
            }
            mask |= (1ULL << io);
        }

        /* Determine mode: if any level argument provided, use its first value (legacy behavior).
           If level==1 -> wake on any HIGH; else wake when all LOW. */
        int level = deep_sleep_args.wakeup_gpio_level->count ? deep_sleep_args.wakeup_gpio_level->ival[0] : 0;
        if (level != 0 && level != 1) {
            ESP_LOGE(TAG, "Invalid wakeup level: %d", level);
            return 1;
        }

        ESP_LOGI(TAG, "Enabling GPIO deep-sleep wakeup mask=0x%llx mode=%s", (unsigned long long)mask,
                 level ? "ANY_HIGH" : "ALL_LOW");

        /* Use ext1 API (recommended in IDF v5.x) */
        esp_err_t err;
        if (level) {
            err = esp_sleep_enable_ext1_wakeup(mask, ESP_EXT1_WAKEUP_ANY_HIGH);
        } else {
            err = esp_sleep_enable_ext1_wakeup(mask, ESP_EXT1_WAKEUP_ALL_LOW);
        }
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_sleep_enable_ext1_wakeup failed: %s", esp_err_to_name(err));
            return 1;
        }
    }

    ESP_LOGI(TAG, "Entering deep sleep...");
    esp_deep_sleep_start();
    return 0;
}

static void register_deep_sleep(void)
{
    deep_sleep_args.wakeup_time = arg_int0("t", "time", "<t>", "Wake up time, ms");
    deep_sleep_args.wakeup_gpio_num = arg_int0(NULL, "io", "<n>", "Wake using GPIO number");
    deep_sleep_args.wakeup_gpio_level = arg_int0(NULL, "io_level", "<0|1>", "GPIO level trigger");
    deep_sleep_args.end = arg_end(3);

    const esp_console_cmd_t cmd = {
        .command = "deep_sleep",
        .help = "Enter deep sleep mode. Wakeup via timer or GPIO.",
        .func = &deep_sleep,
        .argtable = &deep_sleep_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/* ---------- Light Sleep Command ---------- */

static struct {
    struct arg_int *wakeup_time;
    struct arg_int *wakeup_gpio_num;
    struct arg_int *wakeup_gpio_level;
    struct arg_end *end;
} light_sleep_args;

/* Configure a GPIO for wakeup in light sleep: configure pin as input and enable GPIO wakeup.
   Note: for light sleep we use esp_sleep_enable_gpio_wakeup(), and configure the pin via gpio_config.
   Behavior for non-RTC pins may vary depending on chip variant. */
static esp_err_t configure_gpio_wakeup(gpio_num_t io_num, int level)
{
    if (level != 0 && level != 1) {
        return ESP_ERR_INVALID_ARG;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << io_num,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        /* intr_type set to level so the pin is configured appropriately */
        .intr_type = level ? GPIO_INTR_HIGH_LEVEL : GPIO_INTR_LOW_LEVEL
    };
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) return err;

    /* Enable GPIO wakeup (global for light sleep); safe to call multiple times */
    err = esp_sleep_enable_gpio_wakeup();
    return err;
}

static int light_sleep(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&light_sleep_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, light_sleep_args.end, argv[0]);
        return 1;
    }

    /* Clear all existing wakeup sources */
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

    if (light_sleep_args.wakeup_time->count) {
        uint64_t timeout = 1000ULL * (uint64_t)light_sleep_args.wakeup_time->ival[0];
        ESP_LOGI(TAG, "Timer wakeup enabled: %llu us", (unsigned long long)timeout);
        ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(timeout));
    }

    int io_count = light_sleep_args.wakeup_gpio_num->count;
    if (io_count != light_sleep_args.wakeup_gpio_level->count) {
        ESP_LOGE(TAG, "Should have same number of 'io' and 'io_level' arguments");
        return 1;
    }

    for (int i = 0; i < io_count; ++i) {
        int io_num = light_sleep_args.wakeup_gpio_num->ival[i];
        int level = light_sleep_args.wakeup_gpio_level->ival[i];
        if (level != 0 && level != 1) {
            ESP_LOGE(TAG, "Invalid wakeup level: %d", level);
            return 1;
        }
        if (!esp_sleep_is_valid_wakeup_gpio(io_num)) {
            ESP_LOGE(TAG, "GPIO %d is not RTC-capable; cannot use for wakeup on this target", io_num);
            return 1;
        }

        ESP_LOGI(TAG, "Configuring wakeup on GPIO%d, level=%d", io_num, level);

        esp_err_t err = configure_gpio_wakeup((gpio_num_t)io_num, level);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure GPIO%d for wakeup: %s", io_num, esp_err_to_name(err));
            return 1;
        }
    }

#if CONFIG_ESP_CONSOLE_UART_NUM >= 0 && CONFIG_ESP_CONSOLE_UART_NUM <= UART_NUM_1
    ESP_LOGI(TAG, "UART wakeup enabled (press ENTER to wake)");
    ESP_ERROR_CHECK(uart_set_wakeup_threshold(CONFIG_ESP_CONSOLE_UART_NUM, 3));
    ESP_ERROR_CHECK(esp_sleep_enable_uart_wakeup(CONFIG_ESP_CONSOLE_UART_NUM));
#endif

    /* flush console output so prompt/message is visible before sleeping */
    fflush(stdout);
    fsync(fileno(stdout));

    ESP_LOGI(TAG, "Entering light sleep...");
    ESP_ERROR_CHECK(esp_light_sleep_start());

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    const char *cause_str = "unknown";
    switch (cause) {
    case ESP_SLEEP_WAKEUP_GPIO: cause_str = "GPIO"; break;
    case ESP_SLEEP_WAKEUP_UART: cause_str = "UART"; break;
    case ESP_SLEEP_WAKEUP_TIMER: cause_str = "Timer"; break;
    default: break;
    }
    ESP_LOGI(TAG, "Woke up from: %s", cause_str);
    return 0;
}

static void register_light_sleep(void)
{
    light_sleep_args.wakeup_time = arg_int0("t", "time", "<t>", "Wake up time, ms");
    light_sleep_args.wakeup_gpio_num = arg_intn(NULL, "io", "<n>", 0, 8, "Wake using GPIO number");
    light_sleep_args.wakeup_gpio_level = arg_intn(NULL, "io_level", "<0|1>", 0, 8, "GPIO level trigger");
    light_sleep_args.end = arg_end(3);

    const esp_console_cmd_t cmd = {
        .command = "light_sleep",
        .help = "Enter light sleep mode. Wakeup via timer, GPIO, or UART.",
        .func = &light_sleep,
        .argtable = &light_sleep_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
