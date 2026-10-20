# Repository audit

Audit of the repository as found (commits `c5e0d15`, `6fb7321`, clean working tree) and of
what was changed in response. Findings marked **by inspection** were derived by reading
source and ESP-IDF v5.2 headers; they were *not* reproduced with a compiler because no
ESP-IDF/PlatformIO toolchain could be installed in the audit environment (see
[validation_report.md](validation_report.md)).

## 1. State of the repository when audited

| Area | What existed | Real status |
|---|---|---|
| `firmware/bringup/` | Four standalone Phase 0 tests (blink+UART echo, ADC, INA226 register read, DS18B20), a Kconfig test selector, `platformio.ini` | Written, **never built or run** (README said so) |
| `firmware/main/` | One-line README | **No product firmware exists** |
| `docs/` | requirements, roadmap, hardware architecture, CAN, state machine, validation report | Mostly plans and placeholders |
| `hardware/` | `bom.csv` (INR price ranges), empty schematic README | No schematic, no verified wiring |
| `tools/pc_test_script.py` | Docstring + `NotImplementedError` | Placeholder |
| `tests/` | README only | No tests, no recorded results |
| CI / license | none / "TBD" | Unchanged policy: no workflow added, no license added |

Development phase: **Phase 0, bring-up, with no hardware step recorded as completed.** Every
roadmap checkbox was unchecked and still is.

## 2. Findings and actions

| # | Finding | Evidence | Action |
|---|---|---|---|
| A1 | Bring-up `CMakeLists.txt` compiled all four test files regardless of selection, but `CONFIG_BRINGUP_LED_GPIO`, `_ADC_GPIO`, `_I2C_*`, `_ONEWIRE_GPIO` exist only while their test is selected (`depends on`). Three of four selections would fail to compile. | by inspection: `Kconfig.projbuild` vs `main/CMakeLists.txt` | Fixed: only the selected test is compiled |
| A2 | ADC test called `adc_cali_create_scheme_curve_fitting`, which is not declared for classic ESP32 (only line fitting is supported there). | by inspection: ESP-IDF v5.2 `adc_cali_schemes.h` for esp32 defines only `ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED` | Fixed: scheme chosen by the support macros |
| A3 | ADC channel hard-coded to `ADC_CHANNEL_6` while the GPIO is a Kconfig option. | source | Fixed: channel derived with `adc_oneshot_io_to_channel`; non-ADC1 pins rejected |
| A4 | Divider ratio hard-coded (10k/2.2k "example"). Real resistor values are not recorded anywhere. | source, BOM ("precision set") | Ratio now from Kconfig; defaults still examples; **actual values unresolved** |
| A5 | Code needs ESP-IDF >= 5.2 (`driver/i2c_master.h`, `ADC_ATTEN_DB_12`), README said "v5.x"; PlatformIO platform version is unpinned. | by inspection: headers absent/undefined in v5.1 | Compile-time version guard + docs; platform pin left open until a build succeeds |
| A6 | Blink test read the console with blocking `fgets(stdin)` inside the blink loop; this can stall blinking or return EOF repeatedly depending on VFS configuration, so the "ping/pong" gate was unreliable. | source; ESP-IDF console behaviour | Replaced with non-blocking UART-driver polling. **Unverified on hardware** |
| A7 | `ESP_ERROR_CHECK` inside the sensor loops would abort/reboot on a single I2C/CRC error. | source | Errors are logged and the loop continues |
| A8 | DS18B20 power-on value 85.0 C was not treated as suspect. | source | Warned in the DS18B20 test |
| A9 | INA226 test had no identity check, so a wrong device at 0x40 would look like a plausible bus-voltage read. | source | Manufacturer/die ID read added (expected values unconfirmed, datasheet not reachable) |
| A10 | README framed the project as "automotive-adjacent". | README | Reworded to a low-voltage DC bench scope |
| A11 | Hardware block diagram showed INA226 feeding an ADC and the temperature sensor on I2C. INA226 is I2C; DS18B20 is 1-Wire. | `hardware_architecture.md`, bring-up code | Diagram replaced |
| A12 | **SR9 conflict:** requirement calls for physical isolation of control and load domains; hardware doc says "single star-ground point"; the listed BTS7960 and INA226 modules are not isolated parts. | docs + BOM | Left **open**, requirement marked *Blocked* in [traceability.md](traceability.md) |
| A13 | BOM calls the BTS7960 a "high-side smart switch ... HW overcurrent/thermal/short protection". It is an integrated half-bridge (high- and low-side FET); protection thresholds and latch/restart behaviour are datasheet items and are unverified for the module. | BOM | BOM note softened; limitation recorded in hardware doc |
| A14 | Temperature sensor: roadmap says "DS18B20/NTC"; BOM and code contain only DS18B20; no NTC part or circuit exists. | roadmap, BOM, code | DS18B20 treated as the **only** defined sensor; NTC not selected |
| A15 | BOM omits parts the design relies on: DS18B20 pull-up, fuse/PTC rating, TVS, CAN termination, INA226 shunt value, gate/enable pull resistor. | BOM vs hardware doc | Listed as open items; rows **not** invented |
| A16 | FR5 asks for a "hardware-fast" shutdown path; no component in the BOM provides one beyond fuse/PTC and (option A) BTS7960 internal protection. | requirements vs BOM | Recorded as hardware gap; requirement *Blocked* |
| A17 | Two independent voltage measurements exist (divider+ADC and INA226 bus voltage); the authoritative one is undefined. INA226 full-scale shunt voltage is +/-81.92 mV, so the usable current range depends on the module's shunt, which is not recorded. | BOM, INA226 datasheet (from memory) | Recorded as open decisions |
| A18 | `.gitignore` excludes `dependencies.lock` and `sdkconfig`; managed components use `^1.0` ranges, so component versions float. | `.gitignore`, `idf_component.yml` | Unchanged; documented in [setup.md](setup.md) |
| A19 | CAN doc implies PC-side USB-CAN adapter (matches BOM); no STM32/second node exists anywhere in the repo. | docs, BOM | CAN doc states only the PC topology as planned |
| A20 | Roadmap defers "the full 9-phase roadmap" to an external chat/design doc. | `roadmap.md` | Roadmap expanded in-repo with gates |
| A21 | `pc_test_script.py` is a stub; the UART command set it refers to does not exist. | source | Left as-is (honest placeholder); `tools/bringup_capture.py` added for Phase 0 |
| A22 | `hardware/bom.csv` row 1 had an unquoted comma in the item name, so it parsed as 6 fields instead of 5. | CSV parse | Fixed: item name quoted |

## 3. Unresolved hardware choices

Switching stage (BTS7960 module vs MOSFET + driver); gate/enable GPIO and its safe-state
pull; fuse/PTC rating; flyback/TVS part and rating; INA226 shunt value and tap location;
voltage-divider resistor values and maximum sensed voltage; authoritative voltage source;
temperature probe mounting; common-ground vs isolated topology; CAN TX/RX GPIOs, bit rate
and termination; CAN adapter model; test-load ratings (the BOM lists a motor, whose stall
current is unknown).

## 4. Existing tests and evidence

None before this work. Added (software only): host unit tests for the scheduler/debounce
core and Python tests for the capture tool. No bench measurement, build log, or serial log
exists in the repository.
