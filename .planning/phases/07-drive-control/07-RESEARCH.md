# Phase 7: Drive Control - Research

**Researched:** 2026-02-03
**Domain:** Audio saturation/drive DSP for state-variable filter modules
**Confidence:** HIGH

## Summary

Drive/saturation control adds harmonic content and compression to filtered signals through waveshaping. For this Oberheim SEM-style state-variable filter, drive enhances the analog character through blended saturation algorithms.

Research identified three key technical domains:
1. **Saturation algorithms** - Blending soft (tanh) and asymmetric waveshaping for mixed harmonics
2. **Signal path placement** - Drive location affects tone; post-filter placement simplifies implementation while preserving filter topology
3. **Output-specific saturation** - Different drive amounts per output type creates balanced tonal response

The standard approach uses a drive parameter (0-1) to blend between clean signal and saturated signal, with post-filter waveshaping applied per-output. The existing tanh saturation in the feedback path provides resonance limiting; additional drive control shapes the output for character.

**Primary recommendation:** Implement post-filter drive control using blended tanh + asymmetric waveshaping, with output-specific drive scaling (LP/BP more, HP less), linear parameter mapping, and optional gain compensation.

## Standard Stack

This is a VCV Rack C++ module using existing DSP libraries. No external libraries needed beyond what's already in use.

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| VCV Rack SDK | 2.x | Module framework | Required for all VCV Rack modules |
| C++ std::tanh | C++11 | Soft saturation | Fast, built-in, continuous differentiable |
| rack::clamp | VCV Rack | Signal limiting | VCV standard for audio range protection |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| rack::dsp::TExponentialFilter | VCV Rack | Parameter smoothing | Already used for cutoff/resonance, consistent pattern |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| std::tanh | Rational tanh approximation | Faster (4x SSE-friendly) but 2.6% max error; not needed unless CPU-bound |
| Post-filter drive | Pre-filter drive | More interaction with filter but can destabilize state-variable topology |
| Blend parameter | Hardness parameter | More intuitive but requires more complex waveshaping function |

**Installation:**
No additional dependencies required. C++ standard library and VCV Rack SDK provide all necessary functions.

## Architecture Patterns

### Recommended Signal Flow

```
Input → Filter Core → [Output Taps] → Per-Output Drive → Outputs
                ↓
        Feedback Saturation
        (existing tanh)
```

**Key separation:**
- **Feedback saturation** (existing): Controls resonance limiting, stays in filter core
- **Drive saturation** (new): Shapes output character, applied post-filter per output type

### Pattern 1: Post-Filter Drive with Output-Specific Scaling

**What:** Apply drive/saturation after filter processing, with different amounts per output type.

**When to use:** When you want drive to shape tone without affecting filter stability.

**Why:** State-variable filters have carefully balanced feedback topology. Adding drive before the filter can destabilize the integrator states. Post-filter drive is independent and safe.

**Example implementation structure:**
```cpp
// In process() loop, after filter processing:
SVFilterOutputs out = filters[c].process(input);

// Read drive parameter (0-1, linear)
float drive = params[DRIVE_PARAM].getValue();

// Apply output-specific drive
float lpOut = applyDrive(out.lowpass, drive * 1.0f);   // Full drive
float bpOut = applyDrive(out.bandpass, drive * 1.0f);  // Full drive
float hpOut = applyDrive(out.highpass, drive * 0.5f);  // Reduced drive
float notchOut = applyDrive(out.notch, drive * 0.7f);  // Medium drive

outputs[LP_OUTPUT].setVoltage(lpOut, c);
// ... etc
```

### Pattern 2: Blended Saturation for Mixed Harmonics

**What:** Blend soft (tanh) and asymmetric saturation to produce both even and odd harmonics.

**When to use:** For "musical" saturation that sounds warm and fat rather than harsh.

**Why:** Pure tanh produces only odd harmonics (square-wave-like). Asymmetry adds even harmonics for warmth and complexity.

**Example:**
```cpp
float applyDrive(float x, float driveAmount) {
    if (driveAmount < 0.01f) return x;  // Bypass for zero drive

    // Scale input for saturation range
    float gain = 1.0f + driveAmount * 4.0f;  // 1x to 5x gain
    float driven = x * gain;

    // Soft saturation (tanh) - symmetric, odd harmonics
    float soft = std::tanh(driven);

    // Asymmetric component - even harmonics
    float asym;
    if (driven >= 0.0f) {
        asym = std::tanh(driven);  // Same as soft above zero
    } else {
        // Harder saturation below zero for asymmetry
        asym = std::tanh(driven * 1.3f) / 1.3f + driven * 0.1f;
    }

    // Blend: more asymmetry at higher drive
    float blend = driveAmount * 0.4f;  // Up to 40% asymmetric
    float saturated = soft * (1.0f - blend) + asym * blend;

    // Optional: gain compensation to maintain perceived loudness
    float compensated = saturated / (1.0f + driveAmount * 0.5f);

    return compensated;
}
```

### Pattern 3: Linear Drive Mapping (User Decision)

**What:** Map drive parameter 0-1 directly to drive amount (linear).

**When to use:** When user expects consistent drive increase across knob range.

**Why:** User decided on linear knob curve for even drive increase.

**Implementation:**
```cpp
// Simple direct mapping
float drive = params[DRIVE_PARAM].getValue();  // 0.0 to 1.0
// Use drive directly in saturation calculations
```

### Anti-Patterns to Avoid

- **Pre-filter drive without compensation:** Driving the input before the filter can push state-variable integrators into overflow, causing instability or NaN outputs. If pre-filter drive is needed, add input clamping and gain compensation.

- **Hard clipping without soft transition:** Using `rack::clamp()` for drive creates harsh digital sound. Always use smooth saturation curves (tanh, atan, etc.) for musical results.

- **Forgetting DC blocking:** Asymmetric saturation introduces DC offset. Either use DC-blocking filter after saturation or ensure asymmetry is balanced to avoid output offset.

- **Drive affecting resonance feedback:** Don't apply drive to the feedback saturation path (existing tanh in SVFilter). This changes resonance character and can destabilize self-oscillation. Keep drive separate from filter core.

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Fast tanh approximation | Custom polynomial | std::tanh or rational_tanh | std::tanh is optimized by compiler; only optimize if profiling shows CPU bottleneck |
| Parameter smoothing | Manual exponential filter | rack::dsp::TExponentialFilter | Already used in SVFilter for cutoff/resonance; consistent pattern, battle-tested |
| DC blocking | Custom highpass filter | Simple DC blocker (y = x - x_prev + 0.995 * y_prev) | Well-known pattern, minimal state, effective |
| Audio range limiting | Manual if/else | rack::clamp(x, -12.f, 12.f) | VCV standard, clear intent, compiler-optimized |
| Asymmetric waveshaping | Complex piecewise function | Conditional tanh with different curves | Simpler to maintain, easier to tune by ear |

**Key insight:** VCV Rack modules prioritize simplicity and clarity over micro-optimization. Use standard library functions unless profiling proves a bottleneck. The drive control is not CPU-intensive enough to warrant custom approximations.

## Common Pitfalls

### Pitfall 1: Drive Destabilizing Filter

**What goes wrong:** Adding drive to the filter input or feedback path causes oscillation, NaN outputs, or filter instability at high resonance.

**Why it happens:** State-variable filters use trapezoidal integration with carefully balanced feedback. Extra gain in the feedback loop changes the resonance Q and can exceed stability limits.

**How to avoid:** Apply drive post-filter, after output taps. The existing feedback saturation (tanh in SVFilter.hpp line 74) already provides resonance limiting. Drive is for output character, not filter topology modification.

**Warning signs:** NaN values in filter states, unstable self-oscillation, resonance changing when drive increases.

### Pitfall 2: Inconsistent Loudness Across Drive Range

**What goes wrong:** Clean signal (drive=0) is quiet, full drive is much louder, making drive control hard to use musically.

**Why it happens:** Saturation adds gain (5x in example above) plus harmonic content increases perceived loudness. Without compensation, drive acts like a volume knob.

**How to avoid:** Add gain compensation that reduces output level as drive increases. Target: perceptually similar loudness at drive=0 and drive=1.

**Warning signs:** User constantly adjusting output volume when changing drive; drive unusable in performance.

### Pitfall 3: DC Offset from Asymmetric Saturation

**What goes wrong:** Asymmetric saturation shifts waveform up or down, creating DC offset that builds up in signal chain or causes pops.

**Why it happens:** Different transfer curves above/below zero mean average output is not zero even with zero-average input.

**How to avoid:**
- Design asymmetry to be balanced (positive and negative deviations cancel)
- Add DC blocking filter after saturation
- Test with oscilloscope/DC meter in VCV Rack

**Warning signs:** Waveform shifting in scope, pops when patching/unpatching, downstream modules showing offset.

### Pitfall 4: Harsh High-Frequency Aliasing

**What goes wrong:** Drive at high frequencies (near Nyquist) creates harsh, digital-sounding aliasing artifacts.

**Why it happens:** Waveshaping is a nonlinear process that creates harmonics. A 10kHz signal with 3x harmonics creates 30kHz (above 22.05kHz Nyquist at 44.1kHz), which aliases back as inharmonic content.

**How to avoid:**
- Keep drive moderate (not extreme distortion levels)
- Pre-filter high frequencies before saturation if drive > 0.7
- Accept some aliasing as "character" for musical saturation (not brick-wall limiting)

**Warning signs:** Harsh digital sound at high frequencies, filter sounds "fizzy" with drive.

### Pitfall 5: Parameter Smoothing Mismatch

**What goes wrong:** Drive parameter has zipper noise or different response time than cutoff/resonance.

**Why it happens:** Forgetting to smooth drive parameter, or using different time constant than other parameters.

**How to avoid:** Use same rack::dsp::TExponentialFilter pattern as cutoff/resonance (1ms tau). Smooth the drive parameter before use in saturation calculations.

**Warning signs:** Audible clicks when turning drive knob; inconsistent feel compared to cutoff/resonance.

## Code Examples

Verified patterns from DSP community and VCV Rack modules:

### Blended Asymmetric Saturation

```cpp
// Source: Music-DSP archive, Elementary Audio tutorial, adapted for VCV Rack context
// Combines soft tanh (odd harmonics) with asymmetric shaping (even harmonics)

float blendedSaturation(float x, float drive) {
    // drive: 0.0 (clean) to 1.0 (full saturation)
    if (drive < 0.01f) return x;  // Bypass for zero drive

    // Input gain: 1x to 5x based on drive
    float gain = 1.0f + drive * 4.0f;
    float scaled = x * gain;

    // Soft saturation (symmetric - odd harmonics only)
    float soft = std::tanh(scaled);

    // Asymmetric saturation (even + odd harmonics)
    float asym;
    if (scaled >= 0.0f) {
        // Above zero: same as soft
        asym = std::tanh(scaled);
    } else {
        // Below zero: harder curve + slight linear term
        // This creates asymmetry -> even harmonics
        asym = std::tanh(scaled * 1.3f) / 1.3f + scaled * 0.1f;
    }

    // Blend based on drive amount (more asymmetry at high drive)
    float asymBlend = drive * 0.4f;  // Up to 40% asymmetric
    float saturated = soft * (1.0f - asymBlend) + asym * asymBlend;

    // Gain compensation to maintain perceived level
    float makeup = 1.0f / (1.0f + drive * 0.5f);

    return saturated * makeup;
}
```

### Output-Specific Drive Scaling

```cpp
// Source: VCV Rack module patterns, filter design principles
// Different outputs get different drive amounts for balanced tone

void processWithDrive(int channel, SVFilterOutputs filterOut, float driveParam) {
    // Output-specific drive multipliers (user decision from CONTEXT.md)
    const float LP_DRIVE_MULT = 1.0f;   // Full drive on lowpass
    const float BP_DRIVE_MULT = 1.0f;   // Full drive on bandpass
    const float HP_DRIVE_MULT = 0.5f;   // Reduced drive on highpass
    const float NOTCH_DRIVE_MULT = 0.7f; // Medium drive on notch

    // Apply drive with output-specific scaling
    float lpOut = blendedSaturation(filterOut.lowpass, driveParam * LP_DRIVE_MULT);
    float bpOut = blendedSaturation(filterOut.bandpass, driveParam * BP_DRIVE_MULT);
    float hpOut = blendedSaturation(filterOut.highpass, driveParam * HP_DRIVE_MULT);
    float notchOut = blendedSaturation(filterOut.notch, driveParam * NOTCH_DRIVE_MULT);

    // Set outputs
    outputs[LP_OUTPUT].setVoltage(lpOut, channel);
    outputs[BP_OUTPUT].setVoltage(bpOut, channel);
    outputs[HP_OUTPUT].setVoltage(hpOut, channel);
    outputs[NOTCH_OUTPUT].setVoltage(notchOut, channel);
}
```

### Simple DC Blocker (if needed)

```cpp
// Source: Music-DSP archive standard pattern
// Use if asymmetric saturation creates audible DC offset

struct DCBlocker {
    float x_prev = 0.0f;
    float y_prev = 0.0f;

    float process(float x) {
        float y = x - x_prev + 0.995f * y_prev;
        x_prev = x;
        y_prev = y;
        return y;
    }

    void reset() {
        x_prev = 0.0f;
        y_prev = 0.0f;
    }
};

// Add to SVFilter struct if needed:
// DCBlocker dcBlockers[4];  // One per output type
```

### Parameter Smoothing for Drive

```cpp
// Source: Existing pattern from SVFilter.hpp
// Consistent with cutoff/resonance smoothing

// In SVFilter struct or main module:
rack::dsp::TExponentialFilter<float> driveSmoother;

// In constructor:
driveSmoother.setTau(0.001f);  // 1ms tau, matches cutoff/resonance

// In process():
float driveParam = params[DRIVE_PARAM].getValue();
float smoothedDrive = driveSmoother.process(1.f / args.sampleRate, driveParam);
// Use smoothedDrive in saturation calculations
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Hard clipping (foldback, clamp) | Soft saturation (tanh, atan) | ~2000s | Musical warmth vs. harsh digital sound |
| Single saturation curve | Blended/asymmetric curves | ~2010s | Mixed harmonics, richer tone |
| Pre-filter drive | Post-filter drive in digital | ~2015+ | Stability and predictability |
| Fixed drive amount | Parameter-controlled blend | Standard now | User control over saturation character |

**Current best practices (2026):**
- Soft saturation curves (tanh, rational approximations) for musicality
- Asymmetric waveshaping for even harmonics and warmth
- Post-filter placement for stability in digital state-variable designs
- Optional gain compensation for consistent loudness
- Parameter smoothing to prevent zipper noise

**Deprecated/outdated:**
- Hard clipping as primary saturation (sounds harsh and digital)
- Uncompensated drive (makes control act like volume knob)
- Pre-filter drive without stabilization (causes filter instability)

## Open Questions

### 1. Exact Drive Multipliers Per Output

**What we know:** User decided LP/BP get more drive, HP gets less. This matches analog filter behavior (bass content saturates more naturally).

**What's unclear:** Exact multiplier values. Research suggests:
- LP: 1.0x (full drive)
- BP: 1.0x (full drive)
- HP: 0.5x (half drive)
- Notch: 0.7x (medium drive)

**Recommendation:** Start with these values, adjust by ear during implementation. The key is relative scaling, not absolute numbers.

### 2. Gain Compensation Strategy

**What we know:** User marked "gain staging" as Claude's discretion. Options are preserve level or get louder with drive.

**What's unclear:** User preference for musical context.

**Recommendation:** Implement moderate gain compensation (output level grows slightly with drive, but not proportionally to input gain). This is most musical - some "excitement" from drive but not overwhelming. Make it tunable in code for easy adjustment.

### 3. Low Drive Transparency

**What we know:** Default is 0 (clean). User wants intentional drive, not always-on coloration.

**What's unclear:** Should drive=0 be bit-perfect pass-through, or slight coloration acceptable?

**Recommendation:** True bypass at drive=0 (if drive < 0.01f return x). No DSP overhead, perfectly clean. User explicitly wants "no saturation until user adds drive intentionally."

### 4. Drive/Resonance Interaction

**What we know:** User marked as Claude's discretion. In analog filters, drive can "squeeze" resonance at high levels.

**What's unclear:** Should drive affect resonance peak in digital implementation?

**Recommendation:** No interaction. Keep drive post-filter. The existing feedback tanh already handles resonance limiting. Adding drive/resonance interaction complicates implementation and user mental model. Simple and predictable is better.

### 5. Self-Oscillation Tone with Drive

**What we know:** Filter self-oscillates at high resonance. User wants Oberheim character.

**What's unclear:** How should drive affect sine-wave self-oscillation tone?

**Recommendation:** Drive shapes self-oscillation like any other signal. At high drive, self-oscillation becomes richer (more harmonics), slightly compressed. This is natural behavior of post-filter saturation and matches analog expectations.

## Sources

### Primary (HIGH confidence)
- Music-DSP Archive (musicdsp.org) - Rational tanh approximation, variable hardness clipping, waveshaping algorithms
- VCV Rack SDK and existing module codebase (SVFilter.hpp, HydraQuartetVCF.cpp) - Parameter patterns, signal flow, smoothing
- Elementary Audio Tutorial on Distortion/Saturation - Asymmetric waveshaping implementation and theory

### Secondary (MEDIUM confidence)
- [Variable-Hardness Clipping Function - Music DSP](https://www.musicdsp.org/en/latest/Effects/104-variable-hardness-clipping-function.html)
- [Rational tanh Approximation - Music DSP](https://www.musicdsp.org/en/latest/Other/238-rational-tanh-approximation.html)
- [Distortion, Saturation, and Wave Shaping - Elementary Audio](https://www.elementary.audio/docs/tutorials/distortion-saturation-wave-shaping)
- [Variable tanh() for saturation - KVR Audio](https://www.kvraudio.com/forum/viewtopic.php?t=465091)
- [What makes SEM filter so special - KVR Audio](https://www.kvraudio.com/forum/viewtopic.php?t=497961)
- [Self-oscillating State Variable Filter - KVR Audio](https://www.kvraudio.com/forum/viewtopic.php?t=333894)
- [VCV Library - Distortion Tag](https://library.vcvrack.com/?tag=Distortion)
- [Cytomic VCV Rack Modules](https://cytomic.com/vcv-rack-modules/)
- [State Variable Filters - Sound AU](https://sound-au.com/articles/state-variable.htm)
- [Auto Gain Compensation - KVR Audio](https://www.kvraudio.com/forum/viewtopic.php?t=448374)

### Tertiary (LOW confidence)
- WebSearch findings about Oberheim SEM OTA character - Multiple forum discussions indicate "tube-like" saturation, but specific technical details about CA3080 transfer curves not verified with official documentation
- Output-specific drive amounts - No authoritative source found; recommendation based on audio engineering principles (bass saturates more than treble in analog circuits)

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Using existing C++ std library and VCV Rack SDK functions already in codebase
- Architecture: HIGH - Post-filter drive placement is well-established pattern; blended saturation algorithms verified in Music-DSP archive
- Pitfalls: HIGH - Common filter stability and saturation issues are well-documented in DSP community
- Output-specific drive amounts: MEDIUM - Based on analog filter behavior principles, but specific multipliers will need tuning by ear
- Drive/resonance interaction: MEDIUM - Research shows interaction exists in analog, but recommendation to omit is based on digital implementation simplicity

**Research date:** 2026-02-03
**Valid until:** 2026-03-05 (30 days - stable DSP domain, saturation algorithms are well-established)
