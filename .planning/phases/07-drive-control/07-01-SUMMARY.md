---
phase: 07-drive-control
plan: 01
subsystem: audio-processing
tags: [saturation, waveshaping, cv-modulation, oberheim, drive-control]

# Dependency graph
requires:
  - phase: 06-resonance-control
    provides: "Resonance parameter with CV modulation and attenuverter infrastructure"
  - phase: 05-cutoff-control
    provides: "Cutoff parameter with CV modulation pattern"
  - phase: 02-filter-core
    provides: "SVFilter with state-variable topology and parameter smoothing"
provides:
  - "Drive/saturation control with blended tanh + asymmetric waveshaping"
  - "Drive CV input with attenuverter following cutoff/resonance pattern"
  - "Output-specific drive scaling (LP/BP full, HP 50%, Notch 70%)"
  - "True bypass at drive=0 for clean signal path"
affects: [testing, polishing, documentation]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Blended saturation algorithm combining symmetric (tanh) and asymmetric waveshaping"
    - "Output-specific signal processing with differential scaling"
    - "True bypass gate for zero-latency clean path"

key-files:
  created: []
  modified:
    - src/HydraQuartetVCF.cpp
    - src/SVFilter.hpp
    - res/HydraQuartetVCF.svg

key-decisions:
  - "Blended saturation with tanh + asymmetric shaping for even and odd harmonics"
  - "Drive knob positioned at Y=40mm mirroring cutoff/resonance layout"
  - "Output-specific scaling: LP/BP 100%, HP 50%, Notch 70%"
  - "Drive CV scaling 10%/V matching resonance pattern"
  - "True bypass at drive < 0.01 for clean signal path"

patterns-established:
  - "Output-specific processing: Different outputs can receive different processing amounts"
  - "Blended waveshaping: Combining multiple saturation algorithms for richer harmonics"

# Metrics
duration: 2min
completed: 2026-02-03
---

# Phase 7 Plan 1: Drive Control Summary

**Blended saturation with CV modulation adds Oberheim filter character — tanh + asymmetric waveshaping, output-specific scaling, true bypass at drive=0**

## Performance

- **Duration:** 2 min
- **Started:** 2026-02-03T03:11:02Z
- **Completed:** 2026-02-03T03:12:51Z
- **Tasks:** 3
- **Files modified:** 3

## Accomplishments
- Drive knob with CV input and attenuverter following cutoff/resonance pattern
- Blended saturation algorithm combining soft tanh and asymmetric shaping for even+odd harmonics
- Output-specific drive scaling: LP/BP get full effect, HP reduced (50%), Notch medium (70%)
- True bypass at drive < 0.01 for clean signal path
- Per-voice drive parameter smoothing prevents zipper noise

## Task Commits

Each task was committed atomically:

1. **Task 1: Add drive CV input, attenuverter, and panel updates** - `0327058` (feat)
2. **Task 2: Implement blended saturation algorithm** - `02d532a` (feat)
3. **Task 3: Wire drive processing into audio path with output-specific scaling** - `dd28412` (feat)

## Files Created/Modified
- `src/HydraQuartetVCF.cpp` - Added DRIVE_ATTEN_PARAM, DRIVE_CV_INPUT, drive parameter smoothing, CV modulation, output-specific saturation processing
- `src/SVFilter.hpp` - Added blendedSaturation function with tanh + asymmetric blending
- `res/HydraQuartetVCF.svg` - Added drive CV input and attenuverter placeholders at Y=50 and Y=62

## Decisions Made
- **Blended saturation algorithm:** Combines symmetric tanh (odd harmonics) with asymmetric shaping (even harmonics) for rich, musical saturation. Asymmetry blend increases with drive amount (up to 40% at max).
- **True bypass at drive < 0.01:** Early return in blendedSaturation ensures zero latency for clean signal path at minimum drive.
- **Output-specific scaling:** LP/BP outputs receive full drive effect (1.0x), HP reduced (0.5x), Notch medium (0.7x) to match acoustic behavior of frequency ranges.
- **Drive knob repositioned:** Moved from Y=68mm to Y=40mm to align with cutoff/resonance vertical spacing pattern.
- **CV scaling 10%/V:** Matches resonance CV scaling for consistent control behavior across parameters.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Drive control complete — v0.50b milestone achieved. Ready for:
- Testing and verification
- Polishing and refinement
- Documentation

All core filter functionality now implemented:
- ✓ Cutoff with CV modulation
- ✓ Resonance with CV modulation
- ✓ Drive with CV modulation
- ✓ Four filter outputs (LP/HP/BP/Notch)
- ✓ Polyphonic processing (16 voices)

---
*Phase: 07-drive-control*
*Completed: 2026-02-03*
