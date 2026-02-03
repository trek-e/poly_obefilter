# Phase 6: Resonance Control - Context

**Gathered:** 2026-02-03
**Status:** Ready for planning

<domain>
## Phase Boundary

Add user-accessible resonance control with CV modulation to the existing filter. The resonance DSP already exists from Phase 2 (Q range 0.5-20, soft saturation in feedback path). This phase exposes that functionality to panel controls and CV input. Self-oscillation character will be tunable via Drive control in Phase 7.

</domain>

<decisions>
## Implementation Decisions

### Resonance Range & Character
- Q range: 0.5 to 20 (keep current DSP implementation)
- Self-oscillation character: clean at max resonance, Drive (Phase 7) will add aggression
- Default resonance: 0 (Q=0.5, no emphasis) — flat response on module load

### CV Modulation Behavior
- Attenuverter: Yes, include attenuverter knob for resonance CV (mirror cutoff design)
- CV scaling: 10% of range per volt (matches cutoff CV scaling)
- Knob + CV combination: Additive (knob position + scaled CV)
- Bipolar CV: Yes — negative CV can reduce resonance below knob position (clamped at 0)

### Panel Layout
- Resonance knob position: Below cutoff knob (vertical stack, classic filter layout)
- CV jack layout: Mirror cutoff — CV jack with small attenuverter knob nearby
- Consistency: Match cutoff's visual pattern for cohesive control section

### Claude's Discretion
- Knob size: Claude decides based on panel balance (same or slightly smaller than cutoff)
- Label style: Claude picks for consistency with existing panel labels (RES, RESONANCE, or Q)
- Output limiting: Claude decides whether to soft-limit at extreme resonance
- CV clamping: Claude decides whether to hard clamp or allow slight overshoot
- Resonance compensation at frequency extremes: Claude decides based on analog filter behavior
- Parameter smoothing: Claude decides smoothing time (same as cutoff 1ms or faster)

</decisions>

<specifics>
## Specific Ideas

- Resonance should feel musical and usable as a sound design tool, not just a technical parameter
- Self-oscillation should produce a clean sine-like tone suitable for use as an oscillator (Drive phase will add character)
- CV behavior should match cutoff for consistency — users learn one, they know both

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 06-resonance-control*
*Context gathered: 2026-02-03*
