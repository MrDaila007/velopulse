# BikeComp Zephyr firmware — convenience targets around scripts/build.sh
#
# Defaults match a local Zephyr workspace; override via env or make VAR=value.
#
#   make              # build UF2/hex
#   make clean        # remove build/
#   make rebuild      # clean + build
#   make uf2          # print path to zephyr.uf2
#   make bootstrap    # west init/update in repo (optional)

SHELL := /bin/bash
.PHONY: all build clean rebuild bootstrap env env-show uf2 hex upload flash monitor test help native-test

ROOT        := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
REPO_ROOT   := $(abspath $(ROOT)/..)
SCRIPTS     := $(ROOT)/scripts
BUILD_DIR   ?= $(ROOT)/build
BOARD       ?= xiao_ble/nrf52840
OVERLAY     ?= $(ROOT)/app/boards/super_nrf52840.overlay

UF2  := $(BUILD_DIR)/zephyr/zephyr.uf2
HEX  := $(BUILD_DIR)/zephyr/zephyr.hex
ELF  := $(BUILD_DIR)/zephyr/zephyr.elf

export BOARD BUILD_DIR OVERLAY

all: build

build:
	@$(SCRIPTS)/build.sh

clean:
	@rm -rf "$(BUILD_DIR)"
	@echo "Removed $(BUILD_DIR)"

rebuild: clean build

bootstrap:
	@$(SCRIPTS)/bootstrap.sh

env:
	@$(SCRIPTS)/configure-env.sh

env-show:
	@$(SCRIPTS)/configure-env.sh --check

uf2: build
	@test -f "$(UF2)" || (echo "Missing $(UF2)" >&2; exit 1)
	@echo "$(UF2)"

hex: build
	@test -f "$(HEX)" || (echo "Missing $(HEX)" >&2; exit 1)
	@echo "$(HEX)"

# Serial DFU via adafruit-nrfutil (PIO upload_protocol=nrfutil).
upload flash:
	@$(SCRIPTS)/upload.sh

upload-nobuild:
	@$(SCRIPTS)/upload.sh --no-build

monitor:
	@$(SCRIPTS)/monitor.sh

# Host ztest suite (domain parity with pio test -e native).
test:
	@$(SCRIPTS)/test.sh

# Domain regression (shared with Arduino firmware).
native-test:
	@cd "$(REPO_ROOT)/firmware" && pio test -e native

help:
	@echo "BikeComp Zephyr Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  make [build]     Build firmware (default)"
	@echo "  make clean       Delete $(BUILD_DIR)"
	@echo "  make rebuild     clean + build"
	@echo "  make uf2         Build and print UF2 path"
	@echo "  make hex         Build and print HEX path"
	@echo "  make upload      Build + flash over USB serial (adafruit-nrfutil)"
	@echo "  make flash       Alias for upload"
	@echo "  make monitor     Open serial console (115200)"
	@echo "  make test        Run ztest domain suite (host unittest)"
	@echo "  make env         Discover paths and write ignored .env"
	@echo "  make env-show    Validate and print the resolved environment"
	@echo "  make bootstrap   Run west bootstrap in repo"
	@echo "  make native-test Run shared domain tests (pio -e native)"
	@echo ""
	@echo "Variables (override in .env, shell, or make build VAR=...):"
	@echo "  BOARD=$(BOARD)"
	@echo "  BUILD_DIR=$(BUILD_DIR)"
	@echo "  ZEPHYR_BASE / ZEPHYR_SDK_INSTALL_DIR / WEST_TOP / U8G2_ROOT"
	@echo ""
	@echo "Artifacts: $(UF2)"
