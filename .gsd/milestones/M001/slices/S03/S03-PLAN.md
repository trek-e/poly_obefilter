# S03: Filter Modes

**Goal:** Add highpass, bandpass, and notch outputs to the existing SVF

Purpose: Complete the multimode filter by exposing all four filter modes that the SVF topology already computes internally.
**Demo:** Add highpass, bandpass, and notch outputs to the existing SVF

Purpose: Complete the multimode filter by exposing all four filter modes that the SVF topology already computes internally.

## Must-Haves


## Tasks

- [x] **T01: 03-filter-modes 01** `est:34min`
  - Add highpass, bandpass, and notch outputs to the existing SVF

Purpose: Complete the multimode filter by exposing all four filter modes that the SVF topology already computes internally. Phase 2 only outputs lowpass; this phase exposes HP, BP, and Notch through the existing panel jacks.

Output: Working four-output filter where LP, HP, BP, and Notch can all be used simultaneously from a single audio input.

## Files Likely Touched

- `src/SVFilter.hpp`
- `src/CipherOB.cpp`
