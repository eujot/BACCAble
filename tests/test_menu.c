#include "features/drive_mode_check.h"
#include "test_report.h"
#include "features/menu.h"
#include "features/ui_entry.h"
#include "features/my23_ui.h"
#include "features/menu_demo.h"
#include "vehicle/vehicle_profile.h"
#include "vehicle/vehicle_editor.h"
#include "vehicle/steering_controls.h"
#include "features/ibs_override.h"
#include "diagnostics/fault_reader.h"
#include "features/periodic.h"
#include "features/display_stream.h"
#include "app/powertrain.h"
#include "diagnostics/parameter_cache.h"
#include "diagnostics/parameter_request.h"
#include "storage/flash_records.h"
#include "storage/record_store.h"
#include "storage/ui_layout_store.h"
#include "vehicle/vehicle_runtime.h"
#include <assert.h>
static uint8_t combined_record[SETUP_FLASH_PARAM_BUFFER_SIZE * 2 + VEHICLE_PROFILE_WIRE_SIZE];
static bool have_combined;
static bool real_flash;
static uint8_t flash_pages[3][2][2048];
static int flash_budget = -1;
static bool test_erase(void *ctx, unsigned page) {
    if (flash_budget == 0) return false;
    if (flash_budget > 0) --flash_budget;
    memset(((uint8_t (*)[2048])ctx)[page], 0xff, 2048); return true;
}
static bool test_program(void *ctx, unsigned page, size_t offset, uint16_t value) {
    if (flash_budget == 0) return false;
    if (flash_budget > 0) --flash_budget;
    uint8_t *dst = ((uint8_t (*)[2048])ctx)[page] + offset;
    dst[0] &= value; dst[1] &= value >> 8; return true;
}
static RecordStorage test_storage(unsigned slot) {
    assert(slot < 3);
    return (RecordStorage){.pages = {flash_pages[slot][0], flash_pages[slot][1]},
        .erase = test_erase, .program = test_program, .context = flash_pages[slot], .page_size = 2048};
}
#include <stdio.h>

SettingsState settings_state;
SecurityState security_state;
ChassisState chassis_state;
ComfortState comfort_state;
TelemetryState telemetry_state;
DiagnosticsState diagnostics_state;
RuntimeState runtime_state;
DashboardState dashboard_state;
StatisticsState statistics_state;
PedalState pedal_state;
MirrorsState mirrors_state;
DisplayState display_state;
const char *FW_VERSION = "BACCABLE test";
static uint32_t now;
static char screen[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
static uint8_t screen_packet[UART_SCREEN_BUFFER_SIZE];
static uint8_t display_token, sent_dna, sent_mirror;
static unsigned commands, queries, blank_screens;
static unsigned settings_writes, usb_applies;
static bool usb_needs_apply;
static uint16_t fail_type;
static uint16_t settings_record[SETUP_FLASH_PARAM_BUFFER_SIZE];
static size_t settings_record_size;
static uint8_t profile_record[VEHICLE_PROFILE_WIRE_SIZE];
static bool have_profile;
static unsigned old_settings_reads;
static uint8_t last_command;
static uint8_t last_ipc_pace, last_ipc_method, last_driver_rearm;
static uint8_t last_dna_action, last_dna_token;
static bool uart_busy;
static bool fail_save;
uint32_t HAL_GetTick(void) { return now; }
uint8_t board_uart_send(const uint8_t *data, size_t size) {
    if (uart_busy)
        return 0;
    if (data[0] == BhBusIDparamString) {
        assert(size == UART_SCREEN_BUFFER_SIZE);
        memcpy(screen_packet, data, sizeof(screen_packet));
        memcpy(screen, data + 1, DASHBOARD_MESSAGE_MAX_LENGTH);
        screen[DASHBOARD_MESSAGE_MAX_LENGTH] = 0;
        bool blank = true;
        for (unsigned i = 0; i < DASHBOARD_MESSAGE_MAX_LENGTH; ++i) blank &= screen[i] == ' ';
        blank_screens += blank;
    } else {
        if (data[1] == BH_CMD_DISPLAY_PROFILE) display_token = data[2];
        if (data[1] == BHcmdFunctParkMirrorDisabled || data[1] == BHcmdFunctParkMirrorEnabled) sent_mirror = data[1];
        ++commands;
        last_command = size > 1 ? data[1] : 0;
        if (last_command == C2_Bh_cmdSetDriverAids) {
            assert(size == 5);
            last_driver_rearm = data[4];
        }
        if (data[0] == BhBusID && last_command == BH_CMD_DNA_DEFAULT) {
            assert(size == 5); sent_dna = data[2];
            last_dna_action = data[3]; last_dna_token = data[4];
        }
        if (data[0] == BhBusID && last_command == BH_CMD_IPC_OPTIONS) {
            assert(size == 4);
            last_ipc_pace = data[2]; last_ipc_method = data[3];
        }
    }
    return 1;
}
void pedal_booster_set_map(uint8_t map) { (void)map; }
void _putchar(char c) { (void)c; }
bool usb_modes_needs_apply(void) { return usb_needs_apply; }
void usb_modes_apply(void) { ++usb_applies; usb_needs_apply = false; }
void status_led_error(void) {}
void status_led_activity(void) {}
static uint8_t query_payload[8];
static uint32_t query_dlc;
static bool query_fail;
static uint32_t query_ecu;
uint32_t can_tx(CAN_TxHeaderTypeDef *header, uint8_t *data) {
    query_dlc = header->DLC;
    query_ecu = header->IDE == CAN_ID_EXT ? header->ExtId : header->StdId;
    memcpy(query_payload, data, sizeof(query_payload));
    ++queries;
    return query_fail ? HAL_ERROR : HAL_OK;
}
uint32_t can_forward(const CAN_RxHeaderTypeDef *header, uint8_t *data) {
    (void)header;
    (void)data;
    return HAL_OK;
}
static unsigned native_reads;
float native_parameter_read(uint8_t id) { ++native_reads; return (float)id + 0.5f; }
bool flash_record_load(unsigned slot, uint16_t type, void *data, size_t size) {
    if (real_flash) { RecordStorage storage = test_storage(slot); return record_load(&storage, type, data, size); }
    (void)slot;
    if (type == 0x108 && have_combined) { assert(size == sizeof(combined_record)); memcpy(data, combined_record, size); return true; }
    if (type == 0x101 && size == settings_record_size) {
        memcpy(data, settings_record, size);
        if (size == 80) ++old_settings_reads;
        return true;
    }
    if (type == 0x107 && have_profile) {
        assert(size == sizeof(profile_record));
        memcpy(data, profile_record, size);
        return true;
    }
    return false;
}
bool flash_record_save(unsigned slot, uint16_t type, const void *data, size_t size) {
    if (real_flash) { RecordStorage storage = test_storage(slot); return !fail_save && record_save(&storage, type, data, size); }
    (void)slot;
    if (type == 0x101) ++settings_writes;
    if (fail_save || type == fail_type)
        return false;
    if (type == 0x101) {
        assert(size == sizeof(settings_record));
        memcpy(settings_record, data, size);
        settings_record_size = size;
    }
    if (type == 0x108) {
        assert(size == sizeof(combined_record)); memcpy(combined_record, data, size); have_combined = true;
        memcpy(settings_record, data, sizeof(settings_record)); settings_record_size = sizeof(settings_record);
        memcpy(profile_record, (const uint8_t *)data + sizeof(settings_record), sizeof(profile_record)); have_profile = true;
    }
    if (type == 0x107) {
        assert(size == sizeof(profile_record));
        memcpy(profile_record, data, size);
        have_profile = true;
    }
    return true;
}
/* Import a real old settings payload, keep every slot unique, and preserve engine/suspension independently. */
static void test_settings_migration(void) {
    real_flash = false; have_combined = false; have_profile = false; settings_load_begin();
    memset(settings_record, 0, sizeof(settings_record));
    settings_record[35] = 1; /* V6 in the old record. */
    settings_record[37] = 3; /* Race. */
    settings_record[39] = 2; /* Q4. */
#ifdef LARGE_DISPLAY
    const unsigned old_size = 82;
    settings_record[40] = 1;
#else
    const unsigned old_size = 80;
#endif
    settings_record_size = old_size;
    setup_load_from_flash();
    assert(old_settings_reads == (old_size == 80));
    assert(settings_state.gasoline_v6 == 1 && settings_state.default_suspension_softer == (old_size == 82));
    assert(settings_state.rain_lights_enabled == 0);
    assert(settings_state.default_drive_mode == 3 && settings_state.drivetrain == 2);
    {
        const VehicleProfile *profile = vehicle_profile_current();
        assert(profile->engine == VEHICLE_ENGINE_PETROL29);
        assert(profile->drivetrain == VEHICLE_DRIVETRAIN_Q4);
        assert(profile->display_selection != VEHICLE_DISPLAY_MY23);
        assert(!profile->confirmed);
    }
    for (unsigned i = 0; i < setup_params_count; ++i) {
        assert(setup_params[i].flash_index <= SETUP_FLASH_PARAM_BUFFER_SIZE);
        for (unsigned j = 0; j < i; ++j)
            assert(setup_params[i].flash_index != setup_params[j].flash_index);
    }
    settings_state.default_suspension_softer = 1;
    settings_state.rain_lights_enabled = 1;
    fail_save = true;
    assert(settings_save() != 0 && settings_record_size == old_size);
    fail_save = false;
    assert(settings_save() == 0 && settings_record_size == 84);
    assert(have_profile);
    assert(settings_record[35] == 1 && settings_record[40] == 1 && settings_record[41] == 1);
    settings_state.gasoline_v6 = 0;
    settings_state.default_suspension_softer = 0;
    setup_load_from_flash();
    assert(settings_state.gasoline_v6 == 1 && settings_state.default_suspension_softer == 1);
    assert(settings_state.rain_lights_enabled == 1);
    VehicleProfile edited = *vehicle_profile_current();
    edited.engine = VEHICLE_ENGINE_PETROL20;
    assert(settings_profile_save(&edited) == 0);
    assert(settings_record[35] == 0 && settings_record[40] == 1);
    assert(setup_read_flash_value(41, 0xffff) == 0 && setup_read_flash_value(41, 2) == 0);
    assert(vehicle_profile_current()->engine == VEHICLE_ENGINE_PETROL20);
    memset(&settings_state, 0, sizeof(settings_state));
    settings_writes = usb_applies = 0;
}

static void test_vehicle_editor_equipment(void) {
    VehicleProfile p;
    vehicle_profile_default(&p);
    vehicle_profile_commit(&p);

    vehicle_editor_begin();
    for (unsigned i = 0; i < 5; ++i)
        vehicle_editor_navigate(1);
    assert(!strcmp(vehicle_editor_field_label(), "Suspension"));

    char buf[24];
    assert(!strcmp(vehicle_editor_value_label(buf, sizeof(buf)), "Unknown"));

    vehicle_editor_select();
    vehicle_editor_navigate(1);
    assert(!strcmp(vehicle_editor_value_label(buf, sizeof(buf)), "Absent"));
    vehicle_editor_navigate(1);
    assert(!strcmp(vehicle_editor_value_label(buf, sizeof(buf)), "Present"));
    vehicle_editor_navigate(1);
    assert(!strcmp(vehicle_editor_value_label(buf, sizeof(buf)), "Unknown"));

    vehicle_editor_back();
    puts("PASS: vehicle editor edits tri-state equipment");
}


static void test_vehicle_editor(void) {
    VehicleProfile p;
    vehicle_profile_default(&p);
    p.model = VEHICLE_MODEL_GIULIA;
    p.engine = VEHICLE_ENGINE_PETROL20;
    p.drivetrain = VEHICLE_DRIVETRAIN_RWD;
    p.display_selection = VEHICLE_DISPLAY_MY23;
    vehicle_profile_commit(&p);

    vehicle_editor_begin();
    assert(vehicle_editor_state() == VEHICLE_EDITOR_BROWSE);
    assert(!strcmp(vehicle_editor_field_label(), "Model"));

    char buf[24];
    assert(!strcmp(vehicle_editor_value_label(buf, sizeof(buf)), "Giulia"));

    vehicle_editor_navigate(1);
    vehicle_editor_navigate(1);
    assert(!strcmp(vehicle_editor_field_label(), "Engine"));
    assert(!strcmp(vehicle_editor_value_label(buf, sizeof(buf)), "2.0 Petrol"));

    vehicle_editor_select();
    assert(vehicle_editor_state() == VEHICLE_EDITOR_EDIT);
    vehicle_editor_navigate(1);
    assert(!strcmp(vehicle_editor_value_label(buf, sizeof(buf)), "2.9 Petrol"));
    vehicle_editor_navigate(1);
    assert(!strcmp(vehicle_editor_value_label(buf, sizeof(buf)), "2.2 Diesel"));

    assert(vehicle_editor_back() && vehicle_editor_state() == VEHICLE_EDITOR_BROWSE);
    assert(vehicle_editor_back() && vehicle_editor_state() == VEHICLE_EDITOR_CONFIRM);
    assert(!vehicle_editor_back());
    puts("PASS: vehicle editor browses, edits and discards a profile draft");
}

static void test_my23_glyph_budget(void) {
    const char *symbols = "\xe2\x80\xba\xe2\x80\xa2\xe2\x96\xb2\xe2\x96\xbc\xe2\x9c\x93\xc3\x97\xc2\xb0\xc2\xb1\xe2\x86\x92\xe2\x80\xa6";
    const uint16_t expected[] = {0x203a, 0x2022, 0x25b2, 0x25bc, 0x2713, 0xd7, 0xb0, 0xb1, 0x2192, 0x2026};
    uint8_t encoded[16];
    memset(encoded, 'X', sizeof(encoded));
    assert(my23_text_encode(encoded, 10, symbols) == 10);
    for (unsigned i = 0; i < 10; ++i) assert(my23_codepoint(encoded[i]) == expected[i]);
    assert(encoded[10] == 'X');
    for (unsigned limit = 0; limit <= 10; ++limit) {
        memset(encoded, 'X', sizeof(encoded));
        assert(my23_text_encode(encoded, limit, symbols) == limit);
        assert(encoded[limit] == 'X');
    }
    uint8_t packet[MY23_PACKET_SIZE];
    my23_packet_mode(packet, UI_MODE_LIST, "123456789012345", "");
    assert(packet[1] == 0x80 && packet[MY23_L1_VISIBLE] == 0x87 &&
           packet[1 + MY23_L1_VISIBLE] == ' ');
    my23_packet_mode(packet, UI_MODE_LIST, "12345678901234", "next");
    assert(!memcmp(packet + 3, "12345678901234", 14) &&
           !memcmp(packet + 1 + MY23_L1_VISIBLE, "next", 4));
    my23_packet(packet, "1234567890123456EXTRA", "1234567890123456789012EXTRA");
    assert(!memcmp(packet + 1, "1234567890123456", MY23_L1_VISIBLE));
    assert(!memcmp(packet + 1 + MY23_L1_VISIBLE, "1234567890123456789012", 22));
    my23_packet(packet, "Gear", "RPM 3500");
    assert(packet[5] == ' ' && !memcmp(packet + 1 + MY23_L1_VISIBLE, "RPM 3500", 8));
    for (unsigned i = 5; i < 1 + MY23_L1_VISIBLE; ++i) assert(packet[i] == ' ');
}

static void test_shared_renderers(void) {
    char text[25];
    for (size_t size = 19; size <= 25; size += 6) {
        ui_render_toggle(text, size, "Auto rotate", false);
        assert(!strcmp(text, "Auto rotate: OFF"));
        ui_render_toggle(text, size, "Auto rotate", true);
        assert(!strcmp(text, "Auto rotate: ON"));
        ui_render_value(text, size, "ACC resume", "RES");
        assert(!strcmp(text, "ACC resume: RES"));
        ui_render_signed_number(text, size, "Pedal trim", 4, false);
        assert(!strcmp(text, "Pedal trim: +4"));
        ui_render_number(text, size, "Shift RPM", 3500, true);
        assert(!strcmp(text, size == 19 ? "* Shift RPM: 3500" : "* Shift RPM: " "\xAB" " 3500 " "\xBB"));
        ui_render_action(text, size, "Read faults");
        assert(!strcmp(text, "> Read faults"));
        ui_render_pending(text, size, "AWD req", "OFF");
        assert(!strcmp(text, "AWD req: OFF WAIT"));
        ui_render_unavailable(text, size, "Start engine");
        assert(!strcmp(text, "! Start engine"));
    }
    for (size_t size = 0; size < sizeof(text); ++size) {
        memset(text, 'X', sizeof(text));
        ui_render_toggle(text, size, "A very long label", false);
        for (size_t i = size; i < sizeof(text); ++i)
            assert(text[i] == 'X');
        if (size) assert(memchr(text, 0, size));
    }
}

#include "test_vehicle_fixes.c"

int main(void) {
#if defined(IPC_MY23_IS_INSTALLED)
    const char *suite = DASHBOARD_MESSAGE_MAX_LENGTH == 24 ? "menu-my23-24" : "menu-my23-18";
#elif defined(LARGE_DISPLAY)
    const char *suite = "menu-24";
#else
    const char *suite = "menu-18";
#endif
    const HostTest tests[] = {
        HOST_TEST(test_split_profile_migration),
        HOST_TEST(test_cancelled_same_did),
        HOST_TEST(test_profile_atomic_restart),
        HOST_TEST(test_profile_editor_failure),
        HOST_TEST(test_favorite_validation_restart),
        HOST_TEST(test_favorite_widths),
        HOST_TEST(test_favorite_policy_sources),
        HOST_TEST(test_favorite_visible_polling),
        HOST_TEST(test_profile_effective_options),
        HOST_TEST(test_runtime_display),
        HOST_TEST(test_restore_defaults_flow),

        HOST_TEST(test_settings_migration),
        HOST_TEST(test_vehicle_editor),
        HOST_TEST(test_vehicle_editor_equipment),
        HOST_TEST(test_my23_glyph_budget),
        HOST_TEST(test_shared_renderers),
    };
    host_tests_run(suite, tests, sizeof(tests) / sizeof(tests[0]));
    puts("PASS: settings migration, vehicle editor");
}
