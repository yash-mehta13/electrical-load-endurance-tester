/*
 * Phase 0 - Test 1: Blink onboard LED + a trivial UART console.
 *
 * Gate for this test: LED blinks at 1 Hz, and typing "ping" over the
 * UART monitor gets back "pong". No FreeRTOS task structure beyond the
 * default app_main task — that comes in Phase 2/3.
 */

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "test_blink";

#define LED_GPIO CONFIG_BRINGUP_LED_GPIO

static void uart_console_poll(void)
{
    char line[64];
    if (fgets(line, sizeof(line), stdin) != NULL) {
        line[strcspn(line, "\r\n")] = 0;
        if (strcmp(line, "ping") == 0) {
            printf("pong\n");
        } else if (strlen(line) > 0) {
            printf("unrecognized: %s\n", line);
        }
    }
}

void test_blink_run(void)
{
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    bool led_on = false;
    ESP_LOGI(TAG, "Blinking GPIO%d at 1 Hz. Type 'ping' for a UART sanity check.", LED_GPIO);

    while (1) {
        led_on = !led_on;
        gpio_set_level(LED_GPIO, led_on);
        uart_console_poll();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
