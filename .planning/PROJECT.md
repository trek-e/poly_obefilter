# HydraQuartet VCF-OB

## What This Is

A polyphonic 8-voice multimode filter module for VCV Rack 2.x by Synth-etic Intelligence. Inspired by classic Oberheim filters, it offers two switchable filter types (SEM-style 12dB and OB-X style 24dB), four filter modes (LP/HP/BP/Notch), comprehensive CV control, and simultaneous multi-output capability.

## Core Value

The distinctive Oberheim filter sound — musical, characterful, and versatile — in a polyphonic VCV Rack module.

## Requirements

### Validated

(None yet — ship to validate)

### Active

- [ ] 8-voice polyphonic audio processing
- [ ] SEM-style 12dB/oct state-variable filter
- [ ] OB-X style 24dB/oct filter
- [ ] Lowpass filter mode
- [ ] Highpass filter mode
- [ ] Bandpass filter mode
- [ ] Notch filter mode
- [ ] Global cutoff control with CV input and attenuverter
- [ ] Global resonance control with CV input
- [ ] Drive/saturation control for filter character
- [ ] FM input for frequency modulation
- [ ] Panel controls for mode and filter type selection
- [ ] CV control for mode and filter type switching
- [ ] Self-oscillation at high resonance settings
- [ ] Individual outputs for each filter mode (LP/HP/BP/Notch)
- [ ] Mixed/selected output
- [ ] 12-14 HP panel
- [ ] VCV Rack 2.x SDK compatibility
- [ ] Stable operation (no crashes, handles edge cases)
- [ ] Documentation (README, usage instructions)
- [ ] VCV Library publication ready

### Out of Scope

- Per-voice individual control — deferred to v0.80b
- Additional filter types beyond SEM/OB-X — future consideration
- Preset system — not planned for v1

## Context

**Brand:** Synth-etic Intelligence
**Module:** HydraQuartet VCF-OB

**Oberheim Filter Background:**
- SEM (Synthesizer Expander Module): 12dB/oct state-variable filter known for smooth, musical character
- OB-X/OB-Xa: 24dB/oct topology with more aggressive resonance behavior

**VCV Rack Development:**
- First module for this developer
- Targeting VCV Rack 2.x (latest stable SDK)
- Goal is VCV Library publication

**Release Plan:**
| Version | Scope |
|---------|-------|
| 0.50b | Core filter — SEM-style, global controls, basic I/O |
| 0.60b | Add OB-X style 24dB filter |
| 0.70b | CV mode/type switching |
| 0.80b | Per-voice control option |
| 0.90b | Bugfixes, stabilization |
| 1.0 | Full release, VCV Library submission |
| 1.1 | Post-release bugfixes |

## Constraints

- **SDK**: VCV Rack 2.x (latest) — required for library submission
- **Panel Size**: 12-14 HP — comfortable layout for all controls
- **Polyphony**: 8 voices maximum — balances capability with CPU efficiency
- **Target Platform**: Must meet VCV Library quality standards

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Start with SEM-style filter | 12dB state-variable is simpler to implement, classic Oberheim sound | — Pending |
| Global controls first | Simpler architecture, per-voice adds complexity | — Pending |
| Both panel + CV mode switching | Maximum flexibility for users | — Pending |
| Multiple simultaneous outputs | State-variable naturally provides all modes | — Pending |

---
*Last updated: 2025-01-29 after initialization*
