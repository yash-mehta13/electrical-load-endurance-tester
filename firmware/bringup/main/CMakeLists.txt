# Only the selected bring-up test is compiled: each test file uses Kconfig
# symbols that exist only while that test is selected (see Kconfig.projbuild).
set(srcs "app_main.c")
if(CONFIG_BRINGUP_TEST_BLINK)
    list(APPEND srcs "test_blink.c")
elseif(CONFIG_BRINGUP_TEST_ADC_READ)
    list(APPEND srcs "test_adc_read.c")
elseif(CONFIG_BRINGUP_TEST_I2C_INA226)
    list(APPEND srcs "test_i2c_ina226.c")
elseif(CONFIG_BRINGUP_TEST_DS18B20)
    list(APPEND srcs "test_ds18b20.c")
endif()

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "."
    REQUIRES driver esp_adc esp_system
)
