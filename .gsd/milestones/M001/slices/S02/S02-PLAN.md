# S02: Core Filter Dsp

**Goal:** Implement the core SEM-style 12dB state-variable filter DSP that processes audio through the module.
**Demo:** Implement the core SEM-style 12dB state-variable filter DSP that processes audio through the module.

## Must-Haves


## Tasks

- [x] **T01: 02-core-filter-dsp 01** `est:18min`
  - Implement the core SEM-style 12dB state-variable filter DSP that processes audio through the module.

Purpose: This is the heart of the HydraQuartet VCF-OB - the actual filter processing that gives it the distinctive Oberheim sound. Without this, the module is just a pretty panel.

Output: Working lowpass filter with cutoff/resonance control, audible filtering, and self-oscillation at high resonance.

## Files Likely Touched

- `src/SVFilter.hpp`
- `src/HydraQuartetVCF.cpp`
