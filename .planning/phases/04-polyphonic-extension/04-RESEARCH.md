# Phase 4: Polyphonic Extension - Research

**Researched:** 2026-01-31
**Domain:** VCV Rack polyphonic DSP architecture
**Confidence:** HIGH

## Summary

This research covers the transformation of the existing monophonic SVFilter into a polyphonic processor supporting up to 16 voices. VCV Rack provides well-documented polyphony conventions with clear API patterns for channel detection, voltage access, and output configuration.

The standard approach is to maintain an array of 16 filter engine instances and process only the active channels detected from the primary input. For optimal performance, SIMD processing with `float_4` can process 4 channels simultaneously, though this requires templating the filter class. The existing `SVFilter` class can be adapted to a templated design that works with both `float` (scalar) and `simd::float_4` (vector) types.

**Primary recommendation:** Start with a simple array of 16 scalar `SVFilter` instances for correctness, then optimize to SIMD in a follow-up task if performance is insufficient.

## Standard Stack

The established patterns for VCV Rack polyphonic modules:

### Core API
| Component | Purpose | Usage |
|-----------|---------|-------|
| `PORT_MAX_CHANNELS` | Maximum channel constant (16) | Array sizing |
| `inputs[].getChannels()` | Get active channel count | Determine processing scope |
| `inputs[].getPolyVoltage(c)` | Get voltage for channel c | Per-voice CV read |
| `outputs[].setChannels(n)` | Set output polyphony | Must call before writing |
| `outputs[].setVoltage(v, c)` | Set voltage for channel c | Per-voice output write |

### SIMD Types (for optimization)
| Type | Purpose | When to Use |
|------|---------|-------------|
| `simd::float_4` | 4-element SIMD vector | Processing 4 voices at once |
| `simd::int32_4` | 4-element integer vector | SIMD integer operations |
| `getPolyVoltageSimd<float_4>(c)` | Read 4 voltages at once | SIMD input reading |
| `setVoltageSimd(v, c)` | Write 4 voltages at once | SIMD output writing |

### DSP Utilities
| Class | Purpose | Notes |
|-------|---------|-------|
| `TExponentialFilter<T>` | Parameter smoothing | Templated, works with float_4 |
| `simd::ifelse()` | SIMD conditional | Replaces if/else for vectors |
| `simd::clamp()` | SIMD range limiting | Replaces rack::clamp for vectors |

**Note:** `simd::tanh` is NOT available in VCV Rack. Must implement using `simd::exp` or use polynomial approximation.

## Architecture Patterns

### Recommended Project Structure
```
src/
├── SVFilter.hpp           # Templated filter class (T = float or float_4)
├── HydraQuartetVCF.cpp    # Module with filter array
└── plugin.hpp             # Plugin declarations
```

### Pattern 1: Engine Array (Recommended Starting Point)
**What:** Array of 16 filter instances, process active channels in loop
**When to use:** Initial implementation for correctness
**Example:**
```cpp
// Source: VCV Rack Plugin Guide
struct HydraQuartetVCF : Module {
    SVFilter filters[PORT_MAX_CHANNELS];  // 16 filter instances

    void process(const ProcessArgs& args) override {
        // Get channel count from primary input (audio)
        int channels = std::max(1, inputs[AUDIO_INPUT].getChannels());

        // Process each active channel
        for (int c = 0; c < channels; c++) {
            float input = inputs[AUDIO_INPUT].getPolyVoltage(c);

            // Per-voice CV using getPolyVoltage (wraps if fewer CV channels)
            float resCv = inputs[RES_CV_INPUT].getPolyVoltage(c);

            // Process through this voice's filter
            filters[c].setParams(cutoffHz, resonance + resCv * cvAmount, args.sampleRate);
            SVFilterOutputs out = filters[c].process(input);

            // Write outputs for this channel
            outputs[LP_OUTPUT].setVoltage(out.lowpass, c);
            outputs[HP_OUTPUT].setVoltage(out.highpass, c);
            outputs[BP_OUTPUT].setVoltage(out.bandpass, c);
            outputs[NOTCH_OUTPUT].setVoltage(out.notch, c);
        }

        // Set channel count on all outputs
        outputs[LP_OUTPUT].setChannels(channels);
        outputs[HP_OUTPUT].setChannels(channels);
        outputs[BP_OUTPUT].setChannels(channels);
        outputs[NOTCH_OUTPUT].setChannels(channels);
    }
};
```

### Pattern 2: SIMD Optimization (Future Enhancement)
**What:** Process 4 channels at once using float_4 vectors
**When to use:** After correctness verified, if CPU usage is high
**Example:**
```cpp
// Source: VCV Rack Plugin Guide - SIMD section
template <typename T>
struct SVFilterSimd {
    T ic1eq = 0.f;
    T ic2eq = 0.f;
    T g = 0.f;
    T k = 0.f;
    // ... coefficients

    void process(T input, T* lp, T* hp, T* bp, T* notch) {
        // SIMD processing - same math, T = float_4
    }
};

// In process():
int channels = std::max(1, inputs[AUDIO_INPUT].getChannels());
for (int c = 0; c < channels; c += 4) {
    float_4 input = inputs[AUDIO_INPUT].getPolyVoltageSimd<float_4>(c);
    // Process 4 channels at once
    filters[c/4].process(input, ...);
    outputs[LP_OUTPUT].setVoltageSimd(lp, c);
}
```

### Pattern 3: CV Polyphony Handling
**What:** Handle mismatched CV and audio channel counts
**When to use:** Always - VCV standard behavior
**Example:**
```cpp
// Source: VCV Rack Voltage Standards
// getPolyVoltage handles all cases automatically:
// - If CV has 1 channel: copies to all voices
// - If CV has >= audio channels: uses matching channel
// - If CV has fewer channels: uses channel 0 for out-of-bounds

for (int c = 0; c < channels; c++) {
    // This automatically wraps/copies as needed
    float resCv = inputs[RES_CV_INPUT].getPolyVoltage(c);
    float cutoffCv = inputs[CUTOFF_CV_INPUT].getPolyVoltage(c);
}
```

### Anti-Patterns to Avoid
- **Hardcoding channel count:** Always derive from `inputs[].getChannels()`, not fixed value
- **Forgetting setChannels():** Output must have `setChannels()` called BEFORE `setVoltage()`
- **Using getVoltage() in poly loop:** Use `getPolyVoltage(c)` instead for correct channel access
- **Branching on SIMD elements:** Use `simd::ifelse()` instead of if/else statements
- **Not resetting unused voices:** Filter state from previous patches may persist

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| CV channel wrapping | Manual modulo/index logic | `getPolyVoltage(c)` | Handles all edge cases automatically |
| Parameter smoothing | Custom slew limiter | `TExponentialFilter<T>` | Templated, works with SIMD |
| SIMD conditionals | if/else statements | `simd::ifelse(mask, a, b)` | Required for SIMD - can't branch on elements |
| Channel count | Manual counting | `PORT_MAX_CHANNELS` | Constant, 16 |
| Output buffering | Custom voltage arrays | Port's internal voltages | Port handles storage |

**Key insight:** VCV Rack's polyphony API is comprehensive. The `getPolyVoltage(c)` method handles mono-to-poly expansion and channel wrapping automatically - do not reimplement this logic.

## Common Pitfalls

### Pitfall 1: Forgetting to Call setChannels()
**What goes wrong:** Outputs appear monophonic or have garbage in higher channels
**Why it happens:** VCV Rack needs explicit channel count on outputs
**How to avoid:** Always call `outputs[X].setChannels(channels)` before or after setting voltages
**Warning signs:** Polyphonic input, but downstream modules only see 1 channel

### Pitfall 2: Filter State Accumulation
**What goes wrong:** Voices that were previously active have stale state when reactivated
**Why it happens:** Filter integrator states persist between channel count changes
**How to avoid:** Either:
  - Reset filter state when channel becomes active after being inactive
  - Accept that warm-up is part of normal operation (most modules do this)
**Warning signs:** Click/pop when voice count increases

### Pitfall 3: NaN Propagation Across Voices
**What goes wrong:** One voice goes unstable, affects entire poly output
**Why it happens:** NaN in filter state, not caught per-voice
**How to avoid:** Check `std::isfinite()` per-voice in SIMD mode, or rely on existing per-voice check in scalar mode
**Warning signs:** All voices go silent after one blows up

### Pitfall 4: SIMD tanh Not Available
**What goes wrong:** Code doesn't compile when templated for float_4
**Why it happens:** VCV Rack's simd namespace doesn't include tanh
**How to avoid:** Implement tanh using available functions:
```cpp
// tanh(x) approximation using exp
template <typename T>
T simd_tanh(T x) {
    T exp2x = simd::exp(2.f * x);
    return (exp2x - 1.f) / (exp2x + 1.f);
}

// Or use polynomial approximation for better performance
template <typename T>
T fast_tanh(T x) {
    // Pade approximant, accurate for |x| < 3
    T x2 = x * x;
    return x * (27.f + x2) / (27.f + 9.f * x2);
}
```
**Warning signs:** Compilation error on `simd::tanh()`

### Pitfall 5: Mono Input Expansion Confusion
**What goes wrong:** Mono audio input doesn't expand to all voices
**Why it happens:** Using `getVoltage()` instead of poly-aware methods
**How to avoid:** For mono-to-poly expansion with CV determining voice count:
```cpp
int channels = std::max(1, inputs[CV_INPUT].getChannels());
for (int c = 0; c < channels; c++) {
    // getPolyVoltage copies mono input to all channels
    float audio = inputs[AUDIO_INPUT].getPolyVoltage(c);
}
```
**Warning signs:** Only voice 0 has audio when expecting parallel processing

### Pitfall 6: Cutoff CV Per-Voice Decision
**What goes wrong:** Per-voice cutoff modulation causes coefficient recalculation overhead
**Why it happens:** setParams() is called per-voice with different cutoff values
**How to avoid:** For typical filter use, cutoff can be global (computed once, shared across voices). Only resonance needs per-voice modulation for expressive patches. This is a design tradeoff:
  - Global cutoff: Less CPU, simpler, matches most analog filter behavior
  - Per-voice cutoff: More expressive, higher CPU, requires careful coefficient caching
**Recommendation:** Start with per-voice cutoff for full expressiveness (user decided per-voice CV for resonance; cutoff should match)

## Code Examples

Verified patterns from official sources:

### Basic Polyphonic Module Process Loop
```cpp
// Source: https://vcvrack.com/manual/PluginGuide
void process(const ProcessArgs& args) override {
    // Get channels from primary input
    int channels = std::max(1, inputs[AUDIO_INPUT].getChannels());

    for (int c = 0; c < channels; c++) {
        // Read polyphonic input
        float in = inputs[AUDIO_INPUT].getPolyVoltage(c);

        // Process
        float out = engines[c].process(in);

        // Write polyphonic output
        outputs[AUDIO_OUTPUT].setVoltage(out, c);
    }

    // Set output channel count
    outputs[AUDIO_OUTPUT].setChannels(channels);
}
```

### Parameter Smoothing with TExponentialFilter
```cpp
// Source: https://vcvrack.com/docs-v2/structrack_1_1dsp_1_1TExponentialFilter
rack::dsp::TExponentialFilter<float> cutoffSmoother;

// In constructor or init:
cutoffSmoother.setTau(0.001f);  // 1ms time constant

// In process:
float smoothedCutoff = cutoffSmoother.process(args.sampleTime, targetCutoff);
```

### NaN Protection Pattern
```cpp
// Source: https://vcvrack.com/manual/VoltageStandards
SVFilterOutputs out = filter.process(input);

// Per-output NaN check
float lp = std::isfinite(out.lowpass) ? out.lowpass : 0.f;
outputs[LP_OUTPUT].setVoltage(lp, c);
```

### SIMD tanh Implementation
```cpp
// For when SIMD optimization is needed
// Source: Mathematical identity, VCV Rack simd::exp available
template <typename T>
inline T simd_tanh_accurate(T x) {
    T exp2x = simd::exp(T(2.f) * x);
    return (exp2x - T(1.f)) / (exp2x + T(1.f));
}

// Fast approximation (Pade) for soft saturation
template <typename T>
inline T simd_tanh_fast(T x) {
    // Clamp input to avoid overflow
    x = simd::clamp(x, T(-3.f), T(3.f));
    T x2 = x * x;
    return x * (T(27.f) + x2) / (T(27.f) + T(9.f) * x2);
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Scalar-only filters | SIMD-optimized templates | Rack v1 (2019) | 4x performance for 16 voices |
| Manual channel wrapping | `getPolyVoltage()` API | Rack v1 | Simpler, less error-prone |
| setChannels after voltage | setChannels before preferred | Rack v2 | Clearer output state |
| No NaN protection | Cable-level NaN filtering | Rack v2.0 | More stable patches |

**Current best practice:**
- Use scalar engine array first for correctness
- Template filter class for easy SIMD upgrade path
- Trust `getPolyVoltage()` for all channel handling
- Set output channels explicitly even if they match input

## Open Questions

Things that couldn't be fully resolved:

1. **Should filter state reset when voice becomes active?**
   - What we know: Most VCV modules do NOT reset state on channel activation
   - What's unclear: Whether click/pop from stale state is acceptable
   - Recommendation: Match VCV convention - don't reset. Document that voices "warm up"

2. **Cutoff CV: per-voice or global?**
   - What we know: Per-voice resonance CV is decided. Cutoff could go either way.
   - What's unclear: Typical usage patterns and CPU impact
   - Recommendation: Per-voice cutoff for full expressiveness (consistent with resonance decision). The existing SVFilter already computes coefficients per call, so no additional overhead pattern-wise.

3. **Mono expansion default voice count**
   - What we know: User decided "mono input copies to all voices for parallel processing"
   - What's unclear: When audio is mono and no CV is connected, how many voices?
   - Recommendation: Default to 1 voice when no polyphonic reference exists. Only expand when CV defines voice count.

## Sources

### Primary (HIGH confidence)
- [VCV Rack Plugin API Guide](https://vcvrack.com/manual/PluginGuide) - Polyphonic processing patterns, SIMD examples
- [VCV Rack Port API](https://vcvrack.com/docs-v2/structrack_1_1engine_1_1Port) - getChannels, setChannels, getPolyVoltage API
- [VCV Rack Voltage Standards](https://vcvrack.com/manual/VoltageStandards) - CV polyphony rules, NaN handling
- [VCV Rack Polyphony Manual](https://vcvrack.com/manual/Polyphony) - Architecture overview
- [VCV Rack SIMD Namespace](https://vcvrack.com/docs-v2/namespacerack_1_1simd) - float_4, available math functions
- [TExponentialFilter API](https://vcvrack.com/docs-v2/structrack_1_1dsp_1_1TExponentialFilter) - Parameter smoothing

### Secondary (MEDIUM confidence)
- [VCV Community: Polyphony reminders](https://community.vcvrack.com/t/polyphony-reminders-for-plugin-developers/9572) - Best practices
- [VCV Community: Making modules polyphonic](https://community.vcvrack.com/t/making-your-monophonic-module-polyphonic/6926) - Tutorial discussion
- [VCV Rack SIMD functions source](https://vcvrack.com/docs-v2/functions_8hpp_source) - Confirmed tanh NOT available

### Tertiary (LOW confidence)
- SIMD tanh implementation strategies (derived from mathematical identity)

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Official VCV documentation
- Architecture patterns: HIGH - Official Plugin Guide examples
- Pitfalls: HIGH - Combination of official docs and verified community knowledge
- SIMD tanh workaround: MEDIUM - Mathematical derivation, not official VCV pattern

**Research date:** 2026-01-31
**Valid until:** ~60 days (VCV Rack polyphony API is stable)
