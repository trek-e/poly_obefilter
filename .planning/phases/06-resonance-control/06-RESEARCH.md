# Phase 6: Resonance Control - Research

**Researched:** 2026-02-03
**Domain:** VCV Rack resonance parameter control with CV modulation
**Confidence:** HIGH

## Summary

This research covers adding user-accessible resonance control with CV modulation to the HydraQuartet VCF-OB filter module. The resonance DSP implementation already exists from Phase 2 (Q range 0.5-20 mapped from 0-1 parameter, soft saturation in feedback path for Oberheim character). This phase exposes that functionality through panel controls: a resonance knob, CV input jack, and CV attenuverter knob.

The standard approach for resonance control in VCV Rack modules mirrors cutoff control patterns: a main parameter knob (0-1 range), a bipolar attenuverter (-1 to +1, default 0) for CV depth/polarity control, and additive CV combination (knob + scaled CV, clamped to valid range). The existing cutoff control implementation provides a proven reference pattern - resonance should follow the same architecture for consistency.

User decisions from CONTEXT.md specify: Q range 0.5-20 (keep existing DSP), default resonance 0 (no emphasis), CV scaling 10% per volt (matches cutoff), additive knob+CV combination, bipolar CV support, and panel layout mirroring cutoff design (vertical stack with small attenuverter knob near CV jack). Parameter smoothing (1ms tau, already implemented in SVFilter.hpp) prevents zipper noise.

**Primary recommendation:** Add RESONANCE_ATTEN_PARAM following the exact pattern of CUTOFF_ATTEN_PARAM. The DSP, smoothing, and CV infrastructure already exist - this phase is purely about exposing the attenuverter control to the user.

## Standard Stack

The established patterns for VCV Rack resonance control:

### Core (Already Implemented)
| Component | Current Status | Purpose | VCV Standard |
|-----------|----------------|---------|--------------|
| configParam RESONANCE_PARAM | Exists, default 0.f | Main resonance knob | Range 0-1, maps to Q 0.5-20 |
| RESONANCE_CV_INPUT | Exists | CV modulation jack | Standard PJ301MPort |
| SVFilter Q mapping | Implemented | Q = 0.5 + resonance * 19.5 | Covers subtle to self-oscillation |
| TExponentialFilter smoothing | Implemented (1ms tau) | Prevents zipper noise | Standard smoothing approach |
| Soft saturation | Implemented (tanh in feedback) | Oberheim character | Clean self-oscillation |

### Missing (This Phase)
| Component | Need to Add | Purpose | Pattern to Follow |
|-----------|-------------|---------|-------------------|
| RESONANCE_ATTEN_PARAM | Yes | CV attenuverter knob | Copy CUTOFF_ATTEN_PARAM pattern |
| Attenuverter logic | Yes | Scale/invert CV | resonance + resCV * attenAmount |
| Panel widget | Yes | Small knob on panel | RoundSmallBlackKnob like cutoff |

### Reference Implementation (from Cutoff)
```cpp
// Existing cutoff pattern (lines 7, 35, 56, 110 in HydraQuartetVCF.cpp)
enum ParamId {
    CUTOFF_PARAM,
    CUTOFF_ATTEN_PARAM,  // <-- Attenuverter parameter
    ...
};

configParam(CUTOFF_ATTEN_PARAM, -1.f, 1.f, 0.f, "Cutoff CV");

float cvAmount = params[CUTOFF_ATTEN_PARAM].getValue();
if (inputs[CUTOFF_CV_INPUT].isConnected()) {
    float cutoffCV = inputs[CUTOFF_CV_INPUT].getPolyVoltage(c);
    cutoffHz = baseCutoffHz * std::pow(2.f, cutoffCV * cvAmount);
}

addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(35.56, 62.0)),
    module, HydraQuartetVCF::CUTOFF_ATTEN_PARAM));
```

### Current Resonance Implementation (Needs Attenuverter)
```cpp
// Current resonance code (lines 8, 36, 57, 74-76, 111 in HydraQuartetVCF.cpp)
enum ParamId {
    RESONANCE_PARAM,
    // MISSING: RESONANCE_ATTEN_PARAM should go here
};

configParam(RESONANCE_PARAM, 0.f, 1.f, 0.f, "Resonance");
// MISSING: configParam for attenuverter

float resonanceParam = params[RESONANCE_PARAM].getValue();

// Current CV application (hardcoded 0.1 scaling)
if (inputs[RESONANCE_CV_INPUT].isConnected()) {
    float resCV = inputs[RESONANCE_CV_INPUT].getPolyVoltage(c);
    resonance = rack::clamp(resonanceParam + resCV * 0.1f, 0.f, 1.f);  // <-- Replace 0.1f with attenuverter
}

addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(35.56, 88.0)),
    module, HydraQuartetVCF::RESONANCE_PARAM));
// MISSING: RoundSmallBlackKnob for attenuverter
```

## Architecture Patterns

### Pattern 1: Bipolar Attenuverter Parameter
**What:** A small knob controlling CV modulation depth and polarity (-1 to +1)
**When to use:** Always for CV modulation of audio parameters in VCV Rack
**Why:** User expects to control both depth and direction of modulation

**Implementation:**
```cpp
// Add to enum ParamId (after RESONANCE_PARAM, before DRIVE_PARAM)
RESONANCE_ATTEN_PARAM,

// Add to constructor (after RESONANCE_PARAM config)
configParam(RESONANCE_ATTEN_PARAM, -1.f, 1.f, 0.f, "Resonance CV");
```

**Behavior:**
- At 0 (center): CV has no effect (multiplication by 0)
- At +1 (full clockwise): CV adds to resonance (positive modulation)
- At -1 (full counter-clockwise): CV subtracts from resonance (inverted modulation)
- At +0.5: CV scaled to 50% depth
- At -0.5: CV scaled to 50% depth, inverted

### Pattern 2: Additive CV Combination with Scaling
**What:** Combine knob position with scaled CV input, clamp result to valid range
**When to use:** For parameters with 0-1 range (resonance, filter parameters)
**Why:** Allows knob to set base value, CV to modulate around it

**Implementation:**
```cpp
// Replace current hardcoded 0.1 scaling (line 76)
float resonance = resonanceParam;
if (inputs[RESONANCE_CV_INPUT].isConnected()) {
    float resCV = inputs[RESONANCE_CV_INPUT].getPolyVoltage(c);
    float cvAmount = params[RESONANCE_ATTEN_PARAM].getValue();  // -1 to +1
    resonance = rack::clamp(resonanceParam + resCV * cvAmount * 0.1f, 0.f, 1.f);
}
```

**Math:**
- Base resonance from knob: 0-1
- CV input: typically -10V to +10V
- CV scaling: 0.1 (10% per volt, matches cutoff CV scaling from Phase 4 decision)
- Attenuverter: -1 to +1 (scales and inverts)
- Final: `knob + (CV * attenuverter * 0.1)`
- Clamped: 0 to 1

**Examples:**
- Knob at 0.5, CV at +5V, attenuverter at +1.0: `0.5 + (5 * 1.0 * 0.1) = 1.0` (clamped)
- Knob at 0.5, CV at +5V, attenuverter at -1.0: `0.5 + (5 * -1.0 * 0.1) = 0.0` (clamped)
- Knob at 0.5, CV at +5V, attenuverter at 0.0: `0.5 + (5 * 0.0 * 0.1) = 0.5` (no change)
- Knob at 0.5, CV at +2V, attenuverter at +1.0: `0.5 + (2 * 1.0 * 0.1) = 0.7`

### Pattern 3: Panel Layout Mirroring
**What:** Resonance controls mirror cutoff controls for visual consistency
**When to use:** When multiple parameters use the same control scheme
**Why:** User learns one pattern, applies to all similar controls

**Layout Reference (from existing cutoff):**
```cpp
// Cutoff section (vertical stack)
addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(35.56, 40.0)),
    module, HydraQuartetVCF::CUTOFF_PARAM));           // Main knob
addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20.0, 50.0)),
    module, HydraQuartetVCF::CUTOFF_CV_INPUT));        // CV jack (left of knob)
addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(35.56, 62.0)),
    module, HydraQuartetVCF::CUTOFF_ATTEN_PARAM));     // Attenuverter (below main knob)

// Resonance section (should mirror)
addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(35.56, 88.0)),
    module, HydraQuartetVCF::RESONANCE_PARAM));        // Main knob
addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20.0, 88.0)),
    module, HydraQuartetVCF::RESONANCE_CV_INPUT));     // CV jack (already exists)
// MISSING: Small attenuverter knob below resonance knob
```

**Position calculation:**
- Main resonance knob: Y = 88.0mm
- Attenuverter knob should be ~10-15mm below main knob
- Estimated position: Y = 98-100mm (verify panel space available)
- X position: same as main knob (35.56mm) for vertical alignment

### Pattern 4: Default Values for Immediate Use
**What:** Resonance defaults to 0 (no emphasis), attenuverter defaults to 0 (no CV effect)
**When to use:** Always - module should work immediately without configuration
**Why:** User decisions specify "default resonance: 0 (Q=0.5, no emphasis)"

**Implementation:**
```cpp
configParam(RESONANCE_PARAM, 0.f, 1.f, 0.f, "Resonance");         // Default 0 (flat response)
configParam(RESONANCE_ATTEN_PARAM, -1.f, 1.f, 0.f, "Resonance CV"); // Default 0 (CV off)
```

**Behavior at defaults:**
- Resonance knob at 0: Q = 0.5 (flat frequency response, no peak)
- Attenuverter at 0: CV input has no effect even if connected
- Filter produces clean lowpass/highpass/bandpass/notch outputs
- User must explicitly enable resonance (turn knob) or CV modulation (turn attenuverter)

### Anti-Patterns to Avoid
- **Multiplicative CV combination:** Don't use `resonance * (1 + CV)` - doesn't allow negative CV to reduce below knob position
- **Unipolar attenuverter:** Don't use 0 to +1 range - users expect bipolar for CV depth control
- **Different CV scaling:** Don't use different scaling than cutoff (0.1) - breaks consistency
- **Forgetting to clamp:** Don't omit `rack::clamp()` - CV can push resonance outside 0-1 range causing instability
- **Applying CV before attenuverter:** Don't scale CV before multiplying by attenuverter - breaks the depth control

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Parameter smoothing | Custom slew limiter | TExponentialFilter (already in SVFilter) | Already implemented, 1ms tau prevents zipper |
| CV scaling | Custom voltage-to-range mapping | Additive pattern with 0.1 scaling | Matches cutoff, proven in Phase 4 |
| Polyphonic CV | Manual channel handling | getPolyVoltage(c) (already used) | Handles mono/poly expansion automatically |
| Range clamping | Manual min/max checks | rack::clamp(value, 0.f, 1.f) (already used) | Standard library, clean code |
| Attenuverter UI | Custom knob design | RoundSmallBlackKnob (already used for cutoff) | Consistent with cutoff attenuverter |

**Key insight:** This phase is entirely about adding a parameter and following existing patterns. All DSP, smoothing, and CV infrastructure already exists. Don't reinvent anything - copy the cutoff attenuverter pattern exactly.

## Common Pitfalls

### Pitfall 1: Wrong Attenuverter Position in Enum
**What goes wrong:** Compilation error or parameter mapping breaks
**Why it happens:** Adding RESONANCE_ATTEN_PARAM in wrong position changes DRIVE_PARAM index
**How to avoid:** Insert RESONANCE_ATTEN_PARAM directly after RESONANCE_PARAM, before DRIVE_PARAM
**Warning signs:** DRIVE_PARAM behavior changes, parameter indices mismatch

### Pitfall 2: Forgetting CV Scaling Factor
**What goes wrong:** CV has massive effect, resonance jumps from 0 to 1 with tiny CV
**Why it happens:** Using CV voltage directly (10V range) instead of 0.1 scaling
**How to avoid:** Always multiply CV by 0.1 scaling factor: `resCV * cvAmount * 0.1f`
**Warning signs:** 1V CV swings resonance by 100% instead of 10%

### Pitfall 3: Incorrect CV Combination Order
**What goes wrong:** Attenuverter doesn't control CV depth as expected
**Why it happens:** Multiplying attenuverter before CV, or wrong operator precedence
**How to avoid:** Use: `resonanceParam + resCV * cvAmount * 0.1f` (CV scaled, then added)
**Warning signs:** Attenuverter at 0.5 doesn't produce half-depth modulation

### Pitfall 4: Missing Clamp on CV Result
**What goes wrong:** Resonance goes negative or above 1.0, causing filter instability
**Why it happens:** Large CV swings push result outside valid range
**How to avoid:** Always clamp final result: `rack::clamp(resonanceParam + ..., 0.f, 1.f)`
**Warning signs:** Filter self-oscillates wildly, audio blows up, NaN in output

### Pitfall 5: Panel Position Collisions
**What goes wrong:** Attenuverter knob overlaps with other components
**Why it happens:** Not checking panel layout, assuming space is available
**How to avoid:** Verify Y position has clearance (need ~10mm space below resonance knob at Y=88mm)
**Warning signs:** Knob overlaps output jacks (which start around Y=107mm)

### Pitfall 6: Default Attenuverter Non-Zero
**What goes wrong:** CV affects resonance even when user hasn't touched attenuverter
**Why it happens:** Setting default to 1.0 instead of 0.0
**How to avoid:** Default must be 0.0: `configParam(RESONANCE_ATTEN_PARAM, -1.f, 1.f, 0.f, ...)`
**Warning signs:** Resonance modulates unexpectedly when CV connected

### Pitfall 7: Inconsistent Scaling with Cutoff
**What goes wrong:** Cutoff and resonance CV respond at different rates, confusing users
**Why it happens:** Using different scaling factors (e.g., 0.2 instead of 0.1)
**How to avoid:** Use 0.1 scaling (10% per volt) to match cutoff CV scaling from Phase 4 decision
**Warning signs:** User expects same CV source to modulate both similarly, but rates differ

### Pitfall 8: Not Initializing Attenuverter Before Use
**What goes wrong:** Attenuverter value is garbage on first process() call
**Why it happens:** Reading param before module initialized
**How to avoid:** VCV Rack initializes params to defaults automatically - trust it
**Warning signs:** Random resonance behavior on module load (very rare, VCV handles this)

## Code Examples

Verified patterns from existing implementation and VCV standards:

### Complete Attenuverter Implementation Pattern
```cpp
// Source: Existing cutoff attenuverter in HydraQuartetVCF.cpp
// STEP 1: Add parameter to enum (line 7, after RESONANCE_PARAM)
enum ParamId {
    CUTOFF_PARAM,
    CUTOFF_ATTEN_PARAM,
    RESONANCE_PARAM,
    RESONANCE_ATTEN_PARAM,  // <-- Add this line
    DRIVE_PARAM,
    PARAMS_LEN
};

// STEP 2: Configure parameter (line 36, after RESONANCE_PARAM config)
configParam(RESONANCE_PARAM, 0.f, 1.f, 0.f, "Resonance");
configParam(RESONANCE_ATTEN_PARAM, -1.f, 1.f, 0.f, "Resonance CV");  // <-- Add this line
configParam(DRIVE_PARAM, 0.f, 1.f, 0.f, "Drive");

// STEP 3: Read attenuverter and apply to CV (line 74-76, replace current code)
// OLD CODE:
//   float resonance = resonanceParam;
//   if (inputs[RESONANCE_CV_INPUT].isConnected()) {
//       float resCV = inputs[RESONANCE_CV_INPUT].getPolyVoltage(c);
//       resonance = rack::clamp(resonanceParam + resCV * 0.1f, 0.f, 1.f);
//   }

// NEW CODE:
float resonance = resonanceParam;
if (inputs[RESONANCE_CV_INPUT].isConnected()) {
    float resCV = inputs[RESONANCE_CV_INPUT].getPolyVoltage(c);
    float cvAmount = params[RESONANCE_ATTEN_PARAM].getValue();  // <-- Read attenuverter
    resonance = rack::clamp(resonanceParam + resCV * cvAmount * 0.1f, 0.f, 1.f);  // <-- Apply
}

// STEP 4: Add widget to panel (after line 111, after RESONANCE_PARAM widget)
addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(35.56, 88.0)),
    module, HydraQuartetVCF::RESONANCE_PARAM));
addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(35.56, 100.0)),  // <-- Add this
    module, HydraQuartetVCF::RESONANCE_ATTEN_PARAM));  // Y position TBD based on panel space
```

### CV Scaling Math Verification
```cpp
// Test cases for 0.1 scaling factor (10% per volt):
// Knob at 0.5 (mid-resonance), attenuverter at +1.0 (full positive)
float resonanceParam = 0.5f;
float cvAmount = 1.0f;

// CV = 0V: no change
float cv = 0.0f;
float result = resonanceParam + cv * cvAmount * 0.1f;  // = 0.5 + 0 = 0.5 ✓

// CV = +1V: increase by 10%
cv = 1.0f;
result = resonanceParam + cv * cvAmount * 0.1f;  // = 0.5 + 0.1 = 0.6 ✓

// CV = +5V: increase by 50%
cv = 5.0f;
result = resonanceParam + cv * cvAmount * 0.1f;  // = 0.5 + 0.5 = 1.0 ✓

// CV = +10V: increase by 100% (would be 1.5, clamped to 1.0)
cv = 10.0f;
result = rack::clamp(resonanceParam + cv * cvAmount * 0.1f, 0.f, 1.f);  // = clamp(1.5) = 1.0 ✓

// CV = -5V: decrease by 50% (attenuverter at +1.0)
cv = -5.0f;
result = rack::clamp(resonanceParam + cv * cvAmount * 0.1f, 0.f, 1.f);  // = clamp(0.0) = 0.0 ✓

// CV = +5V, attenuverter at -1.0 (inverted): decrease by 50%
cv = 5.0f;
cvAmount = -1.0f;
result = rack::clamp(resonanceParam + cv * cvAmount * 0.1f, 0.f, 1.f);  // = clamp(0.0) = 0.0 ✓

// CV = +5V, attenuverter at 0.5 (half depth): increase by 25%
cv = 5.0f;
cvAmount = 0.5f;
result = resonanceParam + cv * cvAmount * 0.1f;  // = 0.5 + 0.25 = 0.75 ✓
```

### Bipolar Attenuverter Behavior
```cpp
// Source: VCV Rack community best practices
// Attenuverter range: -1.0 to +1.0, default 0.0

// Center (0.0): No CV effect
float cvAmount = 0.0f;
float cv = 5.0f;  // Any CV value
float effect = cv * cvAmount * 0.1f;  // = 5 * 0 * 0.1 = 0 (no change)

// Full positive (+1.0): Normal CV polarity
cvAmount = 1.0f;
effect = cv * cvAmount * 0.1f;  // = 5 * 1 * 0.1 = 0.5 (positive modulation)

// Full negative (-1.0): Inverted CV polarity
cvAmount = -1.0f;
effect = cv * cvAmount * 0.1f;  // = 5 * -1 * 0.1 = -0.5 (negative modulation)

// Half positive (+0.5): 50% depth, normal polarity
cvAmount = 0.5f;
effect = cv * cvAmount * 0.1f;  // = 5 * 0.5 * 0.1 = 0.25 (half-depth positive)

// Half negative (-0.5): 50% depth, inverted polarity
cvAmount = -0.5f;
effect = cv * cvAmount * 0.1f;  // = 5 * -0.5 * 0.1 = -0.25 (half-depth negative)
```

### Panel Widget Position Calculation
```cpp
// Source: Existing panel layout in HydraQuartetVCF.cpp
// Cutoff section layout (reference):
//   - Main knob: Y = 40.0mm
//   - CV jack: Y = 50.0mm (10mm below main knob)
//   - Attenuverter: Y = 62.0mm (22mm below main knob, 12mm below jack)

// Resonance section layout:
//   - Main knob: Y = 88.0mm (existing)
//   - CV jack: Y = 88.0mm (existing, same height as main knob)
//   - Attenuverter: Y = ??? (need to calculate)

// Option 1: Mirror cutoff offset (22mm below main knob)
float resonanceY = 88.0f;
float attenuverterY = resonanceY + 22.0f;  // = 110.0mm
// Problem: Outputs start at 107mm, would overlap!

// Option 2: Tight spacing (10mm below main knob)
attenuverterY = resonanceY + 10.0f;  // = 98.0mm
// Safe: 9mm clearance to first output at 107mm

// Option 3: Between knob and outputs (split difference)
attenuverterY = (88.0f + 107.0f) / 2.0f;  // = 97.5mm
// Safe: balanced spacing

// RECOMMENDATION: Use Y = 98.0mm or 100.0mm
// Final verification: Check SVG panel file for actual available space
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Fixed CV scaling | User-adjustable attenuverter | VCV Rack v1 standard | User controls modulation depth |
| Unipolar attenuator (0 to 1) | Bipolar attenuverter (-1 to +1) | Modular convention | Allows CV inversion |
| Separate attenuator modules | Integrated attenuverters | Modern VCV modules | Simpler patches, less clutter |
| Multiplicative CV (knob * cv) | Additive CV (knob + cv) | Depends on parameter type | Additive for 0-1 ranges, multiplicative for V/Oct |
| Linear resonance mapping | Exponential Q mapping | Analog filter convention | Musically useful sweep |

**Current best practice (VCV Rack 2.x modules):**
- Bipolar attenuverter for every CV input
- Attenuverter defaults to 0 (CV off until user enables)
- Small knob (RoundSmallBlackKnob) for attenuverters
- Positioned near associated CV jack
- Additive combination for 0-1 parameters
- Scaling matched across similar parameters (0.1 for filter params)

## Open Questions

Things that couldn't be fully resolved:

1. **Exact Panel Position for Attenuverter Knob**
   - What we know: Need position between resonance knob (Y=88mm) and outputs (Y=107mm)
   - What's unclear: Exact panel design constraints, whether labels interfere
   - Recommendation: Start with Y=100.0mm (12mm below resonance knob, 7mm above outputs). Adjust after visual inspection of panel SVG.

2. **Resonance Compensation at Frequency Extremes**
   - What we know: Analog filters often lose bass at high resonance, some implement compensation
   - What's unclear: Whether the existing SVF implementation needs compensation
   - Recommendation: User decision defers this to Claude's discretion. Start without compensation (authentic analog behavior). Can add gain compensation later if bass loss is problematic.

3. **Output Limiting at Extreme Resonance**
   - What we know: Existing soft saturation (tanh) in feedback path provides some limiting
   - What's unclear: Whether additional output limiting is needed at very high resonance
   - Recommendation: User decision defers this to Claude's discretion. Current tanh saturation likely sufficient. Monitor for clipping at resonance=1.0, Q=20.

4. **Parameter Smoothing Time**
   - What we know: Current 1ms tau for resonance smoothing (same as cutoff)
   - What's unclear: Whether resonance needs different smoothing than cutoff (faster/slower)
   - Recommendation: User decision defers this to Claude's discretion. Keep 1ms matching cutoff for consistency. Can reduce if zipper noise occurs with fast CV modulation.

5. **Hard Clamp vs Slight Overshoot**
   - What we know: Current code uses `rack::clamp(0.f, 1.f)` for hard clamping
   - What's unclear: Whether allowing slight overshoot (e.g., to 1.05) would sound better
   - Recommendation: User decision defers this to Claude's discretion. Keep hard clamp - resonance above 1.0 would push Q above 20, potentially causing instability. Safety first.

## Sources

### Primary (HIGH confidence)
- [VCV Rack Voltage Standards](https://vcvrack.com/manual/VoltageStandards) - CV ranges (0-10V unipolar, ±5V bipolar)
- [VCV Community: Attenuator Best Practices](https://community.vcvrack.com/t/attenuator-best-practices/13022) - Bipolar range, additive combination
- [VCV Rack Panel Design Guide](https://vcvrack.com/manual/Panel) - Component spacing, thumb clearance
- [VCV Rack RoundSmallBlackKnob API](https://vcvrack.com/docs-v2/structrack_1_1componentlibrary_1_1RoundSmallBlackKnob) - Widget for attenuverters
- Existing implementation in `src/HydraQuartetVCF.cpp` - Cutoff attenuverter pattern, resonance CV code
- Existing implementation in `src/SVFilter.hpp` - Q mapping, parameter smoothing
- Phase 4 STATE.md decision - "Resonance CV scaled 0.1 (1V = 10% change)"
- Phase 2 RESEARCH.md - Q range 0.5-20, soft saturation, trapezoidal SVF

### Secondary (MEDIUM confidence)
- [AAS Multiphonics State Variable Filter Manual](https://www.applied-acoustics.com/multiphonics-cv-1/manual/state-variable-filter/) - SVF resonance behavior
- [Sound on Sound: Responses & Resonance](https://www.soundonsound.com/techniques/responses-resonance) - Filter resonance characteristics
- [Molten Music: Synthesizer Filters Guide](https://moltenmusictechnology.com/a-beginners-guide-to-synthesizer-filters/) - Self-oscillation explanation
- WebSearch: VCV Rack attenuverter implementation patterns - Bipolar range, CV scaling
- WebSearch: State variable filter Q mapping - Q range for self-oscillation
- WebSearch: Filter resonance compensation - Bass loss at high resonance (analog behavior)

### Tertiary (LOW confidence)
- Various forum discussions on self-oscillation (KVR, Mod Wiggler) - General concepts, not implementation-specific
- Community modules (Vult, Bogaudio) - Pattern examples, but not official documentation

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Existing cutoff attenuverter provides proven reference pattern
- Architecture patterns: HIGH - Copying established pattern from same codebase
- Pitfalls: HIGH - Based on existing code analysis and VCV community best practices
- Code examples: HIGH - Derived from actual working cutoff implementation
- Open questions: MEDIUM - Panel layout needs visual verification, DSP options deferred to implementation

**Research limitations:**
- Panel SVG not examined for exact attenuverter placement (needs visual inspection)
- Resonance compensation not hands-on tested (deferred as optional feature)
- No A/B testing of different smoothing tau values
- CV scaling verified by math, not user testing

**Research date:** 2026-02-03
**Valid until:** 2026-04-03 (60 days - stable domain, existing implementation proven, VCV API stable)
