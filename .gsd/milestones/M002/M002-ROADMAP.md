# M002: v0.70b CV Control

**Vision:** The filter's remaining control gaps are filled — filter type, FM, and pitch tracking are all CV-controllable, making the module fully patchable in a modular environment.

## Success Criteria

- A CV signal above threshold switches the filter from 12dB SEM to 24dB OB-X; below threshold switches it back — with click-free crossfade on every transition, per-voice
- An LFO or gate patched to filter type CV rhythmically toggles filter types without clicks, pops, or chatter near the threshold
- An audio-rate FM signal patched to the FM input audibly modulates cutoff frequency (not killed by parameter smoothing)
- FM depth attenuverter scales modulation from zero to full depth, including inverted
- A 1V/Oct signal from a keyboard/sequencer makes the filter track pitch accurately — filter resonant peak follows the played note
- All three new CV inputs work polyphonically (per-voice modulation, up to 16 channels)
- The panel switch still works when filter type CV is disconnected; CV overrides the switch when connected
- Build produces zero errors and zero warnings on macOS

## Key Risks / Unknowns

- Per-voice crossfade refactoring — the crossfade state machine is currently global (`crossfadeCounter` single int, `prevFilterType` single int). CV-driven type switching means each voice can switch independently. Refactoring to per-voice arrays touches the most existing code and could introduce clicks.
- FM killed by parameter smoothing — SVFilter's `cutoffSmoother` (1ms tau) will attenuate FM above ~160Hz. FM must bypass smoothing or be applied after it.
- Schmitt trigger hysteresis — simple threshold without hysteresis will chatter when CV hovers near 2.5V. Need ~0.1V hysteresis band.

## Proof Strategy

- Per-voice crossfade → retire in S01 by building filter type CV with per-voice crossfade arrays and verifying clean build + correct type switching in VCV Rack
- FM smoothing bypass → retire in S02 by applying FM after cutoff smoothing and verifying audio-rate modulation is audible in VCV Rack

## Verification Classes

- Contract verification: `make` produces zero errors and zero warnings
- Integration verification: patching CV cables to new inputs in a running VCV Rack session — filter type toggles via CV, FM modulates cutoff at audio rate, 1V/Oct tracks pitch alongside existing cutoff/resonance/drive CV
- Operational verification: none (desktop audio plugin)
- UAT / human verification: listen for clicks on filter type CV switching, verify FM is audible at audio rate, verify pitch tracking by ear

## Milestone Definition of Done

This milestone is complete only when all are true:

- All new params/inputs are appended to their enums (never inserted — patch compatibility)
- Filter type CV overrides panel switch when connected, with Schmitt trigger hysteresis
- Per-voice crossfade arrays replace global crossfade counter — each voice switches independently without clicks
- FM input with attenuverter modulates cutoff at audio rate (bypasses parameter smoothing)
- 1V/Oct input sums with cutoff in logarithmic domain for true pitch tracking
- All three new CV inputs handle polyphonic cables (up to 16 channels)
- `make` produces zero errors and zero warnings
- Panel SVG updated with all new components positioned within 14 HP

## Requirement Coverage

- Covers: CTRL-06 (filter type CV), CTRL-08 (FM input), CTRL-09 (1V/Oct), CTRL-10 (CV overrides switch), CTRL-11 (Schmitt hysteresis), CTRL-12 (FM bypasses smoothing), CTRL-13 (1V/Oct logarithmic sum)
- Deferred: CTRL-07 (mode selection CV) — architecture doesn't support mode selection since all 4 outputs are always active in 12dB mode. Revisit when mixed/selected output jack is added in v0.90b.
- Orphan risks: none

## Slices

- [x] **S01: Filter Type CV with Per-Voice Crossfade** `risk:high` `depends:[]`
  > After this: a CV signal (gate, LFO, sequencer) patched to the new Filter Type CV input switches between 12dB SEM and 24dB OB-X per-voice with click-free crossfade. Panel switch still works when CV is disconnected. Module builds with zero warnings.
- [ ] **S02: FM Input and 1V/Oct Pitch Tracking** `risk:medium` `depends:[S01]`
  > After this: an audio-rate FM source patched to the FM input audibly modulates cutoff with adjustable depth, and a 1V/Oct CV makes the filter track pitch from a keyboard/sequencer. All new inputs work polyphonically alongside existing CV inputs. Module builds with zero warnings.

## Boundary Map

### S01 → S02

Produces:
- Per-voice crossfade arrays (`crossfadeCounter[PORT_MAX_CHANNELS]`, `prevFilterType[PORT_MAX_CHANNELS]`) replacing global state — S02 can add new CV inputs without touching crossfade logic
- `FILTER_TYPE_CV_INPUT` enum entry appended to `InputId` — S02 appends FM and V/Oct entries after it
- Schmitt trigger state arrays (`filterTypeHigh[PORT_MAX_CHANNELS]`) — established pattern for per-voice boolean state
- Panel SVG with filter type CV jack positioned — S02 adds FM and 1V/Oct jacks to remaining space

Consumes:
- nothing (first slice)
