/* Autonomous 0x41A experiment using a recorded C1 layout; see ACTION_PLAN.md. */
#include "features/ibs_override.h"
#if defined(BACCABLE_C1)
    #include "app/powertrain.h"
static bool enabled;
static uint32_t sent, ignition_at;
static uint8_t ignition, auto_setting;

/* RFHUB RUN arms the saved preference; OFF cancels, cranking preserves RUN. */
void ibs_override_ignition(unsigned code, uint32_t now) {
    if (code != 2 && code != 8 && code != 10) return;
    /* A missed OFF must not carry a manual override into another ignition session.
     * Check before refreshing, including when diagnostics skipped the process loop. */
    if (now - ignition_at > 2000U) {
        ignition = 0;
        enabled = false;
    }
    ignition_at = now;
    if (code == 10) return;
    if (code != ignition) {
        ignition = code;
        ibs_override_enable(code == 8 && settings_state.ibs_soc_auto);
        auto_setting = settings_state.ibs_soc_auto;
    }
}
bool ibs_override_ignition_active(void) {
    return ignition == 8 && currentTime - ignition_at <= 2000U && !runtime_state.low_consume_is_active;
}

/* Manual action changes this ignition session only; startup preference is saved separately. */
void ibs_override_enable(bool on) {
    auto_setting = settings_state.ibs_soc_auto;
    enabled = on;
    if (on) sent = currentTime - 100U;
}

/* Report whether the temporary charging experiment is enabled. */
bool ibs_override_enabled(void) { return enabled; }

/* Emit one report per 100 ms without requiring any incoming IBS data. */
void ibs_override_process(void) {
    if (currentTime - ignition_at > 2000U) {
        ignition = 0;
        enabled = false;
    }
    if (auto_setting != settings_state.ibs_soc_auto) {
        auto_setting = settings_state.ibs_soc_auto;
        ibs_override_enable(auto_setting && ibs_override_ignition_active());
    }
    if (!ibs_override_ignition_active() || !enabled || currentTime - sent < 100U) return;
    /* Do not accumulate periodic samples in a congested transport. Wait for
     * idle without advancing the timer, so normal queued traffic cannot lock
     * this sender into repeatedly missing the same 100 ms phase. */
    if (!can_tx_idle()) return;
    uint8_t target = settings_state.ibs_soc_target;
    if (target > 100 || target % 5) return;
    /* Recorded D1 xx 00 A6 00 00 00 layout, SOC high bit fixed clear.
     * Zero raw current bytes match the captures, not a validated 0 A encoding.
     * The existing Readings decoder maps those bytes to -250 A. */
    uint8_t data[8] = {0xd1, target, 0, 0xa6, 0, 0, 0, 0};
    CAN_TxHeaderTypeDef header = {.StdId = 0x41a, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 7};
    /* Pace queue rejections too; missed cycles never become a catch-up burst. */
    sent = currentTime;
    (void)can_tx(&header, data);
}
#endif
