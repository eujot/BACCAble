# Host regression tests

For this Mac's tool paths, dependencies, test commands and local HTML report
generation, see the [build guide](../firmware/baccable/MAKEFILE.md#run-tests-locally).
Known source compilation blockers are recorded in [ACTION_PLAN.md](../docs/ACTION_PLAN.md).

Run `make -C tests -j2 test`. Assertions and ASan/UBSan failures stop the test run
and fail CI. The cases run in their existing order, once per executable.

In GitHub, open **Actions → workflow run → Summary → Host regression scenarios**.
The table lists scenario results and distinguishes 18/24-character menu/catalog
builds and C2/BH parking builds. Download the **host-test-log** artifact and open
`host-tests.html` for an offline table and expandable full build/test log.

- **PASS**: the case returned successfully.
- **INCOMPLETE**: it started but did not finish; inspect the assertion/sanitizer log.
- **NOT RUN**: announced by the executable but not reached after an earlier failure.

Suites that never started (for example after a compiler error) have no scenario
rows. The report shows the overall test-step outcome; a partial table is not a
successful run. These host checks do not simulate physical dashboard refresh.

Register new case functions in the executable's `HostTest` array using
`HOST_TEST(function_name)`. The readable label comes from the function name.
Keep related assertions together; do not split stateful cases solely for reporting.
The existing CI publishes reports on failures too, without extra token permissions,
third-party report actions, dependencies or a second test execution.

### USB class transition regression

`make -C tests test` includes `test_usb_lifecycle`, built with the bundled ST
USB core, MSC/BOT/SCSI and CDC implementations; only hardware and storage
boundaries are stubbed. ASan/UBSan cover configured MSC-to-CDC teardown, repeated
transitions, unconfigured teardown, failed initialization/start, retry exhaustion,
cancellation during reset, tick wrap and the read-only EP0 status request.
Upstream/auxiliary and UART/menu suites cover addressed mode acknowledgements,
missing/stale peers, queue rejection, timeout/re-entry and lost TX completion.
These checks do not emulate macOS enumeration, UART electrical collisions or
vehicle CAN timing; see the hardware procedure in USB_DIAGNOSTICS.md.

The BH display suite runs at both 18- and 24-character menu widths. It checks
the temporary second line, periodic full retransmission without radio traffic,
complete USB/CarPlay source adoption, rejection of incomplete transfers,
radio/menu separation with bounded deferral, CAN enqueue retry, and replay of
the previous radio text when the menu closes. The suite validates generated
frames and state transitions, not IPC arbitration or visible pixels.
The IPC display experiment also checks source-specific `0x090` headers,
16-bit glyph bytes, complete ordered transfers, 48-character line rulers and
normal-menu recovery after the diagnostic screen. It also checks radio-text
interruption, CAN queue retry and the command lease that releases a stale test.
