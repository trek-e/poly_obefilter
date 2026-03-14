# S05: Cutoff Control

**Goal:** Refine cutoff frequency control default position

Purpose: The filter should start "fully open" (20kHz) so new patches produce immediate audio output.
**Demo:** Refine cutoff frequency control default position

Purpose: The filter should start "fully open" (20kHz) so new patches produce immediate audio output.

## Must-Haves


## Tasks

- [x] **T01: 05-cutoff-control 01** `est:2min`
  - Refine cutoff frequency control default position

Purpose: The filter should start "fully open" (20kHz) so new patches produce immediate audio output. Currently defaults to mid-range (0.5 = 632 Hz) which sounds dull until user adjusts the knob.

Output: Updated configParam with default 1.0 (fully open), verified working cutoff control with existing V/Oct CV modulation and bipolar attenuverter

## Files Likely Touched

- `src/HydraQuartetVCF.cpp`
