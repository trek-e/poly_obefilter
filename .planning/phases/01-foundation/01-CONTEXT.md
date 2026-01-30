# Phase 1: Foundation - Context

**Gathered:** 2026-01-29
**Status:** Ready for planning

<domain>
## Phase Boundary

Plugin scaffold, panel design, and build infrastructure for HydraQuartet VCF-OB. This phase establishes the module shell that compiles and loads in VCV Rack with a complete panel design showing all control positions. DSP implementation begins in Phase 2.

</domain>

<decisions>
## Implementation Decisions

### Panel Layout
- Controls grouped by function (not top-to-bottom flow)
- Filter outputs (LP/HP/BP/Notch) in a clearly labeled "Outputs" section
- CV inputs adjacent to their target knobs (cutoff CV near cutoff knob)
- Audio input at top left (VCV convention)
- Sections separated by lines/dividers
- Minimal labels — only label sections, individual controls obvious from context
- Medium-sized knobs (standard VCV size)
- Fresh design, no specific module reference

### Visual Style
- Match HydraQuartet VCO exactly — same colors, fonts, divider style
- Color palette:
  - Background: `#1a1a2e` (dark blue-grey)
  - Border: `#3a3a5e` (muted purple-grey)
  - Primary text: `#8888aa` (cool lavender)
  - Accents: soft blue, muted teal, dusty brown, mauve
- Filter outputs section uses dusty orange (`#aa8866` range) instead of VCO's ocean blue
- Color-coded labels matching VCO's function-based color mapping
- Module title: "HYDRAQUARTET VCF-OB" (full product name)
- Sans-serif fonts, hierarchical sizing
- Dark industrial aesthetic, vintage electronic equipment feel

### Module Metadata
- Tags: Filter, Polyphonic, VCF, Multimode
- Description: "8-voice Oberheim-style multimode filter"
- Plugin slug: Synth-eticIntelligence
- Author: Synth-etic Intelligence
- License: GPL-3.0 (matching VCO)
- Version: 0.5.0 (semantic versioning)
- Manual URL: GitHub README

### Project Structure
- Standard VCV Rack template structure
- Filter DSP in separate file from module class (for reusability)
- PascalCase for module files (HydraQuartetVCF.cpp), lowercase for standard files (plugin.cpp)
- Panel designed in Inkscape

### Claude's Discretion
- Exact spacing and typography details within the style guidelines
- Component widget choices (standard VCV components)
- Makefile configuration details

</decisions>

<specifics>
## Specific Ideas

- Panel should feel cohesive with HydraQuartet VCO — same product family
- Reference: https://github.com/trek-e/HydraQuartet for VCO panel style
- The dusty orange for filter outputs gives it a "warm analog" feel fitting the Oberheim inspiration

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 01-foundation*
*Context gathered: 2026-01-29*
