# M002: v0.70b CV Control — Research

**Date:** 2026-03-13

## Summary

The module needs three new CV inputs (Filter Type, FM, 1V/Oct) plus one attenuverter knob (FM depth). The codebase is clean at 263 LOC with well-established CV patterns for cutoff, resonance, and drive that should be followed. Panel space at 14 HP is the tightest constraint — the gap between the Audio Input row (Y=90) and the output row (Y=109) has enough room for a new input row at ~Y=90, and the area right of Audio In (X=30-66) can absorb the three new jacks plus one small knob.

The most critical risk is the **crossfade state machine**. Currently `crossfadeCounter` is a single module-level int — fine for a panel switch that changes globally, but CV-driven filter type switching with polyphonic CV means each voice can switch independently. This requires per-voice crossfade counters (`crossfadeCounter[PORT_MAX_CHANNELS]`, `prevFilterType[PORT_MAX_CHANNELS]`). This is the change that touches the most existing code and is most likely to introduce clicks or pops if done wrong. It should be proven first.

The 1V/Oct and FM inputs are lower risk — they follow established patterns from both this module's cutoff CV and the companion VCO module. The VCO uses `dsp::FREQ_C4 * dsp::exp2_taylor5(pitch)` for V/Oct-to-frequency conversion, and the filter should adopt the same approach for its 1V/Oct input rather than the current hand-rolled `20.f * pow(1000.f, param)` mapping. The FM input follows exponential FM convention for filters (V/Oct scaled, not linear FM), summing into the cutoff frequency in the logarithmic domain before frequency warping.

## Recommendation

**Start with per-voice crossfade refactoring + filter type CV**, because it's the highest-risk change and touches existing working code. If the crossfade breaks, nothing else matters. Then add FM input (moderate risk — audio-rate modulation may expose parameter smoothing issues), and finally 1V/Oct (lowest risk — pure additive V/Oct path with no attenuverter).

Panel SVG work should be its own slice after DSP is validated, or combined with the first slice if layout coordinates are determined up front.

## Don't Hand-Roll

| Problem | Existing Solution | Why Use It |
|---------|------------------|------------|
| V/Oct to frequency conversion | `dsp::FREQ_C4 * dsp::exp2_taylor5(pitch)` (Rack SDK) | Already used in companion VCO; accurate Taylor approximation; standard VCV convention |
| Parameter smoothing | `rack::dsp::TExponentialFilter` with 1ms tau | Already used for cutoff, resonance, drive in SVFilter and module; consistent zipper-noise prevention |
| Phase-wrap / crossfade timing | Existing 128-sample crossfade state machine | Proven click-free in v0.60b; reuse the mechanism, extend to per-voice |
| Polyphonic CV reading | `getPolyVoltage(c)` with `isConnected()` guard | Established pattern in all three existing CV inputs; handles mono-to-poly spreading |

## Existing Code and Patterns

- **`src/CipherOB.cpp` lines 79-82** — Filter type read + mode change detection. This is where CV override logic must be inserted. Current: `int filterType = (int)(params[FILTER_TYPE_PARAM].getValue() + 0.5f)` reads from panel switch only.
- **`src/CipherOB.cpp` lines 110-113** — Cutoff CV pattern. V/Oct exponential: `cutoffHz = baseCutoffHz * pow(2, cutoffCV * cvAmount)`. FM and 1V/Oct should sum into cutoff similarly but with different scaling.
- **`src/CipherOB.cpp` lines 136-144** — Crossfade-from capture. Currently `modeJustChanged` is computed once (line 82) and applied to all voices. Must become per-voice: `modeJustChanged[c]` comparing `filterType[c]` with `prevFilterType[c]`.
- **`src/CipherOB.cpp` lines 196-213** — Crossfade application + counter decrement. `crossfadeCounter` is decremented once after all voices. Must become `crossfadeCounter[c]` decremented per-voice within the loop.
- **`src/SVFilter.hpp`** — Filter DSP. Untouched by this milestone. FM modulates cutoff frequency *before* `setParams()` is called — no SVFilter changes needed.
- **`../vco/src/PhantomEight.cpp` lines 586-601** — FM implementation reference. Uses linear FM (`freq + freq * modulator * depth`). Filter FM should use exponential FM instead (`cutoffHz * pow(2, fmCV * fmAmount)`), which is the VCV convention for filter cutoff modulation.

## Constraints

- **14 HP panel** — 71.12mm wide. Current components use X range 10-60mm. New components must fit without moving existing ones (would break saved patches).
- **Patch compatibility** — New params/inputs must be appended to their respective enums, never inserted. Existing PARAMS_LEN position matters for save file format.
- **Zero warnings** — Build currently produces zero warnings with `-Wall -Wextra`. Must maintain this.
- **C++11** — Makefile specifies `-std=c++11`. No structured bindings, no `if constexpr`, no `std::optional`.
- **No SIMD yet** — Module processes voices scalar (unlike the VCO which uses `float_4`). SIMD is v0.90b scope. New code should remain scalar for consistency.
- **Audio-rate FM** — The SVFilter's internal `cutoffSmoother` has 1ms tau. This will low-pass FM modulation at audio rates. For FM to work at audio rate, the FM signal should bypass smoothing or the smoother tau should be reduced/bypassed when FM is connected. This is the biggest DSP pitfall.

## Common Pitfalls

- **Audio-rate FM killed by parameter smoothing** — The SVFilter's `cutoffSmoother` (1ms tau exponential filter) will attenuate FM at frequencies above ~160Hz. Fix: apply FM *after* smoothing, directly to the frequency calculation, or bypass the smoother for the FM component. The base cutoff (knob + cutoff CV) should still be smoothed; only the FM component should be unsmoothed.
- **Global crossfade counter with per-voice CV** — If voice 1 switches type while voice 8 doesn't, a single crossfade counter either crossfades all voices (wrong) or only the switching voice (current code can't do this). Fix: per-voice `crossfadeCounter[c]` and `prevFilterType[c]` arrays.
- **Filter type CV threshold hysteresis** — A simple `> 2.5V` threshold will chatter when CV hovers near 2.5V. Fix: Schmitt trigger behavior with ~0.1V hysteresis (switch at 2.6V rising, 2.4V falling). This is standard VCV practice for gate/trigger detection.
- **Panel switch vs CV priority** — If both the panel switch and CV are present, which wins? Standard VCV pattern: CV overrides the switch when connected. The panel switch still shows its position but has no effect while CV is patched. This should be documented in the `configInput` tooltip.
- **1V/Oct stacking with cutoff CV** — Both inputs modulate cutoff. They must be additive in the V/Oct (logarithmic) domain, not in the Hz domain. Correct: `cutoffHz = baseCutoffHz * pow(2, cutoffCV * cvAmount) * pow(2, voct)`. Not: `cutoffHz = baseCutoffHz * pow(2, cutoffCV * cvAmount) + voct_hz`.
- **Filter state discontinuity on type switch** — When switching from 12dB to 24dB, the stage 2 filter's integrator states (`ic1eq`, `ic2eq`) may contain stale values from the last time 24dB was active. Fix: reset stage 2 filters on transition to 24dB mode, letting the crossfade mask the transient.

## Open Risks

- **FM at audio rate with 16 voices** — Each voice recomputes `tan(π·f_norm)` per sample when FM is active. This is the most expensive math in the filter. With 16 voices and audio-rate FM, this may cause CPU spikes. Mitigation: not a blocker for v0.70b since SIMD optimization is v0.90b scope, but worth measuring.
- **Panel SVG complexity** — Adding 3 jacks and 1 knob to an already-detailed SVG. The SVG is 313 lines / 15KB. Layout changes must be done carefully to avoid overlapping elements. The SVG has hand-drawn label text (paths, not text elements), so labels for new components must match the style.
- **Crossfade audibility with fast CV switching** — If a user sends an audio-rate signal to the filter type CV, the 128-sample crossfade (~2.7ms at 48kHz) will overlap with the next switch, creating a permanent crossfade state. Need to decide: ignore the issue (user error at audio rate), or clamp the minimum time between switches.
- **CTRL-07 (mode selection CV)** is explicitly deferred in M002-CONTEXT.md — the architecture doesn't support "mode selection" since all 4 outputs are always available in 12dB mode. This is the right call; revisit if/when a mixed/selected output jack is added in v0.90b.

## Candidate Requirements

The following behaviors are not in the current requirements but are standard expectations for VCV filter modules. They should be considered for addition:

- **CTRL-10 — Filter type CV overrides panel switch when connected** (table stakes — standard VCV convention; without this, behavior is ambiguous)
- **CTRL-11 — Schmitt trigger hysteresis on filter type CV threshold** (table stakes — prevents chatter; ~0.1V hysteresis is standard)
- **CTRL-12 — FM input bypasses cutoff parameter smoothing** (domain-standard — without this, audio-rate FM is inaudible above ~160Hz)
- **CTRL-13 — 1V/Oct sums with cutoff knob in logarithmic domain** (table stakes — standard pitch-tracking behavior; knob sets base, 1V/Oct offsets)

Advisory only (not candidate requirements):
- Stage 2 filter reset on type transition — implementation detail, not user-visible behavior
- Audio-rate type CV limiting — edge case, defer unless users report problems

## Skills Discovered

| Technology | Skill | Status |
|------------|-------|--------|
| VCV Rack | none relevant found | No VCV Rack skill exists (search returned unrelated Ruby "rack-middleware" results) |
| C++ DSP / audio | none found | No general audio DSP skill available |

## Sources

- VCV Rack SDK `engine/Port.hpp` — `getPolyVoltage()`, `isConnected()`, polyphonic port API
- VCV Rack SDK `dsp/approx.hpp` — `exp2_taylor5()` for V/Oct conversion
- VCV Rack SDK `dsp/common.hpp` — `FREQ_C4 = 261.6256f` constant
- `../vco/src/PhantomEight.cpp` — FM implementation (linear, through-zero), V/Oct pitch handling, polyphonic CV reading patterns
- `src/CipherOB.cpp` — Current module: CV patterns, crossfade state machine, filter type switching
- `src/SVFilter.hpp` — Filter internals: `cutoffSmoother` with 1ms tau, `setParams()` interface
