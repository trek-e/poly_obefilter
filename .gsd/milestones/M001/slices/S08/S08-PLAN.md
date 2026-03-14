# S08: Core 24db Cascade

**Goal:** Implement the cascaded 24dB SVF filter with panel switch, click-free mode switching, and stable self-oscillation.
**Demo:** Implement the cascaded 24dB SVF filter with panel switch, click-free mode switching, and stable self-oscillation.

## Must-Haves


## Tasks

- [x] **T01: 08-core-24db-cascade 01** `est:2min`
  - Implement the cascaded 24dB SVF filter with panel switch, click-free mode switching, and stable self-oscillation.

Purpose: Add OB-X style 24dB/oct lowpass filter mode as the core new feature of v0.60b, alongside the existing 12dB SEM mode, selectable via a 2-position toggle switch.

Output: Working HydraQuartetVCF module with both 12dB and 24dB filter modes, click-free switching, and updated panel SVG.

## Files Likely Touched

- `src/HydraQuartetVCF.cpp`
- `res/HydraQuartetVCF.svg`
