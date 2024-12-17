.PHONY: vendor

JOBS = 4
MAKE = make -j $(JOBS)
SCONS = scons -Q -j $(JOBS)

BUILD_DIR             = build
BOARDLOADER_BUILD_DIR = $(BUILD_DIR)/boardloader
BOAR_DEV_REFLASH_BUILD_DIR  = $(BUILD_DIR)/boardloader_reflash_dev
BOOTLOADER_BUILD_DIR  = $(BUILD_DIR)/bootloader
FIRMWARE_BUILD_DIR    = $(BUILD_DIR)/firmware
UNIX_BUILD_DIR        = $(BUILD_DIR)/unix
RUST_BUILD_DIR        = $(BUILD_DIR)/rust

UNAME_S := $(shell uname -s)
CURRENT_DIR := '\"$(shell pwd)\"'
UNIX_PORT_OPTS ?=
CROSS_PORT_OPTS ?=

PRODUCTION ?= 0
PYOPT      ?= 1
BITCOIN_ONLY ?= 0
TREZOR_MODEL ?= T
TREZOR_MEMPERF ?= 0
ADDRESS_SANITIZER ?= 0
UI2 ?= 0
PRODUCTION_MODEL = H
USE_THD89 ?= 1

# OpenOCD interface default. Alternative: ftdi/olimex-arm-usb-tiny-h
OPENOCD_INTERFACE ?= stlink
# OpenOCD transport default. Alternative: jtag
OPENOCD_TRANSPORT ?= hla_swd
OPENOCD = openocd -f interface/$(OPENOCD_INTERFACE).cfg -c "transport select $(OPENOCD_TRANSPORT)" -f target/stm32f4x.cfg
OPENOCD_T1 = openocd -f interface/$(OPENOCD_INTERFACE).cfg -c "transport select $(OPENOCD_TRANSPORT)" -f target/stm32f2x.cfg

BOARDLOADER_START   = 0x08000000
BOOTLOADER_START    = 0x08020000
FIRMWARE_P1_START   = 0x08060000
FIRMWARE_P2_START   = 0x90000000

BOARDLOADER_MAXSIZE = 49152
BOOTLOADER_MAXSIZE  = 131072
FIRMWARE_P1_MAXSIZE = 786432
FIRMWARE_P2_MAXSIZE = 917504
FIRMWARE_MAXSIZE    = 1703936

SCM_REVISION=$(shell git rev-parse HEAD | sed 's:\(..\):\\x\1:g')
BUILD_DATE=$(shell date "+%m%d")

BOARD_BUILD_VERSION=$(shell ../tools/version.sh ./embed/boardloader/version.h)
BOOT_BUILD_VERSION=$(shell ../tools/version.sh ./embed/bootloader/version.h)
FIRMWARE_BUILD_VERSION=$(shell cat ./embed/firmware/version.h | grep -E '#define ONEKEY_VERSION_(MAJOR|MINOR|PATCH)' | awk '{printf "%s.", $$3}' | sed 's/\.$$//')
ifndef BUILD_ID
BUILD_COMMIT=$(shell git rev-parse HEAD | cut -c1-7)
FIRMWARE_BUILD_NAME=pro.$(FIRMWARE_BUILD_VERSION)-Stable-$(BUILD_DATE)-$(BUILD_COMMIT)
BOOT_BUILD_NAME=pro.bootloader.$(BOOT_BUILD_VERSION)-Stable-$(BUILD_DATE)-$(BUILD_COMMIT)
else
FIRMWARE_BUILD_NAME=pro.$(FIRMWARE_BUILD_VERSION)-Alpha-$(BUILD_DATE)-$(BUILD_ID)
BOOT_BUILD_NAME=pro.bootloader.$(BOOT_BUILD_VERSION)-Alpha-$(BUILD_DATE)-$(BUILD_ID)
endif
CFLAGS += -DBUILD_COMMIT='\"$(BUILD_COMMIT)\"'


TESTPATH = $(CURDIR)/../tests

EMU = $(CURDIR)/emu.py
EMU_LOG_FILE ?= $(TESTPATH)/trezor.log
EMU_TEST_ARGS = --disable-animation --headless --output=$(EMU_LOG_FILE) --temporary-profile
EMU_TEST = $(EMU) $(EMU_TEST_ARGS) -c

JUNIT_XML ?= $(TESTPATH)/junit.xml
PYTEST = pytest --junitxml=$(JUNIT_XML)
TREZOR_FIDO2_UDP_PORT = 21326
RUST_TARGET=$(shell rustc -vV | sed -n 's/host: //p')

## help commands:

help: ## show this help
	@awk -f ../tools/help.awk $(MAKEFILE_LIST)

## dependencies commands:

vendor: ## update git submodules
	git submodule update --init --recursive --force

## emulator commands:

run: ## run unix port
	cd src ; ../$(UNIX_BUILD_DIR)/trezor-emu-core

emu: ## run emulator
	$(EMU)

## test commands:

test: ## run unit tests
	cd tests ; ./run_tests.sh $(TESTOPTS)

test_rust: ## run rs unit tests
	cd embed/rust ; cargo test  --target=$(RUST_TARGET) --no-default-features --features model_t$(shell echo $(TREZOR_MODEL) | tr "TR" "tr"),test -- --test-threads=1

test_emu: ## run selected device tests from python-trezor
	$(EMU_TEST) $(PYTEST) $(TESTPATH)/device_tests $(TESTOPTS)

test_emu_multicore: ## run device tests using multiple cores
	$(PYTEST) -n auto $(TESTPATH)/device_tests $(TESTOPTS) --control-emulators --model=core --random-order-seed=$(shell echo $$RANDOM)

test_emu_monero: ## run selected monero device tests from monero-agent
	cd tests ; $(EMU_TEST) ./run_tests_device_emu_monero.sh $(TESTOPTS)

test_emu_u2f: ## run selected u2f device tests from u2f-tests-hid
	$(EMU_TEST) --slip0014 $(TESTPATH)/fido_tests/u2f-tests-hid/HIDTest $(TREZOR_FIDO2_UDP_PORT) $(TESTOPTS)
	$(EMU_TEST) --slip0014 $(TESTPATH)/fido_tests/u2f-tests-hid/U2FTest $(TREZOR_FIDO2_UDP_PORT) $(TESTOPTS)

test_emu_fido2: ## run fido2 device tests
	cd $(TESTPATH)/fido_tests/fido2 ; \
		$(EMU_TEST) --slip0014 $(PYTEST) --maxfail=5 --sim tests/standard/ --vendor trezor $(TESTOPTS)

test_emu_click: ## run click tests
	$(EMU_TEST) $(PYTEST) $(TESTPATH)/click_tests $(TESTOPTS)

test_emu_ui: ## run ui integration tests
	UI2="$(UI2)" $(EMU_TEST) $(PYTEST) $(TESTPATH)/device_tests --ui=test --ui-check-missing $(TESTOPTS)

test_emu_ui_multicore: ## run ui integration tests using multiple cores
	UI2="$(UI2)" $(PYTEST) -n auto $(TESTPATH)/device_tests $(TESTOPTS) --ui=test --ui-check-missing --control-emulators --model=core --random-order-seed=$(shell echo $$RANDOM)

test_emu_ui_record: ## record and hash screens for ui integration tests
	UI2="$(UI2)" $(EMU_TEST) $(PYTEST) $(TESTPATH)/device_tests --ui=record --ui-check-missing $(TESTOPTS)

test_emu_ui_record_multicore: ## record and hash screens for ui integration tests using multiple cores
	UI2="$(UI2)" $(PYTEST) -n auto $(TESTPATH)/device_tests $(TESTOPTS) --ui=record --ui-check-missing --control-emulators --model=core --random-order-seed=$(shell echo $$RANDOM)

pylint: ## run pylint on application sources and tests
	pylint -E $(shell find src tests -name *.py)

mypy: ## deprecated; use "make typecheck"
	@echo "mypy is deprecated; use 'make typecheck'"
	make typecheck

typecheck: pyright

pyright:
	python ../tools/pyright_tool.py

clippy:
	cd embed/rust ; cargo clippy --all-features --target=$(RUST_TARGET)

## code generation:

templates: ## render Mako templates (for lists of coins, tokens, etc.)
	./tools/build_templates

templates_check: ## check that Mako-rendered files match their templates
	./tools/build_templates --check

## build commands:

build: build_boardloader build_bootloader build_firmware build_unix ## build all

build_embed: build_boardloader build_bootloader build_firmware # build boardloader, bootloader, firmware

build_boardloader: CFLAGS += -DBUILD_ID='\"boardloader.$(BOARD_BUILD_VERSION)\"'
build_boardloader: ## build boardloader
	$(SCONS) CFLAGS="$(CFLAGS)" PRODUCTION="$(PRODUCTION)" TREZOR_MODEL="$(TREZOR_MODEL)" PRODUCTION_MODEL="$(PRODUCTION_MODEL)" $(BOARDLOADER_BUILD_DIR)/boardloader.bin

build_boardloader_reflash_dev: build_boardloader ## build boardloader_reflash_dev
	mkdir -p $(BOAR_DEV_REFLASH_BUILD_DIR)
	python embed/boardloader_reflash_dev/bin2array.py -l 16 -i $(BOARDLOADER_BUILD_DIR)/boardloader.bin -o $(BOAR_DEV_REFLASH_BUILD_DIR)/payload.h
	$(SCONS) CFLAGS="$(CFLAGS)" PRODUCTION="$(PRODUCTION)" TREZOR_MODEL="$(TREZOR_MODEL)" PRODUCTION_MODEL="$(PRODUCTION_MODEL)" $(BOAR_DEV_REFLASH_BUILD_DIR)/boardloader_reflash_dev.bin

build_bootloader: CFLAGS += -DBUILD_ID='\"bootloader.$(BOOT_BUILD_VERSION)-$(BUILD_DATE)-$(BUILD_COMMIT)\"'
build_bootloader: CFLAGS += -DSCM_REVISION='\"$(SCM_REVISION)\"'
build_bootloader: ## build bootloader
	$(SCONS) CFLAGS="$(CFLAGS)" PRODUCTION="$(PRODUCTION)" TREZOR_MODEL="$(TREZOR_MODEL)" PRODUCTION_MODEL="$(PRODUCTION_MODEL)" $(BOOTLOADER_BUILD_DIR)/bootloader.bin
	cp $(BOOTLOADER_BUILD_DIR)/bootloader.bin $(BOOTLOADER_BUILD_DIR)/$(BOOT_BUILD_NAME).bin

build_firmware: CFLAGS += -DBUILD_ID='\"$(FIRMWARE_BUILD_NAME)\"'
build_firmware: CFLAGS += -DSCM_REVISION='\"$(SCM_REVISION)\"'
build_firmware: templates build_cross ## build firmware with frozen modules
	$(SCONS) CFLAGS="$(CFLAGS)" PRODUCTION="$(PRODUCTION)" TREZOR_MODEL="$(TREZOR_MODEL)" PRODUCTION_MODEL="$(PRODUCTION_MODEL)" PYOPT="$(PYOPT)" BITCOIN_ONLY="$(BITCOIN_ONLY)" UI2="$(UI2)" USE_THD89="$(USE_THD89)" $(FIRMWARE_BUILD_DIR)/firmware.bin
	cp $(FIRMWARE_BUILD_DIR)/firmware.bin $(FIRMWARE_BUILD_DIR)/$(FIRMWARE_BUILD_NAME).bin

build_unix: templates ## build unix port
	$(SCONS) CFLAGS="$(CFLAGS)" $(UNIX_BUILD_DIR)/trezor-emu-core $(UNIX_PORT_OPTS) TREZOR_MODEL="$(TREZOR_MODEL)" BITCOIN_ONLY="$(BITCOIN_ONLY)" TREZOR_EMULATOR_ASAN="$(ADDRESS_SANITIZER)" UI2="$(UI2)" UNAME_S="$(UNAME_S)" CURRENT_DIR="$(CURRENT_DIR)"

build_unix_frozen: templates build_cross ## build unix port with frozen modules
	$(SCONS) CFLAGS="$(CFLAGS)" $(UNIX_BUILD_DIR)/trezor-emu-core $(UNIX_PORT_OPTS) TREZOR_MODEL="$(TREZOR_MODEL)" PYOPT="$(PYOPT)" BITCOIN_ONLY="$(BITCOIN_ONLY)" TREZOR_EMULATOR_ASAN="$(ADDRESS_SANITIZER)" UI2="$(UI2)" TREZOR_MEMPERF="$(TREZOR_MEMPERF)" TREZOR_EMULATOR_FROZEN=1

build_unix_debug: templates ## build unix port
	$(SCONS) --max-drift=1 CFLAGS="$(CFLAGS)" $(UNIX_BUILD_DIR)/trezor-emu-core $(UNIX_PORT_OPTS) TREZOR_MODEL="$(TREZOR_MODEL)" BITCOIN_ONLY="$(BITCOIN_ONLY)" TREZOR_EMULATOR_ASAN=1 UI2="$(UI2)" TREZOR_EMULATOR_DEBUGGABLE=1

build_cross: ## build mpy-cross port
	$(MAKE) -C vendor/micropython/mpy-cross $(CROSS_PORT_OPTS)

## clean commands:

clean: clean_boardloader clean_bootloader clean_firmware clean_unix clean_cross ## clean all
	rm -f ".sconsign.dblite"

clean_boardloader: ## clean boardloader build
	rm -rf $(BOARDLOADER_BUILD_DIR)

clean_boardloader_reflash_dev: ## clean boardloader_reflash_dev build
	rm -rf $(BOAR_DEV_REFLASH_BUILD_DIR)

clean_bootloader: ## clean bootloader build
	rm -rf $(BOOTLOADER_BUILD_DIR)

clean_firmware: ## clean firmware build
	rm -rf $(FIRMWARE_BUILD_DIR) $(RUST_BUILD_DIR)

clean_unix: ## clean unix build
	rm -rf $(UNIX_BUILD_DIR) $(RUST_BUILD_DIR)

clean_cross: ## clean mpy-cross build
	$(MAKE) -C vendor/micropython/mpy-cross clean $(CROSS_PORT_OPTS)

## flash commands:

flash_boardloader_jlink: $(BOARDLOADER_BUILD_DIR)/boardloader.bin ## flash bootloader using JLink
	embed/debugger_scripts/jlink/jlink_write.sh $(BOARDLOADER_BUILD_DIR)/boardloader.bin $(BOARDLOADER_START)

flash_boardloader_reflash_dev_jlink: $(BOAR_DEV_REFLASH_BUILD_DIR)/boardloader_reflash_dev.bin ## flash boardloader_reflash_dev using JLink
	embed/debugger_scripts/jlink/jlink_write.sh $(BOAR_DEV_REFLASH_BUILD_DIR)/boardloader_reflash_dev.bin $(BOOTLOADER_START)

flash_bootloader_jlink: $(BOOTLOADER_BUILD_DIR)/bootloader.bin ## flash bootloader using JLink
	embed/debugger_scripts/jlink/jlink_write.sh $(BOOTLOADER_BUILD_DIR)/bootloader.bin $(BOOTLOADER_START)

erase_firmware_jlink:
	embed/debugger_scripts/jlink/jlink_erase.sh $(FIRMWARE_P1_START) 0x180000
	embed/debugger_scripts/jlink/jlink_erase.sh $(FIRMWARE_P2_START) 0x200000

flash_firmware_jlink: $(FIRMWARE_BUILD_DIR)/firmware.bin ## flash firmware using JLink. file names must end in .bin for JLink
	cp -f $<.p1 $<.p1.bin
	cp -f $<.p2 $<.p2.bin
	embed/debugger_scripts/jlink/jlink_write.sh $<.p1.bin $(FIRMWARE_P1_START)
	embed/debugger_scripts/jlink/jlink_write.sh $<.p2.bin $(FIRMWARE_P2_START)
	rm $<.p1.bin
	rm $<.p2.bin

dump_firmware_jlink:
	embed/debugger_scripts/jlink/jlink_read.sh $(FIRMWARE_BUILD_DIR)/firmware.readback.p1.bin $(FIRMWARE_P1_START) 0x180000
	embed/debugger_scripts/jlink/jlink_read.sh $(FIRMWARE_BUILD_DIR)/firmware.readback.p2.bin $(FIRMWARE_P2_START) 0x200000

jlink_device_reboot: ## cause a system reset using JLink
	embed/debugger_scripts/jlink/jlink_rebootDevice.sh 0
jlink_device_enter_board:
	embed/debugger_scripts/jlink/jlink_rebootDevice.sh 1
jlink_device_enter_boot:
	embed/debugger_scripts/jlink/jlink_rebootDevice.sh 2

GDB = arm-none-eabi-gdb --nx -ex 'set remotetimeout unlimited' -ex 'set confirm off' -ex 'target remote 127.0.0.1:3333' -ex 'monitor reset halt'

gdb_boardloader: $(BOARDLOADER_BUILD_DIR)/boardloader.elf ## start remote gdb session to openocd with boardloader symbols
	$(GDB) $<

gdb_bootloader: $(BOOTLOADER_BUILD_DIR)/bootloader.elf ## start remote gdb session to openocd with bootloader symbols
	$(GDB) $<

gdb_firmware: $(FIRMWARE_BUILD_DIR)/firmware.elf ## start remote gdb session to openocd with firmware symbols
	$(GDB) $<

## misc commands:

binctl: ## print info about binary files
	./tools/headertool.py $(BOOTLOADER_BUILD_DIR)/bootloader.bin
	./tools/headertool.py $(FIRMWARE_BUILD_DIR)/firmware.bin

bloaty: ## run bloaty size profiler
	bloaty -d symbols -n 0 -s file $(FIRMWARE_BUILD_DIR)/firmware.elf | less
	bloaty -d compileunits -n 0 -s file $(FIRMWARE_BUILD_DIR)/firmware.elf | less

sizecheck: ## check sizes of binary files
	test $(BOARDLOADER_MAXSIZE) -ge $(shell wc -c < $(BOARDLOADER_BUILD_DIR)/boardloader.bin)
	test $(BOOTLOADER_MAXSIZE) -ge $(shell wc -c < $(BOOTLOADER_BUILD_DIR)/bootloader.bin)
	test $(FIRMWARE_P1_MAXSIZE) -ge $(shell wc -c < $(FIRMWARE_BUILD_DIR)/firmware.bin.p1)
	test $(FIRMWARE_P2_MAXSIZE) -ge $(shell wc -c < $(FIRMWARE_BUILD_DIR)/firmware.bin.p2)
	test $(FIRMWARE_MAXSIZE) -ge $(shell wc -c < $(FIRMWARE_BUILD_DIR)/firmware.bin)

combine: ## combine boardloader + bootloader + prodtest into one combined image
	./tools/combine_firmware \
		$(BOARDLOADER_START) $(BOARDLOADER_BUILD_DIR)/boardloader.bin \
		$(BOOTLOADER_START) $(BOOTLOADER_BUILD_DIR)/bootloader.bin \
		> $(PRODTEST_BUILD_DIR)/combined.bin

coverage:  # generate coverage report
	./tools/coverage-report
