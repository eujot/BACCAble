# Ignition-session LKA correction, 2026-10-13

## Vehicle report and implementation fault

The owner reports that v5-beta-24 disabled LKA once immediately after connection,
but not after subsequent ignition cycles. This confirms one physical toggle,
not repeated-start acceptance. C1 rearmed only after processor wake-up following
CAN inactivity. Ignition cycling with a live bus or USB keeping the device awake
did not rearm C2. A transport resync could erase an undelivered startup event.
C2 also cancelled startup permanently on its first OFF status, including a
possible report before LKA initialization had completed.

## Ignition evidence

Read-only SQL review of original `sessions/<session>/session.sqlite3`, C1
`0x1EF`, DLC 8, byte 0 low nibble. The [transition ledger](20261013-ignition-lka-startup.json)
preserves every transition, frame ID, complete payload, device time, nearest
event and original note. Nearest markers are context, not automatic proof of
causation. Captures and markers were not changed.

| Session | RUN 8 reports | OFF 2 reports | Candidate crank 10 reports |
| --- | ---: | ---: | ---: |
| `20260928-204607Z` | 15,317 | 1,254 | 59 |
| `20261010-111025Z` (`10paz2`) | 62,890 | 1,500 | 177 |

In the latter session, frames 4420710 and 4466357 switch to 2 about 1.09 and
2.00 seconds before `ignition_off` markers 154 and 156. RUN follows at frames
4432055 and 4484171. September frames 169491 and 341055 switch to RUN about
0.45 and 0.39 seconds before `ignition_on` markers. Code 10 occurs briefly
around engine starts, including **8 → 10 → 8**: treating every non-8 code as OFF
would create a false session during cranking. Code 10 remains a candidate
meaning, not a confirmed decoder enum. Marker 158 says `ignition_off`, but its
note mentions starting the car; this contradiction remains in the ledger and
is not used as sole OFF evidence.

## Implemented behavior

- C1 tracks explicit OFF (2) and RUN (8), retaining state across other codes.
  Initial RUN after boot/wake and OFF → RUN authorize one startup action.
  Repeated RUN and RUN → 10 → RUN do not. OFF cancels an unused pending action;
  a transport resync preserves an undelivered startup event.
- The auxiliary command carries action 1 for startup, 0 for resync, 2 for a
  saved setting or ignition OFF. A changed preference applies at the next start.
- `LKA: OFF` preserves Flash slot 39 value 1 (`lane_assist_disabled`). `LKA: ON`
  preserves value 0 and leaves factory behavior; it does not force LKA on.
- C2 waits up to 5 seconds after configuration for enabled `0x547` byte 6 bit 1
  and a released stalk report. Initial OFF reports keep waiting; OFF never
  authorizes a toggle. Driver input or timeout cancels the pending action.
- One C2 `0x384` double-click changes only byte 3 bit 6: press, release, press,
  release, about 100 ms between accepted transmissions. Queue rejection retains
  the current stage. A started gesture finishes its releases even if a save
  arrives. Neither `0x547` nor BH `0x4F6` is rewritten; there is no later override.

The 5-second readiness bound is an implementation choice, not timing proved
by these captures. Repeated ignition starts, cold initialization, manual
re-enable and USB connected/disconnected still need vehicle acceptance. Record
C1 `0x1EF`, C2 `0x384` and C2 `0x547`: check one gesture per start, none after
manual re-enable, enabled status arriving within the window, and no toggle
when LKA is already off.
