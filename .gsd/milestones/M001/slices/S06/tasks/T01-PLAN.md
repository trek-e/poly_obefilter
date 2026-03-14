# T01: 06-resonance-control 01

**Slice:** S06 — **Milestone:** M001

## Description

Add resonance attenuverter control to expose user-adjustable CV modulation depth for the resonance parameter.

Purpose: Users need to control how much CV affects resonance, including the ability to invert the CV signal. This mirrors the cutoff CV attenuverter pattern for consistency.

Output: Working resonance attenuverter with panel widget, matching cutoff's behavior (bipolar -1 to +1, default 0, 10% per volt scaling).

## Must-Haves

- [ ] "Resonance knob increases filter emphasis from subtle to self-oscillation"
- [ ] "CV input modulates resonance amount with attenuverter control"
- [ ] "Attenuverter at center (0) means CV has no effect"
- [ ] "Filter remains stable at all resonance settings"
- [ ] "Resonance control responds smoothly without zipper noise"

## Files

- `src/HydraQuartetVCF.cpp`
- `res/HydraQuartetVCF.svg`
