# Phase 5: Cutoff Control - Research

**Researched:** 2026-01-31
**Domain:** VCV Rack filter cutoff CV modulation and parameter behavior
**Confidence:** HIGH

## Summary

This research covers refining the cutoff frequency control for the HydraQuartet VCF-OB module. The existing implementation already has a functional cutoff knob (0-1 mapped to 20Hz-20kHz), CV input with attenuverter, and V/Oct scaling. This phase ensures the controls work correctly according to VCV Rack conventions and user decisions from CONTEXT.md.

The standard approach for filter cutoff in VCV Rack uses exponential (logarithmic) frequency mapping for the knob (frequency doubles per equal knob increment), 1V/octave exponential scaling for CV input (frequency doubles per volt), and bipolar attenuverters (-1 to +1) that default to center (0) for no CV effect. The existing implementation follows these patterns closely but needs refinement: the default cutoff position should be 1.0 (fully open, 20kHz) rather than 0.5, and the configParam tooltip should display the actual frequency in Hz.

Parameter smoothing (already implemented with 1ms tau) prevents zipper noise effectively. The trapezoidal SVF architecture handles audio-rate modulation well, but the existing smoothing occurs inside SVFilter.hpp - for per-voice CV modulation this is correct as each voice may have different cutoff values.

**Primary recommendation:** Update the configParam default to 1.0 (fully open), add proper Hz display to the tooltip, and verify the existing V/Oct CV scaling is working correctly. The core cutoff control logic is already solid.

## Standard Stack

The established patterns for VCV Rack filter cutoff control:

### Core
| Component | Current Status | Purpose | VCV Standard |
|-----------|----------------|---------|--------------|
| configParam | Default 0.5 | Knob configuration | Default should be 1.0 for "fully open" |
| Exponential knob mapping | `20*pow(1000,p)` | 20Hz-20kHz range | Correct logarithmic sweep |
| V/Oct CV scaling | `pow(2, cv*atten)` | Exponential CV | Correct 1V/octave tracking |
| Attenuverter | -1 to +1, default 0 | CV depth/polarity | Correct bipolar range |
| TExponentialFilter | 1ms tau | Parameter smoothing | Correct for zipper prevention |

### Parameter Display
| Feature | Implementation | Notes |
|---------|----------------|-------|
| Frequency display | Not implemented | Should show Hz in tooltip |
| Unit label | Not implemented | Add " Hz" suffix |
| Display base | Not used | Can use exponential base 2 |

### Existing Code (from HydraQuartetVCF.cpp)
```cpp
// Current implementation
configParam(CUTOFF_PARAM, 0.f, 1.f, 0.5f, "Cutoff");  // Default 0.5 - should be 1.0
configParam(CUTOFF_ATTEN_PARAM, -1.f, 1.f, 0.f, "Cutoff CV");  // Correct

// Current frequency calculation
float baseCutoffHz = 20.f * std::pow(1000.f, cutoffParam);  // 20Hz-20kHz - correct

// Current CV application
cutoffHz = baseCutoffHz * std::pow(2.f, cutoffCV * cvAmount);  // V/Oct - correct
```

## Architecture Patterns

### Recommended Changes

The existing architecture is sound. Changes needed are primarily configuration refinements:

### Pattern 1: Fully Open Default Position
**What:** Default cutoff at 1.0 (20kHz) so filter passes audio immediately on patch load
**Why:** User decision from CONTEXT.md - filter should start "open"
**Implementation:**
```cpp
// Change from:
configParam(CUTOFF_PARAM, 0.f, 1.f, 0.5f, "Cutoff");

// To:
configParam(CUTOFF_PARAM, 0.f, 1.f, 1.f, "Cutoff", " Hz", 0.f, 0.f);
```

### Pattern 2: Frequency Display in Tooltip
**What:** Show actual frequency value in Hz when hovering over cutoff knob
**Why:** User expectation for filter modules
**Implementation:**
```cpp
// Option A: Manual display in getParamQuantity (more control)
// In constructor or dataFromJson:
ParamQuantity* pq = getParamQuantity(CUTOFF_PARAM);
if (pq) {
    pq->displayMultiplier = 1.f;
    pq->displayOffset = 0.f;
    // Custom display via displayFunction if needed
}

// Option B: Compute and set display value per frame (simplest)
// Calculate frequency in process(), store in module member for tooltip
```

Note: VCV Rack's configParam displayBase works for simple exponential relationships, but `20*pow(1000,p)` is not a simple base-2 exponential. Custom display logic may be needed for accurate Hz readout.

### Pattern 3: V/Oct CV Scaling (Already Correct)
**What:** Exponential frequency scaling where each volt doubles/halves frequency
**Why:** VCV Rack standard, matches oscillator pitch tracking
**Current Implementation:**
```cpp
// This is correct - frequency doubles per volt when cvAmount is 1.0
cutoffHz = baseCutoffHz * std::pow(2.f, cutoffCV * cvAmount);
```

When attenuverter is at +1.0, a 1V CV change produces 1 octave of frequency change. When at -1.0, polarity is inverted (1V lowers frequency by 1 octave). When at 0, no CV effect.

### Pattern 4: Attenuverter at Center = No Effect
**What:** When CUTOFF_ATTEN_PARAM is 0, CV input has no effect on cutoff
**Why:** User must consciously enable modulation
**Current Implementation:**
```cpp
// When cvAmount = 0: pow(2, cv * 0) = pow(2, 0) = 1
// baseCutoffHz * 1 = baseCutoffHz (no change) - CORRECT
```

### Anti-Patterns to Avoid
- **Applying CV before knob:** Always compute base frequency from knob first, then apply CV as multiplier
- **Linear CV scaling:** Use `pow(2, cv)` not `baseCutoff + cv * range` - linear sounds unmusical
- **Smoothing CV input directly:** Smooth the final Hz value, not the raw CV - the current implementation does this correctly inside SVFilter

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Parameter smoothing | Custom slew limiter | `TExponentialFilter` (already used) | Sample-rate independent, well-tested |
| Frequency clamping | Complex range logic | `rack::clamp(hz, 20, 20000)` (already used) | Simple, handles edge cases |
| V/Oct conversion | Linear math | `pow(2, voltage)` (already used) | Standard, musically correct |
| Polyphonic CV | Manual channel loop | `getPolyVoltage(c)` (already used) | Handles mono-to-poly expansion |

**Key insight:** The existing implementation already uses the correct patterns. This phase is about configuration refinement, not algorithmic changes.

## Common Pitfalls

### Pitfall 1: Wrong Default Cutoff Position
**What goes wrong:** Filter sounds "dull" or quiet when first patched because cutoff is too low
**Why it happens:** Default was 0.5 (mid-range), which is ~632 Hz
**How to avoid:** Set default to 1.0 (fully open, 20kHz)
**Warning signs:** New users complaining filter "doesn't work" or "is too quiet"

### Pitfall 2: Tooltip Shows Wrong Value
**What goes wrong:** Tooltip shows "0.50" or "50%" instead of "632 Hz"
**Why it happens:** configParam doesn't have proper display configuration
**How to avoid:** Configure displayMultiplier/displayOffset or use custom display function
**Warning signs:** User confusion about actual cutoff frequency

### Pitfall 3: CV Attenuverter Confusion
**What goes wrong:** Users expect CV to work immediately but nothing happens
**Why it happens:** Attenuverter defaults to 0 (center) = no CV effect
**How to avoid:** This is intentional (per CONTEXT.md), but label knob clearly as "Cutoff CV" or "CV Amount"
**Warning signs:** Support questions about "CV not working"

### Pitfall 4: Extreme CV Values Cause Issues
**What goes wrong:** With +/-10V CV range and full attenuverter, cutoff can go far outside audible range
**Why it happens:** `20 * pow(1000, 1) * pow(2, 10)` = 20kHz * 1024 = 20.48 MHz (way above Nyquist)
**How to avoid:** Clamp final frequency to 20Hz-20kHz (already done)
**Warning signs:** None if clamping is working - but verify clamp catches extreme cases

### Pitfall 5: Knob Feels "Backwards" at Low Frequencies
**What goes wrong:** Small knob movements at low end produce tiny frequency changes
**Why it happens:** Logarithmic mapping concentrates resolution at high end
**How to avoid:** This is actually correct for audio! Low frequencies need less resolution. Document that left = closed, right = open.
**Warning signs:** None - this is correct behavior for filters

### Pitfall 6: Zipper Noise During Fast Knob Sweeps
**What goes wrong:** Audible stepping artifacts when sweeping cutoff quickly
**Why it happens:** Parameter smoothing tau too fast or not applied
**How to avoid:** Existing 1ms tau is good starting point. Test with fast sweeps at high resonance.
**Warning signs:** Crackling/stepping during cutoff automation

## Code Examples

Verified patterns from existing implementation and VCV standards:

### Current Cutoff Calculation (Verified Correct)
```cpp
// Source: HydraQuartetVCF.cpp lines 54-70
// Read global parameters (knob positions)
float cutoffParam = params[CUTOFF_PARAM].getValue();
float baseCutoffHz = 20.f * std::pow(1000.f, cutoffParam);  // 20Hz-20kHz log
float cvAmount = params[CUTOFF_ATTEN_PARAM].getValue();

// Calculate per-voice cutoff (CV is polyphonic)
float cutoffHz = baseCutoffHz;
if (inputs[CUTOFF_CV_INPUT].isConnected()) {
    float cutoffCV = inputs[CUTOFF_CV_INPUT].getPolyVoltage(c);
    cutoffHz = baseCutoffHz * std::pow(2.f, cutoffCV * cvAmount);  // V/Oct
}
cutoffHz = rack::clamp(cutoffHz, 20.f, 20000.f);  // Clamp to audible range
```

### Recommended configParam Update
```cpp
// Source: VCV Rack Plugin Guide - configParam
// Before:
configParam(CUTOFF_PARAM, 0.f, 1.f, 0.5f, "Cutoff");

// After (with frequency display):
// Note: Custom display needed because 20*pow(1000,p) is not simple exponential
configParam(CUTOFF_PARAM, 0.f, 1.f, 1.f, "Cutoff", " Hz");
// Then in constructor:
getParamQuantity(CUTOFF_PARAM)->displayBase = 0.f;  // Disable built-in scaling
// Custom display requires overriding ParamQuantity::getDisplayValueString()
```

### Simple Frequency Display Alternative
```cpp
// Simplest approach: just update label to show current Hz value
// In process() or a slower callback, update the parameter description
// Note: This is a less clean solution but avoids ParamQuantity subclassing
```

### Parameter Smoothing (Already in SVFilter.hpp)
```cpp
// Source: SVFilter.hpp lines 23-46
rack::dsp::TExponentialFilter<float> cutoffSmoother;
cutoffSmoother.setTau(0.001f);  // 1ms time constant

// Initialize on first call to avoid ramp-up delay
if (!initialized) {
    cutoffSmoother.out = cutoffHz;
    initialized = true;
}

// Smooth parameters to avoid zipper noise
float smoothedCutoff = cutoffSmoother.process(1.f / sampleRate, cutoffHz);
```

### Verifying V/Oct Tracking
```cpp
// Test values for V/Oct verification:
// At attenuverter = 1.0:
//   CV = 0V -> baseCutoff * pow(2, 0) = baseCutoff * 1 = baseCutoff
//   CV = 1V -> baseCutoff * pow(2, 1) = baseCutoff * 2 (one octave up)
//   CV = -1V -> baseCutoff * pow(2, -1) = baseCutoff * 0.5 (one octave down)
//   CV = 10V -> baseCutoff * pow(2, 10) = baseCutoff * 1024 (clamped to 20kHz)
// At attenuverter = -1.0:
//   CV = 1V -> baseCutoff * pow(2, -1) = baseCutoff * 0.5 (inverted - octave down)
// At attenuverter = 0:
//   Any CV -> baseCutoff * pow(2, 0) = baseCutoff (no effect)
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Linear cutoff knob | Logarithmic (exp) mapping | Always for audio | Musically useful sweep |
| Linear CV | 1V/Oct exponential CV | VCV standard | Tracks pitch properly |
| Unipolar attenuverter | Bipolar (-1 to +1) | Modular convention | Allows CV inversion |
| Hardcoded default 0.5 | Context-aware default | Per project | User experience |

**Current best practice:**
- Cutoff knob: logarithmic mapping (frequency doubles per equal knob increment)
- CV input: 1V/octave exponential scaling
- Attenuverter: bipolar, centered default
- Default position: "fully open" for immediate audio pass-through
- Smoothing: 1-10ms tau for parameter changes

## Open Questions

Things that couldn't be fully resolved:

1. **Custom Hz Display in Tooltip**
   - What we know: VCV Rack's configParam displayBase works for simple exponential (base 2), but our mapping is `20*pow(1000,p)` which is base 1000
   - What's unclear: Best approach - subclass ParamQuantity, use displayBase with offset, or just show percentage
   - Recommendation: Start with simpler label "Cutoff" without Hz display. Functional correctness first, tooltip polish later. Can use displayBase=1000, displayMultiplier=20 as approximation.

2. **Smoothing Tau Value**
   - What we know: 1ms is implemented, prevents most zipper noise
   - What's unclear: Whether 1ms is optimal or if 5-10ms would sound smoother
   - Recommendation: Keep 1ms for now (fast response), test with various modulation sources. Can expose as advanced setting later if needed.

3. **CV Range Documentation**
   - What we know: +/-10V CV with bipolar attenuverter allows 20 octaves of modulation
   - What's unclear: Whether users expect this or if 5V (10 octave) default range would be more intuitive
   - Recommendation: Keep +/-10V support (wider is better), document in module manual that attenuverter scales CV depth.

## Sources

### Primary (HIGH confidence)
- [VCV Rack Voltage Standards](https://vcvrack.com/manual/VoltageStandards) - CV ranges, 1V/oct standard
- [VCV Rack Plugin Guide](https://vcvrack.com/manual/PluginGuide) - configParam syntax and display options
- [VCV Rack TExponentialFilter API](https://vcvrack.com/docs-v2/structrack_1_1dsp_1_1TExponentialFilter) - Parameter smoothing
- Existing implementation in `src/HydraQuartetVCF.cpp` and `src/SVFilter.hpp` - Current working code
- Phase 05 CONTEXT.md - User decisions on defaults and behavior

### Secondary (MEDIUM confidence)
- [Vult Stabile Filter](https://modlfo.github.io/VultModules/stabile/) - Filter open/closed conventions
- [Bogaudio VCF](https://github.com/bogaudio/BogaudioModules) - Multiple CV input patterns
- [KVR Forum: SVF modulation](https://www.kvraudio.com/forum/viewtopic.php?t=511314) - Zipper noise prevention
- [KVR Forum: Parameter smoothing](https://www.kvraudio.com/forum/viewtopic.php?t=427412) - Tau time constant guidance

### Tertiary (LOW confidence)
- [Mod Wiggler: Filter tracking](https://modwiggler.com/forum/viewtopic.php?t=184573) - 1V/oct calibration discussion
- [Learning Modular: 1V/oct](https://learningmodular.com/glossary/1-voct/) - General V/oct explanation

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Existing implementation follows VCV patterns, verified with official docs
- Architecture patterns: HIGH - Minimal changes needed, all patterns already implemented correctly
- Pitfalls: HIGH - Based on existing code analysis and VCV community patterns
- Code examples: HIGH - Derived from actual working implementation

**Research limitations:**
- Custom ParamQuantity for Hz display not fully explored (deferred as polish item)
- No hands-on A/B testing of different tau values for smoothing
- CV range user expectations based on VCV conventions, not user testing

**Research date:** 2026-01-31
**Valid until:** 2026-03-31 (60 days - stable domain, existing implementation proven)
