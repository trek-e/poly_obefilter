---
phase: 09-character-output
plan: 01
subsystem: audio-dsp
tags: [vcvrack, svfilter, oberheim, ob-x, cascade-filter, saturation, resonance]

# Dependency graph
requires:
  - phase: 08-core-24db-cascade
    provides: Cascaded SVF 24dB topology, crossfade mode-switching, panel switch, blendedSaturation

provides:
  - resonance24 Q boost (1.15x Stage 1, 0.65x Stage 2) for OB-X aggressive peaking character
  - Inter-stage tanh saturation (0.12f factor) for CEM3320 diode-pair harmonic character
  - LP-only output routing in 24dB mode (outHP = outBP = outNotch = 0.0f)
  - Click-free 12dB<->24dB switching for all four outputs via existing 128-sample crossfade

affects: [v0.60b milestone, 10-milestone-completion]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "OB-X resonance boost: clamp(resonance * 1.15f, 0, 0.95) for Stage 1, boosted * 0.65 for Stage 2"
    - "Inter-stage saturation: tanh(x * 0.12) / 0.12 — near-unity for small signals, progressive odd harmonics"
    - "LP-only 24dB routing: zero-assign outHP/BP/Notch, crossfade handles artifact-free transitions"

key-files:
  created: []
  modified:
    - src/HydraQuartetVCF.cpp

key-decisions:
  - "resonance24 = clamp(resonance * 1.15, 0, 0.95) — 15% Q boost capped below self-oscillation threshold"
  - "Stage 2 Q = resonance24 * 0.65 (was resonance * 0.7) — maintains stability with boosted Stage 1"
  - "Inter-stage tanh factor 0.12f — mild enough to be nearly linear at typical levels, adds character at high resonance/drive"
  - "outHP = outBP = outNotch = 0.0f in 24dB mode — crossfade fade-to-zero and fade-from-zero handles click-free transitions symmetrically"

patterns-established:
  - "OB-X character pattern: resonance boost + inter-stage saturation + LP-only output = authentic OB-Xa cascade architecture"

requirements-completed: [FILT-08, FILT-09, TYPE-03]

# Metrics
duration: 10min
completed: 2026-02-21
---

# Phase 9 Plan 01: Character Output Summary

**OB-X 24dB mode tuned with 1.15x resonance boost, inter-stage tanh saturation, and authentic LP-only output routing with click-free crossfade for all four outputs**

## Performance

- **Duration:** ~10 min
- **Started:** 2026-02-21T17:34:11Z
- **Completed:** 2026-02-21 (Task 1 complete; Task 2 awaiting human verification)
- **Tasks:** 1 of 2 (Task 2 is human-verify checkpoint)
- **Files modified:** 1

## Accomplishments
- Computed `resonance24 = rack::clamp(resonance * 1.15f, 0.f, 0.95f)` applied to Stage 1 setParams, giving sharper OB-X resonance peaks
- Stage 2 Q lowered to `resonance24 * 0.65f` (from `resonance * 0.7f`) maintaining stability while cascading the boosted character
- Inter-stage signal path uses `tanh(x * 0.12f) / 0.12f` — normalized tanh that is unity-gain for small signals, adds progressive odd harmonics at high levels (authentic CEM3320 diode-pair character)
- `outHP = 0.0f`, `outBP = 0.0f`, `outNotch = 0.0f` in 24dB branch — LP-only authentic OB-Xa routing
- Existing 128-sample crossfade handles both fade-to-zero (12dB->24dB) and fade-from-zero (24dB->12dB) automatically — no additional code needed
- 12dB SEM branch verified bitwise unchanged from Phase 8
- Build: zero errors, zero warnings

## Task Commits

Each task was committed atomically:

1. **Task 1: OB-X character tuning and LP-only output routing** - `69926c5` (feat)
2. **Task 2: Verify OB-X character in VCV Rack** - AWAITING HUMAN VERIFICATION

**Plan metadata:** (pending final commit)

## Files Created/Modified
- `/Users/trekkie/projects/vcvrack_modules/poly_obefilter/src/HydraQuartetVCF.cpp` - OB-X character tuning (resonance24 boost, inter-stage tanh, LP-only output routing)

## Decisions Made
- `resonance24` factor of 1.15x provides noticeable OB-X edge without pushing Stage 1 past self-oscillation at max knob setting (capped at 0.95)
- Stage 2 Q moved from `resonance * 0.7f` to `resonance24 * 0.65f` — uses the boosted base so the cascade character carries through both stages
- Inter-stage tanh factor 0.12f selected for near-linear behavior at typical signal levels while adding progressive harmonic content at high resonance/drive combinations
- Crossfade fade-to-zero / fade-from-zero approach relies entirely on Phase 8 crossfade infrastructure — no code changes needed in crossfade logic

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None — build was clean on first attempt. All three code changes applied and verified.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- After human verification approves, Phase 9 Plan 01 is complete and v0.60b milestone is reached
- REQUIREMENTS.md items FILT-08, FILT-09, TYPE-03 fulfilled
- If user requests adjustments (more edge, less resonance, click fix), return with specific feedback for targeted tuning

---
*Phase: 09-character-output*
*Completed: 2026-02-21*
