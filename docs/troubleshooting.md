# Troubleshooting

| Symptom | Checks |
|---|---|
| `pio run` fails fetching the platform/toolchain | Needs network access to the PlatformIO registry. Corporate proxies/firewalls commonly block it; check proxy/CA settings, or use the raw ESP-IDF route. |
| Build error `#error "... needs ESP-IDF 5.2 or newer"` | Update the PlatformIO platform or ESP-IDF. `driver/i2c_master.h` and `ADC_ATTEN_DB_12` do not exist before 5.2. |
| Build error about `CONFIG_BRINGUP_*` undefined | Run menuconfig and select a test; delete the generated `sdkconfig*` and rebuild if a stale selection persists. |
| Component download fails (`onewire_bus`, `ds18b20`) | First build needs registry access. The DS18B20 API is a managed component with a `^1.0` range; if a newer 1.x release changed function names, pin the version in `main/idf_component.yml`. |
| Cannot open serial port / permission denied | Linux: add your user to `dialout` (or `uucp`), re-login. Close other monitors using the port. Check the cable carries data. |
| Board resets every time the port is opened | Normal DTR/RTS auto-reset. Use `--no-reset` with the capture tool. |
| Garbled boot text | Wrong baud rate (the monitor is set to 115200). |
| Boot loop / `Brownout detector was triggered` | Weak USB cable or port; check the supply. Never power load circuits from the USB rail. |
| Blink test: LED stays off | Check `BRINGUP_LED_GPIO` against your board (GPIO2 is common but not universal). |
| Blink test: no `pong` | Console must be UART (default). Send `ping` followed by Enter/newline. Input is untested on hardware. |
| ADC reads 0 or saturates | Divider not connected, wrong pin (must be ADC1: GPIO32-39), or divider ratio too large. Log warns near full scale. Do **not** connect 12 V directly to the pin. |
| ADC voltage disagrees with a meter | Check the resistor values set in menuconfig against the fitted parts; ADC is nonlinear and only eFuse-calibrated; ESP32 accuracy is limited. Compare at several points. |
| INA226: `manufacturer ID read failed` | Wiring (SDA/SCL swapped?), address (A0/A1), 3.3 V supply, pull-ups. Scan the bus with another tool. |
| INA226 ID reads but values look wrong | Bring-up reads bus voltage only. Current needs the calibration register and the true shunt value, not implemented yet. |
| DS18B20 not found | Data line wiring, 4.7k pull-up to 3.3 V (not in the BOM), sensor orientation. |
| DS18B20 reads 85.00 C | Reset/not-converted value; treat as invalid. Check power and pull-up. |
| Load does not switch | No switching stage exists in the bring-up firmware; the bring-up tests do not drive a load. |
| CAN errors | CAN is not implemented yet. For future work: bit rate identical on all nodes, 120 ohm termination at each end only, common ground reference, transceiver supplied from 3.3 V. |
| Watchdog resets | No watchdog supervision is implemented yet; resets seen today come from `ESP_ERROR_CHECK` aborts (setup failures), brownouts or crashes - read the reset reason printed at boot. |
