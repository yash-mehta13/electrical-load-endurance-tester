idf_component_register(
    SRCS "app_main.c" "test_blink.c" "test_adc_read.c" "test_i2c_ina226.c" "test_ds18b20.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_adc
)
