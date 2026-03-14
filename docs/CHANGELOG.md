# Changelog

All notable changes to HydraQuartet VCF-OB are documented here.

## [0.60b] — 2026-02-21

### Added
- **24dB OB-X filter mode** — cascaded dual-stage SVF producing -24dB/oct lowpass rolloff
- **Filter type toggle switch** — panel CKSS switch selects 12dB SEM or 24dB OB-X
- **Click-free mode switching** — 128-sample linear crossfade with per-voice from-value capture
- **OB-X character tuning** — 1.15× resonance boost in Stage 1, 0.65× stability ratio in Stage 2
- **Inter-stage CEM3320 saturation** — normalized tanh for progressive odd harmonics
- **LP-only output in 24dB mode** — HP/BP/Notch output zero voltage (authentic OB-Xa behavior)
- **Analog noise floor dither** — ~1µV deterministic hash-based dither for self-oscillation seeding

### Changed
- Panel updated to Hurricane-8 design language (gradient background, section header bars, stroke-based letterforms)
- Module description updated in plugin.json

## [0.50b] — 2026-02-03

### Added
- **SEM-style 12dB/oct state-variable filter** — trapezoidal SVF with zero-delay feedback
- **Four simultaneous filter outputs** — LP, HP, BP, Notch from single SVF structure
- **16-voice polyphonic processing** — independent filter state per voice
- **Cutoff control** — 20Hz–20kHz logarithmic with V/Oct CV input and attenuverter
- **Resonance control** — Q 0.5–20 with CV input and attenuverter
- **Drive control** — blended tanh + asymmetric saturation with output-specific scaling
- **Self-oscillation** — stable sine-like tone at maximum resonance
- **Parameter smoothing** — 1ms exponential smoothers on all controls
- **14HP panel** — dark industrial aesthetic matching HydraQuartet VCO
- Plugin scaffold, build system, and documentation
