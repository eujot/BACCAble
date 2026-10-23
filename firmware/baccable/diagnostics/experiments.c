#include "diagnostics/experiments.h"
#if defined(BACCABLE_C1)
#include "app/powertrain.h"
#include "third_party/printf/printf.h"
#include <stdbool.h>

/* Pelican's Giulia manufacturer signals: OBDb/Alfa-Romeo signalsets/v3/default.json
   (CC BY-SA 4.0); its bit positions and conversions are used below. */
typedef struct {
    uint32_t request_id;
    uint32_t response_id;
    uint16_t did;
    uint32_t interval_ms;
} ExperimentCommand;

typedef enum {
    EXP_NUMBER, EXP_HEX, EXP_YESNO, EXP_OFFON, EXP_MAP_BEAM,
    EXP_MAP_WIPER, EXP_MAP_GEAR, EXP_MAP_SHIFT
} ExperimentKind;
typedef enum {
    EXP_NULL_NONE, EXP_NULL_ZERO, EXP_NULL_MINUS50, EXP_NULL_40, EXP_NULL_GEAR
} ExperimentNull;
typedef struct {
    const char *name;
    const char *unit;
    uint16_t bit_index;
    uint8_t bit_length;
    uint8_t command;
    uint8_t group;
    float scale;
    float offset;
    uint8_t kind;
    uint8_t null_rule;
} ExperimentSignal;

static const char *const group_names[EXPERIMENT_GROUP_COUNT] = {
    "Battery", "Control", "Details", "Doors", "GPS", "Movement",
    "Orientation", "Tires", "Transmission"
};
/* Source: https://github.com/OBDb/Alfa-Romeo/blob/main/signalsets/v3/default.json
   Snapshot SHA-256 c28314008a46d69169e0746919c9d5798b92df6eff727591836a60b30a9db59d. */
static const ExperimentCommand commands[] = {
    {0x6B4U, 0x694U, 0xD410U, 1000U},
    {0x740U, 0x4C0U, 0xA0A1U, 5000U},
    {0x740U, 0x4C0U, 0xA0A5U, 5000U},
    {0x740U, 0x4C0U, 0xA0F1U, 5000U},
    {0x740U, 0x4C0U, 0xA0F2U, 5000U},
    {0x740U, 0x4C0U, 0xA0F3U, 5000U},
    {0x740U, 0x4C0U, 0xA0F4U, 5000U},
    {0x742U, 0x0U, 0xA022U, 1000U},
    {0x763U, 0x4E3U, 0x032DU, 250U},
    {0x7ABU, 0x52BU, 0x2802U, 250U},
    {0x7E1U, 0x7E9U, 0x04FEU, 1000U},
    {0x7E1U, 0x7E9U, 0x5503U, 1000U},
    {0x7E1U, 0x7E9U, 0x5504U, 1000U},
    {0x18DA18F1U, 0x18DAF118U, 0x051AU, 500U},
    {0x18DA18F1U, 0x18DAF118U, 0x2024U, 86400000U},
    {0x18DA18F1U, 0x18DAF118U, 0xF158U, 86400000U},
    {0x18DA30F1U, 0x18DAF130U, 0x0885U, 100U},
    {0x18DA30F1U, 0x18DAF130U, 0x2962U, 100U},
    {0x18DAC7F1U, 0x18DAF1C7U, 0x0123U, 86400000U},
    {0x18DAC7F1U, 0x18DAF1C7U, 0x0124U, 86400000U},
    {0x18DAC7F1U, 0x18DAF1C7U, 0x0125U, 86400000U},
    {0x18DAC7F1U, 0x18DAF1C7U, 0x0127U, 86400000U},
    {0x18DAC7F1U, 0x18DAF1C7U, 0x012FU, 5000U},
    {0x18DAC7F1U, 0x18DAF1C7U, 0x013CU, 5000U},
    {0x18DAC7F1U, 0x18DAF1C7U, 0x013DU, 5000U},
    {0x18DAC7F1U, 0x18DAF1C7U, 0x013EU, 5000U},
    {0x18DAC7F1U, 0x18DAF1C7U, 0x013FU, 5000U},
    {0x18DAC7F1U, 0x18DAF1C7U, 0x0302U, 1000U},
};
static const ExperimentSignal signals[] = {
    {"HV SOC", "%", 0, 16, 0, 0, 0.001953125f, 0.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_HVBAT_SOC */
    {"Hi beams", "", 3, 1, 8, 1, 1.0f, 0.0f, EXP_OFFON, EXP_NULL_NONE}, /* ALFAROMEO_HIGHBEAMS_ON */
    {"Beam mode", "", 4, 4, 8, 1, 1.0f, 0.0f, EXP_MAP_BEAM, EXP_NULL_NONE}, /* ALFAROMEO_HIGHBEAMS_MODE */
    {"R wiper", "", 12, 4, 8, 1, 1.0f, 0.0f, EXP_MAP_WIPER, EXP_NULL_NONE}, /* ALFAROMEO_REAR_WIPER */
    {"Steer", "deg", 0, 16, 16, 1, 0.1f, -720.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_STEER_ANGLE */
    {"Year", "", 0, 8, 15, 2, 1.0f, 2000.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_MODEL_YEAR */
    {"RR door", "", 3, 1, 27, 3, 1.0f, 0.0f, EXP_YESNO, EXP_NULL_NONE}, /* ALFAROMEO_RRD_OPEN */
    {"RL door", "", 4, 1, 27, 3, 1.0f, 0.0f, EXP_YESNO, EXP_NULL_NONE}, /* ALFAROMEO_RLD_OPEN */
    {"Pass door", "", 5, 1, 27, 3, 1.0f, 0.0f, EXP_YESNO, EXP_NULL_NONE}, /* ALFAROMEO_PASSENGER_DOOR_OPEN */
    {"Driv door", "", 6, 1, 27, 3, 1.0f, 0.0f, EXP_YESNO, EXP_NULL_NONE}, /* ALFAROMEO_DRIVER_DOOR_OPEN */
    {"Trunk", "", 7, 1, 27, 3, 1.0f, 0.0f, EXP_YESNO, EXP_NULL_NONE}, /* ALFAROMEO_TRUNK_OPEN */
    {"Heading", "deg", 152, 16, 9, 4, 1.0f, 0.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_GPS_HEADING */
    {"Sats trk", "", 200, 4, 9, 4, 1.0f, 0.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_GPS_SATS_TRACKED */
    {"Sats vis", "", 204, 4, 9, 4, 1.0f, 0.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_GPS_SATS_VISIBLE */
    {"FL spd", "km/h", 112, 16, 17, 5, 0.0625f, 0.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_FL_VSS */
    {"RL spd", "km/h", 144, 16, 17, 5, 0.0625f, 0.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_RL_VSS */
    {"FR spd", "km/h", 176, 16, 17, 5, 0.0625f, 0.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_FR_VSS */
    {"RR spd", "km/h", 208, 16, 17, 5, 0.0625f, 0.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_RR_VSS */
    {"Lat a", "m/s2", 4, 12, 17, 6, 0.02f, -40.96f, EXP_NUMBER, EXP_NULL_40}, /* ALFAROMEO_LAT_G */
    {"Long a", "m/s2", 20, 12, 17, 6, 0.02f, -40.96f, EXP_NUMBER, EXP_NULL_40}, /* ALFAROMEO_LNG_G */
    {"FL P AC", "psi", 0, 8, 1, 7, 1.0f, 0.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_TP_FL_AC */
    {"FR P AC", "psi", 8, 8, 1, 7, 1.0f, 0.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_TP_FR_AC */
    {"RR P AC", "psi", 16, 8, 1, 7, 1.0f, 0.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_TP_RR_AC */
    {"RL P AC", "psi", 24, 8, 1, 7, 1.0f, 0.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_TP_RL_AC */
    {"RL valid", "", 4, 1, 2, 7, 1.0f, 0.0f, EXP_YESNO, EXP_NULL_NONE}, /* ALFAROMEO_TP_RL_VALID */
    {"RR valid", "", 5, 1, 2, 7, 1.0f, 0.0f, EXP_YESNO, EXP_NULL_NONE}, /* ALFAROMEO_TP_RR_VALID */
    {"FR valid", "", 6, 1, 2, 7, 1.0f, 0.0f, EXP_YESNO, EXP_NULL_NONE}, /* ALFAROMEO_TP_FR_VALID */
    {"FL valid", "", 7, 1, 2, 7, 1.0f, 0.0f, EXP_YESNO, EXP_NULL_NONE}, /* ALFAROMEO_TP_FL_VALID */
    {"FL P740", "psi", 0, 8, 3, 7, 0.398f, 0.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_TP_FL_V2 */
    {"FL t740", "C", 8, 8, 3, 7, 1.0f, -50.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_TT_FL_V2 */
    {"FR P740", "psi", 0, 8, 4, 7, 0.398f, 0.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_TP_FR_V2 */
    {"FR t740", "C", 8, 8, 4, 7, 1.0f, -50.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_TT_FR_V2 */
    {"RR P740", "psi", 0, 8, 5, 7, 0.398f, 0.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_TP_RR_V2 */
    {"RR t740", "C", 8, 8, 5, 7, 1.0f, -50.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_TT_RR_V2 */
    {"RL P740", "psi", 0, 8, 6, 7, 0.398f, 0.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_TP_RL_V2 */
    {"RL t740", "C", 8, 8, 6, 7, 1.0f, -50.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_TT_RL_V2 */
    {"FL P742", "psi", 24, 8, 7, 7, 1.0f, 0.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_TP_FL_V3 */
    {"FR P742", "psi", 32, 8, 7, 7, 1.0f, 0.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_TP_FR_V3 */
    {"RL P742", "psi", 40, 8, 7, 7, 1.0f, 0.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_TP_RL_V3 */
    {"RR P742", "psi", 48, 8, 7, 7, 1.0f, 0.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_TP_RR_V3 */
    {"Spare P", "psi", 56, 8, 7, 7, 1.0f, 0.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_TP_SPARE */
    {"Tire size", "mm", 64, 16, 14, 7, 1.0f, 0.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_TIRE_SIZE */
    {"FL ID", "", 0, 32, 18, 7, 1.0f, 0.0f, EXP_HEX, EXP_NULL_NONE}, /* ALFAROMEO_TID_FL */
    {"FR ID", "", 0, 32, 19, 7, 1.0f, 0.0f, EXP_HEX, EXP_NULL_NONE}, /* ALFAROMEO_TID_FR */
    {"RL ID", "", 0, 32, 20, 7, 1.0f, 0.0f, EXP_HEX, EXP_NULL_NONE}, /* ALFAROMEO_TID_RL */
    {"RR ID", "", 0, 32, 21, 7, 1.0f, 0.0f, EXP_HEX, EXP_NULL_NONE}, /* ALFAROMEO_TID_RR */
    {"RL bad", "", 4, 1, 22, 7, 1.0f, 0.0f, EXP_YESNO, EXP_NULL_NONE}, /* ALFAROMEO_TP_RL_INVALID */
    {"RR bad", "", 5, 1, 22, 7, 1.0f, 0.0f, EXP_YESNO, EXP_NULL_NONE}, /* ALFAROMEO_TP_RR_INVALID */
    {"FR bad", "", 6, 1, 22, 7, 1.0f, 0.0f, EXP_YESNO, EXP_NULL_NONE}, /* ALFAROMEO_TP_FR_INVALID */
    {"FL bad", "", 7, 1, 22, 7, 1.0f, 0.0f, EXP_YESNO, EXP_NULL_NONE}, /* ALFAROMEO_TP_FL_INVALID */
    {"FL P BCM", "bar", 0, 16, 23, 7, 0.001f, 0.0f, EXP_NUMBER, EXP_NULL_ZERO}, /* ALFAROMEO_TP_FL */
    {"FL tBCM", "C", 16, 8, 23, 7, 1.0f, -50.0f, EXP_NUMBER, EXP_NULL_MINUS50}, /* ALFAROMEO_TT_FL */
    {"FR P BCM", "bar", 0, 16, 24, 7, 0.001f, 0.0f, EXP_NUMBER, EXP_NULL_ZERO}, /* ALFAROMEO_TP_FR */
    {"FR tBCM", "C", 16, 8, 24, 7, 1.0f, -50.0f, EXP_NUMBER, EXP_NULL_MINUS50}, /* ALFAROMEO_TT_FR */
    {"RL P BCM", "bar", 0, 16, 25, 7, 0.001f, 0.0f, EXP_NUMBER, EXP_NULL_ZERO}, /* ALFAROMEO_TP_RL */
    {"RL tBCM", "C", 16, 8, 25, 7, 1.0f, -50.0f, EXP_NUMBER, EXP_NULL_MINUS50}, /* ALFAROMEO_TT_RL */
    {"RR P BCM", "bar", 0, 16, 26, 7, 0.001f, 0.0f, EXP_NUMBER, EXP_NULL_ZERO}, /* ALFAROMEO_TP_RR */
    {"RR tBCM", "C", 16, 8, 26, 7, 1.0f, -50.0f, EXP_NUMBER, EXP_NULL_MINUS50}, /* ALFAROMEO_TT_RR */
    {"Gear temp", "C", 0, 8, 10, 8, 1.0f, -40.0f, EXP_NUMBER, EXP_NULL_NONE}, /* ALFAROMEO_ATF_T */
    {"Curr gear", "", 4, 4, 11, 8, 1.0f, 0.0f, EXP_MAP_GEAR, EXP_NULL_NONE}, /* ALFAROMEO_GEAR */
    {"Tgt gear", "", 4, 4, 12, 8, 1.0f, 0.0f, EXP_MAP_GEAR, EXP_NULL_NONE}, /* ALFAROMEO_GEAR_TARGET */
    {"Des gear", "", 0, 4, 13, 8, 1.0f, 0.0f, EXP_NUMBER, EXP_NULL_GEAR}, /* ALFAROMEO_GEAR_DESIRED */
    {"Curr gear", "", 4, 4, 13, 8, 1.0f, 0.0f, EXP_NUMBER, EXP_NULL_GEAR}, /* ALFAROMEO_GEAR_CURRENT */
    {"Shift gear", "", 0, 8, 13, 8, 1.0f, 0.0f, EXP_MAP_SHIFT, EXP_NULL_NONE}, /* ALFAROMEO_GEAR_SHFT */
};

static uint8_t selected;
static uint8_t active;
static uint8_t valid;
static uint8_t sequence;
static uint16_t received;
static uint16_t total;
static uint8_t enabled;
static uint8_t successful;
static uint32_t response_id;
static uint32_t started;
static uint32_t updated;
static uint32_t last_frame;
static uint32_t raw_value;
static float measured;
static uint8_t payload[40];

/* Return the source group title for the menu. */
const char *experiment_group_name(uint8_t group) {
    return group < EXPERIMENT_GROUP_COUNT ? group_names[group] : "";
}

/* Count the signals available in one source group. */
uint8_t experiment_group_count(uint8_t group) {
    uint8_t count = 0;
    for (unsigned i = 0; i < EXPERIMENT_SIGNAL_COUNT; ++i)
        if (signals[i].group == group) ++count;
    return count;
}

/* Translate a position within a group into its signal index. */
uint8_t experiment_group_signal(uint8_t group, uint8_t position) {
    for (unsigned i = 0; i < EXPERIMENT_SIGNAL_COUNT; ++i)
        if (signals[i].group == group && position-- == 0) return i;
    return 0;
}

/* Return the short display label of a signal. */
const char *experiment_signal_name(uint8_t signal) {
    return signal < EXPERIMENT_SIGNAL_COUNT ? signals[signal].name : "";
}

/* Clear the previous value and request the newly selected signal immediately. */
void experiment_select(uint8_t signal) {
    if (signal >= EXPERIMENT_SIGNAL_COUNT) return;
    selected = signal;
    active = 0;
    valid = 0;
    enabled = 1;
    successful = 0;
    started = currentTime - 1000U;
}

/* Stop polling and discard the previous value when the reader loses ownership. */
void experiment_cancel(void) {
    enabled = active = valid = 0;
}

/* Extract OBDb's MSB-first bit field from the data after the 0x62/DID header. */
static bool read_field(const ExperimentSignal *signal, uint32_t *raw) {
    uint16_t end = signal->bit_index + signal->bit_length;
    if (signal->bit_length == 0 || signal->bit_length > 32 || end > (uint16_t)(total - 3U) * 8U)
        return false;
    uint32_t result = 0;
    for (uint16_t bit = signal->bit_index; bit < end; ++bit)
        result = (result << 1) | ((payload[3U + bit / 8U] >> (7U - bit % 8U)) & 1U);
    *raw = result;
    return true;
}

/* Convert a complete matching reply using the source bounds and null markers. */
static void finish_reply(void) {
    const ExperimentCommand *command = &commands[signals[selected].command];
    active = 0;
    valid = 0;
    successful = 0;
    if (total < 3 || payload[0] != 0x62 || payload[1] != (uint8_t)(command->did >> 8) ||
        payload[2] != (uint8_t)command->did || !read_field(&signals[selected], &raw_value)) return;
    successful = 1;
    const ExperimentSignal *signal = &signals[selected];
    measured = (float)raw_value * signal->scale + signal->offset;
    /* Match the few source ranges that are narrower than their raw field. */
    if (signal->command == 0 && measured > 100.0f) measured = 100.0f;
    if (signal->command == 16) {
        if (measured < -720.0f) measured = -720.0f;
        if (measured > 720.0f) measured = 720.0f;
    }
    /* Convert the source Fahrenheit bounds to Celsius after its affine formula. */
    if (signal->command == 10) {
        if (measured < -34.444444f) measured = -34.444444f;
        if (measured > 148.888889f) measured = 148.888889f;
    }
    if (signal->command == 15 && measured > 2225.0f) measured = 2225.0f;
    if (signal->group == 5 && measured > 4095.0f) measured = 4095.0f;
    if (signal->group == 6 && measured < -40.94f) measured = -40.94f;
    if ((signal->null_rule == EXP_NULL_ZERO && measured <= 0.0f) ||
        (signal->null_rule == EXP_NULL_MINUS50 && measured <= -50.0f) ||
        (signal->null_rule == EXP_NULL_40 && measured >= 40.0f) ||
        (signal->null_rule == EXP_NULL_GEAR && (raw_value == 0 || raw_value >= 13))) return;
    valid = 1;
    updated = currentTime;
}

/* Poll one visible read-only DID; a different selection cancels the transaction. */
void experiment_process(void) {
    if (!enabled) return;
    if (active && currentTime - last_frame < 1000U && currentTime - started < 5000U) return;
    if (active) { active = valid = successful = 0; }
    const ExperimentCommand *command = &commands[signals[selected].command];
    uint32_t interval = successful ? command->interval_ms : 1000U;
    if (currentTime - started < interval) return;
    uint8_t data[8] = {3, 0x22, (uint8_t)(command->did >> 8), (uint8_t)command->did};
    CAN_TxHeaderTypeDef header = {.RTR = CAN_RTR_DATA, .DLC = 8};
    if (command->request_id <= 0x7ffU) {
        header.IDE = CAN_ID_STD;
        header.StdId = command->request_id;
    } else {
        header.IDE = CAN_ID_EXT;
        header.ExtId = command->request_id;
    }
    started = last_frame = currentTime;
    active = can_tx(&header, data) == HAL_OK;
    if (!active) valid = successful = 0;
    response_id = command->response_id;
    received = total = sequence = 0;
}

/* Reassemble a bounded ISO-TP reply, including multi-frame diagnostic values. */
void experiment_receive(const CAN_RxHeaderTypeDef *header, const uint8_t *data) {
    if (!active || currentTime - last_frame >= 1000U || currentTime - started >= 5000U ||
        header->RTR != CAN_RTR_DATA || header->DLC == 0 || header->DLC > 8) return;
    const ExperimentCommand *command = &commands[signals[selected].command];
    uint8_t extended = command->request_id > 0x7ffU;
    uint32_t id = extended ? header->ExtId : header->StdId;
    if (header->IDE != (extended ? CAN_ID_EXT : CAN_ID_STD) ||
        (response_id && id != response_id) || (!response_id && received)) return;
    uint8_t type = data[0] >> 4;
    if (type == 0) {
        uint8_t length = data[0] & 15U;
        if (response_id && length == 3 && header->DLC >= 4 && data[1] == 0x7f && data[2] == 0x22) {
            if (data[3] == 0x78) last_frame = currentTime; /* Response pending. */
            else { active = valid = successful = 0; }
            return;
        }
        if (received) { active = valid = successful = 0; return; }
        if (length < 3 || length > 7 || header->DLC < length + 1U ||
            data[1] != 0x62 || data[2] != (uint8_t)(command->did >> 8) ||
            data[3] != (uint8_t)command->did) return;
        total = length;
        for (unsigned i = 0; i < length; ++i) payload[i] = data[i + 1U];
        finish_reply();
    } else if (type == 1 && header->DLC == 8) {
        uint16_t length = ((uint16_t)(data[0] & 15U) << 8) | data[1];
        if (length < 8 || data[2] != 0x62 ||
            data[3] != (uint8_t)(command->did >> 8) || data[4] != (uint8_t)command->did) return;
        if (received) { active = valid = successful = 0; return; }
        total = length;
        last_frame = currentTime;
        received = 6;
        sequence = 1;
        response_id = id;
        for (unsigned i = 0; i < 6; ++i) payload[i] = data[i + 2U];
        CAN_TxHeaderTypeDef flow = {.RTR = CAN_RTR_DATA, .DLC = 8};
        if (extended) { flow.IDE = CAN_ID_EXT; flow.ExtId = command->request_id; }
        else { flow.IDE = CAN_ID_STD; flow.StdId = command->request_id; }
        uint8_t flow_data[8] = {0x30, 0, 5}; /* Pace replies for the small CAN receive FIFO. */
        if (can_tx(&flow, flow_data) != HAL_OK) active = valid = successful = 0;
    } else if (type == 2 && received && id == response_id) {
        if ((data[0] & 15U) != (sequence & 15U)) { active = valid = successful = 0; return; }
        unsigned count = total - received;
        if (count > 7) count = 7;
        if (header->DLC < count + 1U) { active = valid = successful = 0; return; }
        /* Keep the prefix containing all catalog fields, but validate the entire reply. */
        for (unsigned i = 0; i < count && received + i < sizeof(payload); ++i)
            payload[received + i] = data[i + 1U];
        last_frame = currentTime;
        received += count;
        ++sequence;
        if (received == total) finish_reply();
    }
}

/* Translate only the codes explicitly defined by the selected enumeration. */
static const char *mapped_value(uint8_t kind, uint32_t raw) {
    if (kind == EXP_MAP_BEAM) return raw == 0 ? "OFF" : raw == 1 ? "ON" : raw == 2 ? "HELD" : NULL;
    if (kind == EXP_MAP_WIPER) return raw == 0 ? "OFF" : raw == 1 ? "SPRAY" : raw == 2 ? "AUTO" : raw == 4 ? "ON" : NULL;
    if (kind == EXP_MAP_GEAR || kind == EXP_MAP_SHIFT) {
        if (raw == 0) return "N";
        if (raw == (kind == EXP_MAP_GEAR ? 11U : 238U)) return "R";
        if (raw == (kind == EXP_MAP_GEAR ? 13U : 221U)) return "P";
        if (kind == EXP_MAP_GEAR && raw <= 10) return "gear";
    }
    return NULL;
}

/* Show one named measurement, with -- when no valid reply is available. */
void experiment_format(uint8_t signal, char *output, size_t size) {
    if (!output || !size || signal >= EXPERIMENT_SIGNAL_COUNT) return;
    const ExperimentSignal *item = &signals[signal];
    if (signal != selected || !valid || currentTime - updated > commands[item->command].interval_ms + 5000U) {
        snprintf_(output, size, "%s --", item->name);
        return;
    }
    if (item->kind == EXP_HEX) snprintf_(output, size, "%s %08lX", item->name, (unsigned long)raw_value);
    else if (item->kind == EXP_YESNO || item->kind == EXP_OFFON)
        snprintf_(output, size, "%s %s", item->name, raw_value ? (item->kind == EXP_OFFON ? "ON" : "YES") :
                                                    (item->kind == EXP_OFFON ? "OFF" : "NO"));
    else if (item->kind >= EXP_MAP_BEAM) {
        const char *label = mapped_value(item->kind, raw_value);
        if (item->kind == EXP_MAP_GEAR && raw_value >= 1 && raw_value <= 10)
            snprintf_(output, size, "%s %lu", item->name, (unsigned long)raw_value);
        else snprintf_(output, size, "%s %s", item->name, label ? label : "--");
    } else if (item->scale == 1.0f && item->offset == (float)(int)item->offset)
        snprintf_(output, size, "%s %.0f%s", item->name, (double)measured, item->unit);
    else if (item->unit[0] == 'b' || item->unit[0] == 'p' || item->group == 6)
        snprintf_(output, size, "%s %.2f%s", item->name, (double)measured, item->unit);
    else snprintf_(output, size, "%s %.1f%s", item->name, (double)measured, item->unit);
}
#endif
