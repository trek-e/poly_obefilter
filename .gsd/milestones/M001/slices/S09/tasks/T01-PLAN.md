# T01: 09-character-output 01

**Slice:** S09 — **Milestone:** M001

## Description

Tune OB-X sonic character for the 24dB cascade mode, silence HP/BP/Notch outputs in 24dB mode (authentic OB-Xa LP-only behavior), and verify click-free mode switching works correctly for all four outputs including the fade-to-zero and fade-from-zero transitions.

Purpose: Complete the v0.60b milestone by making the 24dB mode sound like a genuinely different synth architecture (bright, edgy, aggressive OB-Xa character) while the 12dB SEM path remains warm and smooth. This is the final phase before milestone completion.

Output: Modified `src/HydraQuartetVCF.cpp` with OB-X character tuning and LP-only output routing in the 24dB branch. Compiled `plugin.dylib` passing build verification.

## Must-Haves

- [ ] "24dB mode sounds bright/edgy compared to 12dB mode's warm/smooth character"
- [ ] "HP/BP/Notch outputs are silent (0V) in 24dB mode"
- [ ] "Switching from 12dB to 24dB fades HP/BP/Notch to silence over 128 samples (no click)"
- [ ] "Switching from 24dB to 12dB fades HP/BP/Notch back in from silence (symmetric fade-in)"
- [ ] "Drive control produces musical saturation in both 12dB and 24dB modes"
- [ ] "Filter remains stable at all resonance and drive settings in 24dB mode (no NaN, no crashes)"

## Files

- `src/HydraQuartetVCF.cpp`
