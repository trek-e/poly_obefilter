# CIPHER · OB — User Manual

## Overview

CIPHER · OB is a dual-mode multimode filter inspired by two iconic Oberheim filter circuits:

- **SEM (Synthesizer Expander Module)** — a 12dB/oct state-variable filter known for its warm, musical character
- **OB-X / OB-Xa** — a 24dB/oct cascaded topology with brighter, more aggressive resonance

The module provides four simultaneous filter outputs (LP, HP, BP, Notch) in 12dB mode, and a dedicated lowpass output in 24dB mode — just like the original OB-Xa hardware.

## Controls

### Filter Type Switch

A 2-position toggle selects between **12dB SEM** and **24dB OB-X** modes.

- **Up (12dB):** All four outputs are active. The filter has a gentle -12dB/oct slope with warm, musical resonance.
- **Down (24dB):** Only the lowpass output is active (HP/BP/Notch go silent). The filter has a steep -24dB/oct slope with bright, aggressive character.

Switching is click-free — a 128-sample (~2.7ms at 48kHz) crossfade smoothly transitions between modes.

### Cutoff

Controls the filter's cutoff frequency from 20 Hz to 20 kHz on a logarithmic scale.

- **Knob default:** Fully open (20kHz) — so new patches produce immediate audio output
- **CV input:** Exponential V/Oct scaling. With the attenuverter at +100%, 1V = one octave shift.
- **Attenuverter:** -100% to +100%. Negative values invert the CV direction.

The same cutoff frequency is used in both 12dB and 24dB modes — 1kHz stays 1kHz when you switch.

### Resonance

Controls the filter's resonance (Q factor) from subtle emphasis to self-oscillation.

- **Range:** Q 0.5 (no emphasis) to Q 20 (self-oscillation)
- **CV input:** 10%/volt linear scaling. 1V changes resonance by 10%.
- **Attenuverter:** -100% to +100%.

At maximum resonance, the filter self-oscillates — producing a sine-like tone at the cutoff frequency. A tiny analog noise floor dither (~1µV) ensures oscillation starts even from silence.

In 24dB mode, resonance is internally boosted by 1.15× for the first stage to produce the characteristic OB-X aggressive peak, while the second stage runs at 0.65× for stability.

### Drive

Adds harmonic saturation to the filtered signal.

- **Saturation type:** Blended tanh (odd harmonics) + asymmetric shaping (even harmonics)
- **CV input:** 10%/volt linear scaling.
- **Attenuverter:** -100% to +100%.
- **True bypass** at zero drive — completely clean signal path.

Drive is applied per-output with different scaling to match analog circuit behavior:
| Output | Drive Amount | Rationale |
|--------|-------------|-----------|
| Lowpass | 100% | Low-frequency content saturates naturally |
| Bandpass | 100% | Mid-frequency content benefits from full drive |
| Highpass | 50% | High-frequency content is more sensitive to distortion |
| Notch | 70% | Balanced compromise |

In 24dB mode, the lowpass output gets 1.3× the knob setting for extra OB-X edge.

## Outputs

### 12dB Mode (all four active)

| Output | Filter Type | Slope |
|--------|-------------|-------|
| **LP** | Lowpass | -12dB/oct |
| **HP** | Highpass | -12dB/oct |
| **BP** | Bandpass | -6dB/oct per side |
| **Notch** | Notch (band-reject) | LP + HP sum |

All four outputs are available simultaneously from the single state-variable filter structure. This is a key advantage of the SEM topology — you can patch all four to different destinations for complex tonal effects.

### 24dB Mode (lowpass only)

| Output | Filter Type | Slope |
|--------|-------------|-------|
| **LP** | Lowpass | -24dB/oct |
| **HP** | Silent (0V) | — |
| **BP** | Silent (0V) | — |
| **Notch** | Silent (0V) | — |

This matches the authentic OB-Xa behavior — the 24dB cascade was a lowpass-only circuit. When switching from 12dB to 24dB, the HP/BP/Notch outputs fade to zero via the crossfade; switching back fades them in.

## Polyphonic Operation

The module supports up to 16 channels of polyphonic processing.

- **Channel count** is determined by the audio input cable
- Each voice has its own independent filter state (separate integrators, smoothers, and crossfade state)
- All CV inputs use `getPolyVoltage()` — each voice gets its own modulation
- All four outputs match the input channel count

### Monophonic usage

Connect a mono cable and the module processes a single voice. This is the simplest and most CPU-efficient mode.

### Polyphonic usage

Connect a polyphonic cable (2–16 channels) and each channel is filtered independently. This is equivalent to having 16 separate filter modules, but with shared knob positions.

## Patching Ideas

### Basic subtractive synth voice
VCO → **Audio In** → **LP Out** → VCA → Audio Out. Modulate cutoff with an envelope for classic filter sweeps.

### Dual-character patch
Use the 12dB/24dB switch to A/B between warm and aggressive tones on the same patch. Automate the switch with a gate to rhythmically toggle filter character.

### Four-output processing
In 12dB mode, patch all four outputs to separate mixer channels or effects. The LP and HP outputs are complementary — mixing them recreates the original signal (minus the resonance peak).

### Self-oscillation as sine source
Max out resonance, disconnect audio input. The filter becomes a sine oscillator at the cutoff frequency. Modulate cutoff with V/Oct from a sequencer for pitched tones.

### Resonant percussion
Short envelope → Cutoff CV. High resonance. Brief trigger creates a resonant ping — classic analog drum/percussion sound.

## Technical Details

### DSP Implementation

The core is a **trapezoidal (TPT) state-variable filter** with zero-delay feedback:

- Frequency coefficient: `g = tan(π × cutoffHz / sampleRate)`
- Resonance damping: `k = 1/Q` where Q ranges 0.5–20
- Integrator states: two per voice (`ic1eq`, `ic2eq`)
- Soft saturation: `tanh(v1_raw × 2) × 0.5` in feedback path for Oberheim character

The 24dB mode cascades two SVF stages:
- **Stage 1:** Full resonance (Q × 1.15, capped at 0.95) for aggressive OB-X peaks
- **Inter-stage:** Normalized tanh (`tanh(x × 0.12) / 0.12`) for CEM3320 diode-pair character
- **Stage 2:** Reduced resonance (Q × 0.65) for cascade stability

### Parameter Smoothing

All parameters use exponential smoothing with 1ms time constant to prevent zipper noise during automation.

### CPU Usage

The filter processes in scalar mode (no SIMD). At 48kHz with 16 voices, expect approximately 2–4% CPU on a modern system. SIMD optimization is planned for v0.90b.

### Sample Rate Independence

Filter coefficients are recalculated per-sample using the current sample rate via `args.sampleRate`. The module works correctly at any sample rate supported by VCV Rack.
