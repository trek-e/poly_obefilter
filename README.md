# CIPHER · OB

**16-voice polyphonic Oberheim-style multimode filter for [VCV Rack](https://vcvrack.com/)**

by [Synth-etic Intelligence](https://github.com/trek-e)

![License](https://img.shields.io/badge/license-GPL--3.0--or--later-blue)
![Rack 2](https://img.shields.io/badge/VCV%20Rack-2.x-green)

---

CIPHER · OB brings two classic Oberheim filter voices into VCV Rack — the warm, musical SEM 12dB/oct state-variable filter and the bright, aggressive OB-X 24dB/oct cascaded topology. Switch between them with a single toggle, get four simultaneous filter outputs, and run up to 16 voices of polyphony.

## Features

### Dual Filter Modes
- **12dB SEM mode** — classic Oberheim SEM state-variable filter with smooth, musical resonance character
- **24dB OB-X mode** — cascaded dual-stage SVF producing steep -24dB/oct lowpass rolloff with bright, edgy OB-Xa character
- **Click-free switching** — 128-sample crossfade eliminates pops when toggling between modes
- **Panel toggle switch** — instant 12dB ↔ 24dB selection

### Four Simultaneous Outputs
- **Lowpass** — available in both 12dB and 24dB modes
- **Highpass** — active in 12dB mode
- **Bandpass** — active in 12dB mode
- **Notch** — active in 12dB mode
- In 24dB mode, only lowpass is active (authentic OB-Xa behavior) — HP/BP/Notch fade to silence via crossfade

### Polyphony
- **16-voice polyphonic** — full PORT_MAX_CHANNELS support
- Each voice processes independently with its own filter state
- All CV inputs are polyphonic (per-voice modulation)

### Controls & CV
- **Cutoff** — 20 Hz to 20 kHz (logarithmic), defaults to fully open
- **Cutoff CV** — V/Oct exponential scaling with bipolar attenuverter
- **Resonance** — 0% to 100% (Q 0.5–20), with self-oscillation at maximum
- **Resonance CV** — 10%/volt linear scaling with bipolar attenuverter
- **Drive** — blended tanh + asymmetric saturation for even and odd harmonics
- **Drive CV** — 10%/volt linear scaling with bipolar attenuverter
- True bypass at zero drive for clean signal path

### Oberheim Character
- **Trapezoidal SVF** with frequency warping for accurate analog modeling
- **Soft saturation** (tanh) in feedback path — warm self-oscillation without harsh clipping
- **Output-specific drive scaling** — LP/BP 100%, HP 50%, Notch 70% (matches analog circuit behavior)
- **OB-X resonance boost** — 1.15× Q in 24dB Stage 1 for aggressive peaks, 0.65× in Stage 2 for stability
- **Inter-stage CEM3320-style saturation** — normalized tanh adds progressive odd harmonics at high levels
- **Analog noise floor** — ~1µV deterministic dither enables self-oscillation from silence

## Panel Layout

14HP dark-themed panel:

| Section | Controls |
|---------|----------|
| **Filter** | 12dB/24dB toggle switch |
| **Cutoff** | Cutoff knob, CV input, attenuverter |
| **Drive** | Drive knob, CV input, attenuverter |
| **Resonance** | Resonance knob, CV input, attenuverter |
| **Input** | Polyphonic audio input |
| **Outputs** | LP, HP, BP, Notch (4 simultaneous outputs) |

## Installation

### From source

Requires the [VCV Rack SDK](https://vcvrack.com/manual/PluginDevelopmentTutorial).

```bash
# Clone the repository
git clone https://github.com/trek-e/CipherOB
cd CipherOB

# Build (Rack SDK must be one level up, or set RACK_DIR)
make

# Install to your Rack plugins directory
make install
```

### Prerequisites

- VCV Rack SDK 2.x
- C++11 compatible compiler
- Make (included with Xcode Command Line Tools on macOS)

### macOS quick setup

```bash
brew install git wget cmake autoconf automake libtool jq python zstd pkg-config

# Set RACK_DIR (add to ~/.zshrc)
export RACK_DIR=~/Rack-SDK
```

## Quick Start

1. Add **CIPHER · OB** from the module browser (under Synth-etic Intelligence)
2. Connect a polyphonic audio source to the **Input** jack
3. Patch one or more **Output** jacks (LP, HP, BP, Notch) to your mixer
4. Turn the **Cutoff** knob down from maximum — you'll hear the filter close
5. Increase **Resonance** for emphasis at the cutoff frequency
6. Flip the **12dB/24dB switch** to hear the difference between SEM and OB-X character
7. Add **Drive** for saturation and harmonic richness

### Tips

- **Cutoff defaults to fully open** (20kHz) — new patches produce immediate audio output
- **Use all four outputs simultaneously** in 12dB mode — mix LP + HP for creative notch-like effects, or send each output to different effects chains
- **Self-oscillation** at maximum resonance produces a sine-like tone at the cutoff frequency — modulate cutoff with V/Oct for a pitched oscillator
- **24dB mode with Drive** produces the classic OB-X edge — try it with a raw sawtooth input for aggressive lead tones
- **Attenuverters** accept -100% to +100% — negative values invert the CV for ducking effects (LFO opens filter on downstroke)

## Architecture

The DSP is implemented in two files with no external dependencies beyond the VCV Rack SDK:

| File | Purpose |
|------|---------|
| `src/SVFilter.hpp` | Trapezoidal state-variable filter core, blended saturation function |
| `src/CipherOB.cpp` | Module implementation: dual-mode process(), crossfade, parameter handling |

**Total: 391 lines of C++**

### Signal Flow

```
Audio In → [Dither] → Cutoff/Res/Drive CV Processing
                          ↓
              ┌─── 12dB SEM Mode ───┐     ┌─── 24dB OB-X Mode ──────────┐
              │ SVFilter (Stage 1)   │     │ SVFilter Stage 1 (Q×1.15)   │
              │   ↓                  │     │   ↓ tanh inter-stage         │
              │ Drive (per-output)   │     │ SVFilter Stage 2 (Q×0.65)   │
              │   ↓                  │     │   ↓ Drive (1.3×)             │
              │ LP, HP, BP, Notch    │     │ LP only (HP/BP/Notch = 0)   │
              └──────────────────────┘     └─────────────────────────────┘
                          ↓
              128-sample crossfade (on mode switch)
                          ↓
                    Output jacks
```

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 0.60b | 2026-02-21 | Added 24dB OB-X mode, panel switch, click-free switching, OB-X character tuning, LP-only output routing |
| 0.50b | 2026-02-03 | Initial beta — SEM 12dB filter, 4 outputs, 16-voice polyphony, cutoff/resonance/drive with CV |

### Roadmap

| Version | Scope | Status |
|---------|-------|--------|
| 0.50b | Core filter — SEM-style, global controls, basic I/O | ✅ Shipped |
| 0.60b | OB-X style 24dB filter with type switching | ✅ Shipped |
| 0.70b | CV inputs for filter type and FM/pitch tracking | Next |
| 0.80b | Per-voice control options | Planned |
| 0.90b | CPU optimization (SIMD), stabilization | Planned |
| 1.0 | Full release, VCV Library submission | Planned |

## License

[GPL-3.0-or-later](LICENSE.txt)

The VCV Rack Component Library graphics used in this plugin are licensed under [CC BY-NC 4.0](https://creativecommons.org/licenses/by-nc/4.0/).

Copyright © 2026 Synth-etic Intelligence

---

*Part of the COLOSSUS · 16 series — bringing classic analog character to VCV Rack*
