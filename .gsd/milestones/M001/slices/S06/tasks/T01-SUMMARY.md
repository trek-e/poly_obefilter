---
id: T01
parent: S06
milestone: M001
provides:
  - Resonance attenuverter parameter with bipolar range (-1 to +1)
  - CV modulation depth control for resonance (0.1 scaling, 10% per volt)
  - Panel widget at Y=98mm position
requires: []
affects: []
key_files: []
key_decisions: []
patterns_established: []
observability_surfaces: []
drill_down_paths: []
duration: 2min
verification_result: passed
completed_at: 2026-02-03
blocker_discovered: false
---
# T01: 06-resonance-control 01

**# Phase 06 Plan 01: Resonance Attenuverter Summary**

## What Happened

# Phase 06 Plan 01: Resonance Attenuverter Summary

**Resonance CV attenuverter with bipolar control (-1 to +1), 10% per volt scaling, matching cutoff attenuverter pattern**

## Performance

- **Duration:** 2 min
- **Started:** 2026-02-03T07:42:14Z
- **Completed:** 2026-02-03T07:44:20Z
- **Tasks:** 3
- **Files modified:** 2

## Accomplishments
- Added RESONANCE_ATTEN_PARAM with bipolar range allowing CV inversion
- Implemented CV processing using attenuverter: resCV * resCvAmount * 0.1f
- Added panel widget at Y=98mm with SVG component marker
- Verified clean compilation and installation

## Task Commits

Each task was committed atomically:

1. **Task 1: Add resonance attenuverter parameter and CV processing** - `74b1040` (feat)
2. **Task 2: Update panel SVG with resonance attenuverter marker** - `013f400` (docs)
3. **Task 3: Build and functional verification** - (no commit - verification only)

## Files Created/Modified
- `src/CipherOB.cpp` - Added RESONANCE_ATTEN_PARAM enum, configParam, CV processing with attenuverter, widget
- `res/CipherOB.svg` - Added resonance-atten-knob component marker at (35.56, 98)

## Decisions Made

**Variable naming:** Used `resCvAmount` instead of reusing `cvAmount` to avoid shadowing the cutoff CV variable in the same scope. This prevents bugs and improves code clarity.

**Default value 0:** Following the cutoff attenuverter pattern, default is 0 (center) so CV has no effect until user turns the knob. This is an "opt-in" design - prevents unexpected modulation.

**Position Y=98mm:** Placed 10mm below resonance knob (Y=88mm) and 9mm above outputs (Y=107mm). Maintains visual balance and component spacing consistency.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None - all tasks completed successfully without problems.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Resonance control is now complete with both direct control (knob) and CV modulation (attenuverter). Ready to proceed to Phase 7 (Drive Saturation) which will add the final tone-shaping parameter.

**Pattern established:** The attenuverter pattern is now proven across both cutoff and resonance parameters. Phase 7 should follow the same pattern if drive needs CV control.

**Technical note:** The 0.1 scaling factor (10% of parameter range per volt) provides musical control without wild parameter swings. This scaling has worked well for both cutoff and resonance and should be the default for future CV inputs.

---
*Phase: 06-resonance-control*
*Completed: 2026-02-03*
