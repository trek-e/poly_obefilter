---
id: S02
parent: M002
milestone: M002
provides:
  - FM input with bipolar attenuverter (-1 to +1) wired post-smoothing for audio-rate cutoff modulation
  - 1V/Oct pitch tracking input summed in logarithmic domain with cutoff CV
  - Panel SVG with FM input jack, FM attenuverter knob, V/Oct input jack, dot indicators, and labels
  - Polyphonic per-voice FM and V/Oct reading via getPolyVoltage
requires:
  - slice: S01
    provides: Per-voice crossfade arrays, enum append convention, isConnected() hoisting pattern, panel SVG jack pattern
affects: []
key_files:
  - src/SVFilter.hpp
  - src/CipherOB.cpp
  - res/CipherOB.svg
key_decisions:
  - FM offset applied post-smoothing as pow(2, fmOffsetVoct) — preserves audio-rate modulation by bypassing cutoffSmoother
  - V/Oct summed into exponent accumulator with cutoff CV — single pow(2, cutoffExponent) call instead of multiplied pow calls
  - Post-smoothing injection via default-valued parameter (fmOffsetVoct = 0.f) — backward compatible, no changes to existing call sites
  - Purple (#8040a0) dot indicators for modulation inputs, distinct from blue (#3060a0) audio inputs
  - V/Oct label rendered as "V/O" — compact form for panel space
patterns_established:
  - Exponent accumulation in log domain before single pow(2, ...) for multiple pitch-domain CV sources
  - Post-smoothing DSP parameter injection via default-valued function parameter
  - Modulation inputs use distinct dot indicator color from signal-path inputs
observability_surfaces:
  - "grep fmOffsetVoct src/SVFilter.hpp — confirms post-smoothing FM offset in signature and body"
  - "grep 'fmCVConnected\\|voctConnected' src/CipherOB.cpp — confirms hoisted connection checks"
  - "grep -n 'setParams.*fmOffsetVoct' src/CipherOB.cpp — confirms FM offset passed to all three call sites"
  - "xmllint --noout res/CipherOB.svg — validates SVG integrity"
  - "SVFilter NaN/infinity reset handles FM-driven instability — no new failure modes added"
drill_down_paths:
  - .gsd/milestones/M002/slices/S02/tasks/T01-SUMMARY.md
  - .gsd/milestones/M002/slices/S02/tasks/T02-SUMMARY.md
duration: 30m
verification_result: passed
completed_at: 2026-03-13
---

# S02: FM Input and 1V/Oct Pitch Tracking

**Audio-rate FM input with bipolar attenuverter and 1V/Oct pitch tracking input wired into the SVFilter DSP pipeline, bypassing cutoff smoothing for FM fidelity and summing V/Oct in logarithmic domain for true pitch tracking.**

## What Happened

Extended `SVFilter::setParams()` with a `float fmOffsetVoct = 0.f` parameter applied post-smoothing as `smoothedCutoff *= std::pow(2.f, fmOffsetVoct)` before frequency normalization. This is the key design choice — FM bypasses the 1ms cutoff smoother so audio-rate modulation isn't attenuated above ~160Hz, while the base cutoff knob and CV continue to be smoothed for zipper noise prevention.

Three new enum entries appended in order: `FM_ATTEN_PARAM` (after `FILTER_TYPE_PARAM`), `FM_CV_INPUT` and `VOCT_INPUT` (after `FILTER_TYPE_CV_INPUT`). All before `*_LEN` sentinels for patch compatibility.

V/Oct is accumulated in the logarithmic domain alongside cutoff CV: `cutoffExponent = cutoffCV * cvAmount + voct`, then a single `std::pow(2.f, cutoffExponent)` call scales the base cutoff. This correctly handles both CV sources with one `pow` instead of multiplied separate calls.

FM CV is read per-voice, scaled by the bipolar attenuverter (-1 to +1), and passed as `fmOffsetVoct` to all three `setParams()` call sites: the 12dB single-stage filter and both stages of the 24dB cascade. Connection checks for FM and V/Oct are hoisted outside the per-voice loop following S01's pattern.

Panel SVG updated with purple (#8040a0) dot indicators for FM input (34, 90), FM attenuverter (42, 84), and V/Oct input (58, 90), plus pixel-art "FM" and "V/O" labels. Component layer entries match widget coordinates exactly. INPUT section header widened to full panel width (65.12mm).

## Verification

| Check | Status |
|---|---|
| `make -j4` zero warnings/errors | ✅ |
| `fmOffsetVoct` in SVFilter.hpp (≥2: signature + body) | ✅ (2) |
| FM/VOCT/FM_ATTEN references in CipherOB.cpp (≥6) | ✅ (14) |
| `xmllint --noout res/CipherOB.svg` exit 0 | ✅ |
| SVG component layer has fm-cv-input, fm-atten, voct-input | ✅ (3) |
| Widget positions match SVG coordinates | ✅ (34/90, 42/84, 58/90) |
| FM offset passed to all 3 setParams() call sites | ✅ (lines 186, 203, 220) |
| Hoisted isConnected() checks outside per-voice loop | ✅ (lines 89-90) |
| Runtime UAT | ⏳ deferred to human tester |

## Requirements Advanced

- CTRL-08 — FM input with attenuverter fully implemented: bipolar attenuverter, audio-rate capable, exponential FM. Build-verified, awaiting runtime UAT.
- CTRL-09 — 1V/Oct pitch tracking fully implemented: no attenuverter, sums in log domain. Build-verified, awaiting runtime UAT.
- CTRL-12 — FM bypasses smoothing confirmed: `fmOffsetVoct` applied after `cutoffSmoother.process()` in `setParams()`. Build-verified, awaiting runtime UAT.
- CTRL-13 — 1V/Oct logarithmic sum confirmed: V/Oct accumulated in exponent with cutoff CV, single `pow(2, ...)` call. Build-verified, awaiting runtime UAT.

## Requirements Validated

- none — all four requirements need runtime UAT to validate (listening tests for FM audibility, pitch tracking accuracy)

## New Requirements Surfaced

- none

## Requirements Invalidated or Re-scoped

- none

## Deviations

- T02 (panel SVG) was effectively completed during T01 execution — DSP wiring and panel SVG changes were done together since widget positions and component layer entries must agree with C++ code. T02 verified the existing work rather than making new edits.

## Known Limitations

- FM can drive cutoff outside audible range — existing `clamp` in SVFilter normalization (0.001–0.49) prevents instability, but extreme FM depth may push cutoff beyond useful range. This is expected analog filter behavior.
- No FM depth CV input — attenuverter is manual only. Could be added in a future milestone if needed.
- Runtime UAT not yet performed — all verification is build-time and code-structure. Listening tests for FM audibility and pitch tracking accuracy are deferred to human tester.

## Follow-ups

- Human UAT for both S01 and S02 (see S01-UAT.md and S02-UAT.md)
- After UAT passes, milestone M002 is complete — advance CTRL-06/08/09/10/11/12/13 to validated status

## Files Created/Modified

- `src/SVFilter.hpp` — Added `fmOffsetVoct` default parameter to `setParams()`, applied post-smoothing as `pow(2, fmOffsetVoct)` multiplication
- `src/CipherOB.cpp` — Added FM_ATTEN_PARAM, FM_CV_INPUT, VOCT_INPUT enums/config/widgets; wired FM and V/Oct in process loop with hoisted isConnected() checks; FM offset passed to all three setParams() call sites
- `res/CipherOB.svg` — Added FM/V/Oct dot indicators (purple #8040a0), pixel-art labels, component layer entries; widened INPUT section header to full panel width

## Forward Intelligence

### What the next slice should know
- M002 is complete pending UAT — no more slices in this milestone. Next work is M003 or whatever milestone follows.
- All enum entries are now: Params has FM_ATTEN_PARAM before PARAMS_LEN; Inputs has FM_CV_INPUT and VOCT_INPUT before INPUTS_LEN. Any future inputs/params must append after these, before the sentinels.

### What's fragile
- `fmOffsetVoct` post-smoothing placement — if anyone moves it before `cutoffSmoother.process()`, audio-rate FM will be killed by the 1ms smoother. The line order in `setParams()` is load-bearing.
- Exponent accumulation — `cutoffCV * cvAmount + voct` must stay in the exponent, not be applied as a frequency multiplier separately, or the interaction between cutoff CV and V/Oct will be incorrect.

### Authoritative diagnostics
- `grep -n 'setParams.*fmOffsetVoct' src/CipherOB.cpp` — confirms all three call sites pass FM offset; if any are missing, 24dB mode stages won't track FM identically
- `grep -n 'smoothedCutoff \*=' src/SVFilter.hpp` — the post-smoothing FM application; must appear after `cutoffSmoother.process()` line

### What assumptions changed
- T02 (panel SVG) assumed it would need to create new SVG elements — T01 already included all panel changes, so T02 became a verification-only task
