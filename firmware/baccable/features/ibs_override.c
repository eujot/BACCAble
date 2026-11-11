/* Optional IBS experiment adapted from netzmark BACCAble a3ca082. */
#include "features/ibs_override.h"
#if defined(BACCABLE_C1)
    #include "app/powertrain.h"
static bool enabled;
static uint8_t pending, replacement[7];
static uint32_t observed, sent, ignition_at;
static uint8_t ignition, auto_setting;

/* RFHUB RUN arms the saved preference; OFF cancels, cranking preserves RUN. */
void ibs_override_ignition(unsigned code, uint32_t now) {
    if (code != 2 && code != 8 && code != 10) return;
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
    enabled = on;
    pending = 0;
}

/* Report whether the temporary charging experiment is enabled. */
bool ibs_override_enabled(void) { return enabled; }

/* Reuse a complete factory template, independently of its reported SOC. */
void ibs_override_observe(const CAN_RxHeaderTypeDef *h, const uint8_t *data) {
    if (!enabled || h->IDE != CAN_ID_STD || h->RTR != CAN_RTR_DATA || h->StdId != 0x41a || h->DLC < 7)
        return;
    if (!ibs_override_ignition_active() || h->DLC != 7) return;
    memcpy(replacement, data, sizeof(replacement));
    observed = currentTime;
    pending = 10;
}

/* Repeat the upstream SOC override briefly, stopping on stale input or ignition shutdown. */
void ibs_override_process(void) {
    if (auto_setting != settings_state.ibs_soc_auto) {
        auto_setting = settings_state.ibs_soc_auto;
        ibs_override_enable(auto_setting && ibs_override_ignition_active());
    }
    if (!ibs_override_ignition_active()) { pending = 0; return; }
    if (!enabled || !pending)
        return;
    if (currentTime - observed > 100) {
        pending = 0;
        return;
    }
    CAN_TxHeaderTypeDef header = {.StdId = 0x41a, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 7};
    uint8_t target = settings_state.ibs_soc_target;
    if (target > 100 || target % 5) { pending = 0; return; }
    replacement[1] = (replacement[1] & 0x80) | target;
    if (currentTime - sent >= 3 && can_tx(&header, replacement) == HAL_OK) {
        --pending;
        sent = currentTime;
    }
}
#endif
