---
phase: 04-polyphonic-extension
plan: 01
subsystem: dsp
tags: [polyphony, vcvrack, filter, svf]

# Dependency graph
requires:
  - phase: 03-filter-modes
    provides: SVFilter with LP/HP/BP/Notch outputs and stability protection
provides:
  - Polyphonic filter processing (1-16 voices)
  - Per-voice CV modulation for cutoff and resonance
  - Independent filter state per voice
affects: [05-drive-saturation, 06-panel-graphics]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "filters[PORT_MAX_CHANNELS] array for polyphonic state"
    - "getPolyVoltage(c) for per-voice CV reading"
    - "setVoltage(value, c) + setChannels(n) for polyphonic output"

key-files:
  created: []
  modified:
    - src/HydraQuartetVCF.cpp

key-decisions:
  - "Tasks 1+2 committed together (filter array and process loop are interdependent)"
  - "Resonance CV scaled 0.1 (1V = 10% resonance change) to prevent wild swings"
  - "Channel count derived from audio input only (CV wraps via getPolyVoltage)"

patterns-established:
  - "Polyphonic processing loop: for (int c = 0; c < channels; c++)"
  - "Per-voice filter instances: filters[c].process(input)"
  - "Output channel propagation: setChannels(channels) after processing loop"

# Metrics
duration: 2min
completed: 2026-01-31
---

# Phase 4 Plan 1: Polyphonic Extension Summary

**16-voice polyphonic filter with per-voice CV modulation for cutoff and resonance, independent filter state per voice**

## Performance

- **Duration:** 2 min
- **Started:** 2026-01-31T17:53:11Z
- **Completed:** 2026-01-31T17:54:46Z
- **Tasks:** 3
- **Files modified:** 1

## Accomplishments
- Replaced single SVFilter with filters[16] array for polyphonic support
- Implemented per-voice processing loop with independent filter state
- Added polyphonic CV modulation for cutoff and resonance inputs
- All four outputs (LP, HP, BP, Notch) are polyphonic with matching channel count

## Task Commits

Each task was committed atomically:

1. **Task 1+2: Filter array + polyphonic process loop** - `8316e4d` (feat)
2. **Task 3: Verify polyphonic behavior** - `eeac726` (docs)

Note: Tasks 1 and 2 were committed together because the filter array replacement (Task 1) requires the process loop update (Task 2) to compile.

## Files Created/Modified
- `src/HydraQuartetVCF.cpp` - Polyphonic filter processing with filters[16] array and per-voice CV modulation

## Decisions Made
- **Tasks 1+2 atomic commit:** The filter array replacement makes the code non-compilable until the process loop is updated. Committed together as a single atomic change.
- **Resonance CV scaling:** 0.1 factor means 1V = 10% resonance change, preventing extreme parameter jumps from polyphonic CV.
- **Channel count source:** Derived from audio input only; CV inputs use getPolyVoltage() which automatically wraps mono CV to all voices.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Tasks 1+2 committed together**
- **Found during:** Task 1 (Add Filter Array)
- **Issue:** Task 1's verification "make -j4 succeeds" cannot pass because the process() method still references `filter.` instead of `filters[c].`
- **Fix:** Completed both tasks before first commit since they form an atomic compilable unit
- **Files modified:** src/HydraQuartetVCF.cpp
- **Verification:** Build succeeds after both tasks complete
- **Committed in:** 8316e4d (combined Task 1+2 commit)

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** Minor restructuring of commits. No scope creep.

## Issues Encountered
None - implementation matched plan specification exactly.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Polyphonic filter processing complete and verified
- Ready for Phase 5 (Drive/Saturation) - can add per-voice drive processing
- Ready for Phase 6 (Panel Graphics) - polyphonic indicators if desired

---
*Phase: 04-polyphonic-extension*
*Completed: 2026-01-31*
