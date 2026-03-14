# S06: Resonance Control

**Goal:** Add resonance attenuverter control to expose user-adjustable CV modulation depth for the resonance parameter.
**Demo:** Add resonance attenuverter control to expose user-adjustable CV modulation depth for the resonance parameter.

## Must-Haves


## Tasks

- [x] **T01: 06-resonance-control 01** `est:2min`
  - Add resonance attenuverter control to expose user-adjustable CV modulation depth for the resonance parameter.

Purpose: Users need to control how much CV affects resonance, including the ability to invert the CV signal. This mirrors the cutoff CV attenuverter pattern for consistency.

Output: Working resonance attenuverter with panel widget, matching cutoff's behavior (bipolar -1 to +1, default 0, 10% per volt scaling).

## Files Likely Touched

- `src/HydraQuartetVCF.cpp`
- `res/HydraQuartetVCF.svg`
