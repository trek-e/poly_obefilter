# T01: 03-filter-modes 01

**Slice:** S03 — **Milestone:** M001

## Description

Add highpass, bandpass, and notch outputs to the existing SVF

Purpose: Complete the multimode filter by exposing all four filter modes that the SVF topology already computes internally. Phase 2 only outputs lowpass; this phase exposes HP, BP, and Notch through the existing panel jacks.

Output: Working four-output filter where LP, HP, BP, and Notch can all be used simultaneously from a single audio input.

## Must-Haves

- [ ] "Highpass output produces audible high-frequency content"
- [ ] "Bandpass output produces audible band-limited signal"
- [ ] "Notch output produces audible notch-filtered signal"
- [ ] "All four outputs work simultaneously from same input"

## Files

- `src/SVFilter.hpp`
- `src/CipherOB.cpp`
