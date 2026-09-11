/*
 * Phase 0 - Test 4: DS18B20 temperature read over 1-Wire.
 *
 * Uses Espressif's managed `onewire_bus` + `ds18b20` components rather than
 * hand-rolled bit-banging — pulled in via main/idf_component.yml, fetched
 * automatically by `idf.py build` (needs network access to the component
 * registry, which this environment does not have — verify locally).
 *
 * Gate for this test: printed temperature matches a room-temperature
 * reference (thermometer/phone weather app) within a couple of degrees.
 */

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "onewire_bus.h"
#include "ds18b20.h"

static const char *TAG = "test_ds18b20";

void test_ds18b20_run(void)
{
    onewire_bus_config_t bus_cfg = {
        .bus_gpio_num = CONFIG_BRINGUP_ONEWIRE_GPIO,
    };
    onewire_bus_rmt_config_t rmt_cfg = {
        .max_rx_bytes = 10,
    };
    onewire_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(onewire_new_bus_rmt(&bus_cfg, &rmt_cfg, &bus_handle));

    ESP_LOGI(TAG, "Scanning 1-Wire bus on GPIO%d for DS18B20 devices...",
             CONFIG_BRINGUP_ONEWIRE_GPIO);

    onewire_device_iter_handle_t iter;
    ESP_ERROR_CHECK(onewire_new_device_iter(bus_handle, &iter));

    onewire_device_t device;
    ds18b20_device_handle_t ds18b20 = NULL;

    while (onewire_device_iter_get_next(iter, &device) == ESP_OK) {
        ds18b20_config_t ds_cfg = {};
        if (ds18b20_new_device(&device, &ds_cfg, &ds18b20) == ESP_OK) {
            ESP_LOGI(TAG, "Found DS18B20, address: %016llX", device.address);
            break; /* Phase 0 only needs the first sensor found */
        }
    }
    ESP_ERROR_CHECK(onewire_del_device_iter(iter));

    if (ds18b20 == NULL) {
        ESP_LOGE(TAG, "No DS18B20 found on GPIO%d — check wiring and 4.7k pull-up",
                 CONFIG_BRINGUP_ONEWIRE_GPIO);
        return;
    }

    while (1) {
        ESP_ERROR_CHECK(ds18b20_trigger_temperature_conversion(ds18b20));
        float temp_c = 0;
        ESP_ERROR_CHECK(ds18b20_get_temperature(ds18b20, &temp_c));
        ESP_LOGI(TAG, "temperature ≈ %.2f °C", temp_c);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
