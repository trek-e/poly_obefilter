# T01: 07-drive-control 01

**Slice:** S07 — **Milestone:** M001

## Description

Implement drive/saturation control with CV modulation for Oberheim filter character.

Purpose: Complete the v0.50b milestone by adding the final control - drive saturation that gives the filter its warm, fat character with harmonic content. This is the distinguishing feature that makes the filter sound musical rather than sterile.

Output: Working drive control with:
- Drive knob (0-1, default 0 for clean)
- Drive CV input with attenuverter
- Blended saturation algorithm (tanh + asymmetric for even+odd harmonics)
- Output-specific drive scaling (LP/BP full, HP reduced, Notch medium)
- Gain compensation for consistent loudness

## Must-Haves

- [ ] "Drive knob adds harmonic saturation to filtered signal"
- [ ] "Drive at 0 produces clean (unaffected) signal"
- [ ] "Full drive produces thick, compressed saturation (not harsh)"
- [ ] "Drive CV modulates saturation amount with attenuverter control"
- [ ] "LP/BP outputs receive more drive effect than HP output"

## Files

- `src/HydraQuartetVCF.cpp`
- `src/SVFilter.hpp`
- `res/HydraQuartetVCF.svg`
