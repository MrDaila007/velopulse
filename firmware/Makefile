# BikeComp Arduino firmware — convenience targets around PlatformIO.
#
#   make              # build xiao_ble_sense
#   make test         # pio test -e native
#   make upload       # USB serial DFU (nrfutil)
#   make flash-stlink # SWD via ST-Link, keeps Adafruit bootloader
#   make dfu-ble      # BLE OTA via PC Bluetooth (bleak)
#
# Override the PlatformIO env:
#   make build ENV=xiao_ble_sense_128x32
#   make flash-stlink ENV=xiao_ble_sense_deep_sleep

SHELL := /bin/bash
.PHONY: all build clean rebuild test upload flash flash-stlink dfu-ble monitor help

ROOT      := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
ENV       ?= xiao_ble_sense
PIO_BUILD := $(ROOT)/.pio/build/$(ENV)
HEX       := $(PIO_BUILD)/firmware.hex
ZIP       := $(PIO_BUILD)/firmware.zip
SCRIPTS   := $(ROOT)/scripts

all: build

build:
	cd "$(ROOT)" && pio run -e "$(ENV)"

clean:
	cd "$(ROOT)" && pio run -e "$(ENV)" -t clean

rebuild: clean build

test:
	cd "$(ROOT)" && pio test -e native

# USB serial DFU (adafruit-nrfutil). Needs CDC / double-reset bootloader.
upload flash:
	cd "$(ROOT)" && pio run -e "$(ENV)" -t upload

# SWD: program application + Adafruit bootloader CRC, do not erase UF2.
flash-stlink:
	"$(SCRIPTS)/flash_stlink.sh" "$(ENV)"

# Buttonless Adafruit BLE DFU from the PC Bluetooth adapter.
dfu-ble: build
	@test -f "$(ZIP)" || (echo "Missing $(ZIP)" >&2; exit 1)
	python3 "$(SCRIPTS)/ble_dfu.py" "$(ZIP)"

monitor:
	cd "$(ROOT)" && pio device monitor -e "$(ENV)" -b 115200

help:
	@echo "BikeComp Arduino firmware Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  make [build]        Build firmware (default env $(ENV))"
	@echo "  make clean          PlatformIO clean for $(ENV)"
	@echo "  make rebuild        clean + build"
	@echo "  make test           Host domain tests (pio test -e native)"
	@echo "  make upload         USB serial DFU (nrfutil / double-reset)"
	@echo "  make flash          Alias for upload"
	@echo "  make flash-stlink   SWD via ST-Link; keeps Adafruit bootloader"
	@echo "  make dfu-ble        BLE OTA of firmware.zip (needs bleak)"
	@echo "  make monitor        Serial console 115200"
	@echo ""
	@echo "Variables:"
	@echo "  ENV=$(ENV)"
	@echo "    xiao_ble_sense | xiao_ble_sense_128x32 | xiao_ble_sense_deep_sleep"
	@echo ""
	@echo "Artifacts: $(HEX)"
	@echo "           $(ZIP)"
