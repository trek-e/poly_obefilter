# Phase 8: Core 24dB Cascade - Context

**Gathered:** 2026-02-21
**Status:** Ready for planning

<domain>
## Phase Boundary

Add a cascaded SVF topology producing 24dB/oct lowpass rolloff, with a panel switch to select between the existing 12dB SEM mode and the new 24dB OB-X mode. Cutoff must track consistently between modes. Self-oscillation must work in 24dB mode. Requirements: FILT-06, FILT-07, TYPE-01, TYPE-02.

</domain>

<decisions>
## Implementation Decisions

### Switch Placement & Style
- 2-position toggle switch (not 3-position; no reserved future position)
- Labeled "12dB / 24dB" (technical labels, not model names)
- No LED or visual indicator — switch position alone shows active mode
- Placement: Claude's discretion based on 14HP panel layout

### Resonance Character
- Resonance Q range/mapping in 24dB mode: Claude's discretion (pick what sounds most musical and stays stable)
- Self-oscillation should be warm and slightly distorted — harmonic richness from cascade saturation, not a clean sine
- Bass thins out naturally at high resonance (classic analog 4-pole behavior, authentic to OB-X)
- Drive runs slightly hotter in 24dB mode by default — same knob range, higher internal gain for OB-X edge
- Resonance peak in 24dB mode should be louder/more aggressive than 12dB — you hear the mode change
- Tonal difference at moderate resonance: Claude's discretion (natural result of cascade topology)
- Self-oscillation pitch tracking accuracy: Claude's discretion (whatever the cascade achieves naturally)
- Mode switching must be click-free even in Phase 8 — no transients or pops when switching with audio running

### Output Behavior in 24dB Mode
- What HP/BP/Notch outputs produce in 24dB mode: Claude's discretion (Phase 9 finalizes LP-only routing)
- LP output level: natural level difference between modes (no gain matching)
- 12dB mode may receive minor improvements if they benefit overall quality (not required to be bit-identical to v0.50b)
- Cutoff frequency matching between modes: Claude's discretion (balance accuracy vs complexity)

### Claude's Discretion
- Switch placement on panel (wherever fits best with 14HP layout)
- Internal Q mapping/rescaling for 24dB mode
- Tonal difference magnitude at moderate resonance settings
- Self-oscillation pitch tracking precision
- Interim output routing for HP/BP/Notch in 24dB mode
- Cutoff frequency matching tolerance between modes

</decisions>

<specifics>
## Specific Ideas

- Self-oscillation character inspired by OB-X: warm distortion from the cascade, not sterile
- 24dB mode should feel edgier and more aggressive than 12dB — louder resonance peak, hotter drive, bass scoop at high Q
- Click-free switching is non-negotiable even in the initial implementation

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 08-core-24db-cascade*
*Context gathered: 2026-02-21*
