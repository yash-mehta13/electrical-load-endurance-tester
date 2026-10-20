/*
 * Phase 0 - Test 1: Blink onboard LED + a trivial UART console.
 *
 * Gate for this test: LED blinks at 1 Hz, and typing "ping" over the
 * UART monitor gets back "pong". No FreeRTOS task structure beyond the
 * default app_main task - that comes in Phase 2/3.
 *
 * Console input uses the UART driver with non-blocking reads so the LED
 * keeps blinking while waiting for input. Only available when the ESP-IDF
 * console is on a UART (the default on ESP32 DevKits).
 */

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "test_blink";

#define LED_GPIO CONFIG_BRINGUP_LED_GPIO
#define POLL_MS  50
#define BLINK_MS 500

#if CONFIG_ESP_CONSOLE_UART
#define CONSOLE_UART CONFIG_ESP_CONSOLE_UART_NUM
#define LINE_MAX_LEN 64

static void handle_line(const char *line)
{
    if (strcmp(line, "ping") == 0) {
        printf("pong\n");
    } else {
        printf("unrecognized: %s\n", line);
    }
}

/* Accumulate bytes until CR/LF; over-long lines are discarded. */
static void uart_console_poll(void)
{
    static char line[LINE_MAX_LEN];
    static size_t len = 0;
    static bool overflow = false;
    uint8_t ch;

    while (uart_read_bytes(CONSOLE_UART, &ch, 1, 0) == 1) {
        if (ch == '\r' || ch == '\n') {
            if (len > 0 && !overflow) {
                line[len] = '\0';
                handle_line(line);
            } else if (overflow) {
                printf("line too long, discarded\n");
            }
            len = 0;
            overflow = false;
        } else if (len < sizeof(line) - 1) {
            line[len++] = (char)ch;
        } else {
            overflow = true;
        }
    }
}
#endif

void test_blink_run(void)
{
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

#if CONFIG_ESP_CONSOLE_UART
    ESP_ERROR_CHECK(uart_driver_install(CONSOLE_UART, 256, 0, 0, NULL, 0));
    ESP_LOGI(TAG, "Blinking GPIO%d at 1 Hz. Type 'ping' for a UART sanity check.", LED_GPIO);
#else
    ESP_LOGW(TAG, "Console is not a UART; 'ping' check unavailable. Blinking GPIO%d at 1 Hz.", LED_GPIO);
#endif

    bool led_on = false;
    TickType_t next_toggle = xTaskGetTickCount();

    while (1) {
        TickType_t now = xTaskGetTickCount();
        if ((int32_t)(now - next_toggle) >= 0) {
            led_on = !led_on;
            gpio_set_level(LED_GPIO, led_on);
            next_toggle = now + pdMS_TO_TICKS(BLINK_MS);
        }
#if CONFIG_ESP_CONSOLE_UART
        uart_console_poll();
#endif
        vTaskDelay(pdMS_TO_TICKS(POLL_MS));
    }
}
