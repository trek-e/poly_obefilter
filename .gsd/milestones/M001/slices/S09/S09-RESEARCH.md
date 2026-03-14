# Phase 9: Character & Output - Research

**Researched:** 2026-02-21
**Domain:** VCV Rack C++ DSP — analog filter character tuning, output routing, click-free switching
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

- Two modes must sound **dramatically different** — like switching between two different synth architectures, matching the contrast of original SEM vs OB-X hardware
- Reference sound: classic OB-Xa basses — fat, biting, thick low end, aggressive resonance bite (Van Halen "Jump", Depeche Mode)
- HP/BP/Notch outputs must be **silent** in 24dB mode (authentic OB-Xa LP-only behavior)
- When switching **back** from 24dB to 12dB: HP/BP/Notch **fade back in** over the crossfade window (symmetric with fade-out)

### Claude's Discretion

- Edge source in 24dB (resonance shape, saturation curve, frequency tilt, or combination)
- Low-resonance character difference between modes
- Saturation algorithm choice for 24dB mode
- Drive aggressiveness range and zero-drive behavior
- Drive + resonance interaction at extremes
- HP/BP/Notch silence method and fade timing
- Port channel behavior in 24dB mode
- Any visual cues for inactive outputs

### Deferred Ideas (OUT OF SCOPE)

None — discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| FILT-08 | OB-X character tuning (bright/edgy saturation distinct from SEM warm/smooth) | Resonance Q amplification, enhanced inter-stage saturation, and frequency-tilt pre-emphasis all contribute independently to bright/edgy character |
| FILT-09 | Lowpass-only output active in 24dB mode (HP/BP/Notch silent, authentic OB-Xa) | setChannels(1) + setVoltage(0.f) per channel is the safest silent-port approach; crossfade handles fade-to-zero on mode switch and fade-from-zero on mode return |
| TYPE-03 | Click-free mode switching without pops or glitches | 128-sample linear crossfade state machine already exists from Phase 8; Phase 9 must ensure HP/BP/Notch crossfade targets are 0.0f (not previous output values) when switching to 24dB |
</phase_requirements>

---

## Summary

Phase 9 is a focused C++ tuning phase: no new architecture, no new parameters, no new files. Everything is contained in `src/CipherOB.cpp` and `src/SVFilter.hpp`. All three requirements touch the 24dB branch of the `process()` method that Phase 8 created.

**FILT-08 (OB-X character):** The OB-Xa hardware used two completely different filter topologies — a 12dB state-variable SEM filter and a 24dB OTA ladder (CEM3320). Their sonic difference comes from fundamentally different feedback structures: the SEM's multimode topology produces a smooth, mellow resonance that tapers at high Q, while the OTA ladder produces a sharper, more aggressive resonance peak. In a cascaded SVF emulation, this character difference must be synthesized through DSP choices: resonance Q scaling, nonlinearity placement, and saturation shaping. The current code already runs `smoothedDrive * 1.3f` for the 24dB LP path; the remaining work is tuning the resonance curve and inter-stage saturation to maximize the architectural contrast.

**FILT-09 (LP-only output):** The existing Phase 8 code already outputs stage1's HP/BP/Notch in 24dB mode as an interim. Phase 9 must replace those with zero — but zero must be reached via the crossfade, not an abrupt cut. The VCV Rack API provides `setVoltage(0.f, c)` and `setChannels(n)`. The cleanest approach: in 24dB mode, target values for HP/BP/Notch are `0.0f`; the existing crossfade already handles interpolating from the last-output-value to the target over 128 samples. No structural changes needed — only the target values change.

**TYPE-03 (click-free switching):** The 128-sample linear crossfade state machine from Phase 8 is already correct. The click risk in Phase 9 is the asymmetric scenario: when switching FROM 24dB back TO 12dB, HP/BP/Notch must fade in from 0.0f (not from arbitrary stage1 values). The crossfade from-values (`xfHP`, `xfBP`, `xfNotch`) are captured from `outputs[X_OUTPUT].getVoltage(c)` — which will correctly be 0.0f if Phase 9 has been setting them to 0.0f in 24dB mode. This makes the fade-in symmetric automatically.

**Primary recommendation:** Phase 9 requires two targeted edits to `CipherOB.cpp`: (1) tune 24dB character by scaling resonance Q upward and enhancing inter-stage saturation, and (2) replace interim HP/BP/Notch outputs in the 24dB branch with `0.0f` targets. The crossfade handles the rest automatically.

---

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| VCV Rack SDK | 2.x | Module framework | Required; all existing APIs already in use |
| C++ `std::tanh` | C++11 | Nonlinear saturation | Existing pattern in codebase (`blendedSaturation`, feedback path) |
| `rack::clamp` | VCV Rack | Signal safety clamping | Existing pattern at inter-stage boundary |
| `rack::dsp::TExponentialFilter` | VCV Rack | Parameter smoothing | Already used for cutoff, resonance, drive |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `std::isfinite` | C++11 | NaN/infinity guard | Already used in inter-stage check; relevant if resonance Q is increased |

### No New Dependencies

This phase introduces zero new libraries or includes. All tools are already present in the codebase.

---

## Architecture Patterns

### What Already Exists (Phase 8 output)

```
process()
├── filterType == 0 (12dB SEM)
│   └── filters[c].process(input) → {lp, hp, bp, notch}
│       └── blendedSaturation() × 4 outputs
└── filterType == 1 (24dB OB-X)
    ├── filters[c].setParams(cutoffHz, resonance, sr)   ← Stage 1
    ├── s1 = filters[c].process(input)
    ├── interStage = clamp(s1.lowpass, -12, 12)        ← NaN check + clamp
    ├── filters24dB_stage2[c].setParams(cutoffHz, resonance*0.7, sr)  ← Stage 2
    ├── s2 = filters24dB_stage2[c].process(interStage)
    └── outLP  = blendedSaturation(s2.lowpass, drive*1.3)  ← KEEP, tune
        outHP  = blendedSaturation(s1.highpass, drive*0.5) ← REPLACE with 0.0f
        outBP  = blendedSaturation(s1.bandpass, drive*1.0) ← REPLACE with 0.0f
        outNotch = blendedSaturation(s1.notch, drive*0.7)  ← REPLACE with 0.0f

Crossfade (128 samples):
├── captures xfLP/HP/BP/Notch from port getVoltage() on mode change
└── linear blend: from × (1-t) + target × t
```

### Pattern 1: Silence HP/BP/Notch in 24dB Mode

**What:** Replace the interim `blendedSaturation(s1.X, ...)` outputs with `0.0f` for HP/BP/Notch in the 24dB branch.

**Why it's click-free automatically:** The crossfade state machine captures from-values at mode-change time from `outputs[X_OUTPUT].getVoltage(c)`. If the previous mode was 12dB, those getVoltage() calls return whatever the SEM filter produced. If the previous mode was 24dB (staying in 24dB — impossible, only one switch state) those values are 0.0f. The crossfade blends from those values toward 0.0f over 128 samples — click-free.

**Why fade-in works symmetrically:** When switching FROM 24dB back TO 12dB, the crossfade captures from-values from the 24dB outputs — which are `0.0f` for HP/BP/Notch. The SEM outputs become the target. Linear blend from 0.0f to SEM output = fade in. Symmetric by construction.

**Example (24dB branch):**
```cpp
// REPLACE this:
outHP   = blendedSaturation(s1.highpass, smoothedDrive * 0.5f);
outBP   = blendedSaturation(s1.bandpass, smoothedDrive * 1.0f);
outNotch = blendedSaturation(s1.notch,   smoothedDrive * 0.7f);

// WITH this:
outHP    = 0.0f;
outBP    = 0.0f;
outNotch = 0.0f;
```

### Pattern 2: OB-X Character Tuning via Resonance Scaling

**What:** Amplify the effective Q of both stages in 24dB mode to produce the sharper, more peaky resonance characteristic of the OTA ladder topology.

**Why resonance Q is the primary character lever:** The hardware difference between SEM and OB-Xa 24dB mode is fundamentally topological. The SEM state-variable produces a smooth resonance plateau; the OTA ladder produces a sharper, more aggressive peak. In a cascaded SVF emulation, the equivalent of this is running the stages at a higher effective Q — the two-stage cascade amplifies the peak with each stage, making the overall Q-versus-audible-emphasis relationship different from a single-stage SVF.

**Current state (Phase 8):**
- Stage 1: Q = `0.5 + resonance * 19.5` (SVFilter.hpp line 88)
- Stage 2: Q = `0.5 + (resonance * 0.7) * 19.5`
- Drive: `smoothedDrive * 1.3f` on LP output

**Tuning approach (Claude's discretion):** Increase Stage 1 Q scaling to push more resonance character through both stages. Options ranked by implementation risk:

1. **Q scale factor on Stage 1** (lowest risk): Multiply resonance parameter before passing to `setParams()` in Stage 1 only. E.g., `resonance24 = rack::clamp(resonance * 1.15f, 0.f, 1.f)`. This increases resonance prominence without touching the SVFilter internals.

2. **Asymmetric stage Q split** (medium risk): Run Stage 1 at boosted Q, keep Stage 2 at damped Q. E.g., Stage 1 at `resonance * 1.1f`, Stage 2 at `resonance * 0.65f`. The boosted Stage 1 sharpens the peak; Stage 2 damps to maintain stability.

3. **Enhanced inter-stage saturation** (medium risk): Apply stronger nonlinearity at the stage boundary to mimic the odd-harmonic bite of the OTA topology. Replace the existing `rack::clamp(s1.lowpass, -12.f, 12.f)` pass-through with a mild tanh: `interStage = std::tanh(s1.lowpass * 1.2f) * 10.f`. This adds harmonic content that increases with drive and resonance — like the diode-pair saturation in the real CEM3320.

**Recommended approach: combine options 1 and 3** — Q scaling for resonance peak shape, inter-stage saturation for harmonic character. Both are tunable by a single coefficient.

### Pattern 3: Drive Curve Differentiation for 24dB

**What:** The 24dB mode already uses `smoothedDrive * 1.3f`. Phase 9 can additionally tune the saturation curve by adjusting the `blendedSaturation` behavior or applying pre-saturation gain differently.

**Current `blendedSaturation` behavior:**
```cpp
// In SVFilter.hpp
float gain = 1.0f + drive * 4.0f;    // 1x to 5x input gain
float soft = std::tanh(scaled);       // symmetric, odd harmonics
// asymmetric blend up to 40% at max drive
```

**For 24dB mode:** The 1.3x drive multiplier already pushes the input gain higher. The OB-X character also benefits from stronger odd-harmonic saturation (less asymmetric, more tanh-dominant at high drive). This naturally follows from the higher drive multiplier without code changes — the tanh curve already produces more clipping at 1.3x vs 1.0x drive.

**If more character is needed:** A 24dB-specific saturation can be implemented inline (no `blendedSaturation` API change needed):
```cpp
// Inline saturation for 24dB LP — more aggressive, odd-harmonic dominant
float ob24Saturate(float x, float drive) {
    if (drive < 0.01f) return x;
    float gain = 1.0f + drive * 5.5f;   // slightly higher ceiling than blended
    return std::tanh(x * gain) / (1.0f + drive * 0.4f);  // makeup
}
```

### Anti-Patterns to Avoid

- **Abrupt silence in 24dB mode:** Setting HP/BP/Notch to 0.0f only works click-free because the crossfade handles the transition. If crossfade is bypassed or reset() is called at mode switch, clicks occur. Never reset the crossfade on mode change — only start it.
- **Clearing port channels in 24dB mode:** `setChannels(0)` does NOT produce 0-channel output per the VCV API docs ("if 0 is given, channels is set to 1 but all voltages are cleared"). It's simpler and more compatible to keep `setChannels(channels)` always and control output via voltage values. The port will still show as active — this is fine per VCV conventions, and the user context left this to Claude's discretion.
- **Modifying the SEM 12dB path:** All Phase 9 changes must be in the `filterType == 1` branch only. The 12dB path is explicitly "bitwise identical to v0.50b" per Phase 8 decision and must stay that way.
- **Touching SVFilter.hpp internals for character:** The `v1` saturation inside `SVFilter::process()` (`std::tanh(v1_raw * 2.f) * 0.5f`) affects both modes equally. Do not modify this for OB-X character. Character tuning belongs in the `CipherOB.cpp` process() branch.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Crossfade state machine | New crossfade mechanism | Existing 128-sample crossfade from Phase 8 | Already correct, already handles per-voice capture |
| Parameter smoothing for Q scaling | Manual smoothers | `SVFilter`'s existing `resonanceSmoother` | Already smoothes the resonance parameter before Q calc |
| Output silencing | setChannels(0) tricks | setVoltage(0.0f, c) in the 24dB branch | VCV API docs: setChannels(0) actually sets channels=1, not 0 |
| Drive-mode differentiation | Separate drive parameter or new knob | drive multiplier in the existing branch | Phase 8 already established 1.3x as the mode differentiator |

---

## Common Pitfalls

### Pitfall 1: Crossfade Captures Stale Values if Outputs Not Set Before Capture

**What goes wrong:** The crossfade capture `xfHP[c] = outputs[HP_OUTPUT].getVoltage(c)` reads from the port's stored voltage. If Phase 9 correctly sets HP to 0.0f in 24dB mode, switching back to 12dB will capture 0.0f — correct, produces a fade-in. But if there's an off-by-one error where the current frame's output is captured AFTER writing the new value, the crossfade starts from the wrong value.

**Why it happens:** The capture happens at `if (modeJustChanged)` BEFORE `outHP` is assigned. This is the current code's correct order. Any reordering risks a click.

**How to avoid:** Keep the Phase 8 capture order: detect mode change → capture old outputs → process filter → apply crossfade → write outputs. Never capture after writing.

### Pitfall 2: Q Scaling Pushes Resonance Past Stability Limit

**What goes wrong:** Increasing Stage 1 Q scale factor (e.g., `resonance * 1.15f`) maps resonance=1.0 to effective resonance=1.15, which in `setParams()` gives `Q = 0.5 + 1.15 * 19.5 = 22.9`. At this Q, the SVF can become unstable, producing NaN/inf.

**Why it happens:** The existing NaN guard at the inter-stage checks `s1.lowpass` for NaN but not the SVF's internal state before it diverges. High Q + high input signal = potential instability.

**How to avoid:** Clamp boosted resonance before passing to `setParams()`: `rack::clamp(resonance * 1.15f, 0.f, 0.95f)`. The 0.95 cap keeps Q ≤ 19.0, well within the stable range. Alternatively, boost by a smaller factor (1.1x instead of 1.15x).

**Warning signs:** If NaN resets start firing in testing (silent frames in the output), reduce the Q scale factor.

### Pitfall 3: Drive Multiplier Creates Unbalanced Loudness

**What goes wrong:** 24dB LP at `drive * 1.3f` is louder than 12dB LP at `drive * 1.0f`. At max drive, the 24dB mode will be noticeably hotter. This may be musically appropriate (OB-X character = more aggressive) but could surprise users.

**Why it happens:** The gain compensation in `blendedSaturation` (`1.0f / (1.0f + drive * 0.5f)`) applies equally regardless of the drive multiplier. At 1.3x drive, the tanh clips harder and the makeup gain is slightly lower, but net output can still be higher.

**How to avoid:** Accept the slight level increase as authentic OB-X character. Or apply a mode-specific makeup gain (e.g., multiply outLP by 0.9f in 24dB mode). Leave tuning to ear testing.

### Pitfall 4: Inter-Stage Saturation Changes Cutoff Effective Frequency

**What goes wrong:** If inter-stage saturation (tanh) is applied to the signal between Stage 1 and Stage 2, it adds harmonic content. These harmonics pass through Stage 2 and appear in the output at frequencies above the cutoff. This can make the 24dB mode sound brighter in a way that changes with input level — which may be desirable (analog-like) or may sound wrong at low levels.

**Why it happens:** tanh produces odd harmonics proportional to signal amplitude. At low amplitude (quiet signals), tanh is nearly linear — no effect. At high amplitude (loud signals or high resonance), significant harmonics. This is level-dependent behavior.

**How to avoid:** This is actually the desired OB-X behavior — the hardware CEM3320 had similar level-dependent character. Accept it. If the effect is too strong, reduce the gain factor in the inter-stage tanh.

### Pitfall 5: Visual Port State (LOW priority, Claude's discretion)

**What goes wrong:** In 24dB mode, HP/BP/Notch ports remain visually active (bright connection circle) even though they output 0V. Users may be confused.

**Why it happens:** VCV Rack port brightness is based on whether the port has connected cables, not on the voltage value.

**How to avoid:** This is Claude's discretion per CONTEXT.md. The recommendation: do nothing extra. Standard VCV Rack modules do not dim ports based on output routing. The port tooltip still says "Highpass", "Bandpass", "Notch" — informed users will understand. Adding custom dimming code would require ModuleWidget override and is out of scope.

---

## Code Examples

### Example 1: HP/BP/Notch Silence in 24dB Branch (FILT-09)

```cpp
// In process() → filterType == 1 (24dB OB-X) branch:

// Output mapping for 24dB OB-X mode:
// LP: stage2 output with boosted drive (authentic OB-Xa LP-only)
// HP/BP/Notch: silent (zero) — HP/BP/Notch are not outputs of the OB-Xa
outLP    = blendedSaturation(s2.lowpass, smoothedDrive * 1.3f);
outHP    = 0.0f;
outBP    = 0.0f;
outNotch = 0.0f;
```

### Example 2: Resonance Q Boosting for OB-X Character (FILT-08)

```cpp
// In 24dB branch, before setParams calls:
// Boost resonance for Stage 1 to produce sharper OB-X resonance peak
// Cap at 0.95 to prevent SVF instability (effective Q max = 19.0)
float resonance24 = rack::clamp(resonance * 1.15f, 0.f, 0.95f);

filters[c].setParams(cutoffHz, resonance24, args.sampleRate);
SVFilterOutputs s1 = filters[c].process(input);

// Stage 2: stability-damped Q (0.65x of boosted resonance)
float q2 = resonance24 * 0.65f;
filters24dB_stage2[c].setParams(cutoffHz, q2, args.sampleRate);
```

### Example 3: Inter-Stage Saturation for OB-X Harmonic Character (FILT-08)

```cpp
// Replace the existing pass-through clamp with mild tanh saturation:
// EXISTING:
float interStage = rack::clamp(s1.lowpass, -12.f, 12.f);

// ENHANCED for OB-X character:
float interStageRaw = s1.lowpass;
if (!std::isfinite(interStageRaw)) {
    filters[c].reset();
    filters24dB_stage2[c].reset();
    interStageRaw = 0.f;
}
// Mild tanh at inter-stage: adds odd harmonics proportional to signal level
// Factor 0.15f: enough for character, not so much it changes the cutoff feel
float interStage = std::tanh(interStageRaw * 1.15f) * (10.f / std::tanh(10.f * 1.15f));
// Note: denominator normalizes so that small signals are unity gain
// Simplification: std::tanh(10*1.15)/1.15 ≈ 1/1.15 for signals well below clipping
// Practical shortcut:
float interStage = rack::clamp(std::tanh(interStageRaw * 1.15f) / 1.15f, -12.f, 12.f);
```

### Example 4: Crossfade Capture (Phase 8 pattern — DO NOT CHANGE)

```cpp
// This existing code is correct — do not modify:
if (modeJustChanged) {
    xfLP[c]    = outputs[LP_OUTPUT].getVoltage(c);
    xfHP[c]    = outputs[HP_OUTPUT].getVoltage(c);   // will be 0.0f if was in 24dB
    xfBP[c]    = outputs[BP_OUTPUT].getVoltage(c);   // will be 0.0f if was in 24dB
    xfNotch[c] = outputs[NOTCH_OUTPUT].getVoltage(c); // will be 0.0f if was in 24dB
}
// Phase 9 correctness: when switching 12dB→24dB:
//   xfHP = last SEM HP value → fades to 0.0f (fade-out, FILT-09 click-free)
// When switching 24dB→12dB:
//   xfHP = 0.0f (from previous 24dB frame) → fades to SEM HP value (fade-in, TYPE-03)
```

### Example 5: Port Channel Handling (Claude's discretion — keep simple)

```cpp
// RECOMMENDATION: Keep this unchanged from Phase 8:
outputs[LP_OUTPUT].setChannels(channels);
outputs[HP_OUTPUT].setChannels(channels);
outputs[BP_OUTPUT].setChannels(channels);
outputs[NOTCH_OUTPUT].setChannels(channels);
// HP/BP/Notch will be connected cables at 0V in 24dB mode.
// This is standard VCV Rack behavior. No special dimming needed.
```

---

## State of the Art

| Old Approach | Current Approach | Relevance to Phase 9 |
|--------------|------------------|----------------------|
| Identical saturation for all filter modes | Mode-specific saturation (1.3x drive in 24dB) | Phase 8 established; Phase 9 enhances |
| HP/BP/Notch output in 24dB (interim) | HP/BP/Notch = 0.0f in 24dB | Phase 9 finalizes this |
| Crossfade only for LP | Crossfade for all 4 outputs | Already in Phase 8; Phase 9 relies on it |
| Same Q for all modes | Boosted Q in 24dB mode | Phase 9 adds this |

**Relevant hardware facts:**
- OB-X (1979): 12dB SEM-style state-variable LP with discrete transistors; no 24dB option
- OB-Xa (1981): Added 24dB LP via CEM3320 4-pole OTA ladder; LP output only in 24dB mode
- The OB-Xa 24dB mode's aggressive bite comes from the OTA ladder's harder clipping characteristics at high resonance compared to the SEM's smoother state-variable topology

Sources: [Wikipedia OB-Xa](https://en.wikipedia.org/wiki/Oberheim_OB-Xa), [Vintage Synth Explorer OB-X](https://www.vintagesynth.com/oberheim/ob-x), [Sound On Sound OBX/OBXa/OB8 Review](https://www.soundonsound.com/reviews/oberheim-obx-obxa-ob8)

---

## Open Questions

1. **How much Q boosting is enough for dramatic mode contrast?**
   - What we know: 1.15x resonance scaling keeps max Q at 22.9 before the 0.95 clamp; current is Q up to 20.0
   - What's unclear: Whether 1.1x, 1.15x, or 1.2x produces the right audible contrast for the "like a different synth architecture" goal
   - Recommendation: Start at 1.15x, test with resonance at 60-80% on a bass patch; if not dramatically different, increase to 1.2x. Use 0.9 clamp ceiling (not 0.95) for extra headroom.

2. **Should inter-stage saturation be added or left at clamp-only?**
   - What we know: The OB-Xd open-source implementation uses a diode-pair Taylor series approximation at the inter-stage boundary
   - What's unclear: Whether a simple tanh vs clamp produces sufficient character difference at musical signal levels (not just at max drive)
   - Recommendation: Add mild tanh inter-stage saturation with a gain factor of 1.1-1.2x; this is the lowest-risk way to add harmonic content that scales naturally with signal level and drive

3. **Is 128 samples long enough for a click-free transition on HP/BP/Notch fade-out?**
   - What we know: 128 samples = ~2.7ms at 48kHz. This is below the human hearing threshold for transients (~5-10ms), and crossfade is already verified for LP output from Phase 8
   - What's unclear: Whether the HP/BP/Notch fade-out is perceptible because those signals may have more high-frequency content (sharper transients) than LP
   - Recommendation: Keep 128 samples. If testing reveals a perceptible click specifically on HP/BP switching, increase CROSSFADE_SAMPLES to 256 (still ~5.3ms, below perception threshold). Do not change for now.

---

## Sources

### Primary (HIGH confidence)
- `src/CipherOB.cpp` — Current Phase 8 implementation, direct code inspection
- `src/SVFilter.hpp` — Filter internals, `blendedSaturation`, Q mapping formula
- `.planning/phases/08-core-24db-cascade/08-01-SUMMARY.md` — Phase 8 decisions
- [VCV Rack Port API docs](https://vcvrack.com/docs-v2/structrack_1_1engine_1_1Port) — `setChannels(0)` behavior, `clearVoltages()`
- [VCV Rack Polyphony Manual](https://vcvrack.com/manual/Polyphony) — Zero-channel cable conventions

### Secondary (MEDIUM confidence)
- [Electric Druid: Multimode Filters Part 1](https://electricdruid.net/multimode-filters-part-1-reconfigurable-filters/) — OB-Xa/OB-8 topology differences (CEM3320 dual vs reconfigured)
- [EarLevel Engineering: Filters for synths — the 4-pole](https://www.earlevel.com/main/2016/02/22/filters-for-synths-the-4-pole/) — 4-pole character vs 2-pole; resonance behavior
- [bep/Obxd Filter.h](https://github.com/bep/Obxd/) — OB-Xd open-source filter implementation; diode-pair inter-stage saturation pattern
- [Sound On Sound: OBX/OBXa/OB8 Review](https://www.soundonsound.com/reviews/oberheim-obx-obxa-ob8) — "selectable 12dB or 24dB…giving a nice choice of tone colours"

### Tertiary (LOW confidence)
- [Wikipedia: Oberheim OB-Xa](https://en.wikipedia.org/wiki/Oberheim_OB-Xa) — Hardware architecture (LP-only 24dB mode confirmed)
- KVR Audio DSP forums — General saturation and inter-stage nonlinearity discussions (unverified specific claims)

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — All tools are existing codebase, no new dependencies
- Architecture: HIGH — Phase 8 code inspected directly; crossfade mechanism fully understood
- OB-X character tuning: MEDIUM — Technique approach is well-grounded in DSP theory; specific coefficient values require ear testing
- Output routing: HIGH — VCV API behavior confirmed from official docs; crossfade symmetry is mathematically provable from code
- Pitfalls: HIGH — Derived from direct code analysis of the existing implementation

**Research date:** 2026-02-21
**Valid until:** Stable — no external dependencies to track