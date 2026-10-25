# BACCAble v5-beta-33

This beta fixes `Rain test` readings that stopped updating after a few seconds
because diagnostic and display traffic overloaded the shared UART between the
BACCAble boards.

## Included changes

### Rain test UART traffic

- While `Rain test` is open, C1 polls the diagnostic page currently shown rather
  than polling C2 and BH continuously in the background.
- During a forced rain test, C1 renews C2's temporary request every second and
  samples BH about every five seconds. This keeps the existing three-second C2
  force lease alive while leaving room for other UART traffic.
- C2/BH status polls run less often while the test is visible. Unchanged screen
  text is also resent less often while the test or Rain lights is active;
  changed menu text still appears immediately.
- The BH diagnostic timeout now says `UART/BH timeout`. A missing reply can be
  caused by delayed board-link traffic and does not by itself prove that the BH
  board is disconnected.

The rain-light trigger, lamp masks and automatic behavior are unchanged. This
fix addresses the shared UART traffic that caused stale `Rain test` readings; it
does not establish that Rain lights will activate in every vehicle scenario.

## Build and verification

The production-UART host integration test runs both diagnostic pages with Rain
lights OFF and ON, forced output, status and USB polls, and screen traffic for
105 seconds. It verifies fresh replies, no UART queue rejection and no gap in
C2 force renewal. The native host suite, 82 Lab tests, lint and ARM builds for
C1, C2, BH and CAN passed locally with Arm GNU 15.2.Rel1. C1 uses
92,344/98,304 Flash bytes. The repository CI and release workflow provide the
release build with the pinned toolchain.

No board was flashed during this change. Confirm `Rain test` readings and Rain
lights behavior in the vehicle after installing matching firmware on the
required boards. The reported `ARM=1` while Rain lights was believed to be OFF
also remains to be checked against the saved setting.
