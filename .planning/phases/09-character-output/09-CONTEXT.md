# Phase 9: Character & Output - Context

**Gathered:** 2026-02-21
**Status:** Ready for planning

<domain>
## Phase Boundary

OB-X sonic character tuning for 24dB mode, lowpass-only output routing in 24dB mode (HP/BP/Notch silent), and click-free mode switching polish. Requirements: FILT-08, FILT-09, TYPE-03. Does NOT add new outputs, new controls, or new filter modes.

</domain>

<decisions>
## Implementation Decisions

### OB-X Sonic Character
- The two modes should sound **dramatically different** — like switching between two different synth architectures, matching the contrast of original SEM vs OB-X hardware
- Reference sound: classic OB-Xa basses (fat, biting bass — think Jump/Van Halen, Depeche Mode). Thick low end with aggressive resonance bite
- Source of the bright/edgy character: Claude's discretion (resonance emphasis, saturation, or both — whatever produces the most convincing OB-X)
- Character at low resonance (0-20%): Claude's discretion (natural result of the topology)

### Silent Output Behavior
- How HP/BP/Notch go silent in 24dB: Claude's discretion (whatever avoids clicks)
- Port channel reporting in 24dB: Claude's discretion (active at 0V vs 0 channels — follow VCV conventions)
- When switching back from 24dB to 12dB: HP/BP/Notch **fade back in** over the crossfade window (symmetric with fade-out)
- Visual feedback for inactive outputs: Claude's discretion (based on panel aesthetics and VCV conventions)

### Drive Tuning Across Modes
- Saturation curve in 24dB (different algorithm vs same curve at 1.3x gain): Claude's discretion (whatever produces most convincing OB-X)
- Aggressiveness at maximum drive in 24dB: Claude's discretion (sweet spot for OB-X character)
- Drive at 0%: Claude's discretion (completely clean vs hint of color — whatever is most authentic)
- High drive + high resonance behavior in 24dB: Claude's discretion (balance stability and character)

### Claude's Discretion
- Edge source in 24dB (resonance shape, saturation curve, frequency tilt, or combination)
- Low-resonance character difference between modes
- Saturation algorithm choice for 24dB mode
- Drive aggressiveness range and zero-drive behavior
- Drive + resonance interaction at extremes
- HP/BP/Notch silence method and fade timing
- Port channel behavior in 24dB mode
- Any visual cues for inactive outputs

</decisions>

<specifics>
## Specific Ideas

- Reference: classic OB-Xa basses — fat, biting, thick low end, aggressive resonance bite (Van Halen "Jump", Depeche Mode)
- The two modes should feel like genuinely different synth architectures, not just a slope change
- HP/BP/Notch must fade back in symmetrically when returning to 12dB mode (user explicitly chose this)

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 09-character-output*
*Context gathered: 2026-02-21*
