# M001: v0.60b OB-X Filter

**Vision:** A polyphonic 16-voice multimode filter module for VCV Rack 2.

## Success Criteria

- User can select between 12dB SEM and 24dB OB-X modes via panel switch
- 24dB mode produces steeper lowpass rolloff (-24dB/octave) than 12dB mode
- OB-X mode sounds bright/edgy compared to SEM warm/smooth character
- Only lowpass output is active in 24dB mode (HP/BP/Notch silent, authentic OB-Xa)
- Switching between modes produces no audible clicks or pops

## Slices

- [x] **S01: Foundation** `risk:medium` `depends:[]`
  > After this: Establish complete plugin infrastructure and panel design for CIPHER · OB.
- [x] **S02: Core Filter Dsp** `risk:medium` `depends:[S01]`
  > After this: Implement the core SEM-style 12dB state-variable filter DSP that processes audio through the module.
- [x] **S03: Filter Modes** `risk:medium` `depends:[S02]`
  > After this: Add highpass, bandpass, and notch outputs to the existing SVF

Purpose: Complete the multimode filter by exposing all four filter modes that the SVF topology already computes internally.
- [x] **S04: Polyphonic Extension** `risk:medium` `depends:[S03]`
  > After this: Transform the monophonic CipherOB into a polyphonic processor supporting up to 16 voices.
- [x] **S05: Cutoff Control** `risk:medium` `depends:[S04]`
  > After this: Refine cutoff frequency control default position

Purpose: The filter should start "fully open" (20kHz) so new patches produce immediate audio output.
- [x] **S06: Resonance Control** `risk:medium` `depends:[S05]`
  > After this: Add resonance attenuverter control to expose user-adjustable CV modulation depth for the resonance parameter.
- [x] **S07: Drive Control** `risk:medium` `depends:[S06]`
  > After this: Implement drive/saturation control with CV modulation for Oberheim filter character.
- [x] **S08: Core 24db Cascade** `risk:medium` `depends:[S07]`
  > After this: Implement the cascaded 24dB SVF filter with panel switch, click-free mode switching, and stable self-oscillation.
- [x] **S09: Character Output** `risk:medium` `depends:[S08]`
  > After this: Tune OB-X sonic character for the 24dB cascade mode, silence HP/BP/Notch outputs in 24dB mode (authentic OB-Xa LP-only behavior), and verify click-free mode switching works correctly for all four outputs including the fade-to-zero and fade-from-zero transitions.
