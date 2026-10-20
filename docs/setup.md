# Setup and reproducibility

Everything another engineer needs to reproduce the **current** (Phase 0) state. Nothing in
this repository has yet been built with an ESP32 toolchain; the first successful build
should be recorded in [validation_report.md](validation_report.md) with exact versions.

## Hardware (Phase 0)

* ESP32 DevKit, WROOM-32, 38-pin, USB cable with data lines.
* Optional per test: INA226 module (SDA GPIO21, SCL GPIO22, address 0x40), voltage divider
  to GPIO34 (never 12 V directly on a GPIO), DS18B20 on GPIO4 with 4.7k pull-up to 3.3 V.
  These are Kconfig defaults, not verified wiring ([pin map](../assets/diagrams/pin_map.svg)).
* No 12 V load or switching stage is needed or used in Phase 0.

## Software

| Tool | Needed for | Version notes |
|---|---|---|
| PlatformIO Core (or VS Code extension) | Recommended bring-up build | Platform `espressif32`, framework `espidf`; version **not pinned** yet - the bring-up code needs ESP-IDF >= 5.2 inside it; pin after a successful build |
| *or* ESP-IDF | Alternative bring-up build | **>= 5.2** (new I2C master driver, `ADC_ATTEN_DB_12`); the build stops with an `#error` on older versions |
| Python 3 | Capture tool and its tests | Tested with 3.13 |
| `pyserial` >= 3.5 | Live serial capture only | `pip install -r tools/requirements.txt` |
| C99 compiler + `make` | Host unit tests | Tested with gcc 13.3 and clang |

The DS18B20 test pulls `espressif/onewire_bus` and `espressif/ds18b20` (`^1.0`) from the
ESP component registry on first build, so the first build needs network access. Those
version ranges float; `dependencies.lock` is git-ignored. Commit it (remove it from
`.gitignore`) once you have a build you want others to reproduce.

## Build the bring-up firmware

PlatformIO:

```bash
cd firmware/bringup
pio run -t menuconfig        # "Bring-up Test Selection (Phase 0)": BLINK / ADC_READ / I2C_INA226 / DS18B20
pio run                      # build
pio run -t upload            # flash
pio device monitor           # 115200 baud
```

ESP-IDF:

```bash
cd firmware/bringup
idf.py set-target esp32
idf.py menuconfig
idf.py build flash monitor
```

Only the selected test is compiled. Selecting a different test requires menuconfig again
(and a rebuild). The selection is stored in a generated `sdkconfig*` file (git-ignored).

For the ADC test also set the real divider resistor values in menuconfig; the defaults
(10k/2.2k) are only examples.

## Host-side tests (no ESP32 needed)

```bash
make -C tests/unit               # scheduler/debounce core
make -C tests/unit san           # same with AddressSanitizer + UBSan
python3 -m unittest discover -s tests/python -v
```

## Known gaps

* No ESP-IDF/PlatformIO build has been performed on this repository's code
  ([validation_report.md](validation_report.md)).
* PlatformIO platform and component versions are unpinned.
* No schematic; wiring is described only by [hardware_architecture.md](hardware_architecture.md).
