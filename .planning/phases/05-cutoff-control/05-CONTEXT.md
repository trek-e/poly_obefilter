# Phase 5: Cutoff Control - Context

**Gathered:** 2026-01-31
**Status:** Ready for planning

<domain>
## Phase Boundary

Refine the cutoff frequency control with proper CV modulation and attenuverter behavior. The cutoff knob, CV input, and attenuverter already exist from earlier phases - this phase ensures they work correctly with proper scaling, response curves, and smooth operation.

</domain>

<decisions>
## Implementation Decisions

### Knob Range & Response
- Frequency range: 20Hz - 20kHz (full audio spectrum)
- Default position: Fully open (100%) - filter starts with no filtering
- Knob taper: Claude's discretion (exponential/log recommended for filters)

### Attenuverter Behavior
- Range: Bipolar (-1 to +1) - full inversion possible
- Default position: Centered (0) - no CV effect until user turns knob
- Center behavior: Claude's discretion (standard is no CV effect at center)

### CV Scaling/Tracking
- CV standard: Claude's discretion (V/Oct recommended for musical tracking)
- CV range: +/- 10V (allows 20 octaves of modulation range)
- Clamping: Claude's discretion (recommend clamping to audible range 20Hz-20kHz)

### Claude's Discretion
- Exact knob taper curve (log vs exponential vs custom)
- Center position behavior for attenuverter
- CV scaling formula (V/Oct vs linear)
- Whether to clamp cutoff to audible range
- Parameter smoothing behavior (already exists from Phase 2, may need tuning)

</decisions>

<specifics>
## Specific Ideas

- Filter should start "open" (20kHz cutoff) so patching in audio produces immediate output
- Attenuverter at center means user must consciously enable CV modulation
- Wide CV range (+/-10V) allows LFOs to sweep the full spectrum

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 05-cutoff-control*
*Context gathered: 2026-01-31*
