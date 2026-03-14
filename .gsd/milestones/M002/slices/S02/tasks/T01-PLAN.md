---
estimated_steps: 7
estimated_files: 2
---

# T01: Wire FM input, attenuverter, and 1V/Oct into DSP pipeline

**Slice:** S02 — FM Input and 1V/Oct Pitch Tracking
**Milestone:** M002

## Description

Adds FM input with attenuverter and 1V/Oct pitch tracking input to the filter module's DSP pipeline. The key design point: FM must bypass the cutoff smoother to preserve audio-rate modulation. This is achieved by adding an `fmOffsetVoct` parameter to `SVFilter::setParams()` that is applied *after* smoothing but *before* frequency normalization/warping. 1V/Oct sums into the existing cutoff exponent in the logarithmic domain (before `pow(2, ...)`), so it goes through the smoother like any other pitch offset — this is correct since 1V/Oct is typically a slow-moving pitch CV, not audio-rate.

## Steps

1. **Modify `SVFilter::setParams()` signature** — Add `float fmOffsetVoct = 0.f` as a fourth parameter. After the smoother output (line 77) and before normalization (line 82), apply: `smoothedCutoff *= std::pow(2.f, fmOffsetVoct);`. The default of `0.f` means all existing call sites (3 of them) compile unchanged — `pow(2, 0) = 1.0`, no-op.

2. **Append enum entries** — In `ParamId`: `FM_ATTEN_PARAM` after `FILTER_TYPE_PARAM` (before `PARAMS_LEN`). In `InputId`: `FM_CV_INPUT` and `VOCT_INPUT` after `FILTER_TYPE_CV_INPUT` (before `INPUTS_LEN`). Never insert — append only for patch compatibility.

3. **Add `configParam` and `configInput` calls** — `configParam(FM_ATTEN_PARAM, -1.f, 1.f, 0.f, "FM Depth")`. `configInput(FM_CV_INPUT, "FM")`. `configInput(VOCT_INPUT, "V/Oct")`. Place after existing config calls, matching order.

4. **Add widget positions** — FM input: `mm2px(Vec(34.0, 90.0))`. FM attenuverter knob: `mm2px(Vec(42.0, 84.0))` (slightly above input row, visually associated). V/Oct input: `mm2px(Vec(58.0, 90.0))`. All use same widget types as existing inputs/knobs (PJ301MPort, RoundSmallBlackKnob).

5. **Hoist `isConnected()` checks** — Before the per-voice loop, add `bool fmCVConnected = inputs[FM_CV_INPUT].isConnected();` and `bool voctConnected = inputs[VOCT_INPUT].isConnected();`. Follows established pattern from `filterTypeCVConnected`.

6. **Wire 1V/Oct into cutoff calculation** — Inside per-voice loop, after existing cutoff CV calculation: if `voctConnected`, read `float voct = inputs[VOCT_INPUT].getPolyVoltage(c);` and sum into the exponent. The existing pattern is `cutoffHz = baseCutoffHz * pow(2, cutoffCV * cvAmount)`. With V/Oct: `cutoffHz = baseCutoffHz * pow(2, cutoffCV * cvAmount + voct)`. If cutoff CV is disconnected but V/Oct is connected, still apply: `cutoffHz = baseCutoffHz * pow(2, voct)`. The cleanest approach: accumulate the exponent, then apply `pow(2, ...)` once.

7. **Wire FM into setParams calls** — Inside per-voice loop, after cutoff calculation: if `fmCVConnected`, read `float fmCV = inputs[FM_CV_INPUT].getPolyVoltage(c)`, compute `float fmAmount = params[FM_ATTEN_PARAM].getValue()`, compute `float fmOffsetVoct = fmCV * fmAmount`. Pass `fmOffsetVoct` to all three `setParams()` calls (12dB mode: `filters[c]`; 24dB mode: `filters[c]` and `filters24dB_stage2[c]`). When FM is disconnected, pass `0.f` (or omit — default parameter handles it).

## Must-Haves

- [ ] `SVFilter::setParams()` has `float fmOffsetVoct = 0.f` parameter applied post-smoothing
- [ ] FM offset applied as `smoothedCutoff *= std::pow(2.f, fmOffsetVoct)` before normalization
- [ ] `FM_ATTEN_PARAM` appended to `ParamId` (after `FILTER_TYPE_PARAM`, before `PARAMS_LEN`)
- [ ] `FM_CV_INPUT` and `VOCT_INPUT` appended to `InputId` (after `FILTER_TYPE_CV_INPUT`, before `INPUTS_LEN`)
- [ ] 1V/Oct sums in logarithmic domain: exponent accumulation before `pow(2, ...)`
- [ ] FM `fmOffsetVoct` passed to both stages in 24dB mode
- [ ] `isConnected()` hoisted for FM and V/Oct inputs
- [ ] Variable named `fmAmount` (not `cvAmount`)
- [ ] Zero compiler warnings with `-Wall -Wextra`

## Verification

- `make -j4 2>&1 | grep -E "warning|error"` — zero output
- `grep 'fmOffsetVoct' src/SVFilter.hpp` — appears in signature and body
- `grep 'FM_CV_INPUT\|VOCT_INPUT\|FM_ATTEN_PARAM' src/CipherOB.cpp` — all three appear in enum, config, widget, and process sections
- `grep 'fmCVConnected\|voctConnected' src/CipherOB.cpp` — both hoisted outside loop

## Observability Impact

- **New DSP parameter:** `SVFilter::setParams()` gains `fmOffsetVoct` — applied post-smoothing as `smoothedCutoff *= pow(2, fmOffsetVoct)`. A future agent can verify correct wiring by grepping for `fmOffsetVoct` in both the declaration and all call sites.
- **Hoisted connection checks:** `fmCVConnected` and `voctConnected` booleans are computed once outside the per-voice loop. Grep for these names to confirm the fast-path optimization is in place.
- **Failure visibility:** FM driving cutoff to extremes is handled by existing SVFilter clamping (normalization to 0.001–0.49 of sample rate) and NaN/infinity reset. No new failure modes introduced — the existing `!std::isfinite()` check and `reset()` in `SVFilter::process()` covers catastrophic FM-driven instability.
- **Enum stability:** `FM_ATTEN_PARAM`, `FM_CV_INPUT`, `VOCT_INPUT` are appended before `*_LEN` sentinels. Verify with `grep -n 'PARAMS_LEN\|INPUTS_LEN' src/CipherOB.cpp` — new entries must appear on lines immediately before the sentinel.

## Inputs

- `src/SVFilter.hpp` — current `setParams()` at line 68, smoother at line 77, normalization at line 82
- `src/CipherOB.cpp` — enum layouts, config pattern, `setParams()` call sites at lines 165, 182, 199, cutoff CV pattern at lines 127-132
- S01 summary — `isConnected()` hoisting pattern, enum append convention, variable naming (`resCvAmount` pattern)

## Expected Output

- `src/SVFilter.hpp` — `setParams()` signature extended with `fmOffsetVoct`, applied post-smoothing
- `src/CipherOB.cpp` — three new enum entries, three new config calls, three new widget positions, FM/V/Oct reading in process loop with hoisted `isConnected()`, `fmOffsetVoct` passed to all `setParams()` calls
