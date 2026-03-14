# M002: v0.70b CV Control — Context

**Gathered:** 2026-03-13
**Status:** Ready for planning

## Project Description

CIPHER · OB is a polyphonic 16-voice multimode filter module for VCV Rack 2.x. v0.60b shipped with dual filter modes (12dB SEM / 24dB OB-X) selectable via a panel toggle switch. The module currently has CV inputs for cutoff, resonance, and drive — all with attenuverters. The filter type switch and the lack of FM/pitch-tracking inputs are the remaining control gaps.

## Why This Milestone

The module's filter type can only be changed by physically clicking the panel switch. In a modular environment, users expect to automate and modulate everything via CV. Additionally, FM synthesis and pitch-tracking filtering are standard capabilities that require dedicated inputs with proper scaling. Without these, the module is less useful in patches where filter type needs to change dynamically (e.g., sequenced filter type switching, LFO-driven mode morphing) or where the filter needs to track oscillator pitch.

## User-Visible Outcome

### When this milestone is complete, the user can:

- Send a CV signal to switch between 12dB SEM and 24dB OB-X modes automatically
- Use a gate/trigger or LFO to rhythmically toggle filter types in a patch
- Connect FM sources (LFO, envelope, second oscillator) to modulate cutoff at audio rate with an attenuverter for depth control
- Feed 1V/Oct pitch CV from a keyboard/sequencer so the filter tracks pitch accurately
- All new CV inputs work polyphonically (per-voice modulation) consistent with existing CV inputs

### Entry point / environment

- Entry point: VCV Rack 2.x on macOS — add module from browser, patch CV cables to new inputs
- Environment: local dev — build with `make`, copy to VCV Rack plugins directory
- Live dependencies involved: none

## Completion Class

- Contract complete means: module compiles with zero warnings, new inputs produce expected modulation when patched
- Integration complete means: all four new CV inputs work alongside existing cutoff/resonance/drive CV in a polyphonic patch
- Operational complete means: none (desktop audio plugin, no services)

## Final Integrated Acceptance

To call this milestone complete, we must prove:

- A patched VCV Rack session where filter type toggles via CV, FM modulates cutoff, and pitch tracking follows a sequencer — all simultaneously with existing CV inputs
- Build produces zero errors and zero warnings on macOS

## Risks and Unknowns

- Panel space: 14 HP panel already has 4 inputs, 4 outputs, 6 knobs, 1 switch — adding 3-4 more inputs plus attenuverters may be tight
- Filter type CV interaction with panel switch: needs clear priority logic (CV overrides switch? additive? threshold-based?)
- FM input at audio rate may need anti-aliasing or will produce aliasing artifacts at high FM frequencies
- 1V/Oct tracking — the existing cutoff CV already uses V/Oct scaling via `pow(2, cv * cvAmount)`. The attenuverter scales it. A dedicated 1V/Oct input should bypass attenuation for true pitch tracking.

## Existing Codebase / Prior Art

- `src/CipherOB.cpp` — 258 LOC, all module logic including process() with 12dB/24dB branches
- `src/SVFilter.hpp` — 133 LOC, SVFilter struct and blendedSaturation
- `res/CipherOB.svg` — 14 HP panel SVG
- Existing CV pattern: 10%/volt scaling with attenuverter for resonance and drive; V/Oct exponential scaling with attenuverter for cutoff

> See `.gsd/DECISIONS.md` for all architectural and pattern decisions — it is an append-only register; read it during planning, append to it during execution.

## Relevant Requirements

- CTRL-06 — CV input for filter type switching (new input)
- CTRL-07 — CV input for mode selection (new input — note: "mode" may mean LP/HP/BP/Notch selection, distinct from "type" 12dB/24dB)
- CTRL-08 — FM input with attenuverter (new input + knob)
- CTRL-09 — 1V/Oct pitch tracking input (new input, no attenuverter — true tracking)

## Scope

### In Scope

- CV input for filter type switching (12dB ↔ 24dB) with threshold logic
- FM input with attenuverter for audio-rate cutoff modulation
- 1V/Oct pitch tracking input (dedicated, unattenuated)
- Panel SVG updates for new components
- Click-free behavior when CV triggers type switching (reuse existing crossfade)

### Out of Scope / Non-Goals

- CV input for mode selection (CTRL-07) — requires clarification on what "mode" means. In 12dB mode all 4 outputs are always active simultaneously; there's no mode to select. This may only make sense if we add a "selected output" jack. Defer unless user clarifies.
- Mixed/selected output jack — v0.90b scope
- Per-voice control — v0.80b scope
- SIMD optimization — v0.90b scope
- Panel size change — stay at 14 HP

## Technical Constraints

- Must remain at 14 HP panel width
- All CV inputs must be polyphonic (up to 16 channels)
- Filter type CV must interact cleanly with existing panel switch
- FM input scaling must be compatible with audio-rate signals
- Zero new compiler warnings in both Debug and Release builds

## Integration Points

- Filter type switch: CV must coexist with CKSS panel switch — likely CV threshold overrides switch value
- Crossfade state machine: type switching via CV must trigger the same 128-sample crossfade used by panel switch
- Existing cutoff CV: FM input adds to cutoff frequency alongside existing cutoff CV (they're different modulation types)

## Open Questions

- Should filter type CV use a gate threshold (>2.5V = 24dB, <2.5V = 12dB) or should it add to the switch value? Gate threshold is simpler and more predictable.
- Should CTRL-07 (mode selection CV) be deferred entirely or reinterpreted as something useful in the current architecture? Currently all 4 outputs are always available — there's no mode to select per se.
- FM input: linear FM or exponential FM? VCV convention for filter FM is usually exponential (V/Oct scaled). Linear FM is more useful for audio-rate but less common on filters.
- 1V/Oct input: should it sum with or replace the cutoff knob? Standard VCV practice is to sum (knob sets base pitch, 1V/Oct offsets it).
