---
estimated_steps: 8
estimated_files: 1
---

# T01: Per-voice crossfade arrays, Schmitt trigger, and CV override logic

**Slice:** S01 — Filter Type CV with Per-Voice Crossfade
**Milestone:** M002

## Description

Refactor the global crossfade state machine to per-voice arrays and add filter type CV input with Schmitt trigger hysteresis. This is the highest-risk change in the slice — it touches the inner process loop where every sample is computed, replaces shared state with per-voice state, and changes how mode transitions are detected.

The key insight: with CV-driven type switching, voice 0 might switch to 24dB while voice 3 stays at 12dB. The current global `crossfadeCounter` and `prevFilterType` can't express this. Each voice needs its own transition state.

## Steps

1. Append `FILTER_TYPE_CV_INPUT` to `InputId` enum, before `INPUTS_LEN`. Never insert — append only to preserve patch compatibility.
2. Replace member variables: `int prevFilterType = 0` → `int prevFilterType[PORT_MAX_CHANNELS] = {}`, `int crossfadeCounter = 0` → `int crossfadeCounter[PORT_MAX_CHANNELS] = {}`. Add `bool filterTypeHigh[PORT_MAX_CHANNELS] = {}` for Schmitt trigger state.
3. Add `configInput(FILTER_TYPE_CV_INPUT, "Filter Type CV")` in the constructor.
4. Add `addInput(createInputCentered<PJ301MPort>(mm2px(Vec(X, Y)), module, CipherOB::FILTER_TYPE_CV_INPUT))` in the widget constructor. Choose coordinates that place the jack near the filter type switch without overlapping existing components. A good candidate: `Vec(24.0, 32.0)` — directly below the CKSS switch at (24.0, 22.0), with 10mm spacing.
5. Refactor `process()` to determine `filterType` per-voice inside the channel loop:
   - If `FILTER_TYPE_CV_INPUT` is connected: read `getPolyVoltage(c)`, apply Schmitt trigger (set `filterTypeHigh[c] = true` when CV ≥ 2.6V, set `false` when CV ≤ 2.4V, hold previous state otherwise). `filterType = filterTypeHigh[c] ? 1 : 0`.
   - If not connected: `filterType = (int)(params[FILTER_TYPE_PARAM].getValue() + 0.5f)`.
6. Move mode-change detection inside the per-voice loop: `bool modeJustChanged = (filterType != prevFilterType[c])`. On change, set `crossfadeCounter[c] = CROSSFADE_SAMPLES` and capture crossfade-from values for this voice.
7. Make crossfade application per-voice: check `crossfadeCounter[c] > 0`, compute `t` from `crossfadeCounter[c]`, apply crossfade, decrement `crossfadeCounter[c]`. Update `prevFilterType[c] = filterType` per-voice.
8. Remove the post-loop global `crossfadeCounter--` and `prevFilterType = filterType` lines (now handled per-voice inside the loop).

## Must-Haves

- [ ] `FILTER_TYPE_CV_INPUT` appended (not inserted) to `InputId`
- [ ] `prevFilterType` is `int[PORT_MAX_CHANNELS]`, zero-initialized
- [ ] `crossfadeCounter` is `int[PORT_MAX_CHANNELS]`, zero-initialized
- [ ] `filterTypeHigh` is `bool[PORT_MAX_CHANNELS]`, zero-initialized
- [ ] Schmitt trigger thresholds: 2.6V rising, 2.4V falling
- [ ] CV overrides panel switch when connected
- [ ] Crossfade-from capture happens per-voice at per-voice mode change
- [ ] Crossfade counter decrements per-voice
- [ ] `configInput` and `addInput` for new CV input present
- [ ] `make` produces zero errors and zero warnings

## Verification

- `make -j4 2>&1 | grep -cE "warning:|error:"` returns `0`
- Code review: no remaining references to global `crossfadeCounter` or global `prevFilterType` (they should all be array accesses with `[c]`)
- `grep -n "prevFilterType\b\|crossfadeCounter\b" src/CipherOB.cpp` shows only array declarations and `[c]` accesses

## Observability Impact

- **New inspection surface:** `FILTER_TYPE_CV_INPUT` port tooltip shows "Filter Type CV" and per-channel voltage — lets a future agent or user confirm CV is reaching the module.
- **Per-voice state arrays:** `prevFilterType[c]`, `crossfadeCounter[c]`, `filterTypeHigh[c]` — a debugger or `printf` at any point in the loop can dump per-voice transition state. Previously impossible with scalar globals.
- **Schmitt trigger visibility:** The `filterTypeHigh[c]` array makes hysteresis state inspectable. A noisy CV that shouldn't trigger a transition won't — and you can verify by checking the array values don't change in the dead zone (2.4V–2.6V).
- **Failure mode:** If crossfade regresses to global behavior, the symptom is all voices switching simultaneously even with a polyphonic CV — detectable by scoping individual output channels.
- **Build diagnostic:** `make -j4 2>&1 | grep -cE "warning:|error:"` must return `0`. Any nonzero value is the primary failure signal for this task.

## Inputs

- `src/CipherOB.cpp` — current process loop with global crossfade state
- `src/SVFilter.hpp` — SVFilter interface (unchanged by this task)
- Decision: "Schmitt trigger hysteresis (2.6V rising, 2.4V falling)"
- Decision: "Per-voice crossfade arrays replace global crossfadeCounter/prevFilterType"
- Decision: "Filter type CV overrides panel switch when connected"

## Expected Output

- `src/CipherOB.cpp` — refactored with per-voice crossfade arrays, Schmitt trigger, CV input enum/config/widget, and CV override logic. Clean build.
