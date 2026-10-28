CC := clang
FIRMWARE := ../firmware/baccable
CPPFLAGS := -I$(FIRMWARE)
CFLAGS := -std=c11 -Wall -Wextra -Werror -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer
HEADERS := test_report.h $(shell find stubs $(FIRMWARE)/app $(FIRMWARE)/protocol $(FIRMWARE)/diagnostics $(FIRMWARE)/storage $(FIRMWARE)/features $(FIRMWARE)/vehicle $(FIRMWARE)/transport $(FIRMWARE)/platform $(FIRMWARE)/USB_DEVICE/App $(FIRMWARE)/third_party/printf -name '*.h')
SOURCES := test_core.c $(FIRMWARE)/protocol/slcan_codec.c $(FIRMWARE)/diagnostics/uds_decode.c \
 $(FIRMWARE)/storage/record_store.c

test: build/test_core build/test_transport build/test_catalog build/test_menu build/test_menu_large build/test_uart_menu build/test_input_repeat build/test_engine_status build/test_exhaust labels
	./build/test_rain_uart
	./build/test_rain_lights_c1
	./build/test_rain_lights_c2
	./build/test_rain_test_c1
	./build/test_rain_test_c2
	./build/test_rain_test_bh
	./build/test_dna_selector
	./build/test_fatfs
	./build/test_core
	./build/test_transport
	./build/test_catalog
	./build/test_catalog_large
	./build/test_menu
	./build/test_menu_large
	./build/test_uart_menu
	./build/test_input_repeat
	./build/test_engine_status
	./build/test_exhaust
	./build/test_drive_mode
	./build/test_drive_mode_c2
	./build/test_drive_mode_bh
	./build/test_upstream
	./build/test_parking_c2
	./build/test_parking_bh
	./build/test_usb_c2
	./build/test_usb_bh
	./build/test_body_display
	./build/test_body_display_large
	./build/test_usb_lifecycle
	./build/test_menu_my23
	./build/test_menu_my23_large
	./build/test_uart_rx_c2
	./build/test_uart_rx_bh

USB_AUX_SOURCES := test_usb_aux.c $(FIRMWARE)/features/usb_modes.c
build/test_usb_c2: $(USB_AUX_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) -I$(FIRMWARE)/USB_DEVICE/App $(CFLAGS) -DC2_FLAVOR=1 $(USB_AUX_SOURCES) -o $@
build/test_usb_bh: $(USB_AUX_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) -I$(FIRMWARE)/USB_DEVICE/App $(CFLAGS) -DBH_FLAVOR=1 $(USB_AUX_SOURCES) -o $@

test: build/test_usb_c2 build/test_usb_bh

build/test_catalog: test_catalog.c $(FIRMWARE)/diagnostics/parameter_catalog.c $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DGIT_VERSION=\"test\" test_catalog.c $(FIRMWARE)/diagnostics/parameter_catalog.c -lm -o $@

TRANSPORT_SOURCES := test_transport.c $(FIRMWARE)/transport/can_bus.c $(FIRMWARE)/USB_DEVICE/App/usbd_cdc_if.c
build/test_transport: $(TRANSPORT_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) -I$(FIRMWARE)/USB_DEVICE/App $(CFLAGS) -DGIT_VERSION=\"test\" $(TRANSPORT_SOURCES) -o $@

build/test_core: $(SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -lm -o $@

.PHONY: test test-menu-fast labels

test-menu-fast: build/test_menu build/test_menu_my23 build/test_input_repeat build/test_drive_mode_c2 labels
	./build/test_menu
	./build/test_menu_my23
	./build/test_input_repeat
	./build/test_drive_mode_c2

labels:
	python3 test_menu_labels.py

build/test_input_repeat: test_input_repeat.c $(FIRMWARE)/features/menu_input.c $(HEADERS) Makefile
	@mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) test_input_repeat.c $(FIRMWARE)/features/menu_input.c -o $@

build/test_engine_status: test_engine_status.c $(FIRMWARE)/vehicle/engine_status.c $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DC2_FLAVOR=1 test_engine_status.c \
		$(FIRMWARE)/vehicle/engine_status.c -o $@

build/test_exhaust: test_exhaust.c $(FIRMWARE)/features/exhaust.c $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DC1_FLAVOR=1 test_exhaust.c \
		$(FIRMWARE)/features/exhaust.c -lm -o $@

DRIVE_MODE_SOURCES := test_drive_mode.c $(FIRMWARE)/features/dna_selector.c $(FIRMWARE)/vehicle/mirrors_frames.c $(FIRMWARE)/features/drive_mode_check.c $(FIRMWARE)/vehicle/drive_mode.c $(FIRMWARE)/vehicle/display_frames.c $(FIRMWARE)/protocol/checksum.c
build/test_drive_mode: $(DRIVE_MODE_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DC1_FLAVOR=1 -DGIT_VERSION=\"test\" -Wno-unused-parameter $(DRIVE_MODE_SOURCES) -o $@

build/test_drive_mode_c2: $(DRIVE_MODE_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DC2_FLAVOR=1 -Wno-unused-parameter $(DRIVE_MODE_SOURCES) -o $@
build/test_drive_mode_bh: $(DRIVE_MODE_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DBH_FLAVOR=1 -Wno-unused-parameter $(DRIVE_MODE_SOURCES) -o $@

test: build/test_drive_mode build/test_drive_mode_c2 build/test_drive_mode_bh

MENU_SOURCES := test_menu.c $(addprefix $(FIRMWARE)/, features/menu.c features/menu_model.c \
 features/menu_input.c features/menu_demo.c features/my23_ui.c features/favorite_parameters.c features/ui_entry.c features/display_stream.c features/oil_bar_animation.c features/ibs_override.c features/board_sync.c features/drive_mode_check.c features/dashboard.c features/dashboard_format.c features/oil_level.c features/rain_test.c features/rain_lights.c \
 diagnostics/parameter_cache.c diagnostics/parameter_request.c diagnostics/parameter_catalog.c \
 protocol/checksum.c vehicle/steering_controls.c diagnostics/uds_decode.c diagnostics/fault_reader.c settings/persistence.c settings/setup_menu.c settings/setup_entries.c vehicle/vehicle_profile.c \
 third_party/printf/printf.c)
MENU_FIXTURES := test_oil_level.c test_my23_ui.c test_idle_latin1.c test_menu_contract.c test_setup_ui.c test_unified_ui.c
build/test_menu: $(MENU_SOURCES) $(MENU_FIXTURES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DC1_FLAVOR=1 -DGIT_VERSION=\"test\" $(MENU_SOURCES) -lm -o $@
build/test_menu_large: $(MENU_SOURCES) $(MENU_FIXTURES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DC1_FLAVOR=1 -DLARGE_DISPLAY -DGIT_VERSION=\"test\" $(MENU_SOURCES) -lm -o $@

build/test_uart_menu: test_uart_menu.c $(FIRMWARE)/transport/board_uart.c $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DC1_FLAVOR=1 -DGIT_VERSION=\"test\" test_uart_menu.c $(FIRMWARE)/transport/board_uart.c -o $@

UPSTREAM_SOURCES := test_upstream.c $(addprefix $(FIRMWARE)/, features/ui_entry.c diagnostics/fault_reader.c protocol/elm327.c transport/diagnostic_link.c features/ibs_override.c features/usb_modes.c third_party/printf/printf.c)
build/test_upstream: $(UPSTREAM_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) -I$(FIRMWARE)/USB_DEVICE/App $(CFLAGS) -DC1_FLAVOR=1 -DGIT_VERSION=\"test\" $(UPSTREAM_SOURCES) -lm -o $@

test: build/test_upstream

PARKING_SOURCES := test_parking_link.c $(addprefix $(FIRMWARE)/, features/parking.c features/parking_mirrors.c state/parking.c transport/diagnostic_link.c)
build/test_parking_c2: $(PARKING_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DC2_FLAVOR=1 -DGIT_VERSION=\"test\" $(PARKING_SOURCES) -lm -o $@
build/test_parking_bh: $(PARKING_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DBH_FLAVOR=1 -DGIT_VERSION=\"test\" $(PARKING_SOURCES) -lm -o $@

test: build/test_parking_c2 build/test_parking_bh

build/test_catalog_large: test_catalog.c $(FIRMWARE)/diagnostics/parameter_catalog.c $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DLARGE_DISPLAY -DGIT_VERSION=\"test\" test_catalog.c $(FIRMWARE)/diagnostics/parameter_catalog.c -lm -o $@

test: build/test_catalog_large

BODY_DISPLAY_SOURCES := test_body_display.c $(FIRMWARE)/features/body.c $(FIRMWARE)/features/display_stream.c $(FIRMWARE)/features/oil_bar_animation.c $(FIRMWARE)/features/my23_ui.c $(FIRMWARE)/state/display.c
build/test_body_display: test_oil_animation.c $(BODY_DISPLAY_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DBH_FLAVOR=1 $(BODY_DISPLAY_SOURCES) -o $@
build/test_body_display_large: test_oil_animation.c $(BODY_DISPLAY_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DBH_FLAVOR=1 -DLARGE_DISPLAY $(BODY_DISPLAY_SOURCES) -o $@
test: build/test_body_display build/test_body_display_large

# Real USB stack coverage: the ordinary mode tests intentionally mock USB hardware.
USB_STACK := $(FIRMWARE)/Middlewares/ST/STM32_USB_Device_Library
USB_LIFECYCLE_SOURCES := test_usb_lifecycle.c $(FIRMWARE)/USB_DEVICE/App/usb_device.c $(wildcard $(USB_STACK)/Core/Src/*.c) $(wildcard $(USB_STACK)/Class/MSC/Src/*.c) $(wildcard $(USB_STACK)/Class/CDC/Src/*.c)
build/test_usb_lifecycle: $(USB_LIFECYCLE_SOURCES) $(shell find usb_lifecycle_stubs -type f) Makefile
	@mkdir -p build
	$(CC) -Iusb_lifecycle_stubs -I$(USB_STACK)/Core/Inc -I$(USB_STACK)/Class/MSC/Inc -I$(USB_STACK)/Class/CDC/Inc -I$(FIRMWARE)/USB_DEVICE/App $(CPPFLAGS) $(CFLAGS) -Wno-unused-parameter -Wno-missing-field-initializers -DBH_FLAVOR=1 $(USB_LIFECYCLE_SOURCES) -o $@
test: build/test_usb_lifecycle

# IPC generation and legacy transport width are independent build preferences.
build/test_menu_my23: $(MENU_SOURCES) $(MENU_FIXTURES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DC1_FLAVOR=1 -DIPC_MY23_IS_INSTALLED -DGIT_VERSION=\"test\" $(MENU_SOURCES) -lm -o $@
build/test_menu_my23_large: $(MENU_SOURCES) $(MENU_FIXTURES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DC1_FLAVOR=1 -DIPC_MY23_IS_INSTALLED -DLARGE_DISPLAY -DGIT_VERSION=\"test\" $(MENU_SOURCES) -lm -o $@
test: build/test_menu_my23 build/test_menu_my23_large

# Auxiliary boards consume the complete screen MTU and retain shorter commands.
build/test_uart_rx_c2: test_uart_menu.c $(FIRMWARE)/transport/board_uart.c $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DC2_FLAVOR=1 test_uart_menu.c $(FIRMWARE)/transport/board_uart.c -o $@
build/test_uart_rx_bh: test_uart_menu.c $(FIRMWARE)/transport/board_uart.c $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DBH_FLAVOR=1 test_uart_menu.c $(FIRMWARE)/transport/board_uart.c -o $@
test: build/test_uart_rx_c2 build/test_uart_rx_bh

build/test_fatfs: test_fatfs.c $(FIRMWARE)/third_party/fatfs/ff.c $(FIRMWARE)/third_party/fatfs/ff.h $(FIRMWARE)/third_party/fatfs/ffconf.h Makefile
	@mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) -Wno-unused-function test_fatfs.c $(FIRMWARE)/third_party/fatfs/ff.c -o $@
test: build/test_fatfs


build/test_dna_selector: test_dna_selector.c $(FIRMWARE)/features/dna_selector.c $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DBH_FLAVOR=1 $< $(FIRMWARE)/features/dna_selector.c -o $@

test: build/test_dna_selector

RAIN_LIGHT_SOURCES := test_rain_lights.c $(FIRMWARE)/features/rain_lights.c
build/test_rain_lights_c1: $(RAIN_LIGHT_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DC1_FLAVOR=1 $(RAIN_LIGHT_SOURCES) -o $@
build/test_rain_lights_c2: $(RAIN_LIGHT_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DC2_FLAVOR=1 $(RAIN_LIGHT_SOURCES) -o $@
build/test_rain_lights_bh: $(RAIN_LIGHT_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DBH_FLAVOR=1 $(RAIN_LIGHT_SOURCES) -o $@
test: build/test_rain_lights_c1 build/test_rain_lights_c2 build/test_rain_lights_bh

RAIN_TEST_SOURCES := test_rain_test.c $(FIRMWARE)/features/rain_test.c $(FIRMWARE)/features/rain_lights.c $(FIRMWARE)/third_party/printf/printf.c
build/test_rain_test_c1: $(RAIN_TEST_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DC1_FLAVOR=1 $(RAIN_TEST_SOURCES) -o $@
build/test_rain_test_c2: $(RAIN_TEST_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DC2_FLAVOR=1 $(RAIN_TEST_SOURCES) -o $@
build/test_rain_test_bh: $(RAIN_TEST_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DBH_FLAVOR=1 $(RAIN_TEST_SOURCES) -o $@
test: build/test_rain_test_c1 build/test_rain_test_c2 build/test_rain_test_bh

RAIN_UART_SOURCES := test_rain_uart.c $(FIRMWARE)/features/rain_lights.c $(FIRMWARE)/features/rain_test.c $(FIRMWARE)/transport/board_uart.c $(FIRMWARE)/features/usb_modes.c $(FIRMWARE)/third_party/printf/printf.c
build/test_rain_uart: $(RAIN_UART_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -I$(FIRMWARE)/USB_DEVICE/App -DC1_FLAVOR=1 $(RAIN_UART_SOURCES) -o $@
test: build/test_rain_uart


# Scoped vehicle-profile/UI foundation (P0-P7). Built and run only via the
# test-vehicle-ui target and its manifest; these are not part of the broad "test".
READING_CATALOG_SOURCES := $(FIRMWARE)/features/reading_catalog.c $(FIRMWARE)/vehicle/vehicle_profile.c
READING_FORMAT_SOURCES := $(FIRMWARE)/features/reading_format.c $(READING_CATALOG_SOURCES)
FAVORITE_LAYOUT_SOURCES := $(FIRMWARE)/features/favorite_layout.c $(READING_FORMAT_SOURCES)
UI_LAYOUT_SOURCES := $(FIRMWARE)/storage/ui_layout_store.c $(FAVORITE_LAYOUT_SOURCES)

build/test_runner_selection: test_runner_selection.c $(HEADERS) Makefile
	@mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) test_runner_selection.c -o $@

build/test_vehicle_profile: test_vehicle_profile.c $(FIRMWARE)/vehicle/vehicle_profile.c $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) test_vehicle_profile.c $(FIRMWARE)/vehicle/vehicle_profile.c -o $@

build/test_reading_catalog: test_reading_catalog.c $(READING_CATALOG_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) test_reading_catalog.c $(READING_CATALOG_SOURCES) -o $@

build/test_reading_layout: test_reading_layout.c $(READING_FORMAT_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) test_reading_layout.c $(READING_FORMAT_SOURCES) -o $@

build/test_favorite_layout: test_favorite_layout.c $(FAVORITE_LAYOUT_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) test_favorite_layout.c $(FAVORITE_LAYOUT_SOURCES) -o $@

build/test_ui_layout_store: test_ui_layout_store.c $(UI_LAYOUT_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) test_ui_layout_store.c $(UI_LAYOUT_SOURCES) -o $@

.PHONY: test-vehicle-ui
test-vehicle-ui:
	python3 run_vehicle_ui_tests.py --manifest $(MANIFEST) --run
