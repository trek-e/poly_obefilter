# T01: 02-core-filter-dsp 01

**Slice:** S02 — **Milestone:** M001

## Description

Implement the core SEM-style 12dB state-variable filter DSP that processes audio through the module.

Purpose: This is the heart of the HydraQuartet VCF-OB - the actual filter processing that gives it the distinctive Oberheim sound. Without this, the module is just a pretty panel.

Output: Working lowpass filter with cutoff/resonance control, audible filtering, and self-oscillation at high resonance.

## Must-Haves

- [ ] "Audio passes through module with lowpass filtering applied"
- [ ] "Filter cutoff knob sweeps frequency across audible range"
- [ ] "High resonance produces self-oscillation (audible sine tone)"
- [ ] "Filter remains stable (no blow-up, NaN, or crashes)"

## Files

- `src/SVFilter.hpp`
- `src/HydraQuartetVCF.cpp`
