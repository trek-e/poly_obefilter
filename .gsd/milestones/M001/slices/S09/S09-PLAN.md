# S09: Character Output

**Goal:** Tune OB-X sonic character for the 24dB cascade mode, silence HP/BP/Notch outputs in 24dB mode (authentic OB-Xa LP-only behavior), and verify click-free mode switching works correctly for all four outputs including the fade-to-zero and fade-from-zero transitions.
**Demo:** Tune OB-X sonic character for the 24dB cascade mode, silence HP/BP/Notch outputs in 24dB mode (authentic OB-Xa LP-only behavior), and verify click-free mode switching works correctly for all four outputs including the fade-to-zero and fade-from-zero transitions.

## Must-Haves


## Tasks

- [x] **T01: 09-character-output 01** `est:10min`
  - Tune OB-X sonic character for the 24dB cascade mode, silence HP/BP/Notch outputs in 24dB mode (authentic OB-Xa LP-only behavior), and verify click-free mode switching works correctly for all four outputs including the fade-to-zero and fade-from-zero transitions.

Purpose: Complete the v0.60b milestone by making the 24dB mode sound like a genuinely different synth architecture (bright, edgy, aggressive OB-Xa character) while the 12dB SEM path remains warm and smooth. This is the final phase before milestone completion.

Output: Modified `src/HydraQuartetVCF.cpp` with OB-X character tuning and LP-only output routing in the 24dB branch. Compiled `plugin.dylib` passing build verification.

## Files Likely Touched

- `src/HydraQuartetVCF.cpp`
