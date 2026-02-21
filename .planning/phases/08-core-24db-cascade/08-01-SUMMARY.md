---
phase: 08-core-24db-cascade
plan: 01
subsystem: dsp
tags: [vcvrack, svfilter, cascade, 24db, oberheim, polyphonic, cpp]

# Dependency graph
requires:
  - phase: 07-drive-control
    provides: blendedSaturation function and drive parameter infrastructure used in both filter modes
provides:
  - FILTER_TYPE_PARAM configSwitch (0=12dB SEM, 1=24dB OB-X) in ParamId enum
  - filters24dB_stage2 SVFilter array for 16-voice polyphonic cascade
  - 128-sample crossfade state machine for click-free mode switching
  - CKSS toggle widget at (55mm, 28mm) on panel
  - Filter type switch placeholder and 12dB/24dB labels in panel SVG
affects:
  - 09-output-routing (finalizes HP/BP/Notch behavior in 24dB mode)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - Cascaded SVF topology: stage1.lowpass feeds stage2 input for 24dB/oct rolloff
    - Split resonance: 0.7x Q on stage2 for stability while stage1 runs full Q
    - Inter-stage safety: isfinite check with reset and rack::clamp(+-12V) between stages
    - Crossfade state machine: 128-sample linear crossfade on mode change, per-voice from-values captured in xfLP/HP/BP/Notch arrays

key-files:
  created: []
  modified:
    - src/HydraQuartetVCF.cpp
    - res/HydraQuartetVCF.svg

key-decisions:
  - "Stage2 Q at 0.7x resonance (not 0.5x or equal) balances stability against resonance character"
  - "24dB LP drive multiplier 1.3x gives OB-X edge without requiring separate drive knob"
  - "CKSS switch at (55mm, 28mm) - right column above drive knob, no panel element overlap"
  - "HP/BP/Notch in 24dB mode use stage1 outputs as interim; Phase 9 will finalize routing"
  - "Crossfade-from values captured from output port getVoltage before overwriting, preserving last sample"

patterns-established:
  - "Dual-mode filter: filterType branch in process() keeps 12dB path bitwise identical to v0.50b"
  - "Inter-stage NaN safety pattern: isfinite check -> reset both stages -> clamp inter-stage"

requirements-completed: [FILT-06, FILT-07, TYPE-01, TYPE-02]

# Metrics
duration: 2min
completed: 2026-02-21
---

# Phase 8 Plan 01: Core 24dB Cascade Summary

**Cascaded SVF 24dB OB-X filter mode with per-voice polyphony, split resonance stability, 1.3x drive, and 128-sample click-free crossfade on mode switch**

## Performance

- **Duration:** ~2 min
- **Started:** 2026-02-21T17:00:22Z
- **Completed:** 2026-02-21T17:02:12Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- Cascaded dual-stage SVF producing 24dB/oct lowpass rolloff in OB-X mode, feeding stage1.lowpass into stage2 input
- FILTER_TYPE_PARAM configSwitch with CKSS widget provides 12dB/24dB mode selection on panel
- Click-free mode switching via 128-sample linear crossfade state machine with per-voice from-value capture
- Panel SVG updated with switch placeholder rectangle and 12dB/24dB positional labels

## Task Commits

Each task was committed atomically:

1. **Task 1: Add FILTER_TYPE_PARAM, second-stage array, and cascade logic with crossfade** - `3b90f7f` (feat)
2. **Task 2: Add filter type switch placeholder to panel SVG** - `2235cf9` (feat)

## Files Created/Modified

- `/Users/trekkie/projects/vcvrack_modules/poly_obefilter/src/HydraQuartetVCF.cpp` - Added FILTER_TYPE_PARAM enum entry and configSwitch, filters24dB_stage2 array, crossfade state (prevFilterType, crossfadeCounter, CROSSFADE_SAMPLES=128, xfLP/HP/BP/Notch arrays), dual-branch process() with cascade logic, inter-stage NaN safety, CKSS widget registration
- `/Users/trekkie/projects/vcvrack_modules/poly_obefilter/res/HydraQuartetVCF.svg` - Added filter-type-switch-placeholder rect in components layer, label-12db and label-24db paths in visible layer

## Decisions Made

- Stage2 Q at 0.7x resonance: stage1 runs full Q for warm self-oscillation character, stage2 at 0.7x for cascade stability without killing the resonance peak
- 24dB LP drive at 1.3x: same knob range produces OB-X edge without needing a separate control
- CKSS at (55mm, 28mm): right column between title divider (y=22mm) and drive knob (y=40mm), clear of all other components
- Interim HP/BP/Notch use stage1 outputs in 24dB mode: correct and musical, Phase 9 finalizes routing per LP-only spec

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None. Build completed with zero errors and zero warnings on first attempt.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- 24dB OB-X filter mode fully functional and playable in VCV Rack
- FILTER_TYPE_PARAM accessible as configSwitch; both modes compile and link
- Inter-stage NaN safety and clamping confirmed in code
- Phase 9 (output routing) can finalize HP/BP/Notch behavior in 24dB mode, silencing non-LP outputs per LP-only OB-Xa spec

---
*Phase: 08-core-24db-cascade*
*Completed: 2026-02-21*

## Self-Check: PASSED

- FOUND: src/HydraQuartetVCF.cpp
- FOUND: res/HydraQuartetVCF.svg
- FOUND: .planning/phases/08-core-24db-cascade/08-01-SUMMARY.md
- FOUND: plugin.dylib
- FOUND commit 3b90f7f (Task 1 - cascade filter implementation)
- FOUND commit 2235cf9 (Task 2 - SVG panel update)
