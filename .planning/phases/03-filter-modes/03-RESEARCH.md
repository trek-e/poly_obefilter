# Phase 3: Filter Modes - Research

**Researched:** 2026-01-31
**Domain:** Multi-mode SVF output wiring
**Confidence:** HIGH

## Summary

This research investigated exposing additional filter mode outputs (HP, BP, Notch) from an existing trapezoidal SVF that currently only outputs lowpass. The trapezoidal SVF topology naturally computes all filter modes simultaneously during processing - the bandpass (v1) and lowpass (v2) are already calculated as integrator outputs, while highpass and notch are simple algebraic combinations of these values.

The standard SVF output equations are well-established across DSP literature and implementations. Highpass is computed as `HP = input - k*v1 - v2`, and notch is computed as `Notch = HP + LP` (equivalently `input - k*v1`). These are direct output taps requiring no additional state computation or DSP processing.

**Critical implementation detail:** The existing SVFilter.hpp applies soft saturation to v1 in the feedback path (`v1 = tanh(v1_raw * 2) * 0.5`). The question is whether output taps should use `v1_raw` (clean, pre-saturation) or `v1` (saturated). For authentic SVF behavior, outputs should use v1_raw for clean filter response, while only the feedback path uses saturated v1 for stability.

**Primary recommendation:** Modify SVFilter::process() to return all four outputs, compute HP/Notch using v1_raw (pre-saturation bandpass), apply saturation only in feedback path (ic1eq update) for authentic SVF behavior with Oberheim character in resonance only.

## Standard Stack

No new libraries required - Phase 3 uses existing Phase 2 implementation.

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Existing SVFilter.hpp | Phase 2 | Trapezoidal SVF with state variables | Already implements all required computations |
| VCV Rack SDK | 2.6.6+ | Module framework | Established in Phase 2 |

### Supporting
None - this phase only exposes existing calculations.

## Architecture Patterns

### Pattern 1: Multi-Output SVF Process Function
**What:** Modify process() to return all four filter modes simultaneously via output struct
**When to use:** Always - SVF naturally computes all modes, exposing them costs negligible CPU

**Example:**
```cpp
// SVFilter.hpp - Extended process function
struct SVFilterOutputs {
    float lowpass;
    float highpass;
    float bandpass;
    float notch;
};

// Multi-output version
SVFilterOutputs process(float input) {
    input = rack::clamp(input, -12.f, 12.f);

    // Trapezoidal SVF equations
    float v3 = input - ic2eq;
    float v1_raw = a1 * ic1eq + a2 * v3;

    // Soft saturation in FEEDBACK PATH ONLY
    float v1 = std::tanh(v1_raw * 2.f) * 0.5f;

    float v2 = ic2eq + a2 * ic1eq + a3 * v3;

    // Update integrator states with saturated value
    ic1eq = 2.f * v1 - ic1eq;
    ic2eq = 2.f * v2 - ic2eq;

    // NaN protection
    if (!std::isfinite(v2)) {
        reset();
        return {0.f, 0.f, 0.f, 0.f};
    }

    // Output taps using CLEAN (unsaturated) v1_raw
    float bp = v1_raw;                    // Bandpass (before saturation)
    float lp = v2;                        // Lowpass
    float hp = input - k * v1_raw - v2;   // Highpass (note: k coefficient!)
    float notch = lp + hp;                // Notch (LP + HP sum)

    return {lp, hp, bp, notch};
}
```

**Key design choice:** Use `v1_raw` (pre-saturation) for output taps. The saturation is applied only in the feedback path (ic1eq update) to provide Oberheim-style stability and character at high resonance, but outputs remain clean for authentic SVF frequency response.

### Pattern 2: VCV Rack Multiple Simultaneous Outputs
**What:** Module provides all four output jacks wired from same filter core
**When to use:** Standard pattern for multi-mode filters (Vult Stabile, etc.)

**Example:**
```cpp
// In Module::process()
SVFilterOutputs out = filter.process(input);

// Wire all outputs
outputs[LP_OUTPUT].setVoltage(out.lowpass);
outputs[HP_OUTPUT].setVoltage(out.highpass);
outputs[BP_OUTPUT].setVoltage(out.bandpass);
outputs[NOTCH_OUTPUT].setVoltage(out.notch);
```

### Pattern 3: Natural Output Level Behavior
**What:** Each filter mode has its natural gain response - no normalization applied
**When to use:** Authentic SVF behavior per user decision (no volume compensation)

**Rationale:**
- Lowpass: Unity gain at DC, rolls off above cutoff
- Highpass: Zero gain at DC, unity gain above cutoff
- Bandpass: Peak gain at cutoff (amplitude = Q), reduced by resonance damping
- Notch: Unity gain at DC and high frequencies, null at cutoff

These level differences are inherent to filter topology and musically useful.

### Anti-Patterns to Avoid
- **Applying saturation to output taps:** Saturation should only be in feedback path (ic1eq update). Output taps should use clean v1_raw.
- **Using saturated v1 for outputs:** This compresses dynamics and changes frequency response. Use v1_raw for outputs, v1 only for state update.
- **Normalizing output levels:** Destroys authentic filter behavior and relative loudness cues
- **Computing outputs only when jacks connected:** Negligible CPU savings, adds complexity

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| SVF output equations | Custom derivation | Standard formulas: HP = input - k*v1 - v2, Notch = HP + LP | Equations are mathematically proven, variations will sound wrong |
| Output level matching | Gain compensation per mode | Natural SVF levels | User decision: authentic behavior, no compensation |
| Selective output computation | if (connected) compute | Always compute all | SVF computes v1/v2 anyway, taps are free algebraically |
| Bandpass saturation | Apply tanh to BP output | Use v1_raw (pre-saturation) | Saturation in feedback provides stability, outputs should be clean |

**Key insight:** The trapezoidal SVF equations are canonical. Phase 2 already computes v1_raw and v2 (the integrator outputs). HP and Notch are simple algebraic combinations requiring 2-3 arithmetic operations total. The saturation is ONLY for feedback stability, not for output character.

## Common Pitfalls

### Pitfall 1: Using Saturated v1 for Outputs
**What goes wrong:** Using the saturated `v1` value for bandpass output and HP/Notch calculations causes compressed dynamics and altered frequency response
**Why it happens:** Confusion between feedback path (needs saturation for stability) and output taps (should be clean)
**How to avoid:** Always use `v1_raw` (pre-saturation) for output computations. Only the state update `ic1eq = 2*v1 - ic1eq` uses saturated v1.
**Warning signs:** Bandpass sounds compressed even at low input levels, HP/Notch outputs have non-linear distortion

### Pitfall 2: Incorrect Notch Equation
**What goes wrong:** Using `input - 2*k*v1` (allpass) or `input - v1` instead of `HP + LP` or `input - k*v1`
**Why it happens:** Confusion between SVF output modes, incorrect source material
**How to avoid:** Use canonical notch formula: `Notch = LP + HP` or equivalently `input - k*v1`. Verify with test signal that notch nulls at cutoff frequency.
**Warning signs:** Notch doesn't null at cutoff, phase response is wrong, sounds like allpass or comb filter

### Pitfall 3: Missing k Coefficient in HP Calculation
**What goes wrong:** Computing HP as `input - v1 - v2` instead of `input - k*v1 - v2`
**Why it happens:** Forgetting the resonance damping coefficient in highpass formula
**How to avoid:** Always include k in HP equation. This is the standard Cytomic/Simper SVF formula.
**Warning signs:** HP output level varies wildly with resonance, self-oscillation behavior is wrong, frequency response doesn't match theory

### Pitfall 4: Calling process() Multiple Times
**What goes wrong:** Calling process() four times thinking each returns a different mode
**Why it happens:** Misunderstanding that process() advances filter state variables
**How to avoid:** Call process() exactly once, return all outputs from that single call via struct
**Warning signs:** Filter instability, incorrect frequency response, or oscillation

### Pitfall 5: Bandpass Output Gain Variation
**What goes wrong:** Bandpass output level varies significantly with Q/resonance setting (high Q = high gain)
**Why it happens:** Bandpass gain is proportional to Q in SVF topology - this is fundamental mathematics
**How to avoid:** Accept natural behavior as musically correct. Do NOT normalize bandpass output.
**Warning signs:** User reports "bandpass gets louder with resonance" - this is expected SVF behavior, not a bug

## Code Examples

Verified patterns from SVF theory and implementations:

### Complete Multi-Output SVF
```cpp
// Based on trapezoidal SVF (Simper/Cytomic) with clean output taps
// Source: Phase 2 SVFilter.hpp + standard SVF output equations

struct SVFilterOutputs {
    float lowpass;
    float highpass;
    float bandpass;
    float notch;
};

SVFilterOutputs process(float input) {
    input = rack::clamp(input, -12.f, 12.f);

    // Trapezoidal integration
    float v3 = input - ic2eq;
    float v1_raw = a1 * ic1eq + a2 * v3;

    // Saturation ONLY in feedback path (for stability/Oberheim character)
    float v1_saturated = std::tanh(v1_raw * 2.f) * 0.5f;

    float v2 = ic2eq + a2 * ic1eq + a3 * v3;

    // Update states with saturated value (for stability)
    ic1eq = 2.f * v1_saturated - ic1eq;
    ic2eq = 2.f * v2 - ic2eq;

    // NaN check
    if (!std::isfinite(v2)) {
        reset();
        return {0.f, 0.f, 0.f, 0.f};
    }

    // Output taps using CLEAN (unsaturated) v1_raw for authentic response
    float bp = v1_raw;                    // Bandpass
    float lp = v2;                        // Lowpass
    float hp = input - k * v1_raw - v2;   // Highpass (include k!)
    float notch = lp + hp;                // Notch (LP + HP sum)

    return {lp, hp, bp, notch};
}
```

### Alternative: Backward-Compatible Single Output
```cpp
// If maintaining backward compatibility with existing code:
float process(float input) {
    SVFilterOutputs out = processAll(input);
    return out.lowpass;  // Return only LP for compatibility
}

SVFilterOutputs processAll(float input) {
    // ... full implementation as above ...
}
```

### Notch Null Depth Verification
```cpp
// Test pattern: Verify notch nulls at cutoff frequency
// At cutoff with high Q, notch output should be near zero

float testFreq = 1000.f;
filter.setParams(testFreq, 0.9f, sampleRate);  // High resonance

for (int i = 0; i < sampleRate; i++) {
    float t = (float)i / sampleRate;
    float sine = std::sin(2.f * M_PI * testFreq * t);

    SVFilterOutputs out = filter.process(sine);

    // At steady state, notch should null to < -40dB
    if (i > sampleRate / 10) {  // Skip transient
        assert(std::abs(out.notch) < 0.01f);
    }
}
```

### VCV Module Output Wiring
```cpp
// In HydraQuartetVCF.cpp process() function
void process(const ProcessArgs& args) override {
    // ... parameter setup code ...

    float input = inputs[AUDIO_INPUT].getVoltage();

    // Process filter once, get all outputs
    SVFilterOutputs out = filter.process(input);

    // Set all output jacks
    outputs[LP_OUTPUT].setVoltage(out.lowpass);
    outputs[HP_OUTPUT].setVoltage(out.highpass);
    outputs[BP_OUTPUT].setVoltage(out.bandpass);
    outputs[NOTCH_OUTPUT].setVoltage(out.notch);
}
```

## State of the Art

| Aspect | Standard Approach | Notes |
|--------|------------------|-------|
| Output computation | All modes computed simultaneously | v1/v2 exist, HP/Notch are 2-3 ops |
| Saturation location | Feedback path only (ic1eq update) | Outputs use clean v1_raw, feedback uses saturated v1 |
| Output levels | Natural/uncompensated | Authentic SVF behavior per user decision |
| Notch equation | LP + HP or input - k*BP | Both formulations equivalent |
| VCV pattern | Multiple simultaneous outputs | Standard for multi-mode filters (Stabile, etc.) |

**No deprecated approaches** - Phase 3 is simple output wiring, not new DSP techniques.

## Open Questions

Things that couldn't be fully resolved:

1. **Bandpass output: v1_raw vs v1 (saturated)**
   - What we know: Feedback path uses saturated v1 for stability. Outputs can use either.
   - What's unclear: User preference for "Oberheim character" - does this mean saturated BP output or only resonance behavior?
   - Recommendation: Use v1_raw (clean) for BP output. User wants "authentic SVF" which means clean frequency response. Saturation provides stability/character at high resonance, not output distortion. Can be tuned if testing reveals preference for saturated BP.

2. **Notch null depth at maximum resonance**
   - What we know: Notch = HP + LP should null at cutoff. Depth depends on phase alignment and coefficient precision.
   - What's unclear: Expected null depth in dB with high Q (0.9-0.95 resonance)
   - Recommendation: Target -40dB or better null at cutoff. Verify with test signals. If null is poor, check that HP uses v1_raw (not saturated v1) and k coefficient is accurate.

3. **Self-oscillation on non-LP outputs**
   - What we know: At max resonance, filter self-oscillates. All outputs (HP/BP/Notch) produce sine tones.
   - What's unclear: Relative amplitudes and phase relationships at self-oscillation
   - Recommendation: Natural SVF behavior - all outputs oscillate at cutoff frequency. BP has highest amplitude (proportional to Q), LP/HP have equal amplitude, Notch nulls (no oscillation). This is correct SVF behavior.

## Sources

### Primary (HIGH confidence)
- Phase 2 Research: Trapezoidal SVF implementation - `.planning/phases/02-core-filter-dsp/02-RESEARCH.md`
- Existing implementation: SVFilter.hpp - `/Users/trekkie/projects/vcvrack_modules/poly_obefilter/src/SVFilter.hpp` - Shows v1_raw vs v1 saturation pattern
- Phase 3 Context: User decisions - `.planning/phases/03-filter-modes/03-CONTEXT.md` - Confirms "authentic SVF behavior" preference
- Electronics Tutorials: [State Variable Filter Design](https://www.electronics-tutorials.ws/filter/state-variable-filter.html) - Confirms notch = LP + HP
- Wikipedia: [State variable filter](https://en.wikipedia.org/wiki/State_variable_filter) - Standard SVF topology and equations

### Secondary (MEDIUM confidence)
- Cytomic Technical Papers: [https://cytomic.com/technical-papers/](https://cytomic.com/technical-papers/) - Referenced for SvfLinearTrapOptimised2.pdf (PDF not web-readable but authoritative source)
- KVR Audio Forum: [SVF filter (Cytomic) puzzle](https://www.kvraudio.com/forum/viewtopic.php?t=487826) - Community discussion of Simper equations
- Vult Modules Stabile: [https://modlfo.github.io/VultModules/stabile/](https://modlfo.github.io/VultModules/stabile/) - VCV Rack SVF with LP/HP/BP outputs, demonstrates multi-output pattern
- GitHub: [VAStateVariableFilter](https://github.com/JordanTHarris/VAStateVariableFilter) - Virtual analog SVF with 8 output types
- Web search findings: Multiple sources confirm HP = input - k*v1 - v2 and Notch = HP + LP

### Tertiary (LOW confidence)
- Stanford CCRMA: [Digital State-Variable Filters](https://ccrma.stanford.edu/~jos/svf/svf.pdf) - PDF not accessible (binary/compressed), referenced for theoretical foundation

## Metadata

**Confidence breakdown:**
- Standard equations: HIGH - HP and Notch formulas verified across multiple independent sources and existing Phase 2 code
- Architecture: HIGH - Pattern is straightforward extension of existing SVFilter.hpp with proven equations
- Saturation handling: MEDIUM-HIGH - Standard practice is feedback-only saturation. BP output using v1_raw (clean) is recommended for authentic frequency response, but could use v1 if user prefers saturated character (requires testing)
- Pitfalls: HIGH - Identified from common mistakes in SVF implementations (wrong equations, saturating outputs, missing k coefficient)

**Research limitations:**
- Cytomic PDF not directly accessible (binary format), relying on Phase 2 research and secondary sources
- No hands-on testing of actual notch null depth achieved with Phase 2 implementation
- v1_raw vs v1 for BP output is somewhat subjective - choosing clean output (v1_raw) per user "authentic SVF" preference, but this could be tuned during implementation/testing

**Key decision made during research:**
Use v1_raw (pre-saturation) for all output taps. The saturation is exclusively for feedback stability (Oberheim character emerges at high resonance through the feedback loop), not for output processing. This maintains clean SVF frequency response while preserving the Phase 2 stability improvements.

**Research date:** 2026-01-31
**Valid until:** 2026-04-30 (90 days - stable domain, SVF equations unchanged for decades)
