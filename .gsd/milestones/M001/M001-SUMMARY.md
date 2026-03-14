---
id: M001
provides:
  - OB-X style 24dB/oct cascaded SVF filter mode alongside existing 12dB SEM
  - Panel toggle switch for 12dB SEM / 24dB OB-X mode selection
  - 128-sample crossfade for click-free mode switching on all four outputs
  - OB-X character tuning (1.15x resonance boost, inter-stage tanh saturation)
  - LP-only output routing in 24dB mode (HP/BP/Notch silent, authentic OB-Xa)
  - Analog noise floor dither for self-oscillation seeding from zero state
key_decisions:
  - "Cascaded dual-stage SVF for 24dB (not true 4-pole) — simpler, more stable"
  - "Stage2 Q at 0.7x then 0.65x of boosted resonance — balances stability vs character"
  - "24dB LP drive 1.3x — OB-X edge without separate drive knob"
  - "LP-only output in 24dB mode — authentic OB-Xa, HP/BP/Notch zeroed with crossfade"
  - "128-sample linear crossfade — handles both fade-to-zero and fade-from-zero symmetrically"
  - "Inter-stage tanh(x*0.12f)/0.12f — near-linear at normal levels, progressive harmonics at extremes"
patterns_established:
  - "Per-voice crossfade state machine pattern for click-free parameter changes"
  - "Deterministic hash-based dither (no stdlib rand) for analog noise floor"
observability_surfaces:
  - none
requirement_outcomes:
  - id: FILT-06
    from_status: active
    to_status: validated
    proof: "24dB cascade implemented in S08, builds and runs in VCV Rack"
  - id: FILT-07
    from_status: active
    to_status: validated
    proof: "Self-oscillation with dither injection, resonance24 boost enables it"
  - id: FILT-08
    from_status: active
    to_status: validated
    proof: "S09 tuned resonance24 1.15x boost + inter-stage tanh for OB-X character"
  - id: FILT-09
    from_status: active
    to_status: validated
    proof: "S09 zeroes HP/BP/Notch in 24dB branch, crossfade handles transitions"
  - id: TYPE-01
    from_status: active
    to_status: validated
    proof: "CKSS toggle switch at (55mm, 28mm), FILTER_TYPE_PARAM configSwitch"
  - id: TYPE-02
    from_status: active
    to_status: validated
    proof: "Both modes use same cutoff parameter and frequency calculation path"
  - id: TYPE-03
    from_status: active
    to_status: validated
    proof: "128-sample crossfade with per-voice from-value capture, verified click-free in S08/S09"
duration: ~1.5 hours across 9 slices
verification_result: passed
completed_at: 2026-02-21
---

# M001: v0.60b OB-X Filter

**Dual-mode Oberheim filter — 12dB SEM and 24dB OB-X cascaded SVF with click-free switching, OB-X character tuning, and LP-only authentic output routing**

## What Happened

Built incrementally from plugin foundation through DSP core to final character tuning across 9 slices. Slices 1–7 delivered the v0.50b beta: plugin scaffold, SEM-style 12dB SVF with LP/HP/BP/Notch simultaneous outputs, 16-voice polyphony, cutoff/resonance/drive CV control with attenuverters, and blended saturation. Slices 8–9 added the v0.60b feature set: a second SVF stage cascaded for 24dB/oct rolloff, a panel toggle switch, 128-sample crossfade for click-free mode switching, OB-X character tuning (resonance boost + inter-stage tanh saturation), and LP-only output routing in 24dB mode.

The final codebase is 391 LOC C++ across two files. Every slice built cleanly with zero warnings on first attempt. A late debugging session discovered a stale binary deployment issue; resolved with clean rebuild and added deterministic dither for self-oscillation seeding.

## Cross-Slice Verification

- **Panel switch**: CKSS widget at (55mm, 28mm) toggles FILTER_TYPE_PARAM between 0 (12dB) and 1 (24dB)
- **24dB rolloff**: Cascaded stage1→stage2 SVF produces -24dB/oct lowpass slope
- **OB-X character**: resonance24 = clamp(resonance * 1.15, 0, 0.95) with stage2 Q at 0.65x; inter-stage tanh(x*0.12f)/0.12f adds progressive harmonics
- **LP-only routing**: outHP = outBP = outNotch = 0.0f in 24dB branch
- **Click-free switching**: 128-sample linear crossfade captures per-voice from-values before mode change
- **Build verification**: Zero errors, zero warnings on macOS ARM64

## Requirement Changes

- FILT-06: active → validated — Cascaded SVF implemented and producing 24dB/oct rolloff
- FILT-07: active → validated — Self-oscillation works with dither injection at max resonance
- FILT-08: active → validated — OB-X sounds distinctly brighter/edgier than SEM mode
- FILT-09: active → validated — HP/BP/Notch zeroed in 24dB mode, crossfade handles transitions
- TYPE-01: active → validated — Panel CKSS switch selects between modes
- TYPE-02: active → validated — Same cutoff frequency in both modes (shared calculation path)
- TYPE-03: active → validated — 128-sample crossfade verified click-free

## Forward Intelligence

### What the next milestone should know
- The codebase is compact (391 LOC total) — easy to navigate, but adding CV mode/type switching (v0.70b) will grow the process() function significantly. Consider extracting the 12dB and 24dB branches into separate methods.
- Drive CV, resonance CV, and cutoff CV all use the same 10%/volt scaling pattern. New CV inputs should follow this convention.
- The CKSS switch is a configSwitch (integer parameter). CV control of filter type (v0.70b) will need to coexist with the panel switch — likely sum panel value + CV threshold.

### What's fragile
- Resonance stability in 24dB mode relies on precise Q ratios (resonance24 * 0.65 for stage2) — changing these without testing can cause blowup at max resonance
- The crossfade captures from-values from output ports via getVoltage() before overwriting — this coupling to port state is subtle and easy to break during refactoring
- Self-oscillation from zero state depends on the dither injection (~1µV) — removing it silences self-oscillation until audio passes through

### Authoritative diagnostics
- Build with `make` in project root — zero warnings is the baseline, any warning indicates regression
- `plugin.dylib` must be copied to `~/Library/Application Support/Rack2/plugins-mac-arm64/Synth-eticIntelligence/` for testing
- MD5 hash of deployed binary vs build binary catches stale deployment (root cause of Phase 9 debugging session)

### What assumptions changed
- Originally planned SIMD optimization (float_4) for Phase 5 — deferred to v0.90b. Scalar 16-voice processing is fast enough for now.
- Originally planned separate DSP files (SEMFilter.hpp, OBXFilter.hpp) — kept everything in SVFilter.hpp and CipherOB.cpp. Simpler for current scope.
- Resonance was assumed to work identically between modes — actually needed 1.15x boost in 24dB mode for comparable perceived resonance character.

## Files Created/Modified

- `src/CipherOB.cpp` — Main module: dual-mode process() with 12dB/24dB branches, crossfade state machine, all parameter handling (258 LOC)
- `src/SVFilter.hpp` — State-variable filter core: trapezoidal SVF with soft saturation (133 LOC)
- `res/CipherOB.svg` — Panel design: 14 HP with filter type switch, all controls and outputs
- `plugin.json` — Plugin manifest with module metadata and tags
- `README.md` — Build instructions and usage documentation
