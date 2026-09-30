#include "features/body.h"
#include "features/display_stream.h"
#include "features/ipc_display_test.h"
#include "features/my23_ui.h"
#include "features/parking_mirrors.h"
#include "storage/flash_records.h"

_Static_assert(DISPLAY_OUTPUT_LENGTH == MY23_L1_VISIBLE + 1U + MY23_L2_VISIBLE,
               "MY23 lines must fill the complete CAN display stream");

/* Safe default pacing; faster intervals are temporary vehicle-test options. */
#define DISPLAY_FRAGMENT_INTERVAL_MS 50U
#define DISPLAY_KEEPALIVE_INTERVAL_MS 1000U
#define DISPLAY_FACTORY_SETTLE_MS 250U
#define DISPLAY_FACTORY_GUARD_MS 100U
#define DISPLAY_FACTORY_MAX_DEFER_MS 1000U
#define DISPLAY_FACTORY_MAX_FRAMES 32U

#if defined(BACCABLE_BH)

static DisplayStream screen;
static uint8_t display_pace, display_method;
static uint32_t display_options_updated;
static bool display_options_leased;
static const uint8_t display_intervals[] = {50, 20, 10};

static void reset_screen(void) {
    display_stream_reset(&screen);
    display_stream_set_full(&screen, display_method == 0);
}
void body_display_options(uint8_t pace, uint8_t method) {
    if (pace >= 3 || method >= 2) return;
    bool changed = pace != display_pace || method != display_method;
    display_pace = pace;
    display_method = method;
    display_options_updated = currentTime;
    display_options_leased = pace || method;
    if (changed) {
        display_stream_set_full(&screen, method == 0);
        display_stream_restart(&screen);
    }
}

static bool screen_my23;
static uint32_t last_full_refresh;
static uint32_t factory_last_frame_time;
static uint32_t factory_defer_started;
static uint8_t factory_staging[DISPLAY_FACTORY_MAX_FRAMES][8];
static uint8_t factory_saved[DISPLAY_FACTORY_MAX_FRAMES][8];
static uint8_t factory_next, factory_total, factory_code, factory_saved_total, factory_restore_index;
static bool factory_collecting, factory_saved_valid, factory_interrupted, factory_open;
static bool factory_bounded_restart, factory_restore, menu_active;

static bool nonblank(const uint8_t *text, unsigned length) {
    for (unsigned i = 0; i < length; ++i) {
        if (text[i] != ' ')
            return true;
    }
    return false;
}

static bool audio_text_code(uint8_t info_code) {
    /* Vehicle media inputs and the observed CarPlay text context. */
    return (info_code >= 0x05 && info_code <= 0x09) || info_code == 0x21;
}

static void release_display(void) {
    menu_active = false;
    factory_interrupted = false;
    reset_screen();
    if (factory_saved_valid) {
        factory_restore = true;
        factory_restore_index = 0;
    } else {
        uint8_t blank[DISPLAY_OUTPUT_LENGTH];
        memset(blank, ' ', sizeof(blank));
        uint8_t count = screen_my23 ? DISPLAY_FRAGMENT_COUNT : DISPLAY_LEGACY_FRAGMENT_COUNT;
        display_state.telematic_display_info_field_total_frame_number = count - 1;
        display_stream_submit_parts(&screen, blank, count);
    }
}

/* Accept new dashboard content when BACCAble display output is allowed. */
void body_display_submit(const uint8_t *text) {
    if (chassis_state.stability_inverted)
        return;
    if (text[0] == IPC_TEST_SENTINEL) {
        if (ipc_display_test_select(text[1], text[2], currentTime)) {
            menu_active = false;
            factory_restore = false;
            reset_screen();
        }
        return;
    }
    ipc_display_test_stop();
    bool my23 = text[0] == MY23_PACKET_MARKER;
    if (!nonblank(text + my23, my23 ? MY23_PACKET_SIZE - 1 : DASHBOARD_MESSAGE_MAX_LENGTH)) {
        release_display();
        return;
    }

    uint8_t output[DISPLAY_OUTPUT_LENGTH];
    memset(output, ' ', sizeof(output));
    unsigned length = DASHBOARD_MESSAGE_MAX_LENGTH;
    if (screen_my23 != my23) reset_screen();
    screen_my23 = my23;
    if (my23) {
        /* Fixed protocol fields: L2 never shifts with L1's text length. */
        memcpy(output, text + 1, MY23_L1_VISIBLE);
        output[MY23_L1_VISIBLE] = '\r';
        memcpy(output + MY23_L1_VISIBLE + 1, text + 1 + MY23_L1_VISIBLE, MY23_L2_VISIBLE);
    } else {
        while (length && text[length - 1] == ' ')
            --length;
        memcpy(output, text, length);
        output[length] = '\r';
        memcpy(output + length + 1, DISPLAY_FOOTER, DISPLAY_FOOTER_LENGTH);
    }
    bool opening = !menu_active;
    menu_active = true;
    factory_restore = false;
    uint8_t count = screen_my23 ? DISPLAY_FRAGMENT_COUNT : DISPLAY_LEGACY_FRAGMENT_COUNT;
    display_state.telematic_display_info_field_total_frame_number = count - 1;
    display_stream_submit_parts(&screen, output, count);
    if (opening)
        display_stream_restart(&screen);
}

/* Restore BACCAble content after factory display activity when allowed. */
void body_display_refresh(void) {
    if (!chassis_state.stability_inverted)
        display_stream_refresh(&screen);
}

/* Track complete media text and defer menu recovery until factory traffic settles. */
void body_display_factory_frame(const uint8_t data[8], uint8_t dlc) {
    if (dlc < 2)
        return;
    uint8_t total_frame = (data[0] >> 3) & 0x1F;
    uint8_t frame_number = ((data[0] & 0x07) << 2) | (data[1] >> 6);
    uint8_t info_code = data[1] & 0x3F;
    factory_last_frame_time = currentTime;
    factory_open = frame_number < total_frame;
    if ((menu_active || ipc_display_test_active()) && !factory_interrupted) {
        factory_interrupted = true;
        factory_defer_started = currentTime;
        factory_bounded_restart = false;
    }
    if (factory_restore)
        factory_restore = false; /* The radio has taken over the display itself. */

    /* Only complete text transfers select a source or replace the saved radio text. */
    if (dlc != 8 || !total_frame || !audio_text_code(info_code) ||
        total_frame >= DISPLAY_FACTORY_MAX_FRAMES) {
        factory_collecting = false;
        return;
    }
    if (frame_number == 0) {
        factory_collecting = true;
        factory_total = total_frame;
        factory_code = info_code;
        factory_next = 0;
    }
    if (!factory_collecting || total_frame != factory_total || info_code != factory_code ||
        frame_number != factory_next) {
        factory_collecting = false;
        return;
    }
    memcpy(factory_staging[factory_next], data, 8);
    ++factory_next;
    if (frame_number == total_frame) {
        memcpy(factory_saved, factory_staging, (total_frame + 1U) * 8U);
        factory_saved_total = total_frame;
        factory_saved_valid = true;
        factory_collecting = false;
        display_state.telematic_display_info_field_info_code = info_code;
    }
}

/* Prepare body-bus features and restore saved mirror positions. */
void body_init() {
    display_pace = display_method = 0;
    display_options_leased = false;
    reset_screen();
    // let's open the can bus because we may need data
    can_set_bitrate(CAN_BITRATE_125K); // set can speed to 125kpbs
    can_enable();                      // enable can port
    display_state.telematic_display_info_field_total_frame_number = DISPLAY_FRAGMENT_COUNT - 1U;

    // prepare msg to send:
    // total frame number is on byte 0 from bit 7 to 3
    display_state.telematic_display_info_msg_data[0] =
        (display_state.telematic_display_info_msg_data[0] & ~0xF8) |
        ((display_state.telematic_display_info_field_total_frame_number << 3) & 0xF8);
    // infoCode is on byte1 from bit 5 to 0 (0x12=phone connected, 0x13=phone disconnected, 0x15=call in
    // progress, 0x17=call in wait, 0x18=call terminated, 0x11=clear display, ...)
    display_state.telematic_display_info_msg_data[1] =
        (display_state.telematic_display_info_msg_data[1] & ~0x3F) |
        ((display_state.telematic_display_info_field_info_code) & 0x3F);
    // Single-byte IPC glyphs occupy the low bytes; the high bytes remain zero.
    display_state.telematic_display_info_msg_data[2] = 0;
    display_state.telematic_display_info_msg_data[4] = 0;
    display_state.telematic_display_info_msg_data[6] = 0;

    // load stored params
    mirrors_state.left_park_mirror_horizontal_pos = (uint8_t)mirror_positions_read(1);
    mirrors_state.left_park_mirror_vertical_pos = (uint8_t)mirror_positions_read(2);
    mirrors_state.right_park_mirror_horizontal_pos = (uint8_t)mirror_positions_read(3);
    mirrors_state.right_park_mirror_vertical_pos = (uint8_t)mirror_positions_read(4);
    mirrors_state.park_mirror_operative_position_not_stored = (uint8_t)mirror_positions_read(5);
    mirrors_state.left_mirror_horizontal_operative_pos = (uint8_t)mirror_positions_read(6);
    mirrors_state.left_mirror_vertical_operative_pos = (uint8_t)mirror_positions_read(7);
    mirrors_state.right_mirror_horizontal_operative_pos = (uint8_t)mirror_positions_read(8);
    mirrors_state.right_mirror_vertical_operative_pos = (uint8_t)mirror_positions_read(9);
}

/* Update dashboard content, mirrors and enabled body-bus functions. */
void body_process() {
    if (display_options_leased && currentTime - display_options_updated >= 5000U)
        body_display_options(0, 0);
    if (chassis_state.stability_inverted) {
        reset_screen();
        ipc_display_test_stop();
        menu_active = false;
        factory_interrupted = false;
        factory_restore = false;
    } else {
        if (ipc_display_test_expired(currentTime)) {
            ipc_display_test_stop();
            release_display();
        }
        if (factory_restore &&
            currentTime - display_state.last_sent_telematic_display_info_msg_time >= DISPLAY_FRAGMENT_INTERVAL_MS) {
            if (can_tx(&display_state.telematic_display_info_msg_header,
                       factory_saved[factory_restore_index]) == HAL_OK) {
                display_state.last_sent_telematic_display_info_msg_time = currentTime;
                if (factory_restore_index++ == factory_saved_total)
                    factory_restore = false;
            }
        }
        if (factory_interrupted) {
            bool settled = (!factory_open && currentTime - factory_last_frame_time >= DISPLAY_FACTORY_GUARD_MS) ||
                           currentTime - factory_last_frame_time >= DISPLAY_FACTORY_SETTLE_MS;
            if (settled) {
                if (ipc_display_test_active())
                    ipc_display_test_restart(currentTime);
                else
                    display_stream_restart(&screen);
                last_full_refresh = currentTime;
                factory_interrupted = false;
            } else if (currentTime - factory_defer_started >= DISPLAY_FACTORY_MAX_DEFER_MS &&
                       !factory_bounded_restart) {
                /* Continuous factory traffic cannot suppress the menu forever. */
                if (ipc_display_test_active())
                    ipc_display_test_restart(currentTime);
                else
                    display_stream_restart(&screen);
                factory_bounded_restart = true;
            }
            if (!settled && !factory_bounded_restart)
                goto done;
        }
        if (!factory_restore && currentTime - display_state.last_sent_telematic_display_info_msg_time >=
            (ipc_display_test_active() ? DISPLAY_FRAGMENT_INTERVAL_MS : display_intervals[display_pace])) {
            if (ipc_display_test_active()) {
                uint8_t test_frame[8];
                ipc_display_test_refresh(currentTime);
                if (ipc_display_test_peek(test_frame) &&
                    can_tx(&display_state.telematic_display_info_msg_header, test_frame) == HAL_OK) {
                    ipc_display_test_accept();
                    display_state.last_sent_telematic_display_info_msg_time = currentTime;
                }
                goto done;
            }
            /* Reassert the menu even when neither C1 nor the radio changed it. */
            if (menu_active && currentTime - last_full_refresh >= DISPLAY_KEEPALIVE_INTERVAL_MS) {
                display_stream_refresh(&screen);
                last_full_refresh = currentTime;
            }
            uint8_t fragment, text[3];
            if (display_stream_peek(&screen, &fragment, text)) {
                uint8_t *data = display_state.telematic_display_info_msg_data;
                data[0] = (data[0] & ~0xF8) |
                          ((display_state.telematic_display_info_field_total_frame_number << 3) & 0xF8);
                data[0] = (data[0] & ~0x07) | ((fragment >> 2) & 0x07);
                data[1] = ((fragment << 6) & 0xC0) |
                          (display_state.telematic_display_info_field_info_code & 0x3F);
                for (unsigned i = 0; i < 3; ++i) {
                    uint16_t cp = screen_my23 ? my23_codepoint(text[i]) : text[i];
                    data[2 + 2 * i] = cp >> 8;
                    data[3 + 2 * i] = cp;
                }
                if (can_tx(&display_state.telematic_display_info_msg_header, data) == HAL_OK) {
                    display_stream_accept(&screen);
                    display_state.last_sent_telematic_display_info_msg_time = currentTime;
                }
            }
        }
    }

done:
    parking_mirrors_process();
}

/* Remember the mirror positions needed for parking and normal driving. */
uint8_t mirror_positions_save(void) {
    uint16_t values[9] = {mirrors_state.left_park_mirror_horizontal_pos,
                          mirrors_state.left_park_mirror_vertical_pos,
                          mirrors_state.right_park_mirror_horizontal_pos,
                          mirrors_state.right_park_mirror_vertical_pos,
                          mirrors_state.park_mirror_operative_position_not_stored,
                          mirrors_state.left_mirror_horizontal_operative_pos,
                          mirrors_state.left_mirror_vertical_operative_pos,
                          mirrors_state.right_mirror_horizontal_operative_pos,
                          mirrors_state.right_mirror_vertical_operative_pos};
    return flash_record_save(SETTINGS_RECORD, 0x201, values, sizeof(values)) ? 0 : 255;
}

/* Restore a saved mirror position, or report that it is unavailable. */
uint16_t mirror_positions_read(uint8_t id) {
    if (id < 1 || id > 9)
        return 0;
    uint16_t values[9] = {0, 0, 0, 0, 1, 0, 0, 0, 0};
    flash_record_load(SETTINGS_RECORD, 0x201, values, sizeof(values));
    return values[id - 1];
}

#endif
