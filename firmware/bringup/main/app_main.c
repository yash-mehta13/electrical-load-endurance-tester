/*
 * Phase 0 bring-up dispatcher.
 *
 * This is deliberately NOT the product firmware. Phase 0's goal is to prove
 * each piece of hardware works in isolation before any FreeRTOS task
 * structure exists (see docs/roadmap.md). Pick one test via
 * `idf.py menuconfig` -> "Bring-up Test Selection (Phase 0)", flash it,
 * verify the reading looks plausible, then move to the next.
 *
 * Requires ESP-IDF >= 5.2 (new I2C master driver, ADC_ATTEN_DB_12).
 * NOT build-verified yet — see docs/validation_report.md.
 */

#include "sdkconfig.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_system.h"

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 2, 0)
#error "The bring-up firmware needs ESP-IDF 5.2 or newer"
#endif

static const char *TAG = "bringup";

#if CONFIG_BRINGUP_TEST_BLINK
void test_blink_run(void);
#elif CONFIG_BRINGUP_TEST_ADC_READ
void test_adc_read_run(void);
#elif CONFIG_BRINGUP_TEST_I2C_INA226
void test_i2c_ina226_run(void);
#elif CONFIG_BRINGUP_TEST_DS18B20
void test_ds18b20_run(void);
#endif

void app_main(void)
{
    ESP_LOGI(TAG, "Phase 0 bring-up starting (reset reason code %d, ESP-IDF %s)",
             (int)esp_reset_reason(), esp_get_idf_version());

#if CONFIG_BRINGUP_TEST_BLINK
    ESP_LOGI(TAG, "Running: blink + UART console skeleton");
    test_blink_run();
#elif CONFIG_BRINGUP_TEST_ADC_READ
    ESP_LOGI(TAG, "Running: raw ADC read (voltage divider)");
    test_adc_read_run();
#elif CONFIG_BRINGUP_TEST_I2C_INA226
    ESP_LOGI(TAG, "Running: I2C read from INA226");
    test_i2c_ina226_run();
#elif CONFIG_BRINGUP_TEST_DS18B20
    ESP_LOGI(TAG, "Running: DS18B20 / 1-Wire temperature read");
    test_ds18b20_run();
#endif
}
