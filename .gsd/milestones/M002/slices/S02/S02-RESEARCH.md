# S02: FM Input and 1V/Oct Pitch Tracking — Research

**Date:** 2026-03-13

## Summary

This slice adds two new CV inputs (FM, 1V/Oct) and one attenuverter knob (FM depth) to the module. The DSP injection points are clear: 1V/Oct sums with the existing cutoff CV in the V/Oct (logarithmic) domain before `setParams()`, going through the cutoff smoother like any other pitch offset. FM, however, must bypass the smoother — the SVFilter's 1ms tau `cutoffSmoother` attenuates signals above ~160Hz, which would kill audio-rate FM. The solution is to add an `fmOffsetVoct` parameter to `SVFilter::setParams()` with a default of `0.f`, applied post-smoothing but pre-frequency-warp (`tan(π·f_norm)`). This keeps all existing call sites unchanged and localizes the FM bypass to SVFilter's coefficient computation.

Panel space is sufficient. The input row at Y=90 currently has only the audio input at X=18. Three new components fit comfortably: FM input at ~X=34, FM attenuverter knob at ~X=42 (slightly above, at Y=84), and 1V/Oct input at ~X=58. The INPUT section header at Y=80 and OUTPUTS header at Y=97 bound the usable space. All three new components need SVG visual elements (dot indicators, pixel-art labels) plus component layer entries matching the established pattern from S01.

The biggest risk is the `tan(π·f_norm)` recomputation cost with audio-rate FM. Every sample where FM is non-zero forces a new `tan()` call per voice per filter stage. With 16 voices × 2 stages (24dB mode) × 48kHz, that's 1.5M `tan()` calls/second. This is the most expensive math in the module but is unavoidable for accurate FM. SIMD optimization (v0.90b) will address this. For now, it's acceptable — the VCO already does similar per-sample frequency computation at 16 voices.

## Recommendation

**Two tasks:**

**T01 — DSP: FM input with smoother bypass + 1V/Oct pitch tracking.** Modify `SVFilter::setParams()` to accept an `fmOffsetVoct` parameter (default `0.f`). Add `FM_CV_INPUT`, `VOCT_INPUT` to `InputId` enum and `FM_ATTEN_PARAM` to `ParamId` enum (all appended before `*_LEN`). In the process loop: read FM CV and attenuverter, compute `fmOffsetVoct = fmCV * fmAmount`; read 1V/Oct CV and sum into the cutoff exponent. Wire widget positions and config strings. Verify zero-warning build.

**T02 — Panel SVG: FM input, FM attenuverter, 1V/Oct input visual elements.** Add dot indicators, pixel-art labels, and component layer entries for all three new elements. Extend the INPUT section header to span the full row width (currently `width="30"`, needs to cover X=3–68 to include new jacks). Verify valid SVG and coordinate alignment with widget code.

T01 and T02 can be done in either order, but T01 first is preferable since it establishes the enum values and widget positions that the SVG must match.

## Don't Hand-Roll

| Problem | Existing Solution | Why Use It |
|---------|------------------|------------|
| V/Oct to frequency scaling | `std::pow(2.f, voct)` multiplier in log domain | Already used for cutoff CV at line 130; consistent V/Oct convention |
| Polyphonic CV reading | `getPolyVoltage(c)` with `isConnected()` guard | Established pattern in all existing CV inputs (cutoff, resonance, drive, filter type) |
| Parameter smoothing | `cutoffSmoother` inside `SVFilter::setParams()` | Keeps base cutoff smoothed; FM bypasses by going through separate path |
| Frequency warping | `tan(π·f_norm)` in SVFilter | Trapezoidal integration requires this; no shortcut without accuracy loss |
| `isConnected()` hoisting | Check once outside voice loop, branch inside | Established in S01 for `filterTypeCVConnected`; avoids redundant port queries per-voice |

## Existing Code and Patterns

- **`src/CipherOB.cpp` lines 127-132** — Cutoff CV pattern. `cutoffHz = baseCutoffHz * pow(2, cutoffCV * cvAmount)`. 1V/Oct sums into this exponent: `pow(2, cutoffCV * cvAmount + voct)`. FM goes separately via `setParams()`.
- **`src/CipherOB.cpp` lines 165-166, 182-183, 199-200** — `setParams()` call sites. Both 12dB and 24dB branches call `filters[c].setParams(cutoffHz, resonance, sampleRate)`. FM offset must be passed to all three calls (stage 1 and stage 2 in 24dB mode).
- **`src/SVFilter.hpp` lines 68-95** — `setParams()` implementation. Smooths cutoff and resonance, then computes `g = tan(π·f_norm)`. The FM offset must be applied between the smoother output (line 77) and the normalization (line 82): `smoothedCutoff *= pow(2, fmOffsetVoct)` before clamping.
- **`src/CipherOB.cpp` lines 82, 111** — `isConnected()` hoisting pattern from S01. `filterTypeCVConnected` checked once outside the loop. FM and V/Oct inputs should follow the same pattern.
- **`src/CipherOB.cpp` lines 56-58** — Param config pattern for attenuverters. `configParam(X_ATTEN_PARAM, -1.f, 1.f, 0.f, "X CV")`. FM attenuverter follows this: `configParam(FM_ATTEN_PARAM, -1.f, 1.f, 0.f, "FM Depth")` — range -1 to +1 allows inverted FM.
- **`../vco/src/PhantomEight.cpp` lines 371, 596-601** — VCO FM reference. Uses linear FM (`freq + freq * modulator * depth`). Filter FM should use exponential FM instead: `cutoffHz * pow(2, fmCV * fmAmount)`. The VCO's FM knob is 0-10 → scaled 0-1; filter FM attenuverter is -1 to +1 (matches existing attenuverter convention).
- **`res/CipherOB.svg` lines 225-233** — INPUT section header at Y=80 with `width="30"`. Currently only covers the audio input's X range. Needs widening to `width="65.12"` to span the full panel for the new FM and 1V/Oct jacks.
- **`res/CipherOB.svg` line 308** — S01's filter type CV component entry pattern: `<circle style="fill:#00ff00" cx="24" cy="32" r="3.5" id="filter-type-cv-input"/>`. New inputs follow this pattern with appropriate IDs.

## Constraints

- **C++11** — No default parameter values on overloaded functions using `= {}` syntax with complex types, but `float fmOffsetVoct = 0.f` is fine. No `std::optional`.
- **14 HP panel (71.12mm)** — New components must fit within X=3-68mm usable area. The input row (Y=80-97) has ample horizontal space right of the audio input (X=18).
- **Patch compatibility** — `FM_ATTEN_PARAM` appended after `FILTER_TYPE_PARAM` (before `PARAMS_LEN`). `FM_CV_INPUT` and `VOCT_INPUT` appended after `FILTER_TYPE_CV_INPUT` (before `INPUTS_LEN`). Never insert.
- **Zero warnings** — `-Wall -Wextra` via Rack SDK's `compile.mk`. All new variables must be used; no shadowing of existing `cvAmount` (S01 already established `resCvAmount` pattern for this).
- **No SIMD** — Module is scalar. New code stays scalar for consistency. `pow(2, x)` not `dsp::exp2_taylor5(x)` (scalar version, though `exp2_taylor5` accepts scalar `float`; either works, but `pow(2,x)` is more readable for C++11 and matches existing code).
- **Both filter stages need FM** — In 24dB mode, both `filters[c]` (stage 1) and `filters24dB_stage2[c]` (stage 2) must receive the FM offset. Same cutoff, same FM — the cascade should track identically.

## Common Pitfalls

- **FM killed by cutoff smoother** — If FM is passed as part of `cutoffHz` to the existing `setParams()`, the 1ms tau smoother attenuates everything above ~160Hz. Fix: FM offset is a separate parameter applied post-smoothing inside `setParams()`. The base cutoff (knob + cutoff CV + 1V/Oct) is still smoothed.
- **FM attenuverter shadowing `cvAmount`** — The existing cutoff attenuverter uses `cvAmount`. The FM attenuverter must use a distinct name like `fmAmount` or `fmDepth`. Resonance already established this pattern with `resCvAmount`.
- **1V/Oct applied in Hz domain instead of V/Oct domain** — Wrong: `cutoffHz + voct_in_hz`. Correct: `cutoffHz * pow(2, voct)`. The exponent sum (`cutoffCV * cvAmount + voct`) must happen before the `pow(2, ...)`, not after.
- **FM offset not applied to stage 2** — In 24dB mode, if `fmOffsetVoct` is only passed to `filters[c].setParams()` but not `filters24dB_stage2[c].setParams()`, the cascade stages will have different cutoff frequencies. Both calls must pass the same FM offset.
- **1V/Oct range not clamped** — A very large V/Oct value could push cutoff above Nyquist/2 or below 0. The existing clamp at line 132 (`clamp(cutoffHz, 20, 20000)`) handles this for the base cutoff. With 1V/Oct summed into the exponent, the clamp still applies. Inside `setParams`, the post-FM cutoff should also be clamped before normalization (it already clamps via `cutoffNorm` at 0.49 of sample rate).
- **FM with disconnected input reads stale voltage** — `getPolyVoltage(c)` returns 0V when disconnected, but checking `isConnected()` first and skipping the FM offset entirely is cleaner — avoids the `pow(2, 0)` multiplication (which is 1.0, harmless but wasteful).

## Open Risks

- **`tan()` cost at audio-rate FM with 16 voices** — Each sample recomputes `tan(π·f_norm)` per voice per filter stage when FM is active. At 48kHz, 16 voices, 24dB mode (2 stages): 1.536M `tan()` calls/second. This is the dominant CPU cost. Mitigation: defer to SIMD optimization (v0.90b). The VCO already does comparable per-sample trig at 16 voices without issues.
- **FM depth scaling** — The attenuverter range is -1 to +1. With a 10V FM signal and attenuverter at 1.0, the FM offset is 10 V/Oct — that's a 1024× frequency multiplier. This may push cutoff to absurd values. The internal clamping (`cutoffNorm` at 0.49) prevents aliasing, but the extreme modulation depth may not be musically useful. Consider whether the attenuverter should also scale the CV (e.g., `fmCV * fmAmount * 0.5` for ±5 V/Oct range) or leave full range and let users set the attenuverter low. Full range is the VCV convention — modules don't limit CV range.
- **Panel SVG label style** — Existing labels use hand-drawn pixel-art paths (rectangles for vertical/horizontal strokes). New labels ("FM", "V/OCT") must match this style. "FM" is two characters — straightforward. "V/OCT" is 5 characters — may be too wide. Consider "V/O" as shorthand, or use smaller font scale.

## Skills Discovered

| Technology | Skill | Status |
|------------|-------|--------|
| VCV Rack | none relevant | `npx skills find "VCV Rack"` returned only unrelated `rack-middleware` (Ruby) |
| Audio DSP | `erichowens/some_claude_skills@sound-engineer` (74 installs) | Available — production/mixing focus, not DSP coding. Not relevant. |
| C++ DSP | none found | `npx skills find "audio DSP"` returned mixing/production skills only |

## Sources

- `src/SVFilter.hpp` — cutoffSmoother implementation, `setParams()` interface where FM bypass must be injected
- `src/CipherOB.cpp` — current cutoff CV calculation (lines 127-132), `setParams()` call sites (lines 165, 182, 199), attenuverter config pattern
- `../vco/src/PhantomEight.cpp` — FM implementation reference (linear FM, lines 596-601), V/Oct handling (`dsp::FREQ_C4 * dsp::exp2_taylor5(pitch)`, line 524)
- VCV Rack SDK `dsp/approx.hpp` — `exp2_taylor5()` Taylor polynomial for 2^x (6e-06 relative error)
- VCV Rack SDK `dsp/common.hpp` — `FREQ_C4 = 261.6256f` constant
- VCV Rack SDK `compile.mk` — confirms C++11 (`-std=c++11`)
- `.gsd/milestones/M002/slices/S01/S01-SUMMARY.md` — per-voice patterns, `isConnected()` hoisting, enum append convention
