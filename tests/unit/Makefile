# Host-side unit tests for firmware/core (no ESP32, no ESP-IDF needed).
#   make -C tests/unit          build + run
#   make -C tests/unit san      same, with AddressSanitizer + UBSan
CC ?= cc
CFLAGS ?= -std=c99 -pedantic -Wall -Wextra -Werror -O1 -g
SRC = test_endurance_core.c ../../firmware/core/endurance_core.c ../../firmware/core/debounce.c
BUILD = ../../build/unit

.PHONY: test san clean
test: $(BUILD)/test_core
	$(BUILD)/test_core

san: CFLAGS += -fsanitize=address,undefined -fno-sanitize-recover=all
san: clean test

$(BUILD)/test_core: $(SRC) $(wildcard ../../firmware/core/*.h)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(SRC)

clean:
	rm -rf $(BUILD)
