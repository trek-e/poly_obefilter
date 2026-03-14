# Phase 8: Core 24dB Cascade - Research

**Researched:** 2026-02-21
**Domain:** Audio DSP — Cascaded State-Variable Filter with Panel Switch
**Confidence:** HIGH

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

- 2-position toggle switch (not 3-position; no reserved future position)
- Labeled "12dB / 24dB" (technical labels, not model names)
- No LED or visual indicator — switch position alone shows active mode
- Placement: Claude's discretion based on 14HP panel layout
- Self-oscillation should be warm and slightly distorted — harmonic richness from cascade saturation, not a clean sine
- Bass thins out naturally at high resonance (classic analog 4-pole behavior, authentic to OB-X)
- Drive runs slightly hotter in 24dB mode by default — same knob range, higher internal gain for OB-X edge
- Resonance peak in 24dB mode should be louder/more aggressive than 12dB — you hear the mode change
- Mode switching must be click-free even in Phase 8 — no transients or pops when switching with audio running
- LP output level: natural level difference between modes (no gain matching)
- 12dB mode may receive minor improvements if they benefit overall quality (not required to be bit-identical to v0.50b)

### Claude's Discretion

- Switch placement on panel (wherever fits best with 14HP layout)
- Internal Q mapping/rescaling for 24dB mode
- Tonal difference magnitude at moderate resonance settings
- Self-oscillation pitch tracking precision
- Interim output routing for HP/BP/Notch in 24dB mode
- Cutoff frequency matching tolerance between modes
- Resonance Q range/mapping in 24dB mode (pick what sounds most musical and stays stable)

### Deferred Ideas (OUT OF SCOPE)

None — discussion stayed within phase scope
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| FILT-06 | 24dB/oct OB-X style lowpass filter via cascaded SVF topology | Cascade of two existing SVFilter instances; stage1.lowpass feeds stage2.input; produces -24dB/oct rolloff; architecture fully documented in prior v0.60b research |
| FILT-07 | Self-oscillation at high resonance in 24dB mode | Achieved through sufficient k (resonance) in per-stage Q; tanh in feedback path controls amplitude; split resonance prevents instability; warm distortion character from inter-stage cascade saturation |
| TYPE-01 | Panel switch for 12dB SEM / 24dB OB-X filter type selection | configSwitch(FILTER_TYPE_PARAM) with CKSS widget; values 0=12dB/1=24dB; serialized automatically via VCV Rack param system |
| TYPE-02 | Cutoff frequency remains consistent between modes (1kHz stays 1kHz) | Both stages use identical cutoffHz; no compensation factor needed; same tan(pi * cutoffNorm) calculation feeds both stage1 and stage2 |
</phase_requirements>

---

## Summary

Phase 8 extends the existing 12dB trapezoidal SVF by cascading a second SVFilter instance in series to produce -24dB/oct rolloff. The complete architecture, code patterns, and pitfall mitigations were researched for the v0.60b milestone in February 2026 (see `.planning/research-v060b/`). Phase 8 research builds directly on that foundation and resolves four critical implementation questions specific to this phase: exact Q-splitting strategy for self-oscillation character, click-free switching mechanism, CKSS switch placement on the existing 14HP panel, and 24dB drive gain offset.

The core implementation is straightforward: add `SVFilter filters24dB_stage2[PORT_MAX_CHANNELS]`, add `FILTER_TYPE_PARAM` as a configSwitch, and in `process()` route `stage1.lowpass → stage2.input` when filterType == 1. The critical tuning decisions for self-oscillation character and resonance stability both come down to the per-stage Q split value — starting at 0.7x per stage is validated by prior research and matches the historical OB-X cascade. Click-free switching requires a short sample-level crossfade (64-256 samples) between the old and new filter output when filterType changes, using a persisted transition counter.

**Primary recommendation:** Implement cascade logic and configSwitch first, nail stability and self-oscillation character, then add crossfade click removal. Three focused implementation steps, all on existing infrastructure — zero new dependencies.

---

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| SVFilter (existing) | v0.50b | Both filter stages | Validated trapezoidal ZDF SVF; stable at all Q values; already proven in 16-voice polyphony |
| blendedSaturation (existing) | v0.50b | Drive saturation; inter-stage warmth in 24dB mode | Project-standard saturation; applying at lower drive between stages gives OB-X inter-stage compression |
| rack::dsp::TExponentialFilter | v0.50b (built into SVFilter) | Parameter smoothing per stage | Already smoothing cutoff and resonance inside SVFilter; no additional infrastructure needed |
| VCV Rack componentlibrary CKSS | Rack v2 SDK | 2-position panel toggle switch widget | Standard VCV Rack 2-position switch; loads CKSS_0.svg and CKSS_1.svg from system assets; inherits from SvgSwitch |
| configSwitch() | Rack v2 SDK | Parameter with labeled positions | Exposes "12dB SEM"/"24dB OB-X" labels in right-click menu; auto-serialized in patch; floor() for integer read |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| std::isfinite | C++ stdlib | NaN check at inter-stage boundary | After stage1.process() and before feeding into stage2; reset both stages if either is non-finite |
| rack::clamp | Rack v2 SDK | Inter-stage gain limiting | Clamp stage1.lowpass to ±12V before feeding stage2; prevents hard overflow from resonant peak |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| CKSS (2-pos toggle) | CKSSThree (3-pos) | 3-pos adds a reserved/unused third position; user decided 2-position only |
| Split resonance (0.7x/0.7x) | Global feedback around both stages | Global feedback more complex, potentially more authentic, but harder to tune and stabilize; defer |
| Short crossfade for click removal | Hard state reset on switch | State reset causes hard click; crossfade 64-256 samples is inaudible and simpler than full fade module |
| Separate OBXFilter.hpp class | Inline cascade logic in process() | Separate class adds indirection and hidden state; keep cascade logic visible in process() per prior research decision |

**Installation:** No new packages. All components are in the existing codebase or Rack v2 SDK.

---

## Architecture Patterns

### Project File Structure (no changes needed)

```
src/
├── plugin.hpp           # unchanged
├── plugin.cpp           # unchanged
├── SVFilter.hpp         # unchanged — reused for both stages
└── CipherOB.cpp  # ALL Phase 8 changes go here (+40-55 lines)
res/
└── CipherOB.svg  # Add CKSS switch rectangle near top of panel
```

### Pattern 1: Enum + configSwitch for Filter Type

**What:** Add `FILTER_TYPE_PARAM` to `ParamId` enum and configure it as a two-position switch.
**When to use:** Any time a parameter has discrete labeled values; VCV Rack right-click menu shows the labels automatically.

```cpp
// In CipherOB ParamId enum
enum ParamId {
    CUTOFF_PARAM,
    CUTOFF_ATTEN_PARAM,
    RESONANCE_PARAM,
    RESONANCE_ATTEN_PARAM,
    DRIVE_PARAM,
    DRIVE_ATTEN_PARAM,
    FILTER_TYPE_PARAM,    // NEW: 0 = 12dB SEM, 1 = 24dB OB-X
    PARAMS_LEN
};

// In constructor
configSwitch(FILTER_TYPE_PARAM, 0.f, 1.f, 0.f, "Filter Type", {"12dB SEM", "24dB OB-X"});

// In process() — integer read via floor (NOT round, to avoid 0.5 ambiguity)
int filterType = (int)std::floor(params[FILTER_TYPE_PARAM].getValue() + 0.5f);
```

### Pattern 2: Per-Voice Second Stage Array

**What:** Add a second SVFilter array alongside the existing one. Each voice gets its own independent stage2 state.
**When to use:** Any time polyphonic processing needs additional per-voice DSP state.

```cpp
// Member variables in CipherOB struct
SVFilter filters[PORT_MAX_CHANNELS];           // Stage 1 (existing — 12dB always)
SVFilter filters24dB_stage2[PORT_MAX_CHANNELS]; // Stage 2 (NEW — only active in 24dB mode)
```

Memory impact: ~160 bytes per filter × 16 voices × 2 arrays = ~5KB total. Negligible.

### Pattern 3: Conditional Cascade Logic in process()

**What:** Branch on filterType inside the per-voice loop. 12dB path is unchanged. 24dB path adds stage2 processing.
**When to use:** Mode-switched DSP in VCV Rack where each mode is mutually exclusive.

```cpp
// Inside per-voice loop for (int c = 0; c < channels; c++) {

// Stage 1 — always runs (same as v0.50b in 12dB mode)
// In 24dB mode: still apply full resonance to stage1 (warm character requires it)
// Note: do NOT split resonance at stage1 level for self-oscillation richness
float stage1Q = resonance;
filters[c].setParams(cutoffHz, stage1Q, args.sampleRate);
SVFilterOutputs stage1Out = filters[c].process(input);

if (filterType == 0) {
    // 12dB SEM mode: existing behavior unchanged
    // (apply drive and write outputs as in v0.50b)

} else {
    // 24dB OB-X mode: cascade stage1.lowpass into stage2
    float interStage = stage1Out.lowpass;

    // NaN check at inter-stage boundary
    if (!std::isfinite(interStage)) {
        filters[c].reset();
        filters24dB_stage2[c].reset();
        interStage = 0.f;
    }

    // Clamp inter-stage signal to prevent stage2 input overflow
    interStage = rack::clamp(interStage, -12.f, 12.f);

    // Reduced resonance on stage2: prevents explosive cascaded feedback
    // while allowing stage1 to drive resonance peak character
    // Starting value: 0.7x — may need empirical tuning to 0.5-0.8 range
    float stage2Q = resonance * 0.7f;
    filters24dB_stage2[c].setParams(cutoffHz, stage2Q, args.sampleRate);
    SVFilterOutputs stage2Out = filters24dB_stage2[c].process(interStage);

    // 24dB LP output — use stage2.lowpass for true 24dB rolloff
    // Drive hotter in 24dB mode per user decision
    float drive24 = smoothedDrive * 1.3f;  // ~30% hotter than 12dB mode
    outputs[LP_OUTPUT].setVoltage(blendedSaturation(stage2Out.lowpass, drive24), c);

    // HP/BP/Notch in 24dB: Claude's discretion — interim: pass stage1 outputs
    // (Phase 9 will silence these per FILT-09; this keeps signal flowing for now)
    outputs[HP_OUTPUT].setVoltage(blendedSaturation(stage1Out.highpass, smoothedDrive * 0.5f), c);
    outputs[BP_OUTPUT].setVoltage(blendedSaturation(stage1Out.bandpass, smoothedDrive * 1.0f), c);
    outputs[NOTCH_OUTPUT].setVoltage(blendedSaturation(stage1Out.notch, smoothedDrive * 0.7f), c);
}
```

### Pattern 4: Click-Free Mode Switching (Crossfade)

**What:** Detect filterType change sample-by-sample; crossfade output over N samples to eliminate click.
**When to use:** ANY time a DSP mode switch produces a discontinuity in the output signal.

This is the non-negotiable user requirement: "Mode switching must be click-free even in Phase 8."

```cpp
// Member variables
int prevFilterType = 0;
int crossfadeCounter = 0;
static constexpr int CROSSFADE_SAMPLES = 128;  // ~3ms at 44.1kHz, ~2.7ms at 48kHz
float crossfadeFromLP[PORT_MAX_CHANNELS] = {};  // Saved LP from old mode
float crossfadeFromHP[PORT_MAX_CHANNELS] = {};
float crossfadeFromBP[PORT_MAX_CHANNELS] = {};
float crossfadeFromNotch[PORT_MAX_CHANNELS] = {};

// In process(), after computing outputs for current mode:
// Detect mode change
bool modeChanged = (filterType != prevFilterType);
if (modeChanged) {
    crossfadeCounter = CROSSFADE_SAMPLES;
    // Capture current output as crossfade-from values (already computed above)
    // NOTE: compute OLD mode output first, then save, THEN switch
    prevFilterType = filterType;
}

// Apply crossfade blend if in transition
if (crossfadeCounter > 0) {
    float t = 1.f - (float)crossfadeCounter / (float)CROSSFADE_SAMPLES;  // 0.0 → 1.0
    for (int c = 0; c < channels; c++) {
        // Linear crossfade from old output to new output
        float newLP = outputs[LP_OUTPUT].getVoltage(c);
        outputs[LP_OUTPUT].setVoltage(crossfadeFromLP[c] * (1.f - t) + newLP * t, c);
        // ... repeat for HP, BP, Notch
    }
    crossfadeCounter--;
}
```

**Implementation note:** The crossfade requires saving the old-mode output values at the moment of switch detection, then blending toward the new-mode values over CROSSFADE_SAMPLES iterations. Structure the per-voice loop to compute outputs into temporary variables, then apply crossfade before writing to output ports.

### Pattern 5: CKSS Widget Placement on 14HP Panel

**What:** Place the CKSS toggle switch widget on the SVG panel and register it in the widget constructor.
**Panel dimensions:** 71.12mm × 128.5mm (14HP). Available real estate between title (y~20mm) and cutoff section (y~34mm).

The panel currently has a gap between divider at y=22mm and the cutoff knob at y=40mm. A CKSS switch placed around y=28mm, x=55mm (right side, near Drive column) is clear of all existing controls.

```cpp
// In CipherOBWidget constructor
addParam(createParamCentered<CKSS>(
    mm2px(Vec(55.0, 28.0)),  // Right column, below title, above drive knob
    module,
    CipherOB::FILTER_TYPE_PARAM));
```

Label the switch position in SVG: "12dB" above and "24dB" below the switch, using the existing pixel-path label style.

### Anti-Patterns to Avoid

- **Identical Q on both stages:** Never use `resonance` unmodified on stage2. Two high-Q SVFs in series compound resonance exponentially. Always scale stage2 Q down (0.5–0.8x range).
- **No inter-stage NaN check:** The existing NaN check is inside SVFilter::process() for stage2's own state. Without an inter-stage check, stage1 NaN silently corrupts stage2 before any detection.
- **Hard state reset on mode switch:** Calling `filters24dB_stage2[c].reset()` when switching modes creates a discontinuity that sounds like a click. Use crossfade instead; only reset on NaN recovery.
- **Separate OBXFilter.hpp:** Prior research confirmed this adds indirection with no benefit. All cascade logic stays in CipherOB::process().
- **Using `round()` to read filterType:** `std::round(0.5f)` is implementation-defined (rounds to 1 on most platforms but not guaranteed). Use `floor(x + 0.5f)` or `(int)(x + 0.5f)` for safety.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| 4-pole state-space filter | True 4th-order SVF with 4 integrators | Cascade two SVFilter instances | OB-X topology IS a cascade; higher complexity, harder to tune, no sonic benefit |
| Ladder filter | Moog-style transistor ladder | SVF cascade | Wrong topology — Oberheim used state-variable, not ladder |
| External resonance limiter | Separate peak-detector + gain reducer | Stage2 Q scaling (0.7x) + tanh in SVFilter feedback | SVFilter already has tanh in v1_raw path; scaling Q is sufficient |
| Crossfade module | External DSP utility class | Inline counter in process() | 10 lines of inline code; no class needed; single counter per module |
| Click-suppression plugin | Separate declick module chained outside | Internal crossfade in process() | External declick adds latency and couples to another module |

**Key insight:** The 24dB filter is "run the 12dB filter twice" — all infrastructure already exists. The only new code is routing logic and a crossfade counter.

---

## Common Pitfalls

### Pitfall 1: Explosive Resonance at High Q (CRITICAL)

**What goes wrong:** Cascading two high-Q SVF instances compounds resonant feedback non-linearly. At resonance > 70% with full Q applied to both stages, output can saturate to ±12V and sustain there.
**Why it happens:** Each stage's resonant peak drives the next stage's input; tanh saturation in stage1 produces harmonics that excite stage2 at the same frequency.
**How to avoid:** Scale stage2 Q by 0.7x (starting value). Test at 100% resonance with white noise input immediately after first implementation. If output exceeds ±5V sustained, reduce stage2 Q multiplier toward 0.5x.
**Warning signs:** Square-wave-like output at resonance knob > 70%; self-oscillation that sounds harsh rather than smooth; VCV CPU meter spike from saturated computation.

### Pitfall 2: NaN Propagation Across Stages (CRITICAL)

**What goes wrong:** SVFilter::process() only checks its own output for NaN. If stage1 produces NaN (from extreme parameters), it feeds into stage2 as input, corrupting stage2 state before any NaN check runs.
**Why it happens:** The NaN check in SVFilter.hpp is at the output (`if (!std::isfinite(v2))`), not at input validation. A non-finite input propagates through the trapezoidal equations before the check.
**How to avoid:** Add explicit `std::isfinite()` check on `stage1Out.lowpass` before calling `stage2.process()`. On failure: `filters[c].reset(); filters24dB_stage2[c].reset(); interStage = 0.f;`.
**Warning signs:** Silent output (flat 0V) that does not recover; moving any parameter has no effect.

### Pitfall 3: Click on Mode Switch (NON-NEGOTIABLE)

**What goes wrong:** Abrupt switch from 12dB path to 24dB path causes an instantaneous discontinuity in output voltage. Stage2 filter state may be at a non-zero initial condition if it was previously active.
**Why it happens:** Both filter paths compute independently. When filterType flips, the output changes discontinuously — potentially a large voltage step at the sample boundary.
**How to avoid:** Crossfade over 64–256 samples when filterType changes. 128 samples (~2.7ms at 48kHz) is inaudible and effective. Track `prevFilterType` in module state; detect change in `process()`; blend linearly for CROSSFADE_SAMPLES iterations.
**Warning signs:** Audible "tick" or "pop" when toggling switch during audio playback.

### Pitfall 4: Inter-Stage Clipping from Resonant Gain (CRITICAL)

**What goes wrong:** Stage1 at high resonance can output signals exceeding ±12V, overdriving stage2 before its own processing begins. This creates unintentional hard clipping distinct from the intended saturation character.
**Why it happens:** Resonant SVF exhibits >0dB gain near the cutoff frequency. At Q=20 (max resonance), gain at cutoff is approximately 1/k = 20 times input amplitude.
**How to avoid:** Clamp `interStage = rack::clamp(stage1Out.lowpass, -12.f, 12.f)` before passing to stage2. This matches what the existing SVFilter::process() does for its own input.
**Warning signs:** Harsh, crunchy digital distortion at resonance settings above 50%; asymmetric waveform visible on scope.

### Pitfall 5: Q Duplication Destroying Butterworth Response

**What goes wrong:** Cascading two identical Butterworth stages (each at Q=0.707) produces -6dB at cutoff instead of the expected -3dB. The combined frequency response is not Butterworth-shaped.
**Why it happens:** Each stage contributes -3dB at its corner frequency; cascaded responses multiply (sum in dB), producing -6dB total attenuation at the nominal cutoff.
**How to avoid:** The project uses a split resonance approach (stage1 full Q, stage2 at 0.7x) rather than strict Butterworth pole placement. This is musically effective and matches the empirical OB-X character. Do not attempt to reconstruct Butterworth poles from scratch — the 0.7x split is the correct starting point.
**Warning signs:** Resonance peak appears at unexpected frequency; 24dB mode sounds thin or "wrong" compared to reference.

### Pitfall 6: Drive Scale Mismatch Between Modes

**What goes wrong:** 24dB cascade has more gain than single-stage 12dB. Applying identical drive level in both modes makes 24dB sound over-saturated at equivalent knob positions.
**Why it happens:** Two saturation passes compound. Even at "light" drive, both stages accumulate harmonic content.
**How to avoid:** Per user decision, 24dB mode runs slightly hotter (drive × 1.3f as starting value for the LP output drive). This is the opposite of reducing — the goal is OB-X edge. Test at various resonance/drive combinations; adjust multiplier if the character is too harsh.
**Warning signs:** "Fizzy" or "brittle" sound at drive > 40% in 24dB mode.

---

## Code Examples

### Complete Phase 8 Member Variable Additions

```cpp
// Source: existing codebase + Phase 8 additions
struct CipherOB : Module {
    enum ParamId {
        CUTOFF_PARAM,
        CUTOFF_ATTEN_PARAM,
        RESONANCE_PARAM,
        RESONANCE_ATTEN_PARAM,
        DRIVE_PARAM,
        DRIVE_ATTEN_PARAM,
        FILTER_TYPE_PARAM,        // NEW
        PARAMS_LEN
    };
    // ... InputId, OutputId, LightId unchanged ...

    SVFilter filters[PORT_MAX_CHANNELS];             // existing
    SVFilter filters24dB_stage2[PORT_MAX_CHANNELS];  // NEW
    rack::dsp::TExponentialFilter<float> driveSmoothers[PORT_MAX_CHANNELS]; // existing
    bool driveSmootherInitialized = false;           // existing

    // NEW: click-free mode switching state
    int prevFilterType = 0;
    int crossfadeCounter = 0;
    static constexpr int CROSSFADE_SAMPLES = 128;
    float xfLP[PORT_MAX_CHANNELS] = {};
    float xfHP[PORT_MAX_CHANNELS] = {};
    float xfBP[PORT_MAX_CHANNELS] = {};
    float xfNotch[PORT_MAX_CHANNELS] = {};
```

### Constructor Addition

```cpp
// In CipherOB()
configSwitch(FILTER_TYPE_PARAM, 0.f, 1.f, 0.f, "Filter Type", {"12dB SEM", "24dB OB-X"});
```

### Cascade Logic Skeleton in process()

```cpp
void process(const ProcessArgs& args) override {
    int channels = std::max(1, inputs[AUDIO_INPUT].getChannels());
    int filterType = (int)(params[FILTER_TYPE_PARAM].getValue() + 0.5f);

    // Detect mode change for crossfade
    bool modeJustChanged = (filterType != prevFilterType);

    // ... existing parameter reads (cutoff, resonance, drive) ...

    for (int c = 0; c < channels; c++) {
        float input = inputs[AUDIO_INPUT].getPolyVoltage(c);
        // ... cutoffHz, resonance, smoothedDrive computed per-voice ...

        // Compute outputs into temporaries
        float outLP, outHP, outBP, outNotch;

        if (filterType == 0) {
            // 12dB SEM: unchanged from v0.50b
            filters[c].setParams(cutoffHz, resonance, args.sampleRate);
            SVFilterOutputs out = filters[c].process(input);
            outLP    = blendedSaturation(out.lowpass,  smoothedDrive * 1.0f);
            outHP    = blendedSaturation(out.highpass, smoothedDrive * 0.5f);
            outBP    = blendedSaturation(out.bandpass, smoothedDrive * 1.0f);
            outNotch = blendedSaturation(out.notch,    smoothedDrive * 0.7f);

        } else {
            // 24dB OB-X: cascaded
            filters[c].setParams(cutoffHz, resonance, args.sampleRate);
            SVFilterOutputs s1 = filters[c].process(input);

            float inter = s1.lowpass;
            if (!std::isfinite(inter)) {
                filters[c].reset();
                filters24dB_stage2[c].reset();
                inter = 0.f;
            }
            inter = rack::clamp(inter, -12.f, 12.f);

            float q2 = resonance * 0.7f;
            filters24dB_stage2[c].setParams(cutoffHz, q2, args.sampleRate);
            SVFilterOutputs s2 = filters24dB_stage2[c].process(inter);

            float d24 = smoothedDrive * 1.3f;
            outLP    = blendedSaturation(s2.lowpass,  d24);
            outHP    = blendedSaturation(s1.highpass, smoothedDrive * 0.5f);  // interim: stage1
            outBP    = blendedSaturation(s1.bandpass, smoothedDrive * 1.0f);  // interim: stage1
            outNotch = blendedSaturation(s1.notch,    smoothedDrive * 0.7f);  // interim: stage1
        }

        // Crossfade on mode change
        if (modeJustChanged && crossfadeCounter == 0) {
            crossfadeCounter = CROSSFADE_SAMPLES;
            xfLP[c]    = outputs[LP_OUTPUT].getVoltage(c);
            xfHP[c]    = outputs[HP_OUTPUT].getVoltage(c);
            xfBP[c]    = outputs[BP_OUTPUT].getVoltage(c);
            xfNotch[c] = outputs[NOTCH_OUTPUT].getVoltage(c);
        }
        if (crossfadeCounter > 0) {
            float t = 1.f - (float)crossfadeCounter / (float)CROSSFADE_SAMPLES;
            outLP    = xfLP[c]    * (1.f - t) + outLP    * t;
            outHP    = xfHP[c]    * (1.f - t) + outHP    * t;
            outBP    = xfBP[c]    * (1.f - t) + outBP    * t;
            outNotch = xfNotch[c] * (1.f - t) + outNotch * t;
        }

        outputs[LP_OUTPUT].setVoltage(outLP, c);
        outputs[HP_OUTPUT].setVoltage(outHP, c);
        outputs[BP_OUTPUT].setVoltage(outBP, c);
        outputs[NOTCH_OUTPUT].setVoltage(outNotch, c);
    }

    if (crossfadeCounter > 0) crossfadeCounter--;
    prevFilterType = filterType;

    outputs[LP_OUTPUT].setChannels(channels);
    outputs[HP_OUTPUT].setChannels(channels);
    outputs[BP_OUTPUT].setChannels(channels);
    outputs[NOTCH_OUTPUT].setChannels(channels);
}
```

### SVG Panel Change — CKSS Switch Region

Add to `CipherOB.svg` inside `<g id="components">`:

```xml
<!-- Filter Type Switch (CKSS - 2 position toggle) -->
<!-- Positioned at x=55, y=28 (right column, below title bar, above drive knob) -->
<!-- CKSS widget is approximately 4.4mm wide x 10mm tall in Rack units -->
<rect
   style="fill:#333355;stroke:#5555aa;stroke-width:0.3"
   x="52.8" y="23" width="4.4" height="10" rx="1"
   id="filter-type-switch-placeholder" />
<!-- Labels: "12" above, "24" below (rendered as pixel paths in final SVG) -->
```

Add to `CipherOBWidget` constructor:

```cpp
addParam(createParamCentered<CKSS>(
    mm2px(Vec(55.0, 28.0)),
    module,
    CipherOB::FILTER_TYPE_PARAM));
```

---

## Self-Oscillation Character Notes

The user decision: "warm and slightly distorted — harmonic richness from cascade saturation, not a clean sine."

This is achievable through the existing tanh saturation in SVFilter's feedback path (`float v1 = std::tanh(v1_raw * 2.f) * 0.5f`). The cascade means this saturation runs twice — stage1 saturates, then stage2 saturates the already-saturated output. At maximum resonance this produces a slightly distorted sine with odd harmonics: exactly the OB-X self-oscillation character described.

Self-oscillation pitch tracking accuracy is Claude's discretion. The existing `g = tan(pi * cutoffNorm)` warping already provides reasonable 1V/Oct tracking. No additional compensation is needed in Phase 8.

The resonance peak should be "louder/more aggressive than 12dB." With full resonance on stage1 and 0.7x on stage2, the combined resonant amplitude will naturally exceed the 12dB output. This satisfies the user requirement without additional gain compensation.

---

## Q-Split Tuning Guide

The 0.7x stage2 multiplier is a validated starting point. Empirical tuning range:

| Stage2 Q Multiplier | Behavior |
|--------------------|----------|
| 1.0x | Explosive above resonance ~60%; unusable |
| 0.8x | Aggressive; may click at max resonance |
| 0.7x | **Starting point** — balanced musical response |
| 0.6x | Softer; self-oscillation still works |
| 0.5x | Subdued; resonance peak less aggressive |

Test protocol: white noise → filter at max resonance, vary cutoff. Target: clean self-oscillation sine (with harmonic warmth) at max resonance, no hard clipping or sustained saturation.

---

## Cutoff Frequency Consistency (TYPE-02)

Both stages receive identical `cutoffHz`, which maps to identical `g = tan(pi * cutoffNorm)`. The corner frequency of the cascade is the corner frequency of each stage — they multiply in magnitude response but remain aligned at the same frequency. No correction factor is needed. This satisfies TYPE-02 at the mathematical level.

**Tolerance:** The `-3dB` point of the 24dB cascade will be slightly lower than the 12dB mode's `-3dB` point due to the combined transfer function, but the cutoff knob will produce the same notional frequency on the panel (1kHz = 1kHz). This is "Claude's discretion" per the context decision and acceptable for Phase 8.

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| True 4-pole state-space SVF | Cascaded 2-pole SVF sections | Decided v0.60b research | Simpler, historically accurate to OB-X hardware topology |
| Global feedback resonance | Per-stage Q split | Decided v0.60b research | Easier to stabilize; independent per-stage saturation |
| Hard switch between modes | Crossfade on mode change | Phase 8 addition | Click-free per user requirement |
| No inter-stage NaN check | Explicit isfinite() at cascade boundary | Phase 8 addition | Prevents silent corruption in 24dB mode |

**Deprecated/outdated:**
- Naively applying full resonance to both stages: Prior research confirmed this causes instability. Always scale stage2 Q.
- resetting filter state on mode switch: Crossfade is the correct approach.

---

## Open Questions

1. **Exact Stage2 Q Multiplier**
   - What we know: 0.7x is the validated starting point from v0.60b research
   - What's unclear: Whether 0.7x produces optimal resonance character (warm vs harsh) for this specific SVFilter implementation with tanh in feedback
   - Recommendation: Implement at 0.7x, test immediately with resonance sweep; adjust toward 0.5–0.8 range based on ear test

2. **Crossfade Duration**
   - What we know: 128 samples (~2.7ms at 48kHz) is inaudible and effective for most transitions
   - What's unclear: Whether a longer crossfade (256 samples) is needed if stage2 state is far from stage1 state at time of switch
   - Recommendation: Start at 128; if switching from sustained self-oscillation produces audible artifact, increase to 256

3. **Drive Multiplier for 24dB Mode**
   - What we know: User wants 24dB to be "slightly hotter" than 12dB; 1.3x is a reasonable starting estimate
   - What's unclear: Whether 1.3x achieves the right "OB-X edge" character or needs adjustment
   - Recommendation: 1.3x starting value; adjust by ear; range 1.0x–2.0x plausible

4. **Interim HP/BP/Notch Output Routing**
   - What we know: Phase 9 will silence HP/BP/Notch in 24dB mode (FILT-09); Phase 8 is interim
   - What's unclear: Whether routing stage1 HP/BP/Notch is better or worse than routing stage2 equivalents for interim behavior
   - Recommendation: Stage1 outputs for interim (12dB characteristic from upstream stage); easier to explain; Phase 9 will mute them

5. **Self-Oscillation with Click-Free Switching**
   - What we know: If filter is self-oscillating and user switches modes, the crossfade must bridge from oscillating to non-oscillating (or different frequency/character)
   - What's unclear: Whether 128-sample crossfade is sufficient for transitions from sustained self-oscillation
   - Recommendation: This is an edge case; test explicitly during verification; increase crossfade samples if audible pop persists

---

## Sources

### Primary (HIGH confidence)

- **`.planning/research-v060b/ARCHITECTURE.md`** — Complete cascaded SVF architecture, resonance splitting, state management patterns; researched 2026-02-03
- **`.planning/research-v060b/STACK.md`** — No new dependencies, cascade-in-process() pattern, configSwitch usage; researched 2026-02-03
- **`.planning/research-v060b/PITFALLS.md`** — 10 pitfalls catalogued; critical: NaN propagation, explosive resonance, inter-stage clipping; researched 2026-02-03
- **`src/SVFilter.hpp`** (codebase) — Exact trapezoidal SVF implementation; tanh in v1_raw feedback path confirmed; existing NaN check behavior confirmed
- **`src/CipherOB.cpp`** (codebase) — Current ParamId enum, process() structure, drive smoother pattern; baseline for additions
- **`res/CipherOB.svg`** (codebase) — Panel dimensions (71.12mm × 128.5mm = 14HP); component positions confirmed; y=28mm gap available for switch
- **VCV Rack API — componentlibrary.hpp source** (https://vcvrack.com/docs-v2/componentlibrary_8hpp_source) — CKSS loads CKSS_0.svg + CKSS_1.svg, inherits from SvgSwitch; confirmed via web fetch
- **VCV Rack Plugin API Guide** (https://vcvrack.com/manual/PluginGuide) — configSwitch() signature, labeled-value switches, right-click menu behavior; confirmed via web fetch

### Secondary (MEDIUM confidence)

- **KVR Audio — Self-oscillating SVF** (https://www.kvraudio.com/forum/viewtopic.php?t=333894) — minimum damping 0.0625 for max resonance peak gain of 24dB; tanh as soft limiter for stable self-oscillation
- **EarLevel Engineering — Cascading Filters** (https://www.earlevel.com/main/2016/09/29/cascading-filters/) — Naive Q duplication destroys Butterworth response; verified cascade theory
- **DSPRelated — Design IIR Filters Using Cascaded Biquads** (https://www.dsprelated.com/showarticle/1137.php) — Inter-stage clipping from gain staging; order stages by Q
- **VCV Community — Sequential Switch De-click** (https://community.vcvrack.com/t/sequential-switch-with-de-click/21130) — Crossfade is standard VCV community approach for click-free switching
- **KVR Audio — SVF 24dB version** (https://www.kvraudio.com/forum/viewtopic.php?t=263202) — Cascading two SVFs confirmed approach; resonance challenges known

### Tertiary (LOW confidence — informative but not authoritative)

- **GitHub — reales/OB-Xd** (https://github.com/reales/OB-Xd) — Confirms cascade approach; specific implementation details not accessible from public README
- **GitHub — surge-synthesizer/OB-Xf** (https://github.com/surge-synthesizer/OB-Xf) — Modern continuation of OB-Xd; confirms project still actively maintained
- **Oberheim OB-Xa filter — MOD WIGGLER** (https://modwiggler.com/forum/viewtopic.php?t=10709) — OB-Xa dual filter architecture discussion (community)

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — zero new dependencies; all components exist and are validated in v0.50b; CKSS confirmed via API docs
- Architecture: HIGH — cascaded SVF pattern fully documented in prior v0.60b research; patterns confirmed by reading existing code
- Pitfalls: HIGH — critical pitfalls (#1–4) verified from multiple authoritative sources; crossfade click removal is standard practice
- Q-split tuning: MEDIUM — 0.7x is validated starting point; exact value requires ear testing during implementation
- Drive multiplier: MEDIUM — 1.3x is an informed estimate; exact value requires ear testing

**Research date:** 2026-02-21
**Valid until:** 2026-05-21 (90 days — stable domain; VCV Rack API changes infrequently)

**Prior research:** `.planning/research-v060b/` (2026-02-03) — all findings remain valid; this document refines Phase 8 specifics
