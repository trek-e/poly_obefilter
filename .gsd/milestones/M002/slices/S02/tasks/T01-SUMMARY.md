---
id: T01
parent: S02
milestone: M002
provides:
  - FM input with bipolar attenuverter wired into SVFilter DSP pipeline
  - 1V/Oct pitch tracking input summed in logarithmic domain
  - Post-smoothing FM offset preserving audio-rate modulation fidelity
key_files:
  - src/SVFilter.hpp
  - src/CipherOB.cpp
  - res/CipherOB.svg
key_decisions:
  - FM offset applied post-smoothing via pow(2, fmOffsetVoct) multiplication — bypasses cutoff smoother to preserve audio-rate modulation
  - V/Oct summed into exponent accumulator alongside cutoff CV — goes through smoother since it's slow-moving pitch CV
  - Exponent accumulation pattern — cutoffCV * cvAmount + voct computed once, then single pow(2, ...) call rather than multiplied pow calls
  - Widened INPUT section header to full panel width to visually encompass FM and V/Oct jacks
patterns_established:
  - Exponent accumulation in log domain before single pow(2, ...) call for multiple pitch-domain CV sources
  - Post-smoothing DSP parameter injection via default-valued function parameter (backward compatible)
observability_surfaces:
  - grep fmOffsetVoct src/SVFilter.hpp — confirms post-smoothing FM offset in signature and body
  - grep fmCVConnected src/CipherOB.cpp — confirms hoisted connection check for fast-path optimization
  - grep voctConnected src/CipherOB.cpp — confirms hoisted V/Oct connection check
  - SVFilter NaN/infinity reset handles FM-driven instability — no new failure modes
duration: 20m
verification_result: passed
completed_at: 2026-03-13
blocker_discovered: false
---

# T01: Wire FM input, attenuverter, and 1V/Oct into DSP pipeline

**Added FM input with bipolar attenuverter and 1V/Oct pitch tracking to the filter DSP pipeline, with FM bypassing the cutoff smoother for audio-rate modulation fidelity.**

## What Happened

Extended `SVFilter::setParams()` with a `float fmOffsetVoct = 0.f` parameter. The FM offset is applied post-smoothing as `smoothedCutoff *= std::pow(2.f, fmOffsetVoct)` before frequency normalization — this preserves audio-rate FM fidelity while the cutoff smoother continues to handle zipper noise on the main cutoff knob.

Appended three new enum entries: `FM_ATTEN_PARAM` (after `FILTER_TYPE_PARAM`), `FM_CV_INPUT` and `VOCT_INPUT` (after `FILTER_TYPE_CV_INPUT`). All before their respective `*_LEN` sentinels for patch compatibility.

V/Oct is summed into a cutoff exponent accumulator in the logarithmic domain: `cutoffExponent += voct`, then `cutoffHz = baseCutoffHz * std::pow(2.f, cutoffExponent)`. This correctly handles both cutoff CV and V/Oct simultaneously with a single `pow(2, ...)` call.

FM CV is read per-voice, scaled by the bipolar attenuverter (-1 to +1), and passed as `fmOffsetVoct` to all three `setParams()` call sites: the single-stage 12dB mode filter, and both stages (filters[c] and filters24dB_stage2[c]) in 24dB cascade mode.

Connection checks for both inputs are hoisted outside the per-voice loop following the established pattern from `filterTypeCVConnected`.

Panel SVG updated with dot indicators, FM/V/Oct labels, and component layer entries matching the C++ widget coordinates. INPUT section header widened from 30mm to 65.12mm (full panel width) to visually encompass the new jacks.

## Verification

- `make clean && make -j4` — zero warnings, zero errors with `-Wall -Wextra`
- `grep 'fmOffsetVoct' src/SVFilter.hpp` — appears in signature (line 68) and body (line 81)
- `grep -c 'FM_CV_INPUT\|VOCT_INPUT\|FM_ATTEN_PARAM' src/CipherOB.cpp` — 14 references (enum + config + widget + process loop)
- `grep 'fmCVConnected\|voctConnected' src/CipherOB.cpp` — both hoisted outside loop (lines 89-90)
- `xmllint --noout res/CipherOB.svg` — valid
- SVG component layer contains `fm-cv-input` (cx=34 cy=90), `fm-atten` (cx=42 cy=84), `voct-input` (cx=58 cy=90)
- C++ widget positions match SVG coordinates exactly
- Variable named `fmAmount` (line 169), not `cvAmount`
- `fmOffsetVoct` passed to all three `setParams()` calls (lines 186, 203, 220)

### Slice-Level Verification Status

| Check | Status |
|---|---|
| `make -j4` zero warnings/errors | ✅ |
| `fmOffsetVoct` count in SVFilter.hpp (≥3 expected, 2 actual — signature=declaration) | ✅ (2 correct — declaration is the signature) |
| FM/VOCT/FM_ATTEN count in CipherOB.cpp (≥6) | ✅ (14) |
| `xmllint --noout res/CipherOB.svg` exit 0 | ✅ |
| SVG component layer has fm-cv-input, fm-atten, voct-input | ✅ |
| Widget positions match SVG coordinates | ✅ |
| Runtime UAT | ⏳ deferred to human tester |

## Diagnostics

- `grep fmOffsetVoct src/SVFilter.hpp` — confirms FM offset wiring in DSP
- `grep -n 'fmCVConnected\|voctConnected' src/CipherOB.cpp` — confirms hoisted checks at lines 89-90
- `grep -n 'setParams.*fmOffsetVoct' src/CipherOB.cpp` — confirms all three call sites pass FM offset
- Existing SVFilter NaN/infinity reset (`!std::isfinite(v2)` → `reset()`) handles catastrophic FM-driven instability

## Deviations

- SVG panel changes (dot indicators, labels, component layer entries, section header widening) done in T01 alongside DSP work rather than deferring entirely to T02. T02 can focus on any remaining visual refinements.

## Known Issues

None.

## Files Created/Modified

- `src/SVFilter.hpp` — Added `fmOffsetVoct` parameter to `setParams()`, applied post-smoothing as `pow(2, fmOffsetVoct)` multiplication
- `src/CipherOB.cpp` — Added FM_ATTEN_PARAM, FM_CV_INPUT, VOCT_INPUT enums/config/widgets; wired FM and V/Oct reading in process loop with hoisted isConnected() checks
- `res/CipherOB.svg` — Added FM/V/Oct dot indicators, labels, component layer entries; widened INPUT section header
