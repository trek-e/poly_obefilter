# Phase 4: Polyphonic Extension - Context

**Gathered:** 2026-01-31
**Status:** Ready for planning

<domain>
## Phase Boundary

Transform the monophonic filter into a polyphonic processor supporting up to 16 voices. Each voice gets independent filter state while sharing parameter controls. All four filter outputs (LP, HP, BP, Notch) become polyphonic.

</domain>

<decisions>
## Implementation Decisions

### Voice Count Handling
- Match input channel count dynamically (not fixed)
- Support up to 16 voices (VCV Rack maximum)
- Mono input (1 channel) copies to all voices for parallel processing

### CV Polyphony
- Per-voice CV modulation for resonance
- When CV has fewer channels than audio, wrap CV channels (VCV standard behavior)
- Example: 2 CV channels with 8 audio voices → voices cycle through CV 1, 2, 1, 2...

### Output Channel Mapping
- All four outputs (LP, HP, BP, Notch) are polyphonic
- Each output jack carries all voices (8 voices in → 8 channels on each output)

### Claude's Discretion
- Cutoff CV: per-voice or global (pick based on typical filter patterns)
- Mono expansion voice count when no CV reference
- Output channel count matching strategy
- Filter state array sizing and memory management

</decisions>

<specifics>
## Specific Ideas

- Mono-to-poly expansion enables creative parallel filtering (same audio through different CV-modulated filter settings)
- Standard VCV Rack polyphony conventions should be followed for predictable patching

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 04-polyphonic-extension*
*Context gathered: 2026-01-31*
