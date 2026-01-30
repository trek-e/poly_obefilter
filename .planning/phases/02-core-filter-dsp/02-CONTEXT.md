# Phase 2: Core Filter DSP - Context

**Gathered:** 2026-01-30
**Status:** Ready for planning

<domain>
## Phase Boundary

Working SEM-style 12dB state-variable filter with lowpass output. This phase implements the actual DSP code that processes audio through a single-voice filter. Polyphony is Phase 4, additional outputs (HP/BP/Notch) are Phase 3.

</domain>

<decisions>
## Implementation Decisions

### Filter Topology
- Trapezoidal SVF (zero-delay feedback) — more accurate to analog behavior
- Balance warm character where it matters without adding complexity just for authenticity
- Optional 2x oversampling via panel switch (not context menu)

### Cutoff Range & Scaling
- Frequency range: 20Hz - 20kHz (full audible range)
- Exponential scaling — perceptually linear, feels natural
- Two CV inputs: one for modulation, one for 1V/Oct keyboard tracking
- Attenuverter range: ±2 (can amplify CV for more dramatic sweeps)

### Resonance Behavior
- No volume compensation at high Q — natural volume drop (authentic behavior)
- Self-oscillation produces musical sine but not a precision oscillator
- Soft saturation in feedback path — adds warmth and prevents blowup, part of Oberheim character

### Stability & Safety
- Soft saturation in feedback path (decided above)

### Claude's Discretion
- Self-oscillation threshold (tune based on SEM behavior)
- Extreme resonance handling (soft vs hard limit)
- DC blocking approach
- NaN/infinity protection strategy
- Parameter smoothing to avoid zipper noise

</decisions>

<specifics>
## Specific Ideas

- Filter should balance authentic SEM warmth with clean modern precision
- Self-oscillation should be musical for filter pings and effects, not necessarily a precision sine oscillator
- The soft saturation contributes to the Oberheim character

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 02-core-filter-dsp*
*Context gathered: 2026-01-30*
