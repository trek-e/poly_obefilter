# Requirements: HydraQuartet VCF-OB

**Defined:** 2026-01-29
**Core Value:** The distinctive Oberheim filter sound in a polyphonic VCV Rack module

## v0.50b Requirements

Requirements for initial beta release. Each maps to roadmap phases.

### Foundation

- [ ] **FOUND-01**: Plugin scaffold with Makefile and plugin.json
- [ ] **FOUND-02**: Panel design (12-14 HP SVG) following VCV guidelines
- [ ] **FOUND-03**: README with build instructions and usage documentation

### Filter Core

- [ ] **FILT-01**: SEM-style 12dB/oct state-variable filter implementation
- [ ] **FILT-02**: Lowpass filter mode with dedicated output
- [ ] **FILT-03**: Highpass filter mode with dedicated output
- [ ] **FILT-04**: Bandpass filter mode with dedicated output
- [ ] **FILT-05**: Notch filter mode with dedicated output

### Polyphony

- [ ] **POLY-01**: 8-voice polyphonic audio processing with proper channel handling

### Controls

- [ ] **CTRL-01**: Cutoff frequency knob (20Hz - 20kHz range)
- [ ] **CTRL-02**: Cutoff CV input with attenuverter
- [ ] **CTRL-03**: Resonance knob (0-100% range)
- [ ] **CTRL-04**: Resonance CV input
- [ ] **CTRL-05**: Drive/Saturation control for filter character

### I/O

- [ ] **IO-01**: Polyphonic audio input (8 channels)

## v0.60b Requirements

Second beta — adds OB-X filter type.

- **FILT-06**: OB-X style 24dB/oct cascaded filter
- **FILT-07**: Filter type selection switch (SEM/OB-X)

## v0.70b Requirements

Third beta — adds CV control and modulation inputs.

- **CTRL-06**: CV input for filter type switching
- **CTRL-07**: CV input for mode selection
- **CTRL-08**: FM input with attenuverter
- **CTRL-09**: 1V/Oct pitch tracking input

## v0.80b Requirements

Fourth beta — adds per-voice control.

- **POLY-02**: Per-voice cutoff control option
- **POLY-03**: Per-voice resonance control option

## v0.90b Requirements

Pre-release beta — polish and stability.

- **FILT-08**: Self-oscillation at high resonance settings
- **IO-02**: Mixed/selected output jack
- **STAB-01**: Edge case handling (NaN protection, stability)
- **STAB-02**: CPU optimization (SIMD for polyphony)

## v1.0 Requirements

Full release — VCV Library ready.

- **QUAL-01**: VCV Library submission requirements met
- **QUAL-02**: Cross-platform builds verified (Windows, macOS, Linux)
- **QUAL-03**: Complete user documentation

## Out of Scope

Explicitly excluded. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| Visual frequency response display | High complexity, defer to v2+ |
| Per-voice separate outputs | Adds complexity, low demand |
| Built-in envelope generator | Scope creep, defeats modular philosophy |
| Filter morphing between types | Complex DSP, discrete switching matches Oberheim |
| Separate resonance per mode | Confusing UX, single resonance is simpler |
| Stereo I/O | Most VCV synth voices are mono, defer |
| Additional filter topologies | Focus on Oberheim identity first |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| FOUND-01 | — | Pending |
| FOUND-02 | — | Pending |
| FOUND-03 | — | Pending |
| FILT-01 | — | Pending |
| FILT-02 | — | Pending |
| FILT-03 | — | Pending |
| FILT-04 | — | Pending |
| FILT-05 | — | Pending |
| POLY-01 | — | Pending |
| CTRL-01 | — | Pending |
| CTRL-02 | — | Pending |
| CTRL-03 | — | Pending |
| CTRL-04 | — | Pending |
| CTRL-05 | — | Pending |
| IO-01 | — | Pending |

**Coverage:**
- v0.50b requirements: 15 total
- Mapped to phases: 0
- Unmapped: 15 (pending roadmap creation)

---
*Requirements defined: 2026-01-29*
*Last updated: 2026-01-29 after initial definition*
