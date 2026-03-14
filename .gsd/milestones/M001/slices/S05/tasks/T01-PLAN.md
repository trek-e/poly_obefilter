# T01: 05-cutoff-control 01

**Slice:** S05 — **Milestone:** M001

## Description

Refine cutoff frequency control default position

Purpose: The filter should start "fully open" (20kHz) so new patches produce immediate audio output. Currently defaults to mid-range (0.5 = 632 Hz) which sounds dull until user adjusts the knob.

Output: Updated configParam with default 1.0 (fully open), verified working cutoff control with existing V/Oct CV modulation and bipolar attenuverter

## Must-Haves

- [ ] "Filter starts fully open (20kHz cutoff) when module loads"
- [ ] "Cutoff knob sweeps frequency across full audible range (20Hz-20kHz)"
- [ ] "CV input modulates cutoff frequency with V/Oct scaling"
- [ ] "Attenuverter at center (0) means CV has no effect"
- [ ] "Attenuverter scales and inverts CV depth (-1 to +1)"
- [ ] "Cutoff control responds smoothly without zipper noise"

## Files

- `src/HydraQuartetVCF.cpp`
