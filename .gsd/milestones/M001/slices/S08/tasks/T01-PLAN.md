# T01: 08-core-24db-cascade 01

**Slice:** S08 — **Milestone:** M001

## Description

Implement the cascaded 24dB SVF filter with panel switch, click-free mode switching, and stable self-oscillation.

Purpose: Add OB-X style 24dB/oct lowpass filter mode as the core new feature of v0.60b, alongside the existing 12dB SEM mode, selectable via a 2-position toggle switch.

Output: Working HydraQuartetVCF module with both 12dB and 24dB filter modes, click-free switching, and updated panel SVG.

## Must-Haves

- [ ] "User can select between 12dB SEM and 24dB OB-X modes via a 2-position toggle switch on the panel"
- [ ] "24dB mode produces steeper lowpass rolloff (-24dB/octave) than 12dB mode (-12dB/octave)"
- [ ] "Cutoff frequency parameter produces the same cutoff frequency in both modes"
- [ ] "Filter self-oscillates at maximum resonance in 24dB mode with warm, slightly distorted character"
- [ ] "24dB mode remains stable at all resonance settings without crashes or NaN"
- [ ] "Switching between 12dB and 24dB modes produces no audible clicks or pops"

## Files

- `src/HydraQuartetVCF.cpp`
- `res/HydraQuartetVCF.svg`
