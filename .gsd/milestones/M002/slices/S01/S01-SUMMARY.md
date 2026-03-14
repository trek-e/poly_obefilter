---
id: S01
parent: M002
milestone: M002
provides:
  - Per-voice crossfade arrays (crossfadeCounter[PORT_MAX_CHANNELS], prevFilterType[PORT_MAX_CHANNELS]) replacing global scalars
  - Per-voice Schmitt trigger state (filterTypeHigh[PORT_MAX_CHANNELS]) with 2.6V/2.4V hysteresis
  - FILTER_TYPE_CV_INPUT enum entry appended to InputId (patch-compatible)
  - CV-overrides-switch pattern — isConnected() checked once per sample block, per-voice branching
  - Panel SVG with filter type CV jack at (24, 32), 10mm below CKSS switch
requires: []
affects:
  - S02
key_files:
  - src/CipherOB.cpp
  - res/CipherOB.svg
key_decisions:
  - Per-voice crossfade arrays replace global crossfadeCounter/prevFilterType — required for polyphonic CV-driven type switching
  - filterTypeCVConnected check hoisted outside per-voice loop — one isConnected() call per sample block, not per voice
  - prevFilterType[c] updated after crossfade application to avoid one-sample glitch on transition start
  - "CV" label (not "TYPE CV") matches terse style of existing cutoff/drive/resonance CV labels
patterns_established:
  - Per-voice state arrays for CV-driven boolean state: declare array[PORT_MAX_CHANNELS], zero-init, index by [c] inside voice loop
  - CV-overrides-switch pattern: check isConnected() once, branch per-voice between CV read and param read
  - Panel SVG jack: dot indicator circle (r=0.5, section color, opacity 0.4) + component layer circle (fill:#00ff00, r=3.5, id attribute)
observability_surfaces:
  - "grep -n 'prevFilterType\b\|crossfadeCounter\b' src/CipherOB.cpp" — all matches must be array declarations or [c] indexed. Bare scalar = regression.
  - "xmllint --noout res/CipherOB.svg" — exit 0 = valid SVG
  - Filter Type CV port tooltip shows "Filter Type CV" in VCV Rack
drill_down_paths:
  - .gsd/milestones/M002/slices/S01/tasks/T01-SUMMARY.md
  - .gsd/milestones/M002/slices/S01/tasks/T02-SUMMARY.md
duration: 30m
verification_result: passed
completed_at: 2026-03-13
---

# S01: Filter Type CV with Per-Voice Crossfade

**Per-voice crossfade state machine and Schmitt trigger CV input for polyphonic filter type switching, with panel jack and zero-warning build.**

## What Happened

Refactored the global crossfade state machine to per-voice arrays and added a new filter type CV input with Schmitt trigger hysteresis.

**T01** replaced three scalar member variables with per-voice arrays: `prevFilterType[PORT_MAX_CHANNELS]`, `crossfadeCounter[PORT_MAX_CHANNELS]`, and new `filterTypeHigh[PORT_MAX_CHANNELS]` for Schmitt state. The `process()` inner loop now determines filter type per-voice — CV connected reads per-channel voltage with 2.6V rising / 2.4V falling thresholds; CV disconnected falls back to panel switch. Mode change detection, crossfade-from capture, and crossfade counter decrement all operate per-voice. `FILTER_TYPE_CV_INPUT` was appended to `InputId` (before `INPUTS_LEN`, patch-compatible).

**T02** added the panel SVG elements at (24, 32) — 10mm below the CKSS switch: a dot indicator circle matching the filter section's orange color, a "CV" label in the existing pixel-art style, and a component layer entry with `id="filter-type-cv-input"`. Coordinates match the widget code's `mm2px(Vec(24.0, 32.0))` exactly.

## Verification

- `make -j4 2>&1 | grep -E "warning|error"` — zero output ✅
- `grep -n "prevFilterType\b\|crossfadeCounter\b" src/CipherOB.cpp` — all 8 matches are array declarations or `[c]`-indexed accesses, zero bare scalars ✅
- `xmllint --noout res/CipherOB.svg` — exit 0, valid XML ✅
- SVG coordinates (24, 32) match widget code `Vec(24.0, 32.0)` ✅
- No component overlap: nearest elements are CKSS switch at (24, 22) [10mm] and cutoff knob at (20, 43) [~12mm] ✅
- Runtime UAT (VCV Rack listening tests) deferred to human tester — see S01-UAT.md

## Requirements Advanced

- CTRL-06 — Filter type CV input implemented with polyphonic per-voice switching. Build-verified; awaiting runtime UAT.
- CTRL-10 — CV overrides panel switch when connected; switch works normally when disconnected. Logic implemented and build-verified.
- CTRL-11 — Schmitt trigger with 2.6V rising / 2.4V falling thresholds implemented per-voice. Build-verified.

## Requirements Validated

- none — CTRL-06, CTRL-10, CTRL-11 require runtime UAT for full validation

## New Requirements Surfaced

- none

## Requirements Invalidated or Re-scoped

- none

## Deviations

None.

## Known Limitations

- Runtime behavior (click-free crossfade, correct type switching, polyphonic independence) is build-verified but not yet UAT-confirmed in VCV Rack.
- Schmitt trigger hysteresis is 200mV (2.4V–2.6V). If real-world CV sources produce slow ramps through the band, users may perceive latency. This is the standard tradeoff and matches typical module behavior.

## Follow-ups

- S02 (FM + 1V/Oct) appends its enum entries after `FILTER_TYPE_CV_INPUT` — no changes needed to S01 code.
- Human UAT in VCV Rack should cover the test cases in S01-UAT.md before marking CTRL-06/10/11 as validated.

## Files Created/Modified

- `src/CipherOB.cpp` — per-voice crossfade arrays, Schmitt trigger, CV input enum/config/widget, CV override logic
- `res/CipherOB.svg` — filter type CV jack dot indicator, "CV" label, component layer entry

## Forward Intelligence

### What the next slice should know
- Per-voice arrays pattern is established: `array[PORT_MAX_CHANNELS] = {}` declared as member, `[c]` indexed inside the voice loop. S02's FM and 1V/Oct don't need per-voice boolean state (they're continuous), but if any future CV needs Schmitt-style switching, follow this pattern.
- `FILTER_TYPE_CV_INPUT` is the last entry before `INPUTS_LEN`. S02 must append `FM_CV_INPUT` and `VOCT_INPUT` after it.
- `filterTypeCVConnected` is checked once outside the loop — S02 should follow the same hoisting pattern for its `isConnected()` checks.

### What's fragile
- The crossfade-from capture order matters: `prevFilterType[c]` must be updated *after* crossfade application, not before. Moving the update earlier would introduce a one-sample glitch where the crossfade reads the wrong "from" values.
- The 128-sample crossfade window (~2.7ms at 48kHz) is tight. If sample rate is very low (e.g., 22050Hz), the crossfade is ~5.8ms — still fine. At very high rates (192kHz), it's ~0.67ms, which may be audible. No action needed now, but worth knowing.

### Authoritative diagnostics
- `grep -n "prevFilterType\b\|crossfadeCounter\b" src/CipherOB.cpp` — every match must be an array or `[c]` access. Any bare scalar is a regression.
- `make -j4 2>&1 | grep -E "warning|error"` — must produce zero output.

### What assumptions changed
- No assumptions changed — the per-voice refactoring went as planned with no surprises.
