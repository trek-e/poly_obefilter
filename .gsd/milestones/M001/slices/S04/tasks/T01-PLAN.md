# T01: 04-polyphonic-extension 01

**Slice:** S04 — **Milestone:** M001

## Description

Transform the monophonic HydraQuartetVCF into a polyphonic processor supporting up to 16 voices.

Purpose: Enable polyphonic patches where each voice in a chord/sequence gets independent filter processing
Output: Working polyphonic filter module with per-voice CV modulation

## Must-Haves

- [ ] "Polyphonic input (1-16 channels) produces matching polyphonic output on all four outputs"
- [ ] "Each voice is filtered independently (different CV produces different filtering)"
- [ ] "Filter remains stable across all voices (no NaN, no crashes, no blow-up)"

## Files

- `src/HydraQuartetVCF.cpp`
