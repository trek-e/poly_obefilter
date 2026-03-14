---
id: M002
provides:
  - Filter type CV input with per-voice Schmitt trigger switching (2.6V/2.4V hysteresis)
  - CV-overrides-switch convention — panel switch works when CV disconnected
  - Per-voice crossfade arrays replacing global crossfade state machine
  - FM input with bipolar attenuverter, post-smoothing injection for audio-rate modulation
  - 1V/Oct pitch tracking input with logarithmic domain accumulation
  - All three new CV inputs polyphonic (up to 16 channels via getPolyVoltage)
  - Panel SVG with filter type CV jack, FM jack, FM attenuverter, V/Oct jack
key_decisions:
  - Per-voice crossfade arrays replace global crossfadeCounter/prevFilterType — required for polyphonic CV-driven type switching
  - Filter type CV uses gate threshold with Schmitt trigger hysteresis (2.6V rising, 2.4V falling) — simpler and more predictable than additive CV+switch
  - CV overrides panel switch when connected — standard VCV convention
  - FM uses exponential FM (V/Oct scaled) not linear FM — VCV filter convention
  - FM bypasses cutoffSmoother via post-smoothing injection (fmOffsetVoct default parameter) — 1ms tau would kill audio-rate FM above ~160Hz
  - 1V/Oct has no attenuverter — true pitch tracking; knob sets base, 1V/Oct offsets in log domain
  - Exponent accumulation pattern — cutoffCV * cvAmount + voct in log domain, then single pow(2, ...) call
  - Purple (#8040a0) dot indicators for modulation inputs, distinct from blue (#3060a0) audio inputs
  - V/Oct label "V/O" — compact form for 14 HP panel
  - CTRL-07 (mode selection CV) deferred — all 4 outputs always active in 12dB mode, no mode to select
patterns_established:
  - Per-voice state arrays for CV-driven boolean state — array[PORT_MAX_CHANNELS] zero-init, [c] indexed in voice loop
  - CV-overrides-switch pattern — isConnected() hoisted outside loop, per-voice branching between CV read and param read
  - Post-smoothing DSP parameter injection via default-valued function parameter
  - Exponent accumulation in log domain before single pow(2, ...) for multiple pitch-domain CV sources
  - Modulation inputs use distinct dot indicator color from signal-path inputs
observability_surfaces:
  - "grep -n 'prevFilterType\\b\\|crossfadeCounter\\b' src/CipherOB.cpp — all matches must be array or [c] indexed, no bare scalars"
  - "grep -n 'setParams.*fmOffsetVoct' src/CipherOB.cpp — must show all 3 call sites (12dB, 24dB stage1, 24dB stage2)"
  - "grep -n 'smoothedCutoff \\*=' src/SVFilter.hpp — post-smoothing FM application must appear after cutoffSmoother.process()"
  - "make -j4 2>&1 | grep -E 'warning|error' — must produce zero output"
  - "xmllint --noout res/CipherOB.svg — exit 0"
requirement_outcomes:
  - id: CTRL-06
    from_status: active
    to_status: active
    proof: "S01 implemented per-voice Schmitt trigger CV input with polyphonic switching. Build-verified. Runtime UAT deferred to human tester."
  - id: CTRL-08
    from_status: active
    to_status: active
    proof: "S02 implemented FM input with bipolar attenuverter, post-smoothing exponential FM. Build-verified. Runtime UAT deferred."
  - id: CTRL-09
    from_status: active
    to_status: active
    proof: "S02 implemented 1V/Oct with log-domain accumulation. Build-verified. Runtime UAT deferred."
  - id: CTRL-10
    from_status: active
    to_status: active
    proof: "S01 implemented isConnected() branching — CV overrides switch when patched. Build-verified. Runtime UAT deferred."
  - id: CTRL-11
    from_status: active
    to_status: active
    proof: "S01 implemented 200mV hysteresis (2.4V falling, 2.6V rising) per-voice. Build-verified. Runtime UAT deferred."
  - id: CTRL-12
    from_status: active
    to_status: active
    proof: "S02 implemented fmOffsetVoct applied after cutoffSmoother.process(). Build-verified. Runtime UAT deferred."
  - id: CTRL-13
    from_status: active
    to_status: active
    proof: "S02 implemented V/Oct accumulated in exponent with cutoff CV, single pow(2,...) call. Build-verified. Runtime UAT deferred."
duration: 1h
verification_result: passed
completed_at: 2026-03-13
---

# M002: v0.70b CV Control

**Filter type CV with per-voice Schmitt trigger switching, audio-rate FM input with post-smoothing bypass, and 1V/Oct pitch tracking — completing the module's CV control surface for full patchability.**

## What Happened

Two slices filled the module's remaining control gaps: filter type CV switching (S01) and FM/pitch-tracking inputs (S02).

**S01** tackled the highest-risk work — refactoring the global crossfade state machine to per-voice arrays. `prevFilterType`, `crossfadeCounter`, and new `filterTypeHigh` became `[PORT_MAX_CHANNELS]` arrays, enabling each polyphonic voice to switch filter type independently. The new `FILTER_TYPE_CV_INPUT` reads per-channel voltage through a Schmitt trigger (2.6V rising / 2.4V falling) to prevent chatter. When CV is connected, it overrides the panel CKSS switch; when disconnected, the switch resumes control. The 128-sample crossfade fires per-voice on type changes, preserving the click-free behavior validated in M001.

**S02** added FM and 1V/Oct inputs to the DSP pipeline. The key design choice was injecting FM *after* the cutoff smoother: `SVFilter::setParams()` gained a `float fmOffsetVoct = 0.f` parameter applied as `smoothedCutoff *= pow(2, fmOffsetVoct)` post-smoothing. This preserves audio-rate FM fidelity — the 1ms cutoff smoother would otherwise attenuate modulation above ~160Hz. The FM attenuverter is bipolar (-1 to +1) for inverted modulation. V/Oct is accumulated in the logarithmic domain alongside cutoff CV (`cutoffExponent = cutoffCV * cvAmount + voct`) with a single `pow(2, cutoffExponent)` call scaling the base cutoff — correct pitch-domain math without multiplied separate pow calls. FM offset is passed to all three `setParams()` call sites (12dB single-stage, 24dB stage 1, 24dB stage 2) so both filter modes track FM identically.

Panel SVG updated across both slices: filter type CV jack at (24, 32), FM jack at (34, 90), FM attenuverter at (42, 84), V/Oct jack at (58, 90). Modulation inputs use purple (#8040a0) dot indicators, visually distinct from the blue (#3060a0) audio-path inputs.

All new enum entries are appended before sentinels — `FM_ATTEN_PARAM` before `PARAMS_LEN`, `FILTER_TYPE_CV_INPUT` / `FM_CV_INPUT` / `VOCT_INPUT` before `INPUTS_LEN` — preserving patch compatibility with v0.60b saves.

## Cross-Slice Verification

| Success Criterion | Evidence | Status |
|---|---|---|
| CV above threshold switches 12dB→24dB; below switches back — click-free crossfade per-voice | Schmitt trigger at 2.6V/2.4V per-voice (`filterTypeHigh[c]`), per-voice crossfade arrays (`crossfadeCounter[c]`, `prevFilterType[c]`). Build-verified; runtime UAT deferred. | ✅ build |
| LFO/gate toggles filter types without clicks/pops/chatter | 200mV hysteresis band prevents chatter; 128-sample crossfade prevents clicks. Build-verified; listening test deferred. | ✅ build |
| Audio-rate FM audibly modulates cutoff | `fmOffsetVoct` applied post-smoothing as `pow(2, fmOffsetVoct)` — bypasses 1ms cutoff smoother. Confirmed in SVFilter.hpp line 81, after `cutoffSmoother.process()`. | ✅ build |
| FM attenuverter scales zero to full, including inverted | `FM_ATTEN_PARAM` range -1 to +1, default 0. Bipolar scaling applied per-voice. | ✅ build |
| 1V/Oct tracks pitch — resonant peak follows played note | V/Oct accumulated in exponent with cutoff CV, single `pow(2, cutoffExponent)`. Correct log-domain math. Build-verified; pitch accuracy listening test deferred. | ✅ build |
| All three new CV inputs work polyphonically (up to 16 channels) | All three use `getPolyVoltage(c)` inside per-voice loop. Channel count derived from audio input. | ✅ build |
| Panel switch works when CV disconnected; CV overrides when connected | `filterTypeCVConnected` hoisted outside loop; per-voice branching between CV read and `params[FILTER_TYPE_PARAM]`. | ✅ build |
| Build zero errors and zero warnings on macOS | `make -j4` clean build — zero warnings, zero errors confirmed 2026-03-13. | ✅ verified |

**Note:** All criteria are build-verified. Runtime UAT (listening tests in VCV Rack for click-free switching, audible FM, pitch tracking accuracy) is deferred to human tester. Build verification confirms correct code structure, enum ordering, per-voice array usage, post-smoothing FM injection, and log-domain V/Oct math.

## Requirement Changes

All seven requirements covered by M002 remain **active** — they are build-verified but awaiting runtime UAT before they can transition to **validated**:

- CTRL-06: active → active — S01 implemented per-voice CV type switching. Awaiting runtime UAT.
- CTRL-08: active → active — S02 implemented FM with attenuverter, post-smoothing bypass. Awaiting runtime UAT.
- CTRL-09: active → active — S02 implemented 1V/Oct with log-domain sum. Awaiting runtime UAT.
- CTRL-10: active → active — S01 implemented CV-overrides-switch pattern. Awaiting runtime UAT.
- CTRL-11: active → active — S01 implemented Schmitt trigger with 200mV hysteresis. Awaiting runtime UAT.
- CTRL-12: active → active — S02 confirmed FM applied after cutoffSmoother.process(). Awaiting runtime UAT.
- CTRL-13: active → active — S02 confirmed V/Oct in exponent accumulator. Awaiting runtime UAT.

CTRL-07 (mode selection CV) remains **deferred** — architecture doesn't support mode selection since all 4 outputs are always active in 12dB mode.

## Forward Intelligence

### What the next milestone should know
- The module now has 7 CV inputs (audio, cutoff, resonance, drive, filter type, FM, V/Oct) plus 6 knobs and 1 switch on a 14 HP panel. Panel space is getting tight — any future inputs will need careful layout or a panel width increase.
- All enum entries must continue to be appended before sentinels. Current order is documented in both ParamId and InputId enums in CipherOB.cpp.
- The `setParams()` post-smoothing injection pattern (`fmOffsetVoct` default parameter) is clean and extensible — if future modulation sources need to bypass smoothing, add another default parameter.
- The per-voice crossfade state machine is fully refactored. Adding more CV-driven per-voice state follows the established pattern: `array[PORT_MAX_CHANNELS] = {}` member, `[c]` indexed in voice loop.

### What's fragile
- `fmOffsetVoct` post-smoothing placement in SVFilter.hpp — moving it before `cutoffSmoother.process()` kills audio-rate FM. The line order is load-bearing.
- Exponent accumulation — `cutoffCV * cvAmount + voct` must stay in the exponent, not be applied as a separate frequency multiplier, or cutoff CV / V/Oct interaction breaks.
- `prevFilterType[c]` update order — must happen *after* crossfade application. Moving it earlier introduces a one-sample glitch on transition start.
- 128-sample crossfade window is ~0.67ms at 192kHz — potentially audible at very high sample rates. No action needed but worth knowing.

### Authoritative diagnostics
- `make -j4 2>&1 | grep -E "warning|error"` — zero output = healthy build
- `grep -n 'prevFilterType\b\|crossfadeCounter\b' src/CipherOB.cpp` — all matches must be array declarations or `[c]` indexed
- `grep -n 'setParams.*fmOffsetVoct' src/CipherOB.cpp` — must show exactly 3 call sites
- `grep -n 'smoothedCutoff \*=' src/SVFilter.hpp` — must appear after `cutoffSmoother.process()` line

### What assumptions changed
- No major assumption changes. Both slices executed as planned without surprises.
- T02 of S02 (panel SVG) turned out to be verification-only since T01 included all panel changes — minor deviation but no impact.

## Files Created/Modified

- `src/CipherOB.cpp` — Per-voice crossfade arrays, Schmitt trigger, CV override logic, FM/V/Oct reading and wiring, new enum entries, widget code for new panel components
- `src/SVFilter.hpp` — `fmOffsetVoct` post-smoothing parameter in `setParams()`
- `res/CipherOB.svg` — Filter type CV jack, FM jack, FM attenuverter, V/Oct jack with dot indicators and labels
