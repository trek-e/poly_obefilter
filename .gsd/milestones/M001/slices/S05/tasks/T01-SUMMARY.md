---
id: T01
parent: S05
milestone: M001
provides: []
requires: []
affects: []
key_files: []
key_decisions: []
patterns_established: []
observability_surfaces: []
drill_down_paths: []
duration: 2min
verification_result: passed
completed_at: 2026-01-31
blocker_discovered: false
---
# T01: 05-cutoff-control 01

**# Plan 05-01: Cutoff Default Refinement - Summary**

## What Happened

# Plan 05-01: Cutoff Default Refinement - Summary

## What Was Built

Updated the cutoff frequency parameter default from 0.5 (mid-range 632 Hz) to 1.0 (fully open 20kHz) so filters start fully open when module loads. This ensures new patches produce immediate audio output without requiring the user to adjust the cutoff knob.

## Tasks Completed

| # | Task | Status | Commit |
|---|------|--------|--------|
| 1 | Update cutoff parameter default to fully open | Done | 78edff4 |
| 2 | Verify cutoff control behavior | Done | (verification only) |

## Deliverables

| Artifact | Purpose |
|----------|---------|
| `src/HydraQuartetVCF.cpp` | Cutoff configParam with default 1.f |

## Technical Notes

**Change made:**
```cpp
// Before
configParam(CUTOFF_PARAM, 0.f, 1.f, 0.5f, "Cutoff");

// After
configParam(CUTOFF_PARAM, 0.f, 1.f, 1.f, "Cutoff");
```

**Verified existing behavior (no changes needed):**
- Logarithmic frequency mapping: `20.f * std::pow(1000.f, cutoffParam)` correctly maps 0-1 to 20Hz-20kHz
- V/Oct CV scaling: `std::pow(2.f, cutoffCV * cvAmount)` correctly doubles frequency per volt
- Bipolar attenuverter: -1 to +1 range, defaults to 0 (no CV effect until user adjusts)
- Frequency clamping: 20Hz to 20kHz prevents out-of-range values
- Parameter smoothing: 1ms tau in SVFilter.hpp prevents zipper noise

## Deviations

None. The research correctly identified that only the default value needed changing.

---

*Completed: 2026-01-31*
