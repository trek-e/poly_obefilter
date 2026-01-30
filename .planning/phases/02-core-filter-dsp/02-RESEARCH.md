# Phase 2: Core Filter DSP - Research

**Researched:** 2026-01-30
**Domain:** Digital audio signal processing - state-variable filters in VCV Rack
**Confidence:** MEDIUM-HIGH

## Summary

This research investigated implementing a SEM-style 12dB/oct state-variable filter (SVF) with trapezoidal integration (zero-delay feedback) for VCV Rack. The standard approach uses trapezoidal integration with frequency pre-warping to create a digital filter that accurately models analog behavior. The VCV Rack SDK provides essential utilities for parameter smoothing, oversampling/decimation, and numerical stability.

The trapezoidal SVF (popularized by Andy Simper/Cytomic) offers superior numerical stability compared to biquads, handles audio-rate modulation without blowing up, and provides simultaneous access to multiple filter modes (LP/HP/BP/Notch). Implementation requires careful coefficient calculation with frequency warping (tan(π·fc/fs)), exponential frequency scaling for 1V/oct tracking, and soft saturation in the feedback path for Oberheim character.

Key challenges include maintaining stability at high frequencies (requires oversampling or coefficient clamping), preventing DC offset buildup, protecting against NaN/infinity propagation, and smoothing parameter changes to avoid zipper noise.

**Primary recommendation:** Use trapezoidal SVF with Andy Simper's optimized coefficient calculations, VCV Rack's `dsp::Upsampler`/`Decimator` for optional 2x oversampling, `dsp::TExponentialFilter` for parameter smoothing, and tanh() soft saturation in the feedback path.

## Standard Stack

The established libraries/tools for this domain:

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| VCV Rack SDK | 2.6.6+ | Audio plugin framework | Only option for VCV Rack modules |
| rack::dsp namespace | 2.x | DSP utilities (filters, resampling) | Official SDK utilities, optimized and tested |
| C++11 | 11+ | Implementation language | VCV Rack requirement |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| dsp::Upsampler | SDK 2.x | Upsample by integer factor | 2x oversampling implementation |
| dsp::Decimator | SDK 2.x | Downsample by integer factor | Decimate after oversampled processing |
| dsp::TExponentialFilter | SDK 2.x | Exponential parameter smoothing | Prevent zipper noise on cutoff/resonance |
| dsp::TRCFilter | SDK 2.x | Simple RC filter | DC blocking (highpass at ~20Hz) |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Trapezoidal SVF | dsp::TBiquadFilter | Biquads have numerical issues at low frequencies, worse for modulation |
| Custom oversampling | dsp::Upsampler/Decimator | SDK classes are well-tested and efficient |
| Linear parameter ramping | dsp::TExponentialFilter | Exponential smoothing is more natural for audio |

**Installation:**
VCV Rack SDK is installed via the Rack development environment. No additional dependencies required for filter implementation.

## Architecture Patterns

### Recommended Module Structure
```
src/
├── HydraQuartetVCF.cpp      # Main module (UI, params, process())
├── SVFilter.hpp             # SVF filter class (reusable)
└── plugin.hpp               # Plugin definitions
```

### Pattern 1: State-Variable Filter Class
**What:** Encapsulate filter state and processing in a reusable class with clear interface
**When to use:** Always - separates DSP from module boilerplate, enables testing and reuse

**Example:**
```cpp
// SVFilter.hpp - Trapezoidal SVF with soft saturation
struct SVFilter {
    // Filter state (integrator outputs)
    float ic1eq = 0.f;  // First integrator state
    float ic2eq = 0.f;  // Second integrator state

    // Coefficients (updated when params change)
    float g = 0.f;      // tan(π * cutoff / sampleRate)
    float k = 0.f;      // 1 / Q (damping)
    float a1 = 0.f;     // 1 / (1 + g*(g + k))
    float a2 = 0.f;     // g * a1
    float a3 = 0.f;     // g * a2

    // Set filter parameters
    void setParams(float cutoffHz, float resonance, float sampleRate) {
        // Frequency warping for trapezoidal integration
        float cutoffNorm = cutoffHz / sampleRate;
        cutoffNorm = clamp(cutoffNorm, 0.f, 0.49f);  // Stability limit
        g = std::tan(M_PI * cutoffNorm);

        // Resonance to Q (higher resonance = lower damping)
        // Map 0-1 resonance to reasonable Q range (0.5 to ~20)
        k = 1.f / (0.5f + resonance * 19.5f);

        // Pre-compute coefficients
        a1 = 1.f / (1.f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    // Process one sample, return lowpass output
    float process(float input, float drive = 0.f) {
        // Soft saturation on input (optional drive)
        if (drive > 0.f) {
            input = std::tanh(input * (1.f + drive * 4.f));
        }

        // State-space processing with zero-delay feedback
        float v3 = input - ic2eq;
        float v1 = a1 * ic1eq + a2 * v3;
        float v2 = ic2eq + a2 * ic1eq + a3 * v3;

        // Soft saturation in feedback path (Oberheim character)
        float bp = v1;
        bp = std::tanh(bp * 2.f) * 0.5f;  // Soft clip, scale back

        // Update states
        ic1eq = 2.f * v1 - ic1eq;
        ic2eq = 2.f * v2 - ic2eq;

        // Return lowpass output (v2)
        return v2;
    }

    void reset() {
        ic1eq = ic2eq = 0.f;
    }
};
```

### Pattern 2: Exponential Frequency Scaling (1V/oct)
**What:** Convert voltage CV to frequency using exponential scaling
**When to use:** Always for musical pitch tracking and filter cutoff control

**Example:**
```cpp
// In process():
// Base cutoff from knob (0-1, mapped to log frequency range)
float cutoffParam = params[CUTOFF_PARAM].getValue();
// Map 0-1 to 20Hz-20kHz logarithmically
float baseCutoffHz = 20.f * std::pow(1000.f, cutoffParam);  // 20Hz to 20kHz

// 1V/oct CV input with attenuverter
float cutoffCV = inputs[CUTOFF_CV_INPUT].getVoltage();
float cvAmount = params[CUTOFF_ATTEN_PARAM].getValue();  // -2 to +2
cutoffCV *= cvAmount;

// Apply exponential scaling: frequency doubles per volt
float cutoffHz = baseCutoffHz * std::pow(2.f, cutoffCV);

// Clamp to valid range
cutoffHz = clamp(cutoffHz, 20.f, 20000.f);
```

### Pattern 3: Parameter Smoothing
**What:** Use exponential smoothing to prevent zipper noise on parameter changes
**When to use:** For cutoff, resonance, and any other audio-rate parameters

**Example:**
```cpp
// In module struct:
dsp::TExponentialFilter<float> cutoffSmoother;
dsp::TExponentialFilter<float> resonanceSmoother;

// In constructor:
cutoffSmoother.setTau(0.001f);  // 1ms time constant (fast but smooth)
resonanceSmoother.setTau(0.001f);

// In process():
float smoothedCutoff = cutoffSmoother.process(args.sampleTime, cutoffHz);
float smoothedResonance = resonanceSmoother.process(args.sampleTime, resonanceParam);
filter.setParams(smoothedCutoff, smoothedResonance, args.sampleRate);
```

### Pattern 4: 2x Oversampling
**What:** Process audio at 2x sample rate to reduce aliasing, then decimate back
**When to use:** Optional quality mode, toggled via panel switch

**Example:**
```cpp
// In module struct:
dsp::Upsampler<2, 8> upsampler;
dsp::Decimator<2, 8> decimator;
bool oversampleEnabled = false;

// In process():
float input = inputs[AUDIO_INPUT].getVoltage();

if (oversampleEnabled) {
    // Upsample input (produces 2 samples)
    float upsampled[2];
    upsampler.process(input, upsampled);

    // Process both samples at 2x rate
    float processed[2];
    for (int i = 0; i < 2; i++) {
        processed[i] = filter.process(upsampled[i]);
    }

    // Decimate back to 1x
    float output = decimator.process(processed);
    outputs[LP_OUTPUT].setVoltage(output);
} else {
    // Normal 1x processing
    float output = filter.process(input);
    outputs[LP_OUTPUT].setVoltage(output);
}
```

### Pattern 5: DC Blocking
**What:** High-pass filter at very low frequency (~20Hz) to prevent DC buildup
**When to use:** On filter output if self-oscillation or resonance causes DC offset

**Example:**
```cpp
// In module struct:
dsp::TRCFilter<float> dcBlocker;

// In constructor:
dcBlocker.setCutoffFreq(20.f / args.sampleRate);  // 20Hz highpass

// In process():
float filterOut = filter.process(input);
dcBlocker.process(filterOut);
float output = dcBlocker.highpass();
outputs[LP_OUTPUT].setVoltage(output);
```

### Anti-Patterns to Avoid
- **Setting coefficients every sample:** Pre-compute g/k/a1/a2/a3 only when parameters change (use parameter smoothing to avoid recomputing every sample)
- **No frequency clamping:** Cutoff must be < 0.5 * sampleRate (Nyquist), preferably < 0.49 for stability margin
- **No NaN protection:** Check for isfinite() on state variables if extreme parameters are possible
- **Hard clipping:** Use tanh() or other soft saturation for musical distortion, not hard clamps
- **Direct parameter use:** Always smooth parameters with TExponentialFilter to avoid zipper noise

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Parameter smoothing | Simple linear interpolation | `dsp::TExponentialFilter` | Exponential smoothing is more natural, SDK class handles sample-rate independence |
| Oversampling | Custom FIR filters | `dsp::Upsampler` / `dsp::Decimator` | SDK classes use optimized kernels, handle quality/performance tradeoffs |
| DC blocking | Custom 1-pole highpass | `dsp::TRCFilter::highpass()` | SDK class is stable and efficient for DC removal |
| Frequency warping | Linear cutoff mapping | `tan(π * fc/fs)` | Required for accurate analog modeling in trapezoidal integration |
| 1V/oct scaling | Linear or ad-hoc curves | `pow(2.f, voltage)` | Standard exponential scaling, doubles frequency per volt |

**Key insight:** VCV Rack's DSP utilities are battle-tested across thousands of modules. They handle edge cases (sample rate changes, extreme parameters, numerical stability) that custom implementations often miss. The trapezoidal SVF equations are mathematically complex - use established formulas (Simper/Zavalishin) rather than deriving from scratch.

## Common Pitfalls

### Pitfall 1: High-Frequency Instability
**What goes wrong:** Filter becomes unstable or produces incorrect output at high cutoff frequencies (>8kHz at 48kHz sample rate)
**Why it happens:** Trapezoidal integration assumes smooth signals. At fc approaching Nyquist, the tan() function grows rapidly and coefficients become ill-conditioned
**How to avoid:** Clamp normalized cutoff to 0.49 (just under Nyquist). For extended range, implement 2x oversampling
**Warning signs:** Crackling, distortion, or silence at high cutoff settings

### Pitfall 2: Resonance Volume Drop Without Compensation
**What goes wrong:** Resonance attenuates signal, making filter seem quiet at high Q
**Why it happens:** Energy is concentrated in narrow band, reducing overall RMS level. This is authentic analog behavior (per user decision: no volume compensation)
**How to avoid:** Accept as design feature. Document in manual. Users can compensate with output gain if needed
**Warning signs:** User complaints about volume loss at high resonance (expected behavior)

### Pitfall 3: Self-Oscillation Instability
**What goes wrong:** At maximum resonance, filter oscillates but frequency drifts, amplitude explodes, or output goes silent/NaN
**Why it happens:** Without soft saturation, feedback can grow unbounded. With too much saturation, oscillation is suppressed entirely
**How to avoid:** Tune soft saturation carefully - enough to limit amplitude (~10V peak-to-peak), not so much that Q is artificially limited. Map resonance param to Q range like 0.5 to ~20
**Warning signs:** Runaway volume, NaN in output, no oscillation at max resonance

### Pitfall 4: Zipper Noise on Cutoff Modulation
**What goes wrong:** Audible stepping/crackling when cutoff is modulated by CV or knob turning
**Why it happens:** Filter coefficients change discretely each sample, creating discontinuities in signal
**How to avoid:** Use `dsp::TExponentialFilter` with ~1ms tau on cutoff frequency. Smooth the Hz value before converting to coefficients
**Warning signs:** Zipper/crackling sound when tweaking cutoff knob slowly, especially audible at high resonance

### Pitfall 5: DC Offset Buildup
**What goes wrong:** Filter output develops DC offset, especially with self-oscillation or asymmetric input signals
**Why it happens:** Numerical drift in integrator states, nonlinear elements (saturation) introduce harmonics with DC component
**How to avoid:** Optional DC blocker (20Hz highpass) on output. Reset state variables when input is disconnected and output settles near zero
**Warning signs:** Scope shows output waveform shifted above/below zero, downstream modules clipping unexpectedly

### Pitfall 6: NaN Propagation
**What goes wrong:** Filter outputs NaN (not-a-number), breaking downstream modules and audio output
**Why it happens:** Division by zero in coefficient calculation, denormal numbers in state variables, extreme input values, or math operations on invalid numbers
**How to avoid:** Clamp inputs to reasonable range (±12V per VCV standards), check `isfinite()` on coefficients after calculation, periodically validate state variables
**Warning signs:** Silent output, crash logs, red error indicators in VCV Rack

### Pitfall 7: Incorrect 1V/oct Tracking
**What goes wrong:** Filter cutoff doesn't track pitch CV correctly - octave jumps are wrong size or direction
**Why it happens:** Using linear scaling instead of exponential, wrong base frequency, incorrect polarity on CV
**How to avoid:** Always use `pow(2.f, voltage)` for 1V/oct. Test with known voltages: 0V = base freq, 1V = 2x base, -1V = 0.5x base
**Warning signs:** Filter doesn't track keyboard, octaves sound wrong, negative CV causes extreme behavior

## Code Examples

Verified patterns from official sources:

### Complete SVF Process Function
```cpp
// Based on Andy Simper's trapezoidal SVF (Cytomic)
// Source: https://cytomic.com/technical-papers/ (SvfLinearTrapOptimised2.pdf)
// Adapted for VCV Rack module implementation

struct TrapezoidalSVF {
    float ic1eq = 0.f;  // Integrator 1 state
    float ic2eq = 0.f;  // Integrator 2 state
    float g, k, a1, a2, a3;

    void calculateCoefficients(float cutoffHz, float Q, float sampleRate) {
        // Pre-warp frequency for trapezoidal integration
        float wc = 2.f * M_PI * cutoffHz;
        float T = 1.f / sampleRate;
        g = std::tan(wc * T / 2.f);

        // Resonance control
        k = 1.f / Q;

        // Pre-compute coefficients
        float denom = 1.f / (1.f + g * (g + k));
        a1 = denom;
        a2 = g * a1;
        a3 = g * a2;
    }

    void process(float v0, float& lp, float& bp, float& hp) {
        // v0 = input sample
        // lp, bp, hp = output references for simultaneous modes

        float v3 = v0 - ic2eq;
        float v1 = a1 * ic1eq + a2 * v3;
        float v2 = ic2eq + a2 * ic1eq + a3 * v3;

        // Update integrator states
        ic1eq = 2.f * v1 - ic1eq;
        ic2eq = 2.f * v2 - ic2eq;

        // Output taps
        lp = v2;
        bp = v1;
        hp = v0 - k * v1 - v2;
    }
};
```

### Soft Saturation in Feedback Path
```cpp
// Source: KVR DSP forum discussions on SVF saturation
// Links: https://www.kvraudio.com/forum/viewtopic.php?t=538190

// Apply tanh() to bandpass output before feedback
// This is the key to Oberheim SEM character and stability
float processSVFWithSaturation(float input) {
    float v3 = input - ic2eq;
    float v1_raw = a1 * ic1eq + a2 * v3;

    // Soft saturation on feedback path (tune drive amount)
    float v1 = std::tanh(v1_raw * 2.f) * 0.5f;  // Drive then scale back

    float v2 = ic2eq + a2 * ic1eq + a3 * v3;

    ic1eq = 2.f * v1 - ic1eq;
    ic2eq = 2.f * v2 - ic2eq;

    return v2;  // Lowpass output
}

// Alternative: Fast tanh approximation for efficiency
// Source: https://www.musicdsp.org/en/latest/Other/238-rational-tanh-approximation.html
inline float fastTanh(float x) {
    float x2 = x * x;
    return x * (27.f + x2) / (27.f + 9.f * x2);
}
```

### DC Blocking Filter
```cpp
// Source: VCV Rack SDK filter.hpp
// https://vcvrack.com/docs-v2/filter_8hpp_source

dsp::TRCFilter<float> dcBlocker;

// Initialize (in constructor)
void initDCBlocker(float sampleRate) {
    // Set cutoff to 20Hz (standard DC blocking frequency)
    dcBlocker.setCutoffFreq(20.f / sampleRate);
}

// Use in process loop
float removeDC(float input) {
    dcBlocker.process(input);
    return dcBlocker.highpass();
}
```

### Frequency Scaling Examples
```cpp
// Source: VCV Rack Voltage Standards
// https://vcvrack.com/manual/VoltageStandards

// 1V/oct pitch tracking (C4 = 261.6256 Hz at 0V)
float voltageToFrequency(float voltage) {
    const float C4 = 261.6256f;
    return C4 * std::pow(2.f, voltage);
}

// Logarithmic cutoff knob mapping (20Hz to 20kHz)
float knobToCutoff(float knobValue) {
    // knobValue: 0.0 to 1.0
    return 20.f * std::pow(1000.f, knobValue);  // 20 * (1000^x) = 20Hz to 20kHz
}

// Alternative: Linear in log space
float knobToCutoffAlt(float knobValue) {
    float logMin = std::log2(20.f);     // log2(20) ≈ 4.32
    float logMax = std::log2(20000.f);  // log2(20000) ≈ 14.29
    float logFreq = logMin + knobValue * (logMax - logMin);
    return std::pow(2.f, logFreq);
}
```

### NaN/Infinity Protection
```cpp
// Source: Digital filter stability best practices
// https://www.dsprelated.com/freebooks/filters/Stability_Revisited.html

// Validate filter state periodically
void validateState() {
    if (!std::isfinite(ic1eq)) ic1eq = 0.f;
    if (!std::isfinite(ic2eq)) ic2eq = 0.f;
}

// Safe processing wrapper
float processSafe(float input) {
    // Clamp input to VCV Rack standards (±12V max)
    input = clamp(input, -12.f, 12.f);

    // Process normally
    float output = filter.process(input);

    // Validate output
    if (!std::isfinite(output)) {
        filter.reset();
        output = 0.f;
    }

    return output;
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Forward Euler SVF | Trapezoidal SVF | ~2010s (Zavalishin book) | More accurate analog modeling, stable under modulation |
| Biquad filters for everything | SVF where appropriate | ~2015+ | Better numerical stability at low frequencies, easier parameter control |
| Fixed sample rate DSP | Sample-rate-aware processing | VCV Rack 2.0 (2021) | Modules work correctly at any sample rate (44.1k-384k) |
| Hard clipping | Soft saturation (tanh) | ~2010s VA modeling | More musical distortion, prevents hard artifacts |
| Manual coefficient calculation | Pre-warped coefficients | Established DSP practice | Accurate frequency response matching analog prototypes |

**Deprecated/outdated:**
- **Chamberlin SVF (forward Euler):** Older SVF implementation, less accurate than trapezoidal, can be unstable with modulation
- **Fixed 48kHz-only DSP:** VCV Rack 2.0+ supports arbitrary sample rates, must use `args.sampleRate` not hardcoded values
- **Context menu for oversampling:** VCV standards prefer panel switches for common features, context menu for advanced/rare options

## Open Questions

Things that couldn't be fully resolved:

1. **Optimal self-oscillation threshold for SEM character**
   - What we know: Original SEM "never quite reaches full self oscillation" but "sounded best at fullest resonance"
   - What's unclear: Exact Q value where self-oscillation begins, how to tune soft saturation for authentic SEM behavior
   - Recommendation: Start with Q range 0.5-20 (resonance 0-1), tune empirically. Test with audio input at max resonance - should produce clear sine tone without runaway

2. **Cytomic SVF paper (SvfLinearTrapOptimised2.pdf) details**
   - What we know: It's the authoritative source for trapezoidal SVF, cited by many implementations
   - What's unclear: Full mathematical derivation, nonlinear extensions, optimal coefficient scaling
   - Recommendation: PDF wasn't accessible in research. Implementation from secondary sources (musicdsp.org, forum discussions) is sufficient for Phase 2. Consider purchasing Cytomic technical papers for Phase 3+ enhancements

3. **2x vs higher oversampling tradeoffs**
   - What we know: 2x is common compromise, VCV modules typically offer off/2x/4x
   - What's unclear: CPU overhead of 2x Upsampler<2,8> in typical VCV patch, whether 2x is sufficient for distortion-free operation at high resonance
   - Recommendation: Implement optional 2x (panel switch), measure CPU, document findings for future phases

4. **DC blocking necessity**
   - What we know: Some filters accumulate DC offset, especially with nonlinear elements
   - What's unclear: Whether trapezoidal SVF with soft saturation produces significant DC in practice
   - Recommendation: Implement filter without DC blocker initially, test with scope module. Add DC blocker if offset >0.1V at idle or with self-oscillation

## Sources

### Primary (HIGH confidence)
- VCV Rack Manual: DSP Best Practices - https://vcvrack.com/manual/DSP
- VCV Rack API: dsp namespace - https://vcvrack.com/docs-v2/namespacerack_1_1dsp
- VCV Rack API: filter.hpp source - https://vcvrack.com/docs-v2/filter_8hpp_source
- VCV Rack Voltage Standards - https://vcvrack.com/manual/VoltageStandards
- EarLevel Engineering: Digital State Variable Filter - http://www.earlevel.com/main/2003/03/02/the-digital-state-variable-filter/
- Music DSP Archive: State Variable Filter (Double Sampled) - https://www.musicdsp.org/en/latest/Filters/92-state-variable-filter-double-sampled-stable.html
- DSP Related: DC Blocker - https://www.dsprelated.com/freebooks/filters/DC_Blocker.html
- DSP Related: Stability - https://www.dsprelated.com/freebooks/filters/Stability_Revisited.html

### Secondary (MEDIUM confidence)
- Cytomic Technical Papers page - https://cytomic.com/technical-papers/ (cited but PDFs not directly accessible)
- KVR Forum: SVF filter saturation in feedback path - https://www.kvraudio.com/forum/viewtopic.php?t=538190
- KVR Forum: What makes SEM filter so special - https://www.kvraudio.com/forum/viewtopic.php?t=497961
- Music DSP Archive: Rational tanh approximation - https://www.musicdsp.org/en/latest/Other/238-rational-tanh-approximation.html
- Arturia Filter SEM description - https://www.arturia.com/products/software-effects/sem-filter/overview
- VCV Community: DC blocker in Rack API - https://community.vcvrack.com/t/dc-blocker-in-rack-api/8419

### Tertiary (LOW confidence)
- Native Instruments VA Filter Design PDF (Zavalishin) - referenced by many sources but direct extraction failed, content inferred from citations
- GitHub: implicit_svf repository - https://github.com/janne808/implicit_svf (repository structure visible but implementation code not extracted)
- Various GitHub repositories (BogaudioModules, Vult, etc.) - structure and features documented but specific implementation code not reviewed

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - VCV Rack SDK is well-documented, dsp namespace utilities are official and stable
- Architecture: MEDIUM-HIGH - Trapezoidal SVF equations verified from multiple sources, VCV patterns verified from SDK docs, but Cytomic PDF not fully accessed
- Pitfalls: MEDIUM - Common issues documented in forums and DSP literature, but some are context-dependent (may need empirical validation)
- Code examples: MEDIUM-HIGH - SDK examples are HIGH confidence, SVF equations verified across sources, saturation approaches based on forum discussions (MEDIUM)

**Research limitations:**
- Cytomic SvfLinearTrapOptimised2.pdf not directly accessible (referenced extensively but couldn't extract equations)
- No hands-on testing of VCV Rack dsp::Upsampler/Decimator performance characteristics
- SEM filter behavior inferred from hardware descriptions and forum discussions, not from original Oberheim schematics or precise measurements
- Soft saturation "tuning" for Oberheim character requires empirical testing (beyond research scope)

**Research date:** 2026-01-30
**Valid until:** 2026-04-30 (90 days - relatively stable domain, VCV Rack SDK changes infrequently, DSP fundamentals unchanged)
