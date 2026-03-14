# S07: Drive Control

**Goal:** Implement drive/saturation control with CV modulation for Oberheim filter character.
**Demo:** Implement drive/saturation control with CV modulation for Oberheim filter character.

## Must-Haves


## Tasks

- [x] **T01: 07-drive-control 01** `est:2min`
  - Implement drive/saturation control with CV modulation for Oberheim filter character.

Purpose: Complete the v0.50b milestone by adding the final control - drive saturation that gives the filter its warm, fat character with harmonic content. This is the distinguishing feature that makes the filter sound musical rather than sterile.

Output: Working drive control with:
- Drive knob (0-1, default 0 for clean)
- Drive CV input with attenuverter
- Blended saturation algorithm (tanh + asymmetric for even+odd harmonics)
- Output-specific drive scaling (LP/BP full, HP reduced, Notch medium)
- Gain compensation for consistent loudness

## Files Likely Touched

- `src/CipherOB.cpp`
- `src/SVFilter.hpp`
- `res/CipherOB.svg`
