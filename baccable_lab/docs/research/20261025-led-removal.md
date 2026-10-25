# Historical C1 0x412 accelerator decoder

The pre-removal firmware at commit `dde35ed` decoded C1 `0x412` only for the
optional external LED strip. Its `vehicle_handle_accelerator` accepted DLC 5,
read byte 3, and converted raw values from 51 to 230 to approximately 0–100%
with `(raw - 51) * 100 / 180`. Values below 51 were outside its accepted range.
Removing the strip also removed this firmware handler and its dispatcher case;
the current firmware no longer consumes `0x412`.

The catalog retains the historical conversion for offline interpretation.
Existing captures contain 46,084 frames across 18 recordings, but they do not
independently validate the physical percentage. The earlier driving comparison
with C1 `0x0FC` found correlation (r=0.932) with a different level, so the two
signals must not be treated as interchangeable. No captures or markers were
changed for this firmware cleanup.
