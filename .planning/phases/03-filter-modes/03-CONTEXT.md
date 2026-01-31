# Phase 3: Filter Modes - Context

**Gathered:** 2026-01-31
**Status:** Ready for planning

<domain>
## Phase Boundary

Add highpass, bandpass, and notch outputs to the existing SVF. The trapezoidal state-variable filter already computes all four modes internally (v1=BP, v2=LP, HP derived) — this phase exposes the remaining three outputs through the existing panel jacks. Phase 2 only outputs lowpass; Phase 3 completes the multimode capability.

</domain>

<decisions>
## Implementation Decisions

### Output Configuration
- Four separate output jacks (LP, HP, BP, Notch) — always available simultaneously
- Use existing panel output positions from Phase 1 design
- Standard SVF notch calculation: HP + LP combined (not input minus BP)

### Claude's Discretion
- Output level matching across modes (natural vs normalized)
- Resonance behavior across outputs (equal vs LP emphasis)
- Self-oscillation behavior on all outputs (natural SVF behavior)
- Soft saturation application (all outputs or LP only)
- CPU optimization (compute all vs only connected outputs)
- Visual indicators (LEDs or clean panel — match HydraQuartet VCO style)
- Notch width control (resonance-dependent per SVF standard)
- Notch null depth at maximum resonance
- Notch frequency tracking (identical to other outputs)

</decisions>

<specifics>
## Specific Ideas

- The SVF topology naturally provides all four outputs from the same computation — HP is typically `v0 - k*v1 - v2` and notch is `HP + LP`
- This is primarily wiring up outputs that the filter already calculates, not new DSP
- User wants standard/authentic SVF behavior rather than custom tweaks

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 03-filter-modes*
*Context gathered: 2026-01-31*
