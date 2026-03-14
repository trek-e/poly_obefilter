# S01: Filter Type CV with Per-Voice Crossfade

**Goal:** A CV signal patched to a new Filter Type CV input switches between 12dB SEM and 24dB OB-X per-voice with click-free crossfade. Panel switch still works when CV is disconnected.
**Demo:** Patch a gate or LFO to Filter Type CV input → filter type toggles per-voice without clicks. Unplug CV cable → panel switch controls filter type as before. `make` produces zero errors and zero warnings.

## Must-Haves

- `FILTER_TYPE_CV_INPUT` appended to `InputId` enum (never inserted — patch compatibility)
- Per-voice crossfade arrays (`crossfadeCounter[PORT_MAX_CHANNELS]`, `prevFilterType[PORT_MAX_CHANNELS]`) replace global `crossfadeCounter` and `prevFilterType`
- Per-voice Schmitt trigger state (`filterTypeHigh[PORT_MAX_CHANNELS]`) with 2.6V rising / 2.4V falling thresholds
- CV overrides panel switch when `FILTER_TYPE_CV_INPUT` is connected; switch works normally when disconnected
- Crossfade-from values captured per-voice at the moment each voice's filter type changes (not globally)
- Polyphonic CV: each channel of a polyphonic cable independently controls its voice's filter type
- Panel SVG updated with filter type CV jack positioned within 14 HP
- `make` produces zero errors and zero warnings

## Proof Level

- This slice proves: integration (real runtime in VCV Rack)
- Real runtime required: yes (audio-rate crossfade behavior only verifiable by listening)
- Human/UAT required: yes (listen for clicks on filter type transitions)

## Verification

- `make -j4 2>&1 | grep -E "warning|error"` produces no output (zero warnings, zero errors)
- Module loads in VCV Rack without crash
- Patching a gate to Filter Type CV toggles filter type (audible change in timbre)
- Unplugging Filter Type CV cable → panel switch resumes control
- Polyphonic gate cable → different voices can be in different filter modes simultaneously

## Tasks

- [x] **T01: Per-voice crossfade arrays, Schmitt trigger, and CV override logic** `est:1h`
  - Why: The crossfade state machine is global (single `crossfadeCounter`, single `prevFilterType`). CV-driven switching means each voice switches independently. This refactors all crossfade state to per-voice arrays, adds the CV input with Schmitt trigger hysteresis, and wires the CV override logic. This is the core risk of the slice.
  - Files: `src/CipherOB.cpp`
  - Do: (1) Append `FILTER_TYPE_CV_INPUT` to `InputId` before `INPUTS_LEN`. (2) Replace global `prevFilterType` and `crossfadeCounter` with per-voice arrays `prevFilterType[PORT_MAX_CHANNELS]` and `crossfadeCounter[PORT_MAX_CHANNELS]`. (3) Add per-voice Schmitt trigger state array `filterTypeHigh[PORT_MAX_CHANNELS]`. (4) In `process()`, determine `filterType` per-voice: if CV connected, read per-voice CV and apply Schmitt trigger (2.6V rising, 2.4V falling); if CV disconnected, use panel switch value. (5) Move mode-change detection and crossfade-from capture inside the per-voice loop, keyed on per-voice `prevFilterType[c]`. (6) Make crossfade counter per-voice: `crossfadeCounter[c]` decrements independently. (7) Update `prevFilterType[c]` per-voice after processing each channel. (8) Add `configInput` and `addInput` for the new CV input. Ensure all arrays are zero-initialized.
  - Verify: `make -j4 2>&1 | grep -E "warning|error"` produces empty output
  - Done when: `make` succeeds with zero warnings, per-voice crossfade arrays replace all global crossfade state, Schmitt trigger logic is implemented with correct thresholds

- [x] **T02: Panel SVG — add filter type CV jack** `est:30m`
  - Why: The module needs a visible jack on the panel for the new CV input. Without it the input exists in code but has no physical port to patch into.
  - Files: `res/CipherOB.svg`
  - Do: (1) Read existing SVG to understand component positions and 14 HP grid. (2) Add a jack circle for Filter Type CV near the existing filter type switch (Vec 24.0, 22.0). Position the new jack below the switch with adequate spacing (~10mm). (3) Add a label ("TYPE" or "TYPE CV") near the jack. (4) Ensure the jack position in SVG matches the `addInput` mm2px coordinates from T01. (5) Verify no overlap with existing components.
  - Verify: `make -j4` still succeeds; module loads in VCV Rack with new jack visible and correctly positioned
  - Done when: Panel SVG shows filter type CV jack, jack aligns with widget code coordinates, no component overlap, module loads cleanly in VCV Rack

## Observability / Diagnostics

- **Build-time:** `make -j4 2>&1 | grep -E "warning|error"` — zero output means clean build. Any output is a failure signal.
- **Code-level invariants:** `grep -n "prevFilterType\b\|crossfadeCounter\b" src/CipherOB.cpp` — every match must be an array declaration or `[c]` indexed access. Bare scalar access = regression.
- **Runtime inspection:** In VCV Rack, right-click module → inspect port tooltips. `FILTER_TYPE_CV_INPUT` should show "Filter Type CV" and display per-channel voltage when connected.
- **Crossfade audibility:** Mode transitions should produce smooth timbre change over ~2.7ms. Audible clicks on transition = crossfade regression.
- **Failure visibility:** If a voice's Schmitt trigger state is wrong, it manifests as that voice stuck in one filter mode while others switch. Polyphonic scope on the output reveals per-voice behavior.
- **No redaction constraints:** This module handles no PII or secrets — all state is DSP parameters.

## Files Likely Touched

- `src/CipherOB.cpp`
- `res/CipherOB.svg`
