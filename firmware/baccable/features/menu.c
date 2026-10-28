#include "features/menu.h"
#include "features/oil_level.h"
#include "features/body.h"
#include "features/ui_entry.h"
#include "features/my23_ui.h"
#include "features/display_stream.h"
#include "features/menu_demo.h"
#include "storage/ui_layout_store.h"
#include "features/favorite_layout.h"
#include "features/rain_test.h"
#include "diagnostics/fault_reader.h"
#include "features/ibs_override.h"
#include "app/powertrain.h"
#include "diagnostics/parameter_cache.h"
#include "diagnostics/parameter_request.h"
#include "storage/flash_records.h"
#include "vehicle/vehicle_editor.h"
#if defined(BACCABLE_C1)

typedef enum {
    ROOT,
    FAVORITES,
    GROUPS,
    VALUES,
    FUNCTIONS,
    SETTINGS,
    SETUP,
    INFO,
    IPC_OPTIONS,
    FAV_SETS, FAV_ROWS, FAV_ITEMS, FAV_PICK, FAV_FORMAT, SORT_EDIT,
    FAULTS, RAIN_TEST, VEHICLE
} MenuView;
typedef enum {
    ACTION_READ,
    ACTION_CLEAR,
    ACTION_DYNO,
    ACTION_ESC,
    ACTION_BRAKE,
    ACTION_AWD,
    ACTION_HAS,
    ACTION_EXHAUST,
    ACTION_STATS,
    ACTION_PEAK,
    ACTION_IBS,
    ACTION_LAUNCH,
    ACTION_RAIN_TEST,
    ACTION_COUNT
} MenuAction;
typedef struct {
    MenuAction id;
    uint8_t group;
    const char *name;
    UiEntryType type;
} ActionEntry;
static const ActionEntry actions[] = {
    {ACTION_EXHAUST, 0, "QV exhaust req", UI_ENTRY_ACTION},
    {ACTION_HAS, 1, "Press HAS button", UI_ENTRY_ACTION},
    {ACTION_ESC, 1, "Toggle ESC/TC", UI_ENTRY_CONDITIONAL_ACTION},
    {ACTION_DYNO, 2, "Toggle Dyno mode", UI_ENTRY_CONDITIONAL_ACTION},
    {ACTION_BRAKE, 2, "Brake override", UI_ENTRY_CONDITIONAL_ACTION},
    {ACTION_LAUNCH, 2, "Disable launch", UI_ENTRY_CONDITIONAL_ACTION},
    {ACTION_AWD, 2, "AWD off request", UI_ENTRY_CONDITIONAL_ACTION},
    {ACTION_READ, 3, "Read BCM faults", UI_ENTRY_CONDITIONAL_ACTION},
    {ACTION_CLEAR, 3, "Clear DTCs", UI_ENTRY_CONDITIONAL_ACTION},
    {ACTION_STATS, 3, "Reset best times", UI_ENTRY_ACTION},
    {ACTION_PEAK, 3, "Peak hold", UI_ENTRY_TOGGLE},
    {ACTION_IBS, 3, "IBS SOC override", UI_ENTRY_CONDITIONAL_ACTION},
    {ACTION_RAIN_TEST, 2, "Rain test", UI_ENTRY_ACTION}
};
typedef enum { REQUEST_NONE, REQUEST_WAIT, REQUEST_SENT, REQUEST_FAILED, REQUEST_TIMEOUT } RequestState;
typedef struct {
    uint32_t started;
    uint8_t state;
    bool target;
} ActionRequest;
static ActionRequest requests[ACTION_COUNT];
#define ACTION_TIMEOUT_MS 10000U
#define INFO_PAGES 10U
static const char *const roots[] = {"Favorites", "Readings", "Actions", "Settings", "Information", "Vehicle"};
static const char *const settings[] = {"Features", "Sort order", "Favorites", "BACCAble IPC"};
static char peer_versions[2][DASHBOARD_MESSAGE_MAX_LENGTH - 2];
static uint32_t peer_updated[2];
static uint8_t peer_seen[2];

/* Remember a board's version and when it last responded. */
void menu_peer_status(uint8_t peer, const uint8_t *version) {
    if (peer > 1)
        return;
    memcpy(peer_versions[peer], version, sizeof(peer_versions[peer]) - 1);
    peer_versions[peer][sizeof(peer_versions[peer]) - 1] = 0;
    peer_updated[peer] = currentTime;
    peer_seen[peer] = 1;
}
static MenuPreferences preferences;
static FavoriteLayout favorites_layout;
static UiLayoutRecord ui_layout;
static FavoriteSet favorite_draft;
/* Stable set identities; reading/editor lists share a separate cursor. */
static uint8_t selected_favorite[2];
static uint8_t favorite_set, favorite_row, favorite_slot, favorite_pick, favorite_format;
static bool favorite_confirm, sort_draft, sort_confirm;
static uint8_t saved_ui_layout[UI_LAYOUT_RECORD_SIZE];
static bool ui_layout_saved;
#define MENU_STORAGE_SIZE MENU_PREFS_SIZE

static MenuInput input;
static MenuView view = FAVORITES;
static uint8_t gasoline_v6, advanced_pages;
static uint8_t root, group = 1, function, setting, info, engine;
static uint8_t list[MENU_PAGE_CAPACITY], list_count, selection;
static uint8_t setup_last, fault_index;
/* Experimental display options never enter settings_state or Flash records. */
static uint8_t ipc_option, ipc_pace, ipc_method;
static uint32_t ipc_options_sent;
static bool ipc_options_sync;
static const char *const ipc_paces[] = {"Safe 50ms", "Quick 20ms", "Fast 10ms"};
/* Allow one complete display transfer between snapshots of rapid navigation. */
static uint32_t navigation_interval(void) {
    static const uint8_t fragment_ms[] = {50, 20, 10};
    return (settings_state.ipc_my23_is_installed ? DISPLAY_FRAGMENT_COUNT : DISPLAY_LEGACY_FRAGMENT_COUNT) *
           fragment_ms[ipc_pace] + 20U;
}
static void ipc_option_text(char *out, size_t size, unsigned option) {
    if (option == 0) snprintf_(out, size, "%s", ipc_paces[ipc_pace]);
    else if (option == 1) snprintf_(out, size, "Write %s", ipc_method ? "Delta" : "Full");
    else snprintf_(out, size, "Reset default");
}
static bool ipc_options_send(uint8_t pace, uint8_t method) {
    uint8_t command[] = {BhBusID, BH_CMD_IPC_OPTIONS, pace, method};
    if (!board_uart_send(command, sizeof(command))) return false;
    ipc_options_sent = currentTime;
    return true;
}
static uint32_t page_changed, last_render, last_query, notice_started;
static uint32_t navigation_last, navigation_sent;
static bool navigation_active, navigation_pending;
static bool demo_active;
static uint8_t demo_step;
static uint32_t demo_started;
static const char *notice;
static const char *notice_label = "Settings";
static char notice_text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
static uint8_t previous_text[UART_SCREEN_BUFFER_SIZE - 1], previous_valid;
static uint32_t previous_sent;
static uint8_t confirmed_action = 255;
static bool close_pending;
/* A failed exit stays explicit until the user retries or cancels it. */
static bool save_failed, save_close;
static MenuView save_destination;
static uint8_t saved_preferences[MENU_STORAGE_SIZE];
static bool preferences_saved;
#define MENU_IDLE_MS 30000U
#define EDITOR_IDLE_MS 60000U
#define NOTICE_CONFIRM_MS 750U
#define NOTICE_WARNING_MS 1800U
#define NOTICE_REQUEST_MS 1200U
#define NOTICE_SAVED_MS 3000U
static uint32_t notice_duration;
static uint32_t confirm_started, last_input;
static void build_pages(uint16_t selected);

/* Move through a list and continue from the other end at its boundary. */
static unsigned wrap(unsigned value, unsigned count, int delta) {
    if (!count)
        return 0;
    return (value + count + (delta < 0 ? count - 1 : 1)) % count;
}

static uint8_t setting_move(uint8_t current, int direction) { return wrap(current, 4, direction); }
static ReadingSample favorite_sample_cache[READING_COUNT];

/* Resolve a FavoriteSet's item samples for the currently selected profile. */
static void favorite_fill_samples(const FavoriteSet *set) {
    for (unsigned i = 0; i < READING_COUNT; ++i)
        favorite_sample_cache[i] = (ReadingSample){READING_STATE_MISSING, 0.0f};
    uint8_t sources[8];
    unsigned total = favorite_set_unique_sources(set, vehicle_profile_current(), sources);
    uint32_t max_age = (total + 1U) * 500U;
    if (max_age < 3000U)
        max_age = 3000U;
    for (unsigned r = 0; r < FAVORITE_ROWS; ++r) {
        for (unsigned i = 0; i < FAVORITE_ROW_ITEMS; ++i) {
            FavoriteItem item = set->rows[r][i];
            if (favorite_item_empty(item))
                continue;
            const ReadingMeta *meta = reading_lookup(item.id);
            if (!meta)
                continue;
            uint8_t source = reading_source(meta, vehicle_profile_current());
            if (source == READING_NO_SOURCE)
                continue;
            float value = parameter_cache_get_max_age(source, currentTime, max_age);
            favorite_sample_cache[item.id] =
                isfinite(value) ? (ReadingSample){READING_STATE_VALID, value}
                                : (ReadingSample){READING_STATE_STALE, 0.0f};
        }
    }
}

/* UDS sources only; native measurements are fed by their CAN sources. */
static unsigned favorite_uds_sources(const FavoriteSet *set, uint8_t ids[8]) {
    uint8_t sources[8];
    unsigned total = favorite_set_unique_sources(set, vehicle_profile_current(), sources);
    unsigned count = 0;
    for (unsigned i = 0; i < total; ++i)
        if (sources[i] < PARAMETER_DEFINITION_COUNT && parameter_definitions[sources[i]].request_id > 0xff)
            ids[count++] = sources[i];
    return count;
}

/* Present one FavoriteSet row (legacy) or both rows (MY23). */
static void present_favorite_layout(unsigned cell) {
    unsigned set = settings_state.ipc_my23_is_installed ? cell : cell / 2;
    unsigned row = settings_state.ipc_my23_is_installed ? 0 : cell % 2;
    favorite_fill_samples(&favorites_layout.sets[set]);
    char first[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
    char second[MY23_L2_VISIBLE + 1];
    if (settings_state.ipc_my23_is_installed) {
        favorite_row_render(favorites_layout.sets[set].rows[0], favorite_sample_cache, MY23_L1_VISIBLE, first, sizeof(first));
        favorite_row_render(favorites_layout.sets[set].rows[1], favorite_sample_cache, MY23_L2_VISIBLE, second, sizeof(second));
        menu_present_lines(first, second);
    } else {
        favorite_row_render(favorites_layout.sets[set].rows[row], favorite_sample_cache, DASHBOARD_MESSAGE_MAX_LENGTH, first, sizeof(first));
        menu_present(first);
    }
}

/* Editor text helpers. */
static const char *favorite_format_name(ReadingFormat format) {
    static const char *const names[] = {"Auto", "Full", "Medium", "Short"};
    return format <= READING_FORMAT_SHORT ? names[format] : "Auto";
}
static void favorite_item_text(char *out, size_t size, unsigned slot) {
    FavoriteItem item = favorite_draft.rows[favorite_row][slot];
    const ReadingMeta *meta = reading_lookup(item.id);
    snprintf_(out, size, "%s %s", meta ? reading_label_for(item.id, item.format) : "Empty",
              favorite_format_name(item.format));
}
static const char *favorite_pick_label(uint8_t id) {
    if (id >= READING_COUNT)
        return "Empty";
    const ReadingMeta *meta = reading_lookup((ReadingId)id);
    return meta ? reading_label_for((ReadingId)id, READING_FORMAT_FULL) : "Empty";
}
static uint8_t next_favorite_reading(uint8_t id, int direction) {
    return wrap(id, READING_COUNT + 1, direction);
}


/* Check whether a vehicle action is enabled by the user's preferences. */
static bool available(MenuAction id) {
    switch (id) {
    case ACTION_READ:
        return settings_state.read_faults_enabled;
    case ACTION_CLEAR:
        return settings_state.clear_faults_enabled;
    case ACTION_DYNO:
        return settings_state.dyno_mode_master_enabled;
    case ACTION_ESC:
        return settings_state.esc_tc_customizator_enabled;
    case ACTION_BRAKE:
    case ACTION_LAUNCH:
        return settings_state.front_brake_forcer_master;
    case ACTION_AWD:
        return settings_state.awd_disabler_enabled;
    case ACTION_HAS:
        return settings_state.has_function_enabled;
    case ACTION_EXHAUST:
        return settings_state.qv_exhaust_flap_function_enabled;
    default:
        return true;
    }
}

/* Keep a feature permission available until its in-flight menu request resolves. */
bool menu_setting_busy(uint8_t flash_index) {
    MenuAction id;
    switch (flash_index) {
    case 8: id = ACTION_DYNO; break;
    case 10: id = ACTION_BRAKE; break;
    case 13: id = ACTION_CLEAR; break;
    case 14: id = ACTION_ESC; break;
    case 27: id = ACTION_HAS; break;
    default: return false;
    }
    return requests[id].state == REQUEST_WAIT;
}

/* Explain the same known conditions before selection and immediately before execution. */
static const char *action_unavailable(MenuAction id) {
    if (!available(id))
        return "Disabled in setup";
    if (requests[id].state == REQUEST_WAIT)
        return "Request pending";
    switch (id) {
    case ACTION_READ:
        return diagnostics_state.clear_faults_request ? "DTC clear active" : NULL;
    case ACTION_CLEAR:
        if (diagnostics_state.clear_faults_request)
            return "DTC clear active";
        return fault_reader_busy() ? "BCM read active" : NULL;
    case ACTION_IBS:
        return telemetry_state.current_rpm_speed <= 400 ? "Start engine" : NULL;
    case ACTION_DYNO:
        if (chassis_state.front_brake_forced)
            return "Release brake";
        if (chassis_state.stability_inverted)
            return "Reset ESC";
        return runtime_state.car_steady_counter < 100 ? "Stop the car" : NULL;
    case ACTION_ESC:
        return chassis_state.dyno_mode_enabled_on_master || requests[ACTION_DYNO].state == REQUEST_WAIT
                   ? "Dyno active" : NULL;
    case ACTION_BRAKE:
        if (chassis_state.launch_assist_enabled)
            return "Disable launch";
        if (!chassis_state.front_brake_forced) {
            if (telemetry_state.current_speed_km_h != 0)
                return "Stop the car";
            if (!chassis_state.dyno_mode_enabled_on_master)
                return "Enable Dyno";
        }
        return NULL;
    case ACTION_LAUNCH:
        return chassis_state.launch_assist_enabled ? NULL : "Launch OFF";
    case ACTION_AWD:
        return !chassis_state.awd_sequence && runtime_state.car_steady_counter < 100 ? "Stop the car" : NULL;
    default:
        return NULL;
    }
}

/* Track only delivery/acknowledgement evidence; never infer physical vehicle success. */
static void action_requests_process(void) {
    for (unsigned id = 0; id < ACTION_COUNT; ++id) {
        ActionRequest *request = &requests[id];
        if (request->state != REQUEST_WAIT)
            continue;
        if ((id == ACTION_CLEAR && !diagnostics_state.clear_faults_request) ||
            (id == ACTION_HAS && !comfort_state.has_button_press_requested))
            request->state = REQUEST_SENT;
        else if (currentTime - request->started >= ACTION_TIMEOUT_MS)
            request->state = REQUEST_TIMEOUT;
    }
}

/* Finish a menu request only when its existing C2 acknowledgement arrives. */
void menu_action_reply(uint8_t command) {
    MenuAction id;
    bool enabled;
    switch (command) {
    case C1cmdDynoActive: id = ACTION_DYNO; enabled = true; break;
    case C1cmdDynoNotActive: id = ACTION_DYNO; enabled = false; break;
    case C1cmdForceFrontBrake: id = ACTION_BRAKE; enabled = true; break;
    case C1cmdNormalFrontBrake: id = ACTION_BRAKE; enabled = false; break;
    default: return;
    }
    if (requests[id].state == REQUEST_WAIT)
        requests[id].state = requests[id].target == enabled ? REQUEST_SENT : REQUEST_FAILED;
}

/* Mark a successfully queued request as pending without replaying it automatically. */
static void action_requested(MenuAction id, bool target) {
    requests[id] = (ActionRequest){currentTime, REQUEST_WAIT, target};
}

/* Select the next available action or action group. */
static void function_move(int direction, bool next_group) {
    uint8_t old_group = actions[function].group;
    for (unsigned i = 0; i < sizeof(actions) / sizeof(actions[0]); ++i) {
        function = wrap(function, sizeof(actions) / sizeof(actions[0]), direction);
        if (available(actions[function].id) && (!next_group || old_group != actions[function].group))
            return;
    }
}

/* Queue a screen payload, including diagnostic control bytes, with normal coalescing. */
static bool menu_present_packet(const uint8_t *content) {
    /* Keep unchanged screens without consuming the slots needed by C2/BH replies. */
    uint32_t resend_ms = rain_test_forcing() ? 2000U :
                         (view == RAIN_TEST || settings_state.rain_lights_enabled ? 1500U : 500U);
    if (previous_valid && !memcmp(previous_text, content, sizeof(previous_text)) &&
        currentTime - previous_sent < resend_ms)
        return false;
    uint8_t message[UART_SCREEN_BUFFER_SIZE];
    message[0] = BhBusIDparamString;
    memcpy(message + 1, content, UART_SCREEN_BUFFER_SIZE - 1);
    if (!board_uart_send(message, sizeof(message)))
        return false;
    memcpy(previous_text, content, sizeof(previous_text));
    previous_valid = 1;
    previous_sent = currentTime;
    return true;
}

/* Keep one shared renderer in Flash instead of LTO copies at every menu branch. */
__attribute__((noinline)) void menu_present_view(UiMode mode, const char *first, const char *second) {
    if (!settings_state.ipc_my23_is_installed) {
        char legacy[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
        size_t n = 0;
        for (; first[n] && n < sizeof(legacy) - 1; ++n) {
            uint8_t token = first[n];
            legacy[n] = token >= 0x80 && token <= 0x87 ?
                        token == 0x80 || token == 0x86 ? '>' : token == 0x81 ? ':' :
                        token == 0x85 ? UI_GLYPH_CROSS : ' ' : token;
        }
        legacy[n] = 0;
        menu_present(legacy); return;
    }
    uint8_t content[MY23_PACKET_SIZE];
    my23_packet_mode(content, mode, first, second);
    menu_present_packet(content);
}

void menu_present_lines(const char *first, const char *second) {
    menu_present_view(UI_MODE_PARAMETER, first, second);
}

/* Request the current screen while avoiding unnecessary repeated text updates. */
void menu_present(const char *text) {
    if (settings_state.ipc_my23_is_installed && text[0]) {
        uint8_t content[MY23_PACKET_SIZE];
        memset(content, ' ', sizeof(content));
        content[0] = MY23_PACKET_MARKER;
        my23_text_encode(content + 1, MY23_L1_VISIBLE + MY23_L2_VISIBLE, text);
        menu_present_packet(content);
        return;
    }
    uint8_t content[UART_SCREEN_BUFFER_SIZE - 1];
    memset(content, ' ', sizeof(content));
    size_t length = strlen(text);
    if (length > DASHBOARD_MESSAGE_MAX_LENGTH)
        length = DASHBOARD_MESSAGE_MAX_LENGTH;
    memcpy(content, text, length);
    if (menu_present_packet(content) && !text[0])
        close_pending = false;
}

/* Show the complete reading immediately, without a list prefix or title delay. */
void menu_present_reading(const char *text) {
    menu_present(text);
}

/* Show brief feedback about a selection, action or save result. */
void menu_notice(const char *text) {
    snprintf_(notice_text, sizeof(notice_text), "%s", text);
    notice = notice_text;
    notice_started = currentTime;
    size_t length = strlen(text);
    bool toggle = (length >= 4 && !strcmp(text + length - 4, ": ON")) ||
                  (length >= 5 && !strcmp(text + length - 5, ": OFF")) ||
                  ((text[0] == UI_GLYPH_CHECKED || text[0] == UI_GLYPH_UNCHECKED) && text[1] == ' ');
    notice_duration = (text[0] == UI_SYMBOL_WARNING[0] || text[0] == UI_GLYPH_CROSS) ? NOTICE_WARNING_MS
                      : toggle || !strcmp(text, "Records cleared")
                          ? NOTICE_CONFIRM_MS : NOTICE_REQUEST_MS;
    if (dashboard_state.baccable_dashboard_menu_visible)
        menu_present(text);
}

void menu_notice_saved(const char *title) {
    notice_label = title;
    /* Keep the result visible through UART delivery and the IPC fragment transfer. */
    notice = "Saved";
    notice_started = currentTime;
    notice_duration = NOTICE_SAVED_MS;
}

/* Save visibility, sort order and remembered pages for the remaining page editors. */
uint8_t menu_preferences_save(void) {
    uint8_t data[MENU_PREFS_SIZE];
    menu_preferences_encode(&preferences, data);
    if (preferences_saved && !memcmp(data, saved_preferences, sizeof(data)))
        return 0;
    if (!flash_record_save(VISIBILITY_RECORD, 0x105, data, sizeof(data)))
        return 255;
    memcpy(saved_preferences, data, sizeof(data));
    preferences_saved = true;
    return 0;
}

/* Save the FavoriteLayout / last-reading UI record (0x106). */
static uint8_t ui_layout_save(void) {
    uint8_t data[UI_LAYOUT_RECORD_SIZE];
    if (!ui_layout_encode(&ui_layout, data))
        return 255;
    if (ui_layout_saved && !memcmp(data, saved_ui_layout, sizeof(data)))
        return 0;
    if (!flash_record_save(VISIBILITY_RECORD, UI_LAYOUT_RECORD_TYPE, data, sizeof(data)))
        return 255;
    memcpy(saved_ui_layout, data, sizeof(data));
    ui_layout_saved = true;
    return 0;
}

/* Switch parameter catalogs and discard readings from the previous engine profile. */
void menu_engine_changed(void) {
    engine = !!settings_state.is_diesel_enabled;
    gasoline_v6 = !!settings_state.gasoline_v6;
    advanced_pages = !!settings_state.advanced_pages;
    parameter_page_count = menu_page_count(engine);
    list_count = 0;
    parameter_request_cancel();
    parameter_cache_reset();
    dashboard_state.dashboard_page_index = 0;
    favorite_row = 0;
    if (view == FAVORITES || view == VALUES)
        build_pages(view == FAVORITES ? selected_favorite[engine] : preferences.last[engine][group]);
}

/* Restore the user's menu layout or start with useful defaults. */
void menu_init(void) {
    memset(&input, 0, sizeof(input));
    memset(requests, 0, sizeof(requests));
    setup_cancel_edit();
    view = FAVORITES;
    memset(selected_favorite, 0, sizeof(selected_favorite));
    root = 0;
    group = 1;
    function = 0;
    setting = 0;
    info = 0;
    navigation_active = navigation_pending = false;
    demo_active = false;
    ipc_option = 0;
    ipc_pace = IPC_DEFAULT_PACE;
    ipc_method = IPC_DEFAULT_METHOD;
    ipc_options_sync = false;
    setup_last = 0;
    notice = NULL;
    confirmed_action = 255;
    previous_valid = 0;
    last_query = 0;
    last_render = 0;
    save_failed = save_close = close_pending = false;
    preferences_saved = false;
    ui_layout_saved = false;
    last_input = currentTime;

    menu_preferences_default(&preferences);
    uint8_t data[MENU_STORAGE_SIZE] = {0};
    if (flash_record_load(VISIBILITY_RECORD, 0x105, data, sizeof(data)) &&
        menu_preferences_decode(&preferences, data)) {
        memcpy(saved_preferences, data, sizeof(data));
        preferences_saved = true;
    }

    /* New FavoriteLayout record. Old 0x103/0x104/0x105 compound layouts are
       intentionally ignored; this is the one-time UI reset. */
    ui_layout_default(&ui_layout, vehicle_profile_current());
    uint8_t ui_data[UI_LAYOUT_RECORD_SIZE];
    if (flash_record_load(VISIBILITY_RECORD, UI_LAYOUT_RECORD_TYPE, ui_data, sizeof(ui_data)) &&
        ui_layout_decode(ui_data, &ui_layout)) {
        memcpy(saved_ui_layout, ui_data, sizeof(ui_data));
        ui_layout_saved = true;
    } else {
        ui_layout_default(&ui_layout, vehicle_profile_current());
        ui_layout_save(); /* Persist the reset marker once. */
    }
    favorites_layout = ui_layout.favorites;

    favorite_confirm = false;
    favorite_row = 0;
    menu_engine_changed();
}

/* Check whether a visible screen currently needs vehicle readings. */
bool menu_parameters_active(void) {
    return dashboard_state.baccable_dashboard_menu_visible && (view == VALUES || view == FAVORITES) &&
           list_count;
}

/* Only MY23 Readings exposes the following page as a second-line preview. */
static bool reading_preview(void) {
    return view == VALUES && settings_state.ipc_my23_is_installed && list_count;
}

/* Poll the actual filtered/sorted pair, deduplicating shared diagnostics. */
static unsigned reading_queries(uint8_t ids[8]) {
    unsigned count = 0;
    for (unsigned line = 0; line < 2; ++line) {
        const ParameterPage *page = &parameter_pages[engine][list[line ? wrap(selection, list_count, 1) : selection]];
        for (unsigned i = 0; i < parameter_page_elements(page); ++i) {
            uint8_t id = page->parameter_ids[i];
            if (parameter_definitions[id].request_id <= 0xff) continue;
            unsigned j = 0;
            while (j < count && ids[j] != id) ++j;
            if (j == count) ids[count++] = id;
        }
    }
    return count;
}

/* Fill the current screen with its latest usable measurements. */
void menu_parameters_refresh(void) {
    if (dashboard_state.dashboard_page_index >= parameter_page_count)
        return;
    const ParameterPage *page = &parameter_pages[engine][dashboard_state.dashboard_page_index];
    for (unsigned i = 0; i < parameter_page_elements(page); ++i) {
        uint8_t id = page->parameter_ids[i];
        /* Eight diagnostics take four seconds; keep the bound stable across
         * navigation so a prefetched value does not expire on promotion. */
        uint32_t max_age = reading_preview() && parameter_definitions[id].request_id > 0xff ? 4500U : 3000U;
        displayed_parameter_values[i] = parameter_cache_get_max_age(id, currentTime, max_age);
    }
}

/* Remember the selected reading and stop requests belonging to the previous page. */
static void select_page(void) {
    if (!list_count) {
        parameter_request_cancel();
        return;
    }
    uint8_t index = list[selection];
    if (view == FAVORITES) {
        selected_favorite[engine] = index;
        favorite_row = settings_state.ipc_my23_is_installed ? 0 : index % 2;
        dashboard_state.dashboard_page_index = 0;
        parameter_request_cancel();
        parameter_peak_reset();
        selected_parameter_element = 0;
        page_changed = currentTime;
        return;
    }
    dashboard_state.dashboard_page_index = index;
    uint16_t id = parameter_pages[engine][index].id;
    if (view == FAVORITES)
        preferences.last_favorite[engine] = id;
    else
        preferences.last[engine][group] = id;
    if (reading_preview()) parameter_request_follow_page();
    else parameter_request_cancel();
    parameter_peak_reset();
    selected_parameter_element = 0;
    page_changed = currentTime;
    menu_parameters_refresh();
}

/* Prepare the visible, sorted list and restore the requested selection. */
static void build_pages(uint16_t selected) {
    /* Editors always browse the complete engine catalog; the remembered reading
       group must not hide pages from Favorites or Shown pages. */
    if (view == FAVORITES) {
        list_count = 0;
        if (settings_state.ipc_my23_is_installed) {
            for (unsigned i = 0; i < FAVORITE_SET_COUNT; ++i)
                list[list_count++] = i;
        } else {
            for (unsigned s = 0; s < FAVORITE_SET_COUNT; ++s)
                for (unsigned r = 0; r < FAVORITE_ROWS; ++r)
                    if (favorite_row_item_count(favorites_layout.sets[s].rows[r]))
                        list[list_count++] = (uint8_t)(s * 2 + r);
        }
        selection = 0;
        for (unsigned i = 0; i < list_count; ++i)
            if (list[i] == selected) selection = i;
        select_page();
        return;
    }
    list_count = menu_page_list_filtered(&preferences, engine, group, false, false, gasoline_v6,
                                         advanced_pages, list);
    selection = 0;
    for (unsigned i = 0; i < list_count; ++i)
        if (parameter_pages[engine][list[i]].id == selected)
            selection = i;
    select_page();
}

/* Open favorites or a parameter group at its remembered page. */
static void open_pages(bool favorite) {
    view = favorite ? FAVORITES : VALUES;
    build_pages(favorite ? selected_favorite[engine] : preferences.last[engine][group]);
}

/* Show an automatic result without changing the user's visibility preferences. */
void menu_show_parameter(uint8_t index) {
    if (!menu_page_supported(engine, index, gasoline_v6))
        return;
    demo_active = false;
    group = parameter_pages[engine][index].group;
    view = VALUES;
    build_pages(parameter_pages[engine][index].id);
    /* Automatic result screens must not change the user's visibility preference. */
    bool listed = false;
    for (unsigned i = 0; i < list_count; ++i)
        listed |= list[i] == index;
    if (!listed && list_count < sizeof(list)) {
        selection = list_count;
        list[list_count++] = index;
        select_page();
    }
}

/* Confirm and request the selected vehicle action when its conditions are met. */
static void action_run(void) {
    MenuAction id = actions[function].id;
    const char *reason = action_unavailable(id);
    if (reason) {
        char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
        ui_render_unavailable(text, sizeof(text), reason);
        menu_notice(text);
        return;
    }
    if (id == ACTION_READ) {
        parameter_request_cancel();
        fault_reader_start(0x40);
        fault_index = 0;
        view = FAULTS;
        return;
    }
    if (id == ACTION_PEAK) {
        parameter_peak_enable(!parameter_peak_enabled());
        char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
        ui_render_checkbox(text, sizeof(text), "Peak hold", parameter_peak_enabled());
        menu_notice(text);
        return;
    }
    if (id == ACTION_RAIN_TEST) {
        rain_test_enter();
        view = RAIN_TEST;
        return;
    }
    if (confirmed_action != id || currentTime - confirm_started > 3000) {
        confirmed_action = id;
        confirm_started = currentTime;
        menu_notice(id == ACTION_BRAKE && !chassis_state.front_brake_forced ? "Brake+launch? HOLD"
                    : id == ACTION_DYNO ? "Dyno+ESC? HOLD"
                    : id == ACTION_AWD && chassis_state.awd_sequence ? "Stop AWD req?HOLD"
                    : UI_SYMBOL_WARNING " Hold to confirm");
        return;
    }
    confirmed_action = 255;
    uint8_t command[2] = {C2BusID, 0};
    switch (id) {
    case ACTION_IBS:
        if (telemetry_state.current_rpm_speed <= 400) {
            menu_notice(UI_SYMBOL_WARNING " Start engine");
            return;
        }
        ibs_override_enable(!ibs_override_enabled());
        char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
        ui_render_toggle(text, sizeof(text), "IBS override", ibs_override_enabled());
        menu_notice(text);
        return;
    case ACTION_CLEAR:
        diagnostics_state.clear_faults_request = 255;
        action_requested(id, true);
        menu_notice("DTC clear WAIT");
        return;
    case ACTION_STATS:
        menu_notice(statistics_reset() == 0 ? "Records cleared" : UI_SYMBOL_FAILURE " Save failed");
        return;
    case ACTION_DYNO:
        if (runtime_state.car_steady_counter < 100) {
            menu_notice(UI_SYMBOL_WARNING " Stop the car");
            return;
        }
        command[1] = C2cmdtoggleDyno;
        break;
    case ACTION_ESC:
        command[1] = C2cmdtoggleEscTc;
        break;
    case ACTION_HAS:
        command[1] = C2cmdToggleHas;
        break;
    case ACTION_LAUNCH:
        chassis_state.launch_assist_enabled = 0;
        menu_notice("Launch assist: OFF");
        return;
    case ACTION_BRAKE:
        command[1] = chassis_state.front_brake_forced ? C2cmdNormalFrontBrake : C2cmdForceFrontBrake;
        break;
    case ACTION_AWD:
        if (chassis_state.awd_sequence)
            chassis_state.awd_sequence = 0;
        else {
            if (runtime_state.car_steady_counter < 100) {
                menu_notice(UI_SYMBOL_WARNING " Stop the car");
                return;
            }
            chassis_state.awd_sequence = 4;
        }
        menu_notice("AWD requested");
        return;
    case ACTION_EXHAUST:
        comfort_state.force_q_vexhaust_valve_opened = comfort_state.force_q_vexhaust_valve_opened ? 4 : 1;
        comfort_state.chinese_exhaust_valve_request = comfort_state.chinese_valve_is_opened ? 'C' : 'O';
        menu_notice("Exhaust requested");
        return;
    default:
        return;
    }
    if (!board_uart_send(command, sizeof(command))) {
        requests[id].state = REQUEST_FAILED;
        menu_notice(UI_SYMBOL_WARNING " Queue full retry");
        return;
    }
    if (id == ACTION_HAS)
        comfort_state.has_button_press_requested = 5;
    action_requested(id, id == ACTION_DYNO ? !chassis_state.dyno_mode_enabled_on_master
                                          : id == ACTION_BRAKE ? !chassis_state.front_brake_forced : true);
    menu_notice("Request queued");
}

/* Render entry semantics and request evidence through the common presentation helpers. */
static void action_render(char *text, size_t capacity, const ActionEntry *entry) {
    MenuAction id = entry->id;
    ActionRequest *request = &requests[id];
    if (id == ACTION_CLEAR && diagnostics_state.clear_faults_request && request->state != REQUEST_TIMEOUT) {
        ui_render_pending(text, capacity, entry->name, "");
        return;
    }
    if (request->state == REQUEST_WAIT) {
        ui_render_pending(text, capacity, id == ACTION_DYNO ? "Dyno" : id == ACTION_BRAKE ? "Brake"
                          : id == ACTION_HAS ? "HAS button" : id == ACTION_ESC ? "ESC/TC" : entry->name,
                          id == ACTION_DYNO || id == ACTION_BRAKE ? (request->target ? "ON" : "OFF") : "");
        return;
    }
    if (request->state == REQUEST_FAILED || request->state == REQUEST_TIMEOUT) {
        if (request->state == REQUEST_FAILED)
            ui_render_failure(text, capacity, "Request failed");
        else
            snprintf_(text, capacity, UI_SYMBOL_UNKNOWN " No confirmation");
        return;
    }
    const char *reason = action_unavailable(id);
    if (reason) {
        ui_render_unavailable(text, capacity, reason);
        return;
    }
    if (id == ACTION_AWD && chassis_state.awd_sequence) {
        ui_render_pending(text, capacity, "AWD req", "OFF");
    } else if (id == ACTION_EXHAUST && comfort_state.force_q_vexhaust_valve_opened) {
        ui_render_pending(text, capacity, "QV req", comfort_state.force_q_vexhaust_valve_opened == 4 ? "AUTO" : "OPEN");
    } else if ((id == ACTION_HAS || id == ACTION_CLEAR || id == ACTION_ESC) && request->state == REQUEST_SENT) {
        ui_render_value(text, capacity, id == ACTION_HAS ? "HAS" : id == ACTION_CLEAR ? "DTC" : "ESC/TC",
                        id == ACTION_ESC ? "Req sent" : "Request sent");
    } else if (id == ACTION_DYNO) {
        ui_render_toggle(text, capacity, "Dyno", chassis_state.dyno_mode_enabled_on_master);
    } else if (id == ACTION_BRAKE) {
        /* C2 confirms its override sequence, not measured brake pressure. */
        ui_render_value(text, capacity, "Brake req", chassis_state.front_brake_forced ? "ON" : "OFF");
    } else if (entry->type == UI_ENTRY_TOGGLE) {
        ui_render_checkbox(text, capacity, entry->name, parameter_peak_enabled());
    } else if (id == ACTION_IBS) {
        ui_render_toggle(text, capacity, "IBS override", ibs_override_enabled());
    } else {
        ui_render_action(text, capacity, entry->name);
    }
}

/* Present the active menu, reading, editor or feedback message. */
void menu_render(void) {
    if (!dashboard_state.baccable_dashboard_menu_visible)
        return;
    /* Event-driven renders also start the next periodic refresh interval. */
    last_render = currentTime;
    if (demo_active) {
        menu_demo_render(demo_step);
        return;
    }
    action_requests_process();
    if (save_failed) {
        menu_present(UI_SYMBOL_FAILURE " Save failed: RES");
        return;
    }
    if (view == FUNCTIONS && confirmed_action == actions[function].id && currentTime - confirm_started <= 3000) {
        if (settings_state.ipc_my23_is_installed)
            menu_present_view(UI_MODE_CONFIRM, actions[function].name, "HOLD APPLY" MY23_BULLET "2X BACK");
        else menu_present(notice ? notice : "Hold to confirm");
        return;
    }
    if (notice && currentTime - notice_started < notice_duration) {
        if (settings_state.ipc_my23_is_installed && !strcmp(notice, "Saved")) menu_present_view(UI_MODE_NOTICE, notice_label, "SAVED");
        else menu_present(notice);
        return;
    }
    notice = NULL;
    if (settings_state.awd_disabler_enabled && chassis_state.awd_sequence &&
        currentTime - last_input > 1500 && currentTime % 6000 < 1000) {
        menu_present(UI_SYMBOL_WARNING " AWD OFF request");
        return;
    }
    char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
    switch (view) {
    case ROOT:
        if (settings_state.ipc_my23_is_installed) {
            menu_present_view(UI_MODE_LIST, roots[root], roots[(root + 1) % (sizeof(roots) / sizeof(roots[0]))]); return;
        }
        snprintf_(text, sizeof(text), UI_SYMBOL_ENTER " %s", roots[root]);
        break;
    case GROUPS:
        if (settings_state.ipc_my23_is_installed) {
            menu_present_view(UI_MODE_LIST, menu_group_names[group], menu_group_names[(group + 1) % MENU_GROUPS]); return;
        }
        snprintf_(text, sizeof(text), UI_SYMBOL_ENTER " %s", menu_group_names[group]);
        break;
    case FAVORITES:
        if (!list_count) { menu_present("No favorites"); return; }
        present_favorite_layout(list[selection]);
        return;
    case VALUES:
        if (!list_count) { menu_present("No pages"); return; }
        menu_parameters_refresh();
        if (settings_state.ipc_my23_is_installed) {
            const ParameterPage *page = &parameter_pages[engine][list[selection]];
            const ParameterPage *next = &parameter_pages[engine][list[wrap(selection, list_count, 1)]];
            dashboard_format_my23_page(page, displayed_parameter_values, text);
            menu_present_lines(text, next->label);
            return;
        }
        dashboard_send_values();
        return;
    case FUNCTIONS: {
        if (!available(actions[function].id))
            function_move(1, false);
        action_render(text, sizeof(text), &actions[function]);
        if (settings_state.ipc_my23_is_installed) {
            const char *reason = action_unavailable(actions[function].id);
            if (reason) menu_present_view(UI_MODE_NOTICE, actions[function].name, reason);
            else if (text[0] != '>') menu_present_lines(actions[function].name, text);
            else {
                unsigned next = function;
                do { next = (next + 1) % ACTION_COUNT; } while (!available(actions[next].id));
                menu_present_view(UI_MODE_LIST, actions[function].name, actions[next].name);
            }
            return;
        }
        break;
    }
    case SETTINGS:
        if (settings_state.ipc_my23_is_installed) {
            menu_present_view(UI_MODE_LIST, settings[setting], settings[setting_move(setting, 1)]); return;
        }
        if (setting == 1)
            ui_render_value(text, sizeof(text), "Sort", preferences.alphabetical ? "A-Z" : "groups");
        else
            snprintf_(text, sizeof(text), "%s%s", setting != 1 ? UI_SYMBOL_ENTER " " : "", settings[setting]);
        break;
    case VEHICLE: {
        char value[24];
        const char *v = vehicle_editor_value_label(value, sizeof(value));
        VehicleEditorState st = vehicle_editor_state();
        if (settings_state.ipc_my23_is_installed) {
            if (st == VEHICLE_EDITOR_CONFIRM)
                menu_present_view(UI_MODE_CONFIRM, vehicle_editor_field_label(), "HOLD SAVE" MY23_BULLET "2X DISCARD");
            else
                menu_present_view(st == VEHICLE_EDITOR_EDIT ? UI_MODE_EDIT : UI_MODE_LIST,
                                  vehicle_editor_field_label(), v);
            return;
        }
        if (st == VEHICLE_EDITOR_CONFIRM)
            snprintf_(text, sizeof(text), "Hold save/2x drop");
        else
            snprintf_(text, sizeof(text), "%s %s", vehicle_editor_field_label(), v);
        break;
    }
    case IPC_OPTIONS: {
        char next[24];
        ipc_option_text(text, sizeof(text), ipc_option);
        ipc_option_text(next, sizeof(next), wrap(ipc_option, 3, 1));
        menu_present_view(UI_MODE_LIST, text, next);
        return;
    }
    case SETUP:
        dashboard_send_setup();
        return;
    case SORT_EDIT:
        menu_present_view(sort_confirm ? UI_MODE_CONFIRM : UI_MODE_EDIT, "Sort order",
            sort_confirm ? "HOLD SAVE" MY23_BULLET "2X DISCARD" : sort_draft ? "A-Z" MY23_BULLET "HOLD SAVE" : "GROUPS" MY23_BULLET "HOLD SAVE");
        if (!settings_state.ipc_my23_is_installed)
            menu_present(sort_confirm ? "Hold save/2x drop" : sort_draft ? "Sort: A-Z" : "Sort: groups");
        return;
    case FAV_SETS: {
        char next_favorite[16];
        snprintf_(text, sizeof(text), "Favorite %u", favorite_set + 1);
        snprintf_(next_favorite, sizeof(next_favorite), "Favorite %u", wrap(favorite_set, FAVORITE_SET_COUNT, 1) + 1);
        menu_present_view(UI_MODE_LIST, text, next_favorite);
        return;
    }
    case FAV_ROWS: {
        static const char *const rows[] = {"Row A", "Row B"};
        menu_present_view(UI_MODE_LIST, rows[favorite_row], rows[wrap(favorite_row, FAVORITE_ROWS, 1)]);
        return;
    }
    case FAV_ITEMS: {
        char next[40] = {0};
        favorite_item_text(text, sizeof(text), favorite_slot);
        favorite_item_text(next, sizeof(next), wrap(favorite_slot, FAVORITE_ROW_ITEMS, 1));
        menu_present_view(favorite_confirm ? UI_MODE_CONFIRM : UI_MODE_LIST, text,
                          favorite_confirm ? "HOLD SAVE" MY23_BULLET "2X DISCARD" : next);
        if (favorite_confirm && !settings_state.ipc_my23_is_installed) menu_present("Hold save/2x drop");
        return;
    }
    case FAV_PICK: {
        char next[40] = {0};
        snprintf_(text, sizeof(text), "%s", favorite_pick_label(favorite_pick));
        snprintf_(next, sizeof(next), "%s", favorite_pick_label(next_favorite_reading(favorite_pick, 1)));
        menu_present_view(UI_MODE_LIST, text, next);
        return;
    }
    case FAV_FORMAT: {
        static const char *const formats[] = {"Auto", "Full", "Medium", "Short"};
        menu_present_view(UI_MODE_LIST, formats[favorite_format], formats[wrap(favorite_format, 4, 1)]);
        return;
    }
    case FAULTS:
        fault_reader_text(fault_index, text, sizeof(text));
        if (settings_state.ipc_my23_is_installed) {
            char next[24];
            if (fault_reader_count() > 1) {
                fault_reader_text(wrap(fault_index, fault_reader_count(), 1), next, sizeof(next));
                menu_present_view(UI_MODE_LIST, text, next);
            } else menu_present_lines(text, "2X Back");
            return;
        }
        break;
    case RAIN_TEST: {
        char next[24];
        rain_test_render(text, sizeof(text), next, sizeof(next));
        menu_present_lines(text, next);
        return;
    }
    case INFO:
        if (settings_state.ipc_my23_is_installed && info < 5) {
            if (info == 0) menu_present_lines("C1 firmware", !strncmp(FW_VERSION, "BACCABLE ", 9) ? FW_VERSION + 9 : FW_VERSION);
            else if (info == 1 || info == 2) {
                unsigned peer = info - 1;
                menu_present_lines(peer ? "BH firmware" : "C2 firmware", peer_seen[peer] && currentTime - peer_updated[peer] <= 5000 ? peer_versions[peer] : "No reply");
            } else if (info == 3) menu_present_lines("IPC MY23", "16 / 22 glyphs");
            else menu_present_lines("Immobilizer", security_state.immobilizer_enabled ? "ON" : "OFF");
            return;
        }
        if (info == 5)
            snprintf_(text, sizeof(text), "Reports:%lu", (unsigned long)input.reports_seen);
        else if (info == 6)
            snprintf_(text, sizeof(text), "Gaps:%lu", (unsigned long)input.stream_gaps);
        else if (info == 7)
            snprintf_(text, sizeof(text), "Max gap:%lums", (unsigned long)input.max_gap_ms);
        else if (info == 8)
            if (input.reports_seen)
                snprintf_(text, sizeof(text), "Input age:%lums",
                          (unsigned long)(currentTime - input.last_seen));
            else
                snprintf_(text, sizeof(text), "Input age:--");
        else if (info == 9)
            ui_render_action(text, sizeof(text), "Demo");
        else if (info == 0)
            ui_render_value(text, sizeof(text), "C1",
                            !strncmp(FW_VERSION, "BACCABLE ", 9) ? FW_VERSION + 9 : FW_VERSION);
        else if (info == 4)
            ui_render_toggle(text, sizeof(text), "Immobilizer", security_state.immobilizer_enabled);
        else if (info == 3)
            snprintf_(text, sizeof(text), "MY23:%s %uch",
                      settings_state.ipc_my23_is_installed ? "ON" : "OFF", DASHBOARD_MESSAGE_MAX_LENGTH);
        else {
            unsigned peer = info - 1;
            if (peer_seen[peer] && currentTime - peer_updated[peer] <= 5000)
                ui_render_value(text, sizeof(text), peer ? "BH" : "C2", peer_versions[peer]);
            else
                snprintf_(text, sizeof(text), UI_SYMBOL_UNKNOWN " %s no reply", peer ? "BH" : "C2");
        }
        if (settings_state.ipc_my23_is_installed) {
            if (info == 9) {
                menu_present_view(UI_MODE_LIST, "Demo", "C1 firmware");
                return;
            }
            static const char *const next_info[] = {"C1 firmware", "C2 firmware", "BH firmware",
                "IPC MY23", "Immobilizer", "Reports", "Gaps", "Max gap", "Input age", "Demo"};
            menu_present_view(UI_MODE_LIST, text, next_info[(info + 1) % INFO_PAGES]);
            return;
        }
        break;
    }
    menu_present(text);
}

/* Release the display, retrying a rejected clear until the UART accepts it. */
static void close_menu(void) {
    demo_active = false;
    dashboard_state.baccable_dashboard_menu_visible = 0;
    parameter_request_cancel();
    confirmed_action = 255;
    close_pending = true;
    /* Force a fresh clear even if an earlier blank screen was already accepted. */
    previous_valid = 0;
    dashboard_clear();
}

/* Persist each changed domain independently before completing a requested exit. */
static void persist_exit(void) {
    uint8_t settings_result = settings_save();
    uint8_t preferences_result = menu_preferences_save();
    save_failed = settings_result != 0 || preferences_result != 0;
    if (save_failed) {
        menu_present(UI_SYMBOL_FAILURE " Save failed: RES");
        return;
    }
    notice = NULL;
    parameter_request_cancel();
    if (save_close) {
        fault_reader_cancel();
        close_menu();
    } else if (save_destination == FAVORITES) {
        close_pending = false;
        confirmed_action = 255;
        last_input = currentTime;
        open_pages(true);
        menu_render();
    } else
        view = save_destination;
}

/* Remember the destination so retries never reinterpret a navigation event. */
static void request_exit(MenuView destination, bool close) {
    save_destination = destination;
    save_close = close;
    persist_exit();
}

/* Whether the FavoriteSet draft differs from the saved layout. */
static bool favorite_draft_dirty(void) {
    return memcmp(&favorite_draft, &favorites_layout.sets[favorite_set], sizeof(favorite_draft)) != 0;
}

/* Cancel one unfinished workflow or return to its parent after persistence. */
static void back(void) {
    if (view == SORT_EDIT) {
        if (!sort_confirm && sort_draft != !!preferences.alphabetical) { sort_confirm = true; return; }
        sort_confirm = false; view = SETTINGS; return;
    }
    if (view == FAV_FORMAT) { view = FAV_ITEMS; return; }
    if (view == FAV_PICK) { view = FAV_ITEMS; return; }
    if (view == FAV_ITEMS) {
        if (!favorite_confirm && favorite_draft_dirty()) { favorite_confirm = true; return; }
        favorite_draft = favorites_layout.sets[favorite_set];
        favorite_confirm = false;
        view = FAV_ROWS;
        return;
    }
    if (view == FAV_ROWS) { view = FAV_SETS; return; }
    if (view == FAV_SETS || view == IPC_OPTIONS) { view = SETTINGS; return; }
    if (view == FAULTS) {
        fault_reader_cancel();
        view = FUNCTIONS;
        return;
    }
    if (view == RAIN_TEST) {
        rain_test_exit();
        view = FUNCTIONS;
        return;
    }
    if (view == VEHICLE) {
        if (!vehicle_editor_back()) { vehicle_editor_cancel(); view = ROOT; }
        return;
    }
    if (view == ROOT) {
        request_exit(ROOT, true);
        return;
    }
    if (view == SETUP) {
        if (setup_stage_back() || setup_back())
            return;
        setup_last = setup_dashboardPageIndex;
        request_exit(SETTINGS, false);
    } else if (view == SETTINGS) {
        request_exit(ROOT, false);
    } else if (view == VALUES)
        view = GROUPS;
    else
        view = ROOT;
    parameter_request_cancel();
}

/* Apply one navigation gesture to the currently visible menu. */
/* Keep editor save handling outside the navigation switch on the small C1 target. */
static __attribute__((noinline)) void menu_hold(void) {
    if (view == VEHICLE) {
        vehicle_editor_save();
        menu_notice_saved("Vehicle");
        return;
    }
    if (view == SORT_EDIT) {
        bool original = preferences.alphabetical;
        preferences.alphabetical = sort_draft;
        if (menu_preferences_save()) {
            preferences.alphabetical = original;
            menu_notice(UI_SYMBOL_FAILURE " Save failed");
        } else { sort_confirm = false; menu_notice_saved("Sort order"); }
        return;
    }
    if (view == SETUP) { setup_stage_save(); return; }
    if (view == FUNCTIONS && confirmed_action == actions[function].id && currentTime - confirm_started <= 3000)
        action_run();
    if (view == FAV_ITEMS || view == FAV_ROWS) {
        FavoriteSet original = favorites_layout.sets[favorite_set];
        favorites_layout.sets[favorite_set] = favorite_draft;
        ui_layout.favorites = favorites_layout;
        if (ui_layout_save()) {
            favorites_layout.sets[favorite_set] = original;
            ui_layout.favorites = favorites_layout;
            menu_notice(UI_SYMBOL_FAILURE " Save failed");
        } else { favorite_confirm = false; menu_notice_saved("Favorites"); }
        return;
    }
}

void menu_event(MenuEvent event) {
    if (!event)
        return;
    if (demo_active) {
        if (event == MENU_BACK) {
            demo_active = false;
            last_input = currentTime;
            menu_render();
        }
        return;
    }
    bool direction_event = event == MENU_NEXT || event == MENU_PREVIOUS ||
                           event == MENU_NEXT_GROUP || event == MENU_PREVIOUS_GROUP;
    if (!direction_event)
        navigation_active = navigation_pending = false;
    last_input = currentTime;
    if (engine != !!settings_state.is_diesel_enabled || gasoline_v6 != !!settings_state.gasoline_v6 ||
        advanced_pages != !!settings_state.advanced_pages)
        menu_engine_changed();
    if (!dashboard_state.baccable_dashboard_menu_visible) {
        if (event == MENU_BACK || event == MENU_HOLD) {
            notice = NULL;
            close_pending = false;
            root = 0;
            dashboard_state.baccable_dashboard_menu_visible = 1;
            open_pages(true);
            menu_render();
        }
        return;
    }
    if (save_failed) {
        if (event == MENU_SELECT || event == MENU_HOLD)
            persist_exit();
        else if (event == MENU_BACK) {
            save_failed = false;
            notice = NULL;
        }
        if (dashboard_state.baccable_dashboard_menu_visible)
            menu_render();
        return;
    }
    if (event == MENU_HOLD) {
        menu_hold();
        menu_render();
        return;
    }
    if (event != MENU_SELECT)
        confirmed_action = 255;
    notice = NULL;
    if (event == MENU_BACK) {
        back();
        menu_render();
        return;
    }
    int direction = event == MENU_PREVIOUS || event == MENU_PREVIOUS_GROUP ? -1 : 1;
    bool jump = event == MENU_NEXT_GROUP || event == MENU_PREVIOUS_GROUP;
    if (event != MENU_SELECT) {
        switch (view) {
        case SORT_EDIT: if (!sort_confirm) sort_draft = !sort_draft; break;
        case FAV_SETS: favorite_set = wrap(favorite_set, FAVORITE_SET_COUNT, direction); break;
        case FAV_ROWS: favorite_row = wrap(favorite_row, FAVORITE_ROWS, direction); break;
        case FAV_ITEMS: if (!favorite_confirm) favorite_slot = wrap(favorite_slot, FAVORITE_ROW_ITEMS, direction); break;
        case FAV_PICK: favorite_pick = next_favorite_reading(favorite_pick, direction); break;
        case FAV_FORMAT: favorite_format = wrap(favorite_format, 4, direction); break;
        case ROOT:
            root = wrap(root, sizeof(roots) / sizeof(roots[0]), direction);
            break;
        case GROUPS:
            group = wrap(group, MENU_GROUPS, direction);
            break;
        case FUNCTIONS:
            function_move(direction, jump);
            break;
        case IPC_OPTIONS:
            ipc_option = wrap(ipc_option, 3, direction);
            break;
        case SETTINGS:
            setting = setting_move(setting, direction);
            break;
        case FAULTS:
            fault_index = wrap(fault_index, fault_reader_count(), direction);
            break;
        case RAIN_TEST:
            rain_test_move_page();
            break;
        case VEHICLE:
            vehicle_editor_navigate(direction);
            break;
        case INFO:
            info = wrap(info, INFO_PAGES, direction);
            break;
        case SETUP:
            if (jump)
                setup_move_group(direction);
            else
                setup_move_page(direction);
            break;
        case FAVORITES:
        case VALUES:
            if (jump && view != FAVORITES) {
                group = wrap(group, MENU_GROUPS, direction);
                build_pages(preferences.last[engine][group]);
            } else {
                selection = wrap(selection, list_count, direction);
                select_page();
            }
            break;
        }
    } else {
        switch (view) {
        case SORT_EDIT: if (!sort_confirm) sort_draft = !sort_draft; break;
        case FAV_SETS:
            favorite_draft = favorites_layout.sets[favorite_set];
            favorite_row = 0; favorite_slot = 0; favorite_confirm = false;
            view = FAV_ROWS;
            break;
        case FAV_ROWS:
            favorite_slot = 0; view = FAV_ITEMS;
            break;
        case FAV_ITEMS:
            if (!favorite_confirm) {
                favorite_pick = favorite_draft.rows[favorite_row][favorite_slot].id;
                view = FAV_PICK;
            }
            break;
        case FAV_PICK:
            favorite_draft.rows[favorite_row][favorite_slot].id = (ReadingId)favorite_pick;
            favorite_draft.rows[favorite_row][favorite_slot].format = READING_FORMAT_AUTO;
            if (favorite_pick >= READING_COUNT) {
                view = FAV_ITEMS;
            } else {
                favorite_format = READING_FORMAT_AUTO;
                view = FAV_FORMAT;
            }
            break;
        case FAV_FORMAT:
            favorite_draft.rows[favorite_row][favorite_slot].format = (ReadingFormat)favorite_format;
            view = FAV_ITEMS;
            break;
        case ROOT:
            switch (root) {
            case 0:
                open_pages(true);
                break;
            case 1:
                view = GROUPS;
                break;
            case 2:
                view = FUNCTIONS;
                break;
            case 3:
                view = SETTINGS;
                break;
            case 4:
                view = INFO;
                break;
            case 5:
                view = VEHICLE;
                vehicle_editor_begin();
                break;
            }
            break;
        case GROUPS:
            open_pages(false);
            break;
        case FAVORITES:
        case VALUES:
            break; /* Status pages do not use SELECT as hidden backward navigation. */
        case FUNCTIONS:
            if (confirmed_action != actions[function].id || currentTime - confirm_started > 3000) action_run();
            break; /* Confirmation requires the separately labelled hold. */
        case IPC_OPTIONS: {
            uint8_t pace = ipc_pace, method = ipc_method;
            if (ipc_option == 0) pace = (pace + 1) % 3;
            else if (ipc_option == 1) method = !method;
            else { pace = IPC_DEFAULT_PACE; method = IPC_DEFAULT_METHOD; }
            if (ipc_options_send(pace, method)) {
                ipc_pace = pace; ipc_method = method;
                ipc_options_sync = pace != IPC_DEFAULT_PACE || method != IPC_DEFAULT_METHOD;
            } else menu_notice("UART busy: retry");
            break;
        }
        case SETTINGS:
            switch (setting) {
            case 0:
                view = SETUP;
                setup_dashboardPageIndex = setup_last;
                break;
            case 1:
                sort_draft = preferences.alphabetical; sort_confirm = false; view = SORT_EDIT;
                break;
            case 2: favorite_set = 0; view = FAV_SETS; break;
            case 3: ipc_option = 0; view = IPC_OPTIONS; break;
            }
            break;
        case SETUP:
            setup_stage_select();
            break;
        case FAULTS:
            if (diagnostics_state.clear_faults_request)
                menu_notice(UI_SYMBOL_WARNING " DTC clear active");
            else {
                fault_reader_start(0x40);
                fault_index = 0;
            }
            break;
        case RAIN_TEST:
            rain_test_toggle_force();
            break;
        case VEHICLE:
            vehicle_editor_select();
            break;
        case INFO:
            if (info == 9) {
                demo_active = true;
                demo_step = 0;
                demo_started = currentTime;
            }
            break;
        }
    }
    if (direction_event) {
        bool burst = navigation_active && currentTime != navigation_last &&
                     currentTime - navigation_last <= 350U;
        navigation_active = true;
        navigation_last = currentTime;
        if (burst && currentTime - navigation_sent < navigation_interval()) {
            navigation_pending = true;
            return;
        }
        navigation_pending = false;
        navigation_sent = currentTime;
    }
    menu_render();
}

/* Turn eligible steering-wheel button reports into menu gestures. */
void menu_button(uint8_t button, bool allowed) {
    #ifndef HIDE_DASHBOARD_MENU
    MenuEvent event = menu_input_update(&input, button, allowed, currentTime);
    bool repeat_allowed = allowed && !save_failed && dashboard_state.baccable_dashboard_menu_visible &&
                          (view == FAVORITES || view == VALUES || view == GROUPS ||
                           view == SETTINGS || view == SETUP || view == IPC_OPTIONS ||
                           view == SORT_EDIT || view == FAV_SETS || view == FAV_ROWS || view == FAV_ITEMS ||
                           view == FAV_PICK || view == FAV_FORMAT || view == VEHICLE);
    if (event == MENU_NONE)
        event = menu_input_repeat(&input, repeat_allowed, currentTime);
    if (allowed && input.armed && button != 0x10)
        last_input = currentTime;
    menu_event(event);
    #else
    (void)button;
    (void)allowed;
    #endif
}

/* Refresh the display and request readings at their intended intervals. */
void menu_process(void) {
    /* Keep non-default IPC display options synchronized with BH. */
    if (ipc_options_sync && currentTime - ipc_options_sent >= 1000U) {
        ipc_options_send(ipc_pace, ipc_method);
        /* Bound queue-full retries too, rather than hammering UART each loop. */
        ipc_options_sent = currentTime;
    }
    menu_event(menu_input_poll(&input, input.armed, currentTime));
    fault_reader_process();
    rain_test_process();
    action_requests_process();
    if (engine != !!settings_state.is_diesel_enabled || gasoline_v6 != !!settings_state.gasoline_v6 ||
        advanced_pages != !!settings_state.advanced_pages)
        menu_engine_changed();
    if (close_pending)
        dashboard_clear();
    if (!dashboard_state.baccable_dashboard_menu_visible) {
        demo_active = false;
        return;
    }
    if (demo_active) {
        uint16_t duration;
        while ((duration = menu_demo_duration(demo_step)) &&
               currentTime - demo_started >= duration) {
            demo_started += duration;
            ++demo_step;
        }
        if (!duration) {
            demo_active = false;
            last_input = currentTime;
        }
        if (!demo_active || currentTime - last_render >= 100U)
            menu_render();
        return;
    }
    bool editor = view == SETTINGS || view == SETUP || view == IPC_OPTIONS ||
                  view == SORT_EDIT || view == FAV_SETS || view == FAV_ROWS || view == FAV_ITEMS ||
                  view == FAV_PICK || view == FAV_FORMAT;
    /* Reading screens are intentionally persistent; active diagnostics get a fresh grace period. */
    if (fault_reader_busy() || diagnostics_state.clear_faults_request) {
        last_input = currentTime;
    } else if (view != FAVORITES && view != VALUES && !save_failed &&
               currentTime - last_input >= (editor ? EDITOR_IDLE_MS : MENU_IDLE_MS)) {
        if (view == SETUP) {
            setup_cancel_edit();
            setup_last = setup_dashboardPageIndex;
        }
        confirmed_action = 255;
        if (view == RAIN_TEST)
            rain_test_exit();
        request_exit(FAVORITES, false);
        if (!save_failed)
            return;
    }
    if (menu_parameters_active() && settings_state.rotate_readings && currentTime - page_changed >= 5000) {
        selection = wrap(selection, list_count, 1);
        select_page();
    }
    if (menu_parameters_active() && view == FAVORITES &&
        !diagnostics_state.clear_faults_request && currentTime - page_changed >= 150 && currentTime - last_query >= 500) {
        unsigned set = settings_state.ipc_my23_is_installed ? list[selection] : list[selection] / 2;
        uint8_t ids[8];
        unsigned total = favorite_uds_sources(&favorites_layout.sets[set], ids);
        if (total) {
            unsigned position = selected_parameter_element % total;
            parameter_request_begin_id(ids[position], 255);
            selected_parameter_element = (position + 1) % total;
        }
        last_query = currentTime;
    } else if (menu_parameters_active() && reading_preview() && !diagnostics_state.clear_faults_request &&
        currentTime - page_changed >= 150 && currentTime - last_query >= 500) {
        uint8_t ids[8];
        unsigned total = reading_queries(ids);
        if (total) {
            unsigned position = selected_parameter_element % total;
            /* Cache-only reply: the preview must not overwrite current elements. */
            parameter_request_begin_id(ids[position], 255);
            selected_parameter_element = (position + 1) % total;
        }
        last_query = currentTime;
    } else if (menu_parameters_active() && !diagnostics_state.clear_faults_request &&
        currentTime - page_changed >= 150 && currentTime - last_query >= 500) {
        const ParameterPage *page = &parameter_pages[engine][dashboard_state.dashboard_page_index];
        /* One UDS transaction per interval; native values are fed by their CAN sources. */
        unsigned first = selected_parameter_element;
        for (unsigned i = 0; i < parameter_page_elements(page); ++i) {
            unsigned element = (first + i) % parameter_page_elements(page);
            if (parameter_definitions[page->parameter_ids[element]].request_id > 0xff) {
                selected_parameter_element = element;
                parameter_request_begin();
                break;
            }
        }
        selected_parameter_element = (selected_parameter_element + 1) % parameter_page_elements(page);
        last_query = currentTime;
    }
    if (navigation_pending && (currentTime - navigation_last >= 120U ||
                               currentTime - navigation_sent >= navigation_interval())) {
        navigation_pending = false;
        navigation_sent = currentTime;
        menu_render();
    } else if (!navigation_pending && currentTime - last_render >= 100)
        menu_render();
}
#endif
