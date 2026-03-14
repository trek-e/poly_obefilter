# Requirements

## Active

### CTRL-06 — CV input for filter type switching (12dB ↔ 24dB)

- Status: active
- Class: core-capability
- Source: M002-CONTEXT
- Primary Slice: M002/S01
- Progress: S01 implemented — per-voice Schmitt trigger CV input, build-verified. Awaiting runtime UAT.

CV input that accepts a gate, trigger, or continuous CV to switch between 12dB SEM and 24dB OB-X filter modes. Polyphonic (per-voice switching).

### CTRL-08 — FM input with attenuverter for audio-rate cutoff modulation

- Status: active
- Class: core-capability
- Source: M002-CONTEXT
- Primary Slice: M002/S02

Dedicated FM input with attenuverter knob for modulating cutoff frequency. Must work at audio rates (bypass parameter smoothing). Exponential FM (V/Oct scaled).

### CTRL-09 — 1V/Oct pitch tracking input

- Status: active
- Class: core-capability
- Source: M002-CONTEXT
- Primary Slice: M002/S02

Dedicated 1V/Oct input for pitch tracking. No attenuverter — true tracking. Sums with cutoff knob in logarithmic domain.

### CTRL-10 — Filter type CV overrides panel switch when connected

- Status: active
- Class: convention
- Source: M002-RESEARCH (candidate requirement)
- Primary Slice: M002/S01
- Progress: S01 implemented — isConnected() branching logic, build-verified. Awaiting runtime UAT.

When filter type CV is patched, CV determines filter type and panel switch has no effect. When CV is disconnected, panel switch works normally.

### CTRL-11 — Schmitt trigger hysteresis on filter type CV threshold

- Status: active
- Class: convention
- Source: M002-RESEARCH (candidate requirement)
- Primary Slice: M002/S01
- Progress: S01 implemented — 200mV hysteresis band (2.4V falling, 2.6V rising), per-voice state arrays, build-verified. Awaiting runtime UAT.

Filter type CV uses Schmitt trigger with ~0.1V hysteresis (switch at 2.6V rising, 2.4V falling) to prevent chatter near threshold.

### CTRL-12 — FM input bypasses cutoff parameter smoothing

- Status: active
- Class: domain-standard
- Source: M002-RESEARCH (candidate requirement)
- Primary Slice: M002/S02

FM modulation component bypasses the SVFilter's cutoffSmoother so audio-rate FM is not attenuated above ~160Hz. Base cutoff (knob + cutoff CV) still smoothed.

### CTRL-13 — 1V/Oct sums with cutoff knob in logarithmic domain

- Status: active
- Class: convention
- Source: M002-RESEARCH (candidate requirement)
- Primary Slice: M002/S02

1V/Oct input sums in V/Oct (logarithmic) domain: `cutoffHz * pow(2, voct)`. Knob sets base frequency, 1V/Oct offsets it.

## Deferred

### CTRL-07 — CV input for mode selection

- Status: deferred
- Class: core-capability
- Source: M002-CONTEXT
- Deferred reason: Architecture doesn't support mode selection — all 4 outputs are always active in 12dB mode. Revisit when mixed/selected output jack is added in v0.90b.

CV input for selecting between LP/HP/BP/Notch modes. Currently meaningless since all outputs are simultaneously available.

## Validated

### FILT-08 — OB-X character tuning (bright/edgy saturation distinct from SEM warm/smooth)

- Status: validated
- Class: core-capability
- Source: inferred
- Primary Slice: S09
- Validated: M001 — resonance24 1.15x boost + inter-stage tanh saturation

OB-X character tuning (bright/edgy saturation distinct from SEM warm/smooth)

### FILT-09 — Lowpass-only output active in 24dB mode (HP/BP/Notch silent, authentic OB-Xa)

- Status: validated
- Class: core-capability
- Source: inferred
- Primary Slice: S09
- Validated: M001 — outHP/BP/Notch zeroed in 24dB branch, crossfade handles transitions

Lowpass-only output active in 24dB mode (HP/BP/Notch silent, authentic OB-Xa)

### TYPE-03 — Click-free mode switching without pops or glitches

- Status: validated
- Class: core-capability
- Source: inferred
- Primary Slice: S08
- Validated: M001 — 128-sample linear crossfade with per-voice from-value capture

Click-free mode switching without pops or glitches

### FILT-06 — 24dB/oct OB-X style lowpass filter via cascaded SVF topology

- Status: validated
- Class: core-capability
- Source: inferred
- Primary Slice: none yet

24dB/oct OB-X style lowpass filter via cascaded SVF topology

### FILT-07 — Self-oscillation at high resonance in 24dB mode

- Status: validated
- Class: core-capability
- Source: inferred
- Primary Slice: none yet

Self-oscillation at high resonance in 24dB mode

### TYPE-01 — Panel switch for 12dB SEM / 24dB OB-X filter type selection

- Status: validated
- Class: core-capability
- Source: inferred
- Primary Slice: none yet

Panel switch for 12dB SEM / 24dB OB-X filter type selection

### TYPE-02 — Cutoff frequency remains consistent between modes (1kHz stays 1kHz)

- Status: validated
- Class: core-capability
- Source: inferred
- Primary Slice: none yet

Cutoff frequency remains consistent between modes (1kHz stays 1kHz)

## Deferred

## Out of Scope
