# HydraQuartet VCF-OB

8-voice polyphonic Oberheim-style multimode filter for VCV Rack 2.x

By Synth-etic Intelligence

## Description

HydraQuartet VCF-OB is a polyphonic multimode filter module inspired by classic Oberheim synthesizer filters. It brings the distinctive Oberheim filter sound — musical, characterful, and versatile — to VCV Rack's modular environment.

### Features

- **8-voice polyphonic processing** - Supports 1-8 channels on polyphonic cables
- **Oberheim SEM-style 12dB/oct state-variable filter** - Smooth, musical character
- **Four simultaneous filter modes** - Lowpass, Highpass, Bandpass, and Notch outputs
- **Comprehensive CV control** - Cutoff and Resonance with dedicated CV inputs
- **Cutoff attenuverter** - Bipolar CV control for expressive modulation
- **Drive control** - Adds saturation character for warmth and grit
- **Part of HydraQuartet series** - Matches the visual style and workflow of HydraQuartet VCO

## Building from Source

### Prerequisites

- VCV Rack SDK 2.x (download from https://vcvrack.com/downloads/)
- Make (included with Xcode Command Line Tools on Mac)
- C++11 compatible compiler
- Git
- Inkscape (optional, for panel editing)

### Mac Installation

1. Install dependencies via Homebrew:

```bash
brew install git wget cmake autoconf automake libtool jq python zstd pkg-config
```

2. Download and extract VCV Rack SDK 2.x from https://vcvrack.com/downloads/

3. Set RACK_DIR environment variable (add to ~/.zshrc or ~/.bashrc):

```bash
export RACK_DIR=~/Rack-SDK
```

4. Clone the repository:

```bash
git clone https://github.com/trek-e/HydraQuartetVCF
cd HydraQuartetVCF
```

5. Build the plugin:

```bash
make
```

6. Install to VCV Rack plugins folder:

```bash
make install
```

### Troubleshooting

**Error: "No rule to make target 'plugin.mk'"**

This means RACK_DIR is not set correctly. Verify with:

```bash
echo $RACK_DIR
```

The path should point to your Rack SDK installation directory. Restart your terminal after setting the environment variable.

## Usage

### Controls

- **Audio Input (top left)** - Connect your audio source here. Accepts mono or polyphonic cables (1-8 channels).

- **Cutoff Section**
  - **Cutoff Knob** - Main filter cutoff frequency control
  - **Cutoff CV Input** - CV modulation input for cutoff frequency
  - **CV Attenuverter** - Bipolar control for CV amount (-100% to +100%)

- **Resonance Section**
  - **Resonance Knob** - Filter resonance/emphasis control
  - **Resonance CV Input** - CV modulation input for resonance

- **Drive Control** - Adds saturation character before the filter for additional harmonic content

- **Filter Outputs (bottom section)**
  - **LP Output** - Lowpass filtered signal
  - **HP Output** - Highpass filtered signal
  - **BP Output** - Bandpass filtered signal
  - **Notch Output** - Notch (band-reject) filtered signal

### Polyphonic Operation

HydraQuartetVCF supports up to 8 channels of polyphonic processing. When you connect a polyphonic cable to the audio input, the module automatically processes each channel independently with the same filter settings. All outputs will match the number of input channels.

### Workflow Tips

1. Start with the Cutoff knob at 12 o'clock (middle position)
2. Adjust Resonance to taste - higher values add emphasis at the cutoff frequency
3. Use the CV Attenuverter to fine-tune modulation depth from envelopes or LFOs
4. Experiment with Drive for additional harmonic richness
5. All four filter outputs are available simultaneously - try mixing them for unique tonal colors

## Current Status

**Version:** 0.5.0 (beta)

**Development Phase:** Phase 1 - Foundation Complete

This is an early beta release. The core infrastructure is complete, with DSP implementation in progress.

- ✅ Phase 1: Foundation - Plugin compiles, loads in VCV Rack, panel design complete
- 🚧 Phase 2: DSP implementation in progress
- ⏳ Future phases: OB-X style 24dB filter, CV mode switching, per-voice control

### Known Limitations

- DSP filter implementation is stubbed (no audio processing yet)
- Self-oscillation not yet implemented
- Filter type switching (SEM vs OB-X) not yet available

### Reporting Issues

Found a bug or have a feature request? Please report it on the GitHub issues page:
https://github.com/trek-e/HydraQuartetVCF/issues

## License

This plugin is licensed under GPL-3.0-or-later.

The VCV Rack Component Library graphics used in this plugin are licensed under CC BY-NC 4.0 (Creative Commons Attribution-NonCommercial). This means the plugin is for non-commercial use. For commercial licensing, custom component graphics would be required.

See LICENSE.txt for the complete GPL-3.0 license text.

## Credits

- **Inspired by classic Oberheim SEM and OB-X filters** - The iconic sound that defined 1970s-80s analog synthesis
- **Built with VCV Rack SDK** - The open-source virtual modular synthesizer platform
- **Developed by Synth-etic Intelligence** - Part of the HydraQuartet series

---

*HydraQuartet VCF-OB - Bringing classic Oberheim filter character to VCV Rack*
