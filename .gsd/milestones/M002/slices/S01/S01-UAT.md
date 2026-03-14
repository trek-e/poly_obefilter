# S01: Filter Type CV with Per-Voice Crossfade — UAT

**Milestone:** M002
**Written:** 2026-03-13

## UAT Type

- UAT mode: mixed
- Why this mode is sufficient: Build and code invariants are verified by automated checks. Audio-rate crossfade behavior and click-free transitions can only be verified by human listening in a running VCV Rack session.

## Preconditions

- VCV Rack 2.x installed and running on macOS
- HydraQuartet VCF-OB module built with `make install` (plugin installed to VCV Rack plugins directory)
- A polyphonic oscillator module available (e.g., VCV VCO or Bogaudio VCO)
- A gate/trigger source available (e.g., VCV GATE, clock module, or MIDI-CV)
- An LFO module available (e.g., VCV LFO)
- A polyphonic merge/split module available for polyphonic cable testing
- Headphones or monitors connected for audio monitoring

## Smoke Test

1. Launch VCV Rack → right-click → search "HydraQuartet VCF-OB" → add to patch.
2. Confirm the module loads without crash and the new "CV" jack is visible in the filter section, below the 12/24 toggle switch.
3. Patch a polyphonic oscillator → VCF-OB audio input → VCF-OB LP output → Audio output. Confirm sound passes through.

## Test Cases

### 1. Gate toggles filter type via CV

1. Set filter type switch to 12dB (down position).
2. Patch a gate source (e.g., clock at ~1Hz) to the new Filter Type CV input.
3. Listen to the LP output.
4. **Expected:** When gate goes high (>2.6V), filter switches to 24dB OB-X character — audible timbre change (darker, more resonant). When gate goes low (<2.4V), filter switches back to 12dB SEM. Transitions should be smooth — no clicks, pops, or discontinuities.

### 2. CV overrides panel switch when connected

1. With a gate still patched to Filter Type CV, toggle the panel switch between 12dB and 24dB positions.
2. **Expected:** Panel switch has no audible effect — the gate signal controls filter type regardless of switch position.

### 3. Panel switch resumes control when CV disconnected

1. Disconnect the cable from the Filter Type CV input.
2. Toggle the panel switch between 12dB and 24dB.
3. **Expected:** Panel switch controls filter type again — audible timbre change when toggling. Transitions are click-free.

### 4. LFO modulation of filter type

1. Patch an LFO (unipolar 0-10V, ~2Hz) to Filter Type CV.
2. Listen to LP output.
3. **Expected:** Filter type rhythmically alternates between 12dB and 24dB. Transitions are smooth. No clicks or chatter — the Schmitt trigger hysteresis prevents rapid oscillation near the threshold.

### 5. Polyphonic per-voice independence

1. Set up a 4-voice polyphonic patch: polyphonic oscillator → VCF-OB.
2. Use a Merge module to create a 4-channel polyphonic gate where channels 1 and 3 are high (e.g., 5V) and channels 2 and 4 are low (0V).
3. Patch this polyphonic gate to Filter Type CV.
4. Listen to individual voices (use Split module on VCF-OB LP output).
5. **Expected:** Voices 1 and 3 have 24dB OB-X character. Voices 2 and 4 have 12dB SEM character. Each voice operates independently.

### 6. Click-free crossfade on rapid switching

1. Patch a fast clock or square LFO (~20Hz) to Filter Type CV.
2. Set resonance to ~50% and cutoff to ~1kHz to make timbre changes prominent.
3. Listen carefully.
4. **Expected:** Rapid switching produces smooth alternation between filter characters. No clicks, pops, or glitches — the 128-sample crossfade handles the transitions even at this rate.

## Edge Cases

### Schmitt trigger hysteresis — slow ramp through threshold

1. Patch a very slow triangle LFO (0.1Hz, 0-5V range) to Filter Type CV.
2. Listen as the voltage slowly crosses the 2.4V–2.6V hysteresis band.
3. **Expected:** Exactly one clean transition as voltage rises through 2.6V, and one clean transition as it falls through 2.4V. No chatter or rapid toggling in the hysteresis band.

### Monophonic CV to polyphonic audio

1. Patch a polyphonic oscillator (4+ voices) to VCF-OB.
2. Patch a monophonic gate to Filter Type CV.
3. **Expected:** All voices switch filter type simultaneously (monophonic CV is broadcast to all channels per VCV Rack convention). No voice gets stuck in the wrong mode.

### No CV connected — default behavior preserved

1. Ensure no cable is connected to Filter Type CV.
2. Use the module with panel switch, cutoff, resonance, drive — full normal operation.
3. **Expected:** Module behaves identically to pre-S01 behavior. No regressions in existing functionality.

## Failure Signals

- Audible clicks, pops, or discontinuities during filter type transitions → crossfade regression
- Filter type not changing when gate goes high/low → Schmitt trigger thresholds wrong or CV input not wired
- Panel switch still affecting sound when CV is connected → override logic broken
- Panel switch not working when CV is disconnected → fallback logic broken
- All voices switching together when polyphonic CV has different per-channel values → per-voice arrays not indexed correctly
- Module crash on load → SVG or widget code mismatch (check jack coordinates)
- Jack not visible on panel → SVG missing or coordinates misaligned
- Build warnings or errors → `make -j4 2>&1 | grep -E "warning|error"`

## Requirements Proved By This UAT

- CTRL-06 — Test cases 1, 4, 5, 6 prove CV-driven filter type switching with polyphonic per-voice behavior
- CTRL-10 — Test cases 2 and 3 prove CV override and switch fallback behavior
- CTRL-11 — Test cases 1 and edge case "slow ramp through threshold" prove Schmitt trigger hysteresis prevents chatter

## Not Proven By This UAT

- CTRL-08 (FM input) — S02 scope
- CTRL-09 (1V/Oct pitch tracking) — S02 scope
- CTRL-12 (FM bypasses smoothing) — S02 scope
- CTRL-13 (1V/Oct logarithmic sum) — S02 scope
- Crossfade behavior at extreme sample rates (22kHz, 192kHz) — edge case noted in summary, not critical for v0.70b

## Notes for Tester

- The crossfade window is 128 samples (~2.7ms at 48kHz). Listen for it at transitions — you should hear a brief, smooth morph between filter characters, not an instant switch and not a long fade.
- The 24dB mode only outputs LP — HP/BP/Notch outputs go silent. This is expected behavior (authentic OB-Xa). When testing, monitor the LP output for timbre changes.
- If using a scope module, patch it to LP output and watch for waveform discontinuities during transitions. Smooth = pass, sharp transient = fail.
- The "CV" label is intentionally terse — it matches the labeling convention of the existing cutoff/resonance/drive CV inputs on this module.
