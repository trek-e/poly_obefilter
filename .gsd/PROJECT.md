# HydraQuartet VCF-OB

## What This Is

A polyphonic 16-voice multimode filter module for VCV Rack 2.x by Synth-etic Intelligence. Implements SEM-style 12dB state-variable topology with four simultaneous outputs (LP/HP/BP/Notch), comprehensive CV control with attenuverters, and blended saturation for Oberheim character.

## Core Value

The distinctive Oberheim filter sound — musical, characterful, and versatile — in a polyphonic VCV Rack module.

## Current Milestone: v0.70b CV Control (in progress)

**Goal:** Add CV inputs for filter type switching, FM, and pitch tracking

## Current State

**Shipped:** v0.60b OB-X Filter (2026-02-21)

**Codebase:**
- 391 LOC C++ (src/HydraQuartetVCF.cpp, src/SVFilter.hpp)
- 14 HP panel with dark industrial aesthetic
- Builds on macOS with VCV Rack 2.x SDK

**Capabilities:**
- SEM-style 12dB/oct state-variable filter
- OB-X style 24dB/oct cascaded SVF with character tuning
- Panel toggle switch for 12dB SEM / 24dB OB-X mode selection
- Filter type CV input with per-voice Schmitt trigger switching (2.6V/2.4V hysteresis)
- CV overrides panel switch when connected; switch resumes when disconnected
- Click-free 128-sample per-voice crossfade on mode switching
- LP-only output in 24dB mode (HP/BP/Notch silent, authentic OB-Xa)
- 4 simultaneous outputs (LP, HP, BP, Notch) in 12dB mode
- 16-voice polyphony with independent filter state
- Cutoff (20Hz-20kHz) with V/Oct CV + attenuverter
- Resonance (0.5-20 Q) with CV + attenuverter
- Drive with blended saturation and output-specific scaling
- Self-oscillation at high resonance (with analog noise floor dither)

## Requirements

### Validated

- ✓ Plugin scaffold with Makefile and plugin.json — v0.50b
- ✓ Panel design (14 HP SVG) following VCV guidelines — v0.50b
- ✓ README with build instructions and usage documentation — v0.50b
- ✓ SEM-style 12dB/oct state-variable filter — v0.50b
- ✓ Lowpass filter mode with dedicated output — v0.50b
- ✓ Highpass filter mode with dedicated output — v0.50b
- ✓ Bandpass filter mode with dedicated output — v0.50b
- ✓ Notch filter mode with dedicated output — v0.50b
- ✓ 16-voice polyphonic audio processing — v0.50b
- ✓ Cutoff frequency knob (20Hz-20kHz) — v0.50b
- ✓ Cutoff CV input with attenuverter — v0.50b
- ✓ Resonance knob (0-100% range) — v0.50b
- ✓ Resonance CV input with attenuverter — v0.50b
- ✓ Drive/saturation control for filter character — v0.50b
- ✓ Polyphonic audio input — v0.50b

### Active

- [ ] OB-X style 24dB/oct filter (v0.60b)
- [ ] Filter type selection switch (v0.60b)
- [x] CV input for filter type switching (v0.70b, S01)
- [ ] CV input for mode selection (v0.70b)
- [ ] FM input with attenuverter (v0.70b)
- [ ] 1V/Oct pitch tracking input (v0.70b)
- [ ] Per-voice cutoff control option (v0.80b)
- [ ] Per-voice resonance control option (v0.80b)
- [ ] Mixed/selected output jack (v0.90b)
- [ ] CPU optimization (SIMD) (v0.90b)
- [ ] VCV Library submission (v1.0)
- [ ] Cross-platform builds (v1.0)

### Out of Scope

- Visual frequency response display — High complexity, defer to v2+
- Per-voice separate outputs — Adds complexity, low demand
- Built-in envelope generator — Scope creep, defeats modular philosophy
- Filter morphing between types — Complex DSP, discrete switching matches Oberheim
- Separate resonance per mode — Confusing UX, single resonance is simpler
- Stereo I/O — Most VCV synth voices are mono, defer
- Additional filter topologies — Focus on Oberheim identity first

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
| Version | Scope | Status |
|---------|-------|--------|
| 0.50b | Core filter — SEM-style, global controls, basic I/O | ✅ Shipped |
| 0.60b | Add OB-X style 24dB filter | ✅ Shipped |
| 0.70b | CV mode/type switching | Next |
| 0.80b | Per-voice control option | Planned |
| 0.90b | Bugfixes, stabilization | Planned |
| 1.0 | Full release, VCV Library submission | Planned |

## Constraints

- **SDK**: VCV Rack 2.x (latest) — required for library submission
- **Panel Size**: 14 HP — comfortable layout for all controls
- **Polyphony**: 16 voices (PORT_MAX_CHANNELS) — full VCV Rack polyphony support
- **Target Platform**: Must meet VCV Library quality standards

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Start with SEM-style filter | 12dB state-variable is simpler to implement, classic Oberheim sound | ✓ Good |
| Global controls first | Simpler architecture, per-voice adds complexity | ✓ Good |
| 14 HP panel width | Comfortable spacing for all controls | ✓ Good |
| Trapezoidal SVF integration | Accurate analog modeling, good frequency response | ✓ Good |
| Soft saturation in feedback | Oberheim character, prevents harsh self-oscillation | ✓ Good |
| Q range 0.5-20 | Covers subtle to self-oscillation | ✓ Good |
| Cytomic direct form HP | Correct cutoff tracking for HP output | ✓ Good (after fix) |
| 16-voice polyphony | Full VCV Rack support via PORT_MAX_CHANNELS | ✓ Good |
| 10%/volt CV scaling | Musical control without wild parameter swings | ✓ Good |
| Blended saturation | tanh + asymmetric gives even+odd harmonics | ✓ Good |
| Output-specific drive | LP/BP 100%, HP 50%, Notch 70% matches acoustic behavior | ✓ Good |
| Both panel + CV mode switching | Maximum flexibility for users | — Pending (v0.60b+) |
| Multiple simultaneous outputs | State-variable naturally provides all modes | ✓ Good |

---
*Last updated: 2026-03-13 after M002/S01 completion*
