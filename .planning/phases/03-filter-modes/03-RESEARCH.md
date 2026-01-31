# Phase 3: Filter Modes - Research

**Researched:** 2026-01-31
**Domain:** State Variable Filter multimode outputs
**Confidence:** HIGH

## Summary

This phase exposes the remaining three filter modes (highpass, bandpass, notch) from the existing trapezoidal SVF implementation. The filter already computes all necessary state variables internally - this is primarily a wiring task rather than new DSP.

The standard Cytomic/Andrew Simper trapezoidal SVF implementation computes intermediate values v1 (bandpass), v2 (lowpass), and v3 (highpass reference) during each process() call. The current code returns only v2 (lowpass). Phase 3 needs to capture and return all four outputs: LP, HP, BP, and Notch.

**Primary recommendation:** Modify SVFilter::process() to return a struct containing all four outputs, compute highpass as `input - k*v1 - v2` and notch as `highpass + lowpass`, then wire these outputs in HydraQuartetVCF::process().

## Standard Stack

This phase requires no external libraries - it modifies existing VCV Rack C++ code.

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| VCV Rack SDK | 2.x | Module framework | Required for all VCV Rack modules |
| C++ Standard Library | C++11+ | Math functions (std::tanh) | Already in use |

### Existing Code Structure
The project already has:
- `src/SVFilter.hpp` - Trapezoidal SVF implementation (Phase 2)
- `src/HydraQuartetVCF.cpp` - Module with four output jacks defined but stubbed

**No installation required** - all dependencies already present.

## Architecture Patterns

### Pattern 1: Multi-Output Filter Structure
**What:** Return all filter modes from a single process() call using a struct.
**When to use:** When multiple outputs share the same internal computation (avoid recomputing).
**Example:**
```cpp
// Source: Standard SVF pattern from Cytomic trapezoidal SVF
struct SVFilterOutputs {
    float lowpass;
    float highpass;
    float bandpass;
    float notch;
};

SVFilterOutputs process(float input) {
    // ... existing computation produces v1, v2 ...
    float v1 = /* bandpass (already computed) */;
    float v2 = /* lowpass (already computed) */;
    float v3 = input - ic2eq;  // highpass reference

    // Compute additional outputs from existing state
    float highpass = input - k * v1 - v2;
    float notch = highpass + v2;  // LP + HP

    return {v2, highpass, v1, notch};
}
```

### Pattern 2: SVF Output Equations (Cytomic/Simper Standard)
**What:** Standard equations for deriving all four modes from SVF state variables.
**When to use:** Any trapezoidal integrated state variable filter.
**Equations:**
```
Given state variables after integration:
- v1 = bandpass output (directly available)
- v2 = lowpass output (directly available)
- v3 = input - ic2eq (highpass reference signal)

Output formulas:
- lowpass  = v2
- bandpass = v1
- highpass = input - k*v1 - v2
- notch    = highpass + lowpass = input - k*v1
```

**Source:** KVR Audio discussion of Andy Simper's Cytomic SVF

### Pattern 3: Simultaneous Output Wiring
**What:** Wire all outputs from single filter computation without reprocessing.
**When to use:** Always, for efficiency - SVF computes all modes in one pass.
**Example:**
```cpp
void process(const ProcessArgs& args) override {
    // ... parameter setup ...
    float input = inputs[AUDIO_INPUT].getVoltage();

    SVFilterOutputs out = filter.process(input);

    outputs[LP_OUTPUT].setVoltage(out.lowpass);
    outputs[HP_OUTPUT].setVoltage(out.highpass);
    outputs[BP_OUTPUT].setVoltage(out.bandpass);
    outputs[NOTCH_OUTPUT].setVoltage(out.notch);
}
```

### Anti-Patterns to Avoid
- **Calling process() multiple times:** The filter has state - calling process() again would advance the state incorrectly
- **Computing outputs outside the filter:** The equations need internal variables (v1, k, ic2eq) that shouldn't be exposed
- **Normalizing all outputs to unity gain:** Natural SVF behavior has different gains at resonance peaks - this is expected and musically useful

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Notch filter DSP | Custom notch algorithm | Sum HP + LP outputs | SVF outputs are already phase-aligned for notch, custom algorithm risks phase errors |
| Output level matching | Manual gain compensation per mode | Natural SVF levels | Different mode gains at resonance are inherent to SVF topology and musically expected |
| Alternative filter topologies | Ladder/biquad for other modes | Existing SVF state variables | All four modes come "for free" from the same SVF state - no need for additional filters |

**Key insight:** The SVF topology naturally produces all four filter modes from the same internal state variables. Any attempt to compute them separately or use different topologies adds complexity without benefit.

## Common Pitfalls

### Pitfall 1: Bandpass Output Gain Variation
**What goes wrong:** Bandpass output level varies significantly with Q/resonance setting. At high Q (low k), bandpass has high gain; at low Q (high k), it's attenuated.
**Why it happens:** Bandpass gain is proportional to Q in the SVF topology - this is fundamental mathematics, not a bug.
**How to avoid:** Accept natural behavior as musically correct. Do NOT normalize bandpass output.
**Warning signs:** User reports "bandpass gets louder with resonance" - this is expected SVF behavior.

### Pitfall 2: Notch Depth Varies with Resonance
**What goes wrong:** At low resonance, notch has shallow null. At high resonance, notch has deep null. This seems inconsistent.
**Why it happens:** Notch depth depends on phase cancellation accuracy between HP and LP, which is resonance-dependent in the SVF.
**How to avoid:** Accept as natural SVF characteristic. Context notes this is "Claude's discretion" - choose natural behavior.
**Warning signs:** Expecting constant notch depth like a dedicated notch filter would provide.

### Pitfall 3: Incorrect Highpass Equation
**What goes wrong:** Using `v0 - v2` for highpass instead of `input - k*v1 - v2`.
**Why it happens:** Confusion between different SVF formulations (some use v0, some use input directly).
**How to avoid:** Use the Cytomic/Simper formula: `highpass = input - k*v1 - v2`. The existing code uses `v3 = input - ic2eq` as an intermediate, but final HP needs the k*v1 term.
**Warning signs:** Highpass output has incorrect frequency response or phase at resonant frequency.

### Pitfall 4: Notch Phase Issues
**What goes wrong:** Using `input - v1` instead of `HP + LP` for notch, leading to incorrect phase relationships.
**Why it happens:** Some SVF formulations derive notch from input minus bandpass, but this requires careful phase handling.
**How to avoid:** Always use `notch = highpass + lowpass` for the Cytomic trapezoidal SVF. The HP and LP outputs are correctly phase-aligned.
**Warning signs:** Notch filter doesn't produce deep null at cutoff frequency.

### Pitfall 5: State Corruption from Multiple Calls
**What goes wrong:** Calling `filter.process()` four times thinking each returns a different mode.
**Why it happens:** Misunderstanding that process() advances filter state.
**How to avoid:** Call process() exactly once, return all outputs from that single call.
**Warning signs:** Filter instability, incorrect frequency response, or oscillation.

## Code Examples

### Complete SVFilter Modification
```cpp
// Source: Based on existing SVFilter.hpp trapezoidal implementation
// Add to SVFilter.hpp

struct SVFilterOutputs {
    float lowpass;
    float highpass;
    float bandpass;
    float notch;
};

// Modify existing process() method:
SVFilterOutputs process(float input) {
    // Clamp input to VCV Rack standard range
    input = rack::clamp(input, -12.f, 12.f);

    // Trapezoidal SVF equations with zero-delay feedback
    float v3 = input - ic2eq;
    float v1_raw = a1 * ic1eq + a2 * v3;

    // Soft saturation in feedback path for Oberheim character
    float v1 = std::tanh(v1_raw * 2.f) * 0.5f;

    float v2 = ic2eq + a2 * ic1eq + a3 * v3;

    // Update integrator states
    ic1eq = 2.f * v1 - ic1eq;
    ic2eq = 2.f * v2 - ic2eq;

    // Check for NaN/infinity and reset if needed
    if (!std::isfinite(v2)) {
        reset();
        return {0.f, 0.f, 0.f, 0.f};
    }

    // Compute all four outputs
    // Source: Cytomic SVF equations
    float lowpass = v2;
    float bandpass = v1;
    float highpass = input - k * v1 - v2;
    float notch = highpass + lowpass;  // Equivalent to: input - k * v1

    return {lowpass, highpass, bandpass, notch};
}
```

### Module Wiring
```cpp
// Source: Pattern for HydraQuartetVCF.cpp process() method
void process(const ProcessArgs& args) override {
    // ... existing parameter setup code ...

    // Update filter parameters
    filter.setParams(cutoffHz, resonanceParam, args.sampleRate);

    // Process audio - single call returns all outputs
    float input = inputs[AUDIO_INPUT].getVoltage();
    SVFilterOutputs out = filter.process(input);

    // Wire all four outputs
    outputs[LP_OUTPUT].setVoltage(out.lowpass);
    outputs[HP_OUTPUT].setVoltage(out.highpass);
    outputs[BP_OUTPUT].setVoltage(out.bandpass);
    outputs[NOTCH_OUTPUT].setVoltage(out.notch);
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Chamberlin SVF | Trapezoidal integrated SVF | ~2014 (Cytomic papers) | Better frequency response, less warping at high frequencies |
| Input - BP for notch | HP + LP for notch | Standard in modern SVF | Better phase alignment, deeper null |
| Separate filters per mode | Simultaneous outputs from one topology | Always standard for SVF | Single computation, perfect frequency tracking |

**Deprecated/outdated:**
- Chamberlin digital SVF: Has frequency warping issues at high frequencies, replaced by trapezoidal integration (already implemented in Phase 2)
- Multiple filter instances: Using separate LP/HP/BP/Notch filters wastes CPU and prevents frequency tracking

## Open Questions

1. **Output level matching preference**
   - What we know: Natural SVF has different output levels per mode, especially bandpass which varies with Q
   - What's unclear: User preference for natural vs normalized levels
   - Recommendation: Use natural SVF levels (no normalization). Context says "Claude's discretion" and "natural SVF behavior" - this means accept the mathematical behavior as-is.

2. **Soft saturation application**
   - What we know: Current code applies tanh saturation to v1 (bandpass) in feedback path
   - What's unclear: Whether this affects all outputs equally or should be mode-specific
   - Recommendation: Current implementation saturates v1 which affects LP via integration but HP/Notch see unsaturated input. This is natural behavior - keep as-is. All outputs receive the same "Oberheim character" through the shared state variables.

3. **CPU optimization for unconnected outputs**
   - What we know: All four outputs require the same computation (no savings possible)
   - What's unclear: Whether to skip setVoltage() calls for unconnected outputs
   - Recommendation: Always compute all four - the computation is shared. VCV Rack's setVoltage() is cheap even for disconnected outputs. No optimization needed.

## Sources

### Primary (HIGH confidence)
- KVR Audio Forum - [SVF filter (Cytomic) puzzle](https://www.kvraudio.com/forum/viewtopic.php?t=487826) - Verified equations: lowpass = v2, bandpass = v1, highpass = input - k*v1 - v2
- Cytomic Technical Papers - [Technical Papers index](https://cytomic.com/technical-papers/) - Andrew Simper's authoritative SVF implementations (PDFs not web-readable but referenced in community)
- Existing codebase - `/Users/trekkie/projects/vcvrack_modules/poly_obefilter/src/SVFilter.hpp` - Current trapezoidal implementation with v1, v2, v3, k variables

### Secondary (MEDIUM confidence)
- Electronics Tutorials - [State Variable Filter Design](https://www.electronics-tutorials.ws/filter/state-variable-filter.html) - Confirms notch = HP + LP summing
- Sound AU Articles - [State Variable Filters](https://sound-au.com/articles/state-variable.htm) - Confirms bandpass output level varies with Q, HP/LP outputs same level
- Wikipedia - [State variable filter](https://en.wikipedia.org/wiki/State_variable_filter) - General SVF topology and simultaneous outputs

### Tertiary (LOW confidence)
- None - all key findings verified against authoritative sources or existing code

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - No external dependencies, existing code structure verified
- Architecture: HIGH - Equations verified from Cytomic implementation discussion, existing code matches pattern
- Pitfalls: HIGH - Based on known SVF characteristics documented in multiple authoritative sources

**Research date:** 2026-01-31
**Valid until:** 2026-07-31 (6 months - SVF mathematics are stable, but VCV Rack API could change)

---

**Research notes:**
- This is an unusually simple phase - the "research" is mostly validating that the existing code already has everything needed
- The trapezoidal SVF implementation in Phase 2 already computes v1 and v2; extracting them is trivial
- Main complexity is understanding SVF output equations (verified from multiple sources)
- User context emphasizes "natural SVF behavior" - this means no normalization, no custom tweaks, just expose what the filter already computes
