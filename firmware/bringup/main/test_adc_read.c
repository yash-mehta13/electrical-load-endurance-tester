/*
 * Phase 0 - Test 2: Raw ADC read of the load-side voltage divider.
 *
 * Gate for this test: with the divider fed from a known bench voltage,
 * the printed value should track it within the divider's expected error
 * (resistor tolerance, no per-board calibration beyond the eFuse scheme).
 * This is a plausibility check, not a calibrated measurement.
 *
 * Wiring: divider output -> CONFIG_BRINGUP_ADC_GPIO (must be an ADC1 pin).
 * NEVER connect the 12 V rail directly to the GPIO. The divider resistor
 * values are set in menuconfig and must match what is actually fitted; the
 * defaults (10k/2.2k) are only an example.
 *
 * On classic ESP32 only the line-fitting calibration scheme exists;
 * curve fitting is used on chips that support it.
 */

#include <stdbool.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

static const char *TAG = "test_adc_read";

#define ADC_ATTEN ADC_ATTEN_DB_12 /* ~0-3.1 V usable range on ESP32 */

#define DIVIDER_RATIO \
    ((float)(CONFIG_BRINGUP_ADC_DIVIDER_R_TOP_OHM + CONFIG_BRINGUP_ADC_DIVIDER_R_BOTTOM_OHM) / \
     (float)CONFIG_BRINGUP_ADC_DIVIDER_R_BOTTOM_OHM)

/* Warn when the pin is this close to the top of the ADC range (saturation). */
#define ADC_NEAR_FULL_SCALE_MV 3000

static bool create_cali_scheme(adc_unit_t unit, adc_channel_t chan, adc_cali_handle_t *out)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cfg = {
        .unit_id = unit,
        .chan = chan,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    return adc_cali_create_scheme_curve_fitting(&cfg, out) == ESP_OK;
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cfg = {
        .unit_id = unit,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    return adc_cali_create_scheme_line_fitting(&cfg, out) == ESP_OK;
#else
    (void)unit;
    (void)chan;
    (void)out;
    return false;
#endif
}

void test_adc_read_run(void)
{
    adc_unit_t unit;
    adc_channel_t channel;
    esp_err_t err = adc_oneshot_io_to_channel(CONFIG_BRINGUP_ADC_GPIO, &unit, &channel);
    if (err != ESP_OK || unit != ADC_UNIT_1) {
        ESP_LOGE(TAG, "GPIO%d is not an ADC1 pin; choose GPIO32-39", CONFIG_BRINGUP_ADC_GPIO);
        return;
    }

    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_cfg = { .unit_id = unit };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, channel, &chan_cfg));

    adc_cali_handle_t cali_handle = NULL;
    bool calibrated = create_cali_scheme(unit, channel, &cali_handle);
    if (!calibrated) {
        ESP_LOGW(TAG, "ADC calibration unavailable (no eFuse calibration data?); raw counts only");
    }

    ESP_LOGI(TAG, "Reading ADC1 channel %d (GPIO%d) every 500 ms, divider ratio %.3f",
             (int)channel, CONFIG_BRINGUP_ADC_GPIO, (double)DIVIDER_RATIO);

    while (1) {
        int raw = 0;
        err = adc_oneshot_read(adc_handle, channel, &raw);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "adc_oneshot_read failed: %s", esp_err_to_name(err));
        } else if (calibrated) {
            int mv = 0;
            if (adc_cali_raw_to_voltage(cali_handle, raw, &mv) == ESP_OK) {
                float divider_input_v = (mv / 1000.0f) * DIVIDER_RATIO;
                ESP_LOGI(TAG, "raw=%d  adc_mv=%d  divider_input=%.2fV", raw, mv, (double)divider_input_v);
                if (mv >= ADC_NEAR_FULL_SCALE_MV) {
                    ESP_LOGW(TAG, "ADC pin near full scale - reading may be saturated; check divider ratio");
                }
            } else {
                ESP_LOGE(TAG, "calibration conversion failed (raw=%d)", raw);
            }
        } else {
            ESP_LOGI(TAG, "raw=%d (uncalibrated)", raw);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
