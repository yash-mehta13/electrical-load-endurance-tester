/*
 * Phase 0 - Test 2: Raw ADC read of the load-side voltage divider.
 *
 * Gate for this test: with the divider fed from a known bench voltage,
 * the printed value should track it within the divider's expected error
 * (breadboard resistor tolerance, no calibration yet). This is a plausibility
 * check, not a calibrated measurement.
 *
 * Wiring: divider output -> CONFIG_BRINGUP_ADC_GPIO. Use a divider ratio
 * that keeps the ADC pin within 0-3.3V for your expected 12V rail
 * (e.g. 10k/2.2k gives ~12V -> ~2.16V).
 */

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

static const char *TAG = "test_adc_read";

/* GPIO34 = ADC1_CH6 on most ESP32 DevKits; adjust the channel enum below
 * if you change CONFIG_BRINGUP_ADC_GPIO to a different ADC1 pin. */
#define ADC_UNIT     ADC_UNIT_1
#define ADC_CHANNEL  ADC_CHANNEL_6
#define ADC_ATTEN    ADC_ATTEN_DB_12   /* ~0-3.3V input range */

/* If you're using a divider ratio other than what's wired, update this
 * so the printed "divider input" figure means something. */
#define DIVIDER_RATIO 5.545f  /* e.g. (10k+2.2k)/2.2k for a 10k/2.2k divider */

void test_adc_read_run(void)
{
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_cfg = { .unit_id = ADC_UNIT };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_cfg));

    adc_cali_handle_t cali_handle = NULL;
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT,
        .chan = ADC_CHANNEL,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    bool calibrated = (adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle) == ESP_OK);
    if (!calibrated) {
        ESP_LOGW(TAG, "ADC calibration scheme unavailable on this chip/eFuse combo, showing raw counts only");
    }

    ESP_LOGI(TAG, "Reading ADC1_CH6 (GPIO%d) every 500 ms", CONFIG_BRINGUP_ADC_GPIO);

    while (1) {
        int raw = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw));

        if (calibrated) {
            int mv = 0;
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw, &mv));
            float divider_input_v = (mv / 1000.0f) * DIVIDER_RATIO;
            ESP_LOGI(TAG, "raw=%d  adc_mv=%d  divider_input≈%.2fV", raw, mv, divider_input_v);
        } else {
            ESP_LOGI(TAG, "raw=%d (uncalibrated)", raw);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
