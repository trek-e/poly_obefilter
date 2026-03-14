# S02: FM Input and 1V/Oct Pitch Tracking

**Goal:** Audio-rate FM and 1V/Oct pitch tracking inputs are wired, functional, polyphonic, and build with zero warnings.
**Demo:** An audio-rate FM source patched to the FM input audibly modulates cutoff with adjustable depth via attenuverter; a 1V/Oct CV from a keyboard/sequencer makes the filter track pitch. Both work polyphonically alongside existing CV inputs.

## Must-Haves

- `FM_CV_INPUT` and `VOCT_INPUT` appended to `InputId` after `FILTER_TYPE_CV_INPUT` (before `INPUTS_LEN`)
- `FM_ATTEN_PARAM` appended to `ParamId` after `FILTER_TYPE_PARAM` (before `PARAMS_LEN`)
- FM attenuverter range -1 to +1 (bipolar — allows inverted FM)
- `SVFilter::setParams()` accepts `float fmOffsetVoct = 0.f` parameter, applied post-smoothing
- FM offset multiplies smoothed cutoff as `smoothedCutoff *= pow(2, fmOffsetVoct)` before frequency normalization
- 1V/Oct sums into cutoff exponent: `pow(2, cutoffCV * cvAmount + voct)` — logarithmic domain
- FM and 1V/Oct both applied to stage 2 `setParams()` in 24dB mode (cascade tracks identically)
- `isConnected()` checks hoisted outside per-voice loop for FM and V/Oct inputs
- FM variable named `fmAmount` (not `cvAmount`) to avoid shadowing
- Panel SVG updated: FM input jack, FM attenuverter knob, 1V/Oct input jack with dot indicators and labels
- INPUT section header widened to span full panel width for new jacks
- `make` produces zero errors and zero warnings

## Proof Level

- This slice proves: integration (DSP wiring + panel layout must agree)
- Real runtime required: yes (audio-rate FM and pitch tracking must be heard, not just compiled)
- Human/UAT required: yes (listening test for FM audibility and pitch tracking accuracy)

## Verification

- `make -j4 2>&1 | grep -E "warning|error"` — zero output
- `grep -c 'fmOffsetVoct' src/SVFilter.hpp` — at least 3 (parameter, application, declaration)
- `grep -c 'FM_CV_INPUT\|VOCT_INPUT\|FM_ATTEN_PARAM' src/CipherOB.cpp` — at least 6 (enum + config + widget + process loop references)
- `xmllint --noout res/CipherOB.svg` — exit 0
- SVG component layer has entries for `fm-cv-input`, `fm-atten`, `voct-input`
- Widget positions in C++ match SVG component layer coordinates
- Runtime UAT deferred to human tester (see S02-UAT.md after implementation)

## Integration Closure

- Upstream surfaces consumed: S01's per-voice crossfade arrays, `isConnected()` hoisting pattern, enum append convention, panel SVG jack pattern
- New wiring introduced in this slice: `SVFilter::setParams()` gains `fmOffsetVoct` parameter (default 0.f keeps all existing call sites unchanged); process loop reads 2 new inputs and 1 new param
- What remains before the milestone is truly usable end-to-end: human UAT for S01 + S02 in VCV Rack (listening tests), then milestone complete

## Tasks

- [x] **T01: Wire FM input, attenuverter, and 1V/Oct into DSP pipeline** `est:45m`
  - Why: Core DSP work — adds the three new enum entries, modifies `SVFilter::setParams()` to accept FM offset post-smoothing, and wires FM/V/Oct CV reading in the process loop. This is the functional heart of the slice.
  - Files: `src/SVFilter.hpp`, `src/CipherOB.cpp`
  - Do: (1) Add `fmOffsetVoct` parameter with default `0.f` to `SVFilter::setParams()`, apply it post-smoothing as `smoothedCutoff *= std::pow(2.f, fmOffsetVoct)` before normalization. (2) Append `FM_ATTEN_PARAM` to `ParamId`, `FM_CV_INPUT` and `VOCT_INPUT` to `InputId`. (3) Add `configParam`/`configInput` calls. (4) Add widget positions for FM input (~34mm, 90mm), FM attenuverter knob (~42mm, 84mm), V/Oct input (~58mm, 90mm). (5) Hoist `isConnected()` checks for both new inputs outside per-voice loop. (6) In per-voice loop: read V/Oct and sum into cutoff exponent; read FM CV, scale by attenuverter, pass as `fmOffsetVoct` to all `setParams()` calls (both stages in 24dB mode). (7) Skip FM offset when FM input disconnected (avoid wasteful `pow(2,0)`).
  - Verify: `make -j4 2>&1 | grep -E "warning|error"` produces zero output; `fmOffsetVoct` appears in `SVFilter.hpp` setParams signature and body
  - Done when: Zero-warning build with FM bypassing smoother and V/Oct summing in log domain

- [x] **T02: Add FM, FM attenuverter, and V/Oct visual elements to panel SVG** `est:30m`
  - Why: Panel must show the new components — dot indicators, pixel-art labels, and component layer entries so VCV Rack renders jacks/knobs at the right positions.
  - Files: `res/CipherOB.svg`
  - Do: (1) Widen INPUT section header from `width="30"` to `width="65.12"` to span full panel. (2) Add dot indicators for FM input, FM attenuverter, and V/Oct input at coordinates matching T01's widget positions. (3) Add pixel-art labels "FM" and "V/O" in the established rectangle-stroke style. (4) Add component layer entries: `fm-cv-input`, `fm-atten`, `voct-input` with correct cx/cy/r matching widget code. (5) Verify coordinates align with T01's `mm2px(Vec(...))` calls.
  - Verify: `xmllint --noout res/CipherOB.svg` exits 0; component IDs `fm-cv-input`, `fm-atten`, `voct-input` all present; coordinates match widget code
  - Done when: Valid SVG with all three new components visible, positioned to match widget code

## Observability / Diagnostics

- **FM signal path verification:** Patch an audible oscillator into FM input. Sweep attenuverter from -1 to +1 — filter cutoff modulation should be clearly audible and invert direction at negative values. Zero attenuverter = no modulation (silent path).
- **V/Oct tracking verification:** Patch a sequencer or keyboard V/Oct output. Filter should track pitch — each octave up doubles cutoff frequency. Compare against a VCO tracking the same V/Oct to hear unison.
- **Polyphonic behavior:** Both FM and V/Oct read `getPolyVoltage(c)` — each voice gets its own modulation. Verify with a polyphonic source that voices don't bleed.
- **Disconnected input fast path:** When FM_CV_INPUT is disconnected, `fmOffsetVoct` is not computed and `pow(2, 0)` multiplication is skipped entirely (hoisted `isConnected()` check). No CPU waste on unused inputs.
- **Build-time signals:** Zero warnings from `make -j4` with implicit `-Wall -Wextra` from the VCV Rack SDK Makefile. Any warning is a regression.
- **Failure modes:** If FM input drives cutoff out of [20Hz, 20kHz] range, the existing `rack::clamp` in `SVFilter::setParams()` (line 82 normalization to 0.001–0.49) prevents instability. No additional clamping needed — the SVFilter's NaN/infinity reset handles catastrophic cases.

## Files Likely Touched

- `src/SVFilter.hpp`
- `src/CipherOB.cpp`
- `res/CipherOB.svg`
