/*
 * Phase 0 - Test 3: I2C read from the INA226 current/voltage sensor.
 *
 * This test reads the manufacturer/die ID registers once (identity check)
 * and then the bus voltage register (no calibration/current config yet) —
 * the goal is just "can the ESP32 talk to the chip and get a plausible
 * number back". Full current-sense calibration (shunt value,
 * cal register) belongs to Phase 2's proper sensing task.
 *
 * Gate for this test: printed bus voltage roughly matches what a multimeter
 * reads on the INA226's VBUS input.
 */

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

static const char *TAG = "test_i2c_ina226";

#define INA226_ADDR        0x40   /* default address, A0/A1 tied low */
#define REG_BUS_VOLTAGE    0x02
#define REG_MANUFACTURER   0xFE   /* expected 0x5449 ("TI"); from memory of the datasheet - confirm */
#define REG_DIE_ID         0xFF   /* expected 0x2260; from memory of the datasheet - confirm */
#define BUS_VOLTAGE_LSB_MV 1.25f  /* datasheet: 1.25 mV per bit */

static esp_err_t ina226_read_reg(i2c_master_dev_handle_t dev, uint8_t reg, uint16_t *out)
{
    uint8_t data[2] = {0};
    esp_err_t err = i2c_master_transmit_receive(dev, &reg, 1, data, 2, 100);
    if (err == ESP_OK) {
        *out = (uint16_t)((data[0] << 8) | data[1]);
    }
    return err;
}

void test_i2c_ina226_run(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = CONFIG_BRINGUP_I2C_SDA_GPIO,
        .scl_io_num = CONFIG_BRINGUP_I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = INA226_ADDR,
        .scl_speed_hz = 100000,
    };
    i2c_master_dev_handle_t dev_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    ESP_LOGI(TAG, "Reading INA226 bus voltage register every 500 ms (SDA=GPIO%d SCL=GPIO%d)",
             CONFIG_BRINGUP_I2C_SDA_GPIO, CONFIG_BRINGUP_I2C_SCL_GPIO);

    uint16_t id = 0;
    if (ina226_read_reg(dev_handle, REG_MANUFACTURER, &id) == ESP_OK) {
        ESP_LOGI(TAG, "manufacturer ID = 0x%04X (expected 0x5449 - unconfirmed)", id);
    } else {
        ESP_LOGE(TAG, "manufacturer ID read failed - device absent or wrong address/wiring");
    }
    if (ina226_read_reg(dev_handle, REG_DIE_ID, &id) == ESP_OK) {
        ESP_LOGI(TAG, "die ID = 0x%04X (expected 0x2260 - unconfirmed)", id);
    }

    while (1) {
        uint16_t raw = 0;

        esp_err_t err = ina226_read_reg(dev_handle, REG_BUS_VOLTAGE, &raw);
        if (err == ESP_OK) {
            float bus_v = raw * BUS_VOLTAGE_LSB_MV / 1000.0f;
            ESP_LOGI(TAG, "bus_voltage = %.3f V (raw=0x%04X)", (double)bus_v, raw);
        } else {
            ESP_LOGE(TAG, "I2C read failed: %s (check wiring/address 0x%02X)",
                     esp_err_to_name(err), INA226_ADDR);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
