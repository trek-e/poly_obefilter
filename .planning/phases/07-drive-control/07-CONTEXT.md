# Phase 7: Drive Control - Context

**Gathered:** 2026-02-03
**Status:** Ready for planning

<domain>
## Phase Boundary

Add drive/saturation control for filter character. This is the final phase of v0.50b milestone. The saturation adds harmonic content and compression to the filtered signal, giving the Oberheim filter its characteristic warmth and grit. The existing tanh saturation in the feedback path provides a starting point.

</domain>

<decisions>
## Implementation Decisions

### Saturation Character
- Saturation flavor: Blend of soft (tanh) and harder asymmetric edge
- Max drive effect: Thick & compressed — heavy saturation, sounds fat and sustaining
- Harmonic content: Mix of even and odd harmonics from blended saturation types

### Output Routing
- LP/BP outputs: More drive effect (low-frequency content saturates more)
- HP output: Less drive effect (high-frequency content less affected)
- Different saturation amounts per output type for balanced tone

### Control Behavior
- Default value: 0 (clean) — no saturation until user adds drive intentionally
- Knob curve: Linear — even drive increase across the range
- CV input: Yes, with attenuverter (mirrors cutoff/resonance pattern)

### Panel Layout
- Label: DRIVE
- Layout: Mirror cutoff/resonance pattern (knob + CV jack + attenuverter)
- Visual consistency with existing control sections

### Claude's Discretion
- Drive placement in signal path: Claude decides based on Oberheim filter topology
- Low drive transparency: Claude decides whether slight coloration at low settings
- Gain staging: Claude decides whether to preserve level or get louder with drive
- Drive/resonance interaction: Claude decides based on analog filter behavior
- Self-oscillation tone with drive: Claude decides based on Oberheim character
- CV scaling: Claude decides (10% per volt for consistency, or different for optimal feel)
- Knob position: Claude decides based on panel space and visual balance
- Knob size: Claude decides based on panel balance

</decisions>

<specifics>
## Specific Ideas

- Drive should enhance the Oberheim character — warm, fat, musical saturation
- At maximum drive, sound should be thick and compressed, not harsh or digital
- Blending soft and asymmetric saturation gives both warmth and some edge
- LP/BP getting more drive effect than HP makes tonal sense (bass content saturates naturally)

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 07-drive-control*
*Context gathered: 2026-02-03*
