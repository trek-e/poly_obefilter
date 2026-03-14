---
id: T01
parent: S01
milestone: M002
provides:
  - Per-voice crossfade arrays replacing global crossfade state
  - Schmitt trigger CV input for filter type switching
  - FILTER_TYPE_CV_INPUT enum, configInput, and addInput wiring
key_files:
  - src/HydraQuartetVCF.cpp
key_decisions:
  - Schmitt trigger thresholds at 2.6V rising / 2.4V falling (200mV hysteresis band)
  - filterTypeCVConnected check hoisted outside loop — one isConnected() call per sample block, not per voice
  - prevFilterType[c] updated after crossfade application to avoid one-sample glitch on transition start
patterns_established:
  - Per-voice state arrays for any future CV-driven mode switching (pattern: declare array[PORT_MAX_CHANNELS], index by [c] inside voice loop)
  - CV-overrides-switch pattern: check isConnected() once, branch per-voice between CV read and param read
observability_surfaces:
  - grep for bare prevFilterType/crossfadeCounter (non-array) detects regressions
  - Filter Type CV port tooltip shows "Filter Type CV" in VCV Rack
  - Per-voice Schmitt state (filterTypeHigh[c]) inspectable in debugger
duration: 20min
verification_result: passed
completed_at: 2026-03-13
blocker_discovered: false
---

# T01: Per-voice crossfade arrays, Schmitt trigger, and CV override logic

**Refactored global crossfade state machine to per-voice arrays and added filter type CV input with Schmitt trigger hysteresis.**

## What Happened

Replaced three scalar member variables with per-voice arrays:
- `int prevFilterType` → `int prevFilterType[PORT_MAX_CHANNELS] = {}`
- `int crossfadeCounter` → `int crossfadeCounter[PORT_MAX_CHANNELS] = {}`
- Added `bool filterTypeHigh[PORT_MAX_CHANNELS] = {}` for Schmitt trigger state

Appended `FILTER_TYPE_CV_INPUT` to `InputId` enum (before `INPUTS_LEN`, preserving patch compatibility). Added `configInput` in constructor and `addInput` at `Vec(24.0, 32.0)` in widget — 10mm below the CKSS filter type switch.

Refactored `process()` inner loop:
1. Filter type is now determined per-voice: CV connected → Schmitt trigger per channel (2.6V rising, 2.4V falling); CV disconnected → panel switch value.
2. Mode change detection moved inside the voice loop: `filterType != prevFilterType[c]`.
3. Crossfade-from capture triggers per-voice on that voice's mode change, setting `crossfadeCounter[c] = CROSSFADE_SAMPLES`.
4. Crossfade application and counter decrement are per-voice: `crossfadeCounter[c]--`.
5. `prevFilterType[c] = filterType` updated per-voice after crossfade application.

Removed the post-loop global `crossfadeCounter--` and `prevFilterType = filterType` lines entirely.

## Verification

- `make -j4 2>&1 | grep -cE "warning:|error:"` → `0` (clean build, zero warnings, zero errors)
- `make -j4 2>&1 | grep -E "warning|error"` → empty output (slice-level build check)
- `grep -n "prevFilterType\b\|crossfadeCounter\b" src/HydraQuartetVCF.cpp` → all 8 matches are array declarations or `[c]` indexed accesses. No bare scalar references remain.
- Code review confirmed: no global crossfade state anywhere in the process loop.

**Slice-level checks (partial — intermediate task):**
- ✅ `make` zero warnings/errors
- ⏳ Module loads in VCV Rack — needs T02 SVG update
- ⏳ Gate toggles filter type — needs runtime
- ⏳ Unplug CV → switch resumes — needs runtime
- ⏳ Polyphonic per-voice independence — needs runtime

## Diagnostics

- **Regression detection:** `grep -n "prevFilterType\b\|crossfadeCounter\b" src/HydraQuartetVCF.cpp` — every match must be array declaration or `[c]` access.
- **Runtime:** Right-click module in VCV Rack → port tooltip for Filter Type CV shows per-channel voltage.
- **Debug:** Set breakpoint on `crossfadeCounter[c] = CROSSFADE_SAMPLES` to observe per-voice transitions.

## Deviations

None.

## Known Issues

None.

## Files Created/Modified

- `src/HydraQuartetVCF.cpp` — per-voice crossfade arrays, Schmitt trigger, CV input enum/config/widget, CV override logic
- `.gsd/milestones/M002/slices/S01/S01-PLAN.md` — added Observability / Diagnostics section
- `.gsd/milestones/M002/slices/S01/tasks/T01-PLAN.md` — added Observability Impact section
