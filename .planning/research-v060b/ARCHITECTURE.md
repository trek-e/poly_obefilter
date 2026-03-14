# Architecture Research: v0.60b OB-X Filter Integration

**Domain:** VCV Rack Filter Module — 24dB/oct Cascaded SVF Extension
**Milestone:** v0.60b — Add OB-X 24dB filter to existing 12dB SEM filter
**Researched:** 2026-02-03
**Confidence:** HIGH

## Executive Summary

This research addresses how to integrate a 24dB/oct (4-pole) OB-X-style filter with the existing 12dB/oct (2-pole) SVFilter architecture. The recommended approach is **cascading two existing SVFilter instances in series** with a **configSwitch parameter** for filter type selection. This minimizes code changes, leverages proven DSP, and follows VCV Rack parameter configuration patterns.

## Integration Approach

### Current Architecture (v0.50b)

The existing module uses:
- **SVFilter struct** (SVFilter.hpp): Trapezoidal state-variable filter with two integrators (ic1eq, ic2eq)
- **Per-voice processing**: `SVFilter filters[16]` array in CipherOB
- **Four simultaneous outputs**: LP, HP, BP, Notch from single SVF pass
- **Parameter smoothing**: Built into SVFilter via TExponentialFilter
- **Saturation**: Applied post-filter via blendedSaturation function

### Proposed Extension (v0.60b)

Add 24dB capability by:
1. **Add second SVFilter instance per voice**: `SVFilter filters24dB_stage2[16]`
2. **Conditional cascading**: When 24dB mode selected, route stage1 LP output → stage2 input
3. **Filter type parameter**: configSwitch for "12dB SEM" vs "24dB OB-X"
4. **Preserve existing behavior**: 12dB mode uses only stage1, no architectural changes

### Why Cascade Two SVFs?

**Advantages:**
- **Proven stability**: Reuses existing tested SVFilter implementation
- **Minimal code changes**: No need to rewrite filter core
- **Independent tuning**: Each stage has own resonance feedback loop
- **Flexibility**: Could expose stage 2 separately in future (expander module)
- **Performance**: Two 2-pole filters similar CPU cost to one 4-pole

**Drawbacks (addressed):**
- **Resonance complexity**: Cascaded filters need resonance compensation (see below)
- **Limited multi-mode**: 24dB mode only provides meaningful LP output (other modes are "LP-XX cascades of little meaning") — this matches OB-X hardware behavior (4-pole is LP only)

**Alternative (not recommended):** True 4-pole state-variable filter with four integrators. More authentic but:
- Higher implementation risk (new DSP code)
- More complex stability analysis
- Doesn't match existing code patterns
- Harder to maintain

## Cascaded SVF Design

### Signal Flow for 24dB Mode

```
Input (per voice)
    ↓
SVFilter Stage 1 (12dB)
    ├── LP output → Stage 2 input
    ├── HP output → (not used in cascade)
    ├── BP output → (not used in cascade)
    └── Notch output → (not used in cascade)
    ↓
SVFilter Stage 2 (12dB)
    ├── LP output → 24dB LP (primary output)
    ├── HP output → 24dB HP-LP cascade (hybrid character)
    ├── BP output → 24dB BP-LP cascade (hybrid character)
    └── Notch output → 24dB Notch-LP cascade (hybrid character)
    ↓
blendedSaturation (drive applied)
    ↓
Output ports
```

### Resonance Handling for Cascaded Stages

**The Problem:**
When cascading two resonant filters, the combined resonance peak can become excessive or unstable. Two options:

1. **Split resonance**: Divide resonance parameter between stages (e.g., stage1 = 70%, stage2 = 70%)
2. **Global feedback**: Route stage2 output back to stage1 input (true 4-pole topology)

**Recommended Approach: Split Resonance**

```cpp
// In CipherOB::process() for 24dB mode

// Calculate split resonance (distribute across stages)
float stage1Resonance = resonance * 0.7f;  // 70% to stage 1
float stage2Resonance = resonance * 0.7f;  // 70% to stage 2

// Stage 1: Process with reduced resonance
filters[c].setParams(cutoffHz, stage1Resonance, args.sampleRate);
SVFilterOutputs stage1Out = filters[c].process(input);

// Stage 2: Process stage1 LP output with reduced resonance
filters24dB_stage2[c].setParams(cutoffHz, stage2Resonance, args.sampleRate);
SVFilterOutputs stage2Out = filters24dB_stage2[c].process(stage1Out.lowpass);

// Use stage2 outputs for 24dB filter response
```

**Rationale:**
- 0.7 × 0.7 = 0.49, so combined resonance feels similar to original
- Avoids instability from excessive cascaded feedback
- Simpler than global feedback (no additional feedback path)
- Matches approach used in Oberheim OB-Xa (two CEM3320 chips, each with own resonance)

### Cutoff Frequency Tracking

Both stages should use **identical cutoff frequency**:

```cpp
// Both stages track same cutoff parameter
float cutoffHz = baseCutoffHz * std::pow(2.f, cutoffCV * cvAmount);

filters[c].setParams(cutoffHz, stage1Resonance, args.sampleRate);
filters24dB_stage2[c].setParams(cutoffHz, stage2Resonance, args.sampleRate);
```

This ensures the 24dB filter resonates at the expected frequency, not an octave shifted or detuned.

### State Management

Each SVFilter instance maintains its own state (ic1eq, ic2eq). When switching between 12dB and 24dB modes:

**Option A: Preserve state (recommended for v0.60b)**
```cpp
// State persists when switching modes
// Allows smooth transitions during live performance
// May cause brief transient when switching
```

**Option B: Reset stage2 when leaving 24dB mode**
```cpp
// In process() after mode change detected
if (previousFilterType == FILTER_24DB && currentFilterType == FILTER_12DB) {
    for (int i = 0; i < PORT_MAX_CHANNELS; i++) {
        filters24dB_stage2[i].reset();
    }
}
```

Option A simpler for MVP. Option B can be added if users report switching artifacts.

## Filter Type Switching

### Parameter Definition

Add to CipherOB enums:

```cpp
enum ParamId {
    CUTOFF_PARAM,
    CUTOFF_ATTEN_PARAM,
    RESONANCE_PARAM,
    RESONANCE_ATTEN_PARAM,
    DRIVE_PARAM,
    DRIVE_ATTEN_PARAM,
    FILTER_TYPE_PARAM,        // NEW: Filter type switch
    PARAMS_LEN
};
```

### Configuration in Constructor

```cpp
CipherOB() {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

    // ... existing params ...

    // Configure filter type switch
    configSwitch(FILTER_TYPE_PARAM, 0.f, 1.f, 0.f, "Filter Type",
                 {"12dB SEM", "24dB OB-X"});
}
```

This creates a switch parameter with two positions:
- **0 = 12dB SEM**: Uses only stage1 (existing behavior)
- **1 = 24dB OB-X**: Uses cascaded stage1 + stage2

### Reading Parameter in process()

```cpp
void process(const ProcessArgs& args) override {
    int channels = std::max(1, inputs[AUDIO_INPUT].getChannels());

    // Read filter type (0 = 12dB, 1 = 24dB)
    int filterType = (int)params[FILTER_TYPE_PARAM].getValue();

    // ... read other params ...

    for (int c = 0; c < channels; c++) {
        float input = inputs[AUDIO_INPUT].getPolyVoltage(c);

        // ... calculate cutoffHz, resonance ...

        if (filterType == 0) {
            // 12dB SEM mode (existing behavior)
            filters[c].setParams(cutoffHz, resonance, args.sampleRate);
            SVFilterOutputs out = filters[c].process(input);

            // Apply drive and write outputs
            outputs[LP_OUTPUT].setVoltage(
                blendedSaturation(out.lowpass, smoothedDrive), c);
            // ... other outputs ...

        } else {
            // 24dB OB-X mode (cascaded)
            float stage1Resonance = resonance * 0.7f;
            float stage2Resonance = resonance * 0.7f;

            filters[c].setParams(cutoffHz, stage1Resonance, args.sampleRate);
            SVFilterOutputs stage1Out = filters[c].process(input);

            filters24dB_stage2[c].setParams(cutoffHz, stage2Resonance, args.sampleRate);
            SVFilterOutputs stage2Out = filters24dB_stage2[c].process(stage1Out.lowpass);

            // Apply drive and write outputs (24dB response)
            outputs[LP_OUTPUT].setVoltage(
                blendedSaturation(stage2Out.lowpass, smoothedDrive), c);
            // ... other outputs from stage2 ...
        }
    }

    // Set output channel counts
    outputs[LP_OUTPUT].setChannels(channels);
    // ... other outputs ...
}
```

### UI Widget Placement

Add switch widget to panel (CipherOBWidget):

```cpp
// Add filter type switch (e.g., at top center of panel)
addParam(createParamCentered<CKSS>(
    mm2px(Vec(35.56, 20.0)),
    module,
    CipherOB::FILTER_TYPE_PARAM));
```

`CKSS` is a standard VCV Rack 2-position toggle switch. Position on panel should be prominent since it fundamentally changes filter character.

### Alternative: CV Control for Type Switching (v0.70b)

For future milestone, add CV input for filter type switching:

```cpp
// In constructor
configInput(FILTER_TYPE_CV_INPUT, "Filter Type CV");

// In process()
float typeCV = inputs[FILTER_TYPE_CV_INPUT].getVoltage();
int filterType = (typeCV >= 5.f) ? 1 : 0;  // Gate-style: <5V = 12dB, >=5V = 24dB
```

This enables automated switching via sequencer or logic modules. Defer to v0.70b per project roadmap.

## Recommended Build Order

### Phase 1: Core Structure (2-4 hours)
1. Add FILTER_TYPE_PARAM to enums
2. Add configSwitch() in constructor
3. Add filters24dB_stage2[16] array to member variables
4. Compile and verify no regressions

### Phase 2: Conditional Logic (2-3 hours)
1. Add filterType parameter read in process()
2. Implement if/else for 12dB vs 24dB processing
3. Test 12dB mode still works identically
4. Test 24dB mode produces output (initial resonance split)

### Phase 3: Resonance Tuning (2-4 hours)
1. Implement split resonance (start with 0.7/0.7)
2. Test resonance response at 0%, 50%, 100%
3. Adjust split factors if needed (may need 0.6/0.6 or 0.8/0.8)
4. Verify self-oscillation behavior in 24dB mode
5. Test for instability (run at 100% resonance for 10 minutes)

### Phase 4: Output Routing (1-2 hours)
1. Route stage2 outputs to output ports in 24dB mode
2. Test HP, BP, Notch outputs (expect hybrid character)
3. Document behavior: "24dB mode outputs are LP-cascaded variants"
4. Apply drive scaling (may need different scaling than 12dB)

### Phase 5: UI Integration (1-2 hours)
1. Update panel SVG with switch widget location
2. Add switch parameter to widget
3. Test parameter switching in VCV Rack
4. Verify right-click menu shows "12dB SEM" / "24dB OB-X" labels

### Phase 6: Polish & Verification (2-3 hours)
1. Test polyphonic behavior (1, 2, 8, 16 channels)
2. Test CV modulation on cutoff/resonance in both modes
3. Verify no NaN/infinite values in 24dB mode
4. CPU meter check: ensure <5% for single instance (both modes)
5. Document filter type in MODULE.md or README

**Total estimated time: 10-18 hours**

## Architecture Patterns to Follow

### Pattern 1: Conditional DSP Processing

```cpp
if (condition) {
    // Path A processing
} else {
    // Path B processing
}
```

Standard approach for switchable behavior. No virtual functions needed (not real-time safe anyway).

### Pattern 2: Parameter-Driven Behavior

All mode switching via parameters, not internal state flags. This ensures:
- Behavior is serializable (params auto-saved)
- Right-click menu exposes options
- CV modulation possible (future)
- No hidden state confusion

### Pattern 3: Preserve Existing Code Paths

12dB mode should be **identical** to v0.50b processing. Don't refactor existing code to fit new architecture. Keep proven DSP untouched.

### Pattern 4: Per-Voice State Independence

Each voice (c = 0..15) processes independently:
- Own filter state (stage1 and stage2)
- Own smoothed drive value
- Own CV modulation values

No cross-voice dependencies. Enables polyphonic processing.

## Pitfalls to Avoid

### Pitfall 1: Resonance Instability in Cascade

**Risk:** Cascading two high-resonance filters can cause runaway feedback.

**Prevention:**
- Split resonance between stages (0.7/0.7 recommended starting point)
- Implement NaN check after stage2 processing (existing check in SVFilter::process)
- Test at 100% resonance with noise input for 10 minutes
- If instability occurs, reduce split factors or add soft clipping between stages

### Pitfall 2: Cutoff Frequency Mismatch

**Risk:** Stage1 and stage2 using different cutoff frequencies causes detuned response.

**Prevention:**
- Use identical cutoffHz for both stages
- Apply CV modulation before splitting to stages
- Verify with spectrum analyzer that peak is at expected frequency

### Pitfall 3: CPU Usage Doubling

**Risk:** 24dB mode uses twice the filters but shouldn't double CPU cost excessively.

**Prevention:**
- Target <7% CPU for 16-voice 24dB mode (vs <5% for 12dB)
- Profile with VCV CPU meter
- If needed, reduce parameter smoothing rate (less critical in cascade)

### Pitfall 4: State Reset Artifacts

**Risk:** Switching between modes while audio playing causes pops or clicks.

**Prevention:**
- Don't reset filter state on mode change (let state persist)
- If pops occur, add short fade (10-50ms) when mode changes
- Document expected behavior: "Mode changes may cause brief transient"

### Pitfall 5: Drive Scaling Mismatch

**Risk:** 24dB filter has more gain (two stages) so same drive amount sounds harsher.

**Prevention:**
- Test drive control in both modes
- May need to reduce drive amount in 24dB mode: `smoothedDrive * 0.7f`
- Or adjust makeup gain in blendedSaturation
- Keep drive behavior consistent with user expectations

## Performance Considerations

### Memory Impact

```cpp
// v0.50b: 1 SVFilter per voice
SVFilter filters[16];  // ~100 bytes per filter = 1.6 KB

// v0.60b: 2 SVFilters per voice
SVFilter filters[16];            // 1.6 KB
SVFilter filters24dB_stage2[16]; // 1.6 KB
// Total: 3.2 KB (negligible)
```

Memory impact is trivial. Modern systems have MB/GB RAM.

### CPU Impact

Rough estimate:
- 12dB mode: 100% (baseline)
- 24dB mode: 180-200% (not quite 2x due to shared parameter processing)

Target: <7% CPU for 16-voice 24dB mode on reference system (2020 laptop).

### Optimization Opportunities (post-v0.60b)

If CPU becomes issue:
1. **SIMD vectorization**: Process 4 voices at once (see v0.90b milestone)
2. **Shared coefficients**: Calculate g/k once, reuse for both stages
3. **Clock-divided parameter updates**: Update smoothers every 2-4 samples instead of every sample
4. **Approximated transcendentals**: Use fast_tan approximation for g calculation

Defer optimization until profiling shows actual bottleneck.

## Testing Strategy

### Functional Tests

1. **Mode switching**: Toggle FILTER_TYPE_PARAM, verify output changes
2. **Resonance sweep**: 0-100% in both modes, verify no NaN/infinity
3. **Cutoff sweep**: 20Hz-20kHz in both modes, verify correct tracking
4. **Self-oscillation**: 100% resonance should generate tone in both modes
5. **Polyphonic**: Test with 1, 2, 8, 16 channels

### Audio Tests

1. **White noise input**: Verify 24dB has steeper rolloff than 12dB
2. **Sine sweep**: Verify 24dB peak is at same frequency as 12dB
3. **Square wave input**: Verify 24dB removes more harmonics
4. **Drive sweep**: Verify saturation character in both modes

### Integration Tests

1. **Save/load patch**: Verify filter type parameter persists
2. **CPU meter**: Verify <5% (12dB) and <7% (24dB) for 16 voices
3. **Long-term stability**: Run at high resonance for 1 hour
4. **Multiple instances**: 4 modules in patch, all 24dB mode

## Future Extensions (Post-v0.60b)

### v0.70b: CV Control for Filter Type

Add CV input for automated switching:

```cpp
configInput(FILTER_TYPE_CV_INPUT, "Filter Type CV");

// In process()
if (inputs[FILTER_TYPE_CV_INPUT].isConnected()) {
    float typeCV = inputs[FILTER_TYPE_CV_INPUT].getVoltage();
    filterType = (typeCV >= 5.f) ? 1 : 0;
} else {
    filterType = (int)params[FILTER_TYPE_PARAM].getValue();
}
```

### v0.80b: Per-Voice Filter Type (Advanced)

Allow different voices to use different filter types:

```cpp
// Per-voice CV modulation
int filterType[16];
for (int c = 0; c < channels; c++) {
    float typeCV = inputs[FILTER_TYPE_CV_INPUT].getPolyVoltage(c);
    filterType[c] = (typeCV >= 5.f) ? 1 : 0;

    // Process voice with its specific filter type
}
```

Enables complex polyphonic patches where some voices are 12dB, others 24dB.

### v0.90b: SIMD Optimization

Refactor to process 4 voices at once using simd::float_4:

```cpp
simd::float_4 filterState1[4];  // 4 vectors of 4 voices = 16 total
simd::float_4 filterState2[4];

for (int c = 0; c < channels; c += 4) {
    simd::float_4 input = inputs[AUDIO_INPUT].getVoltageSimd<simd::float_4>(c);
    // SIMD processing...
}
```

Expected 3-4x speedup. Defer until CPU profiling shows need.

## Sources

### Cascaded Filter Theory
- [Cascading filters | EarLevel Engineering](https://www.earlevel.com/main/2016/09/29/cascading-filters/) - Theory of cascading 2-pole filters to create 4-pole response
- [State Variable Filter -24dB version? - KVR Audio](https://www.kvraudio.com/forum/viewtopic.php?t=263202) - Discussion of cascading two SVFs and resonance challenges
- [Multimode filters, Part 2: Pole-mixing filters – Electric Druid](https://electricdruid.net/multimode-filters-part-2-pole-mixing-filters/) - Multi-pole filter architectures

### VCV Rack Parameter Configuration
- [Plugin API Guide - VCV Rack Manual](https://vcvrack.com/manual/PluginGuide) - configSwitch() documentation and parameter patterns
- [VCV Rack API: rack::engine::Module](https://vcvrack.com/docs-v2/structrack_1_1engine_1_1Module) - Module lifecycle and parameter handling

### Oberheim Filter Architecture
- [Oberheim OB-Xa filter? - MOD WIGGLER](https://modwiggler.com/forum/viewtopic.php?t=10709) - Discussion of OB-Xa dual filter architecture (12dB + 24dB)
- [CEM3320 Filter designs – Electric Druid](https://electricdruid.net/cem3320-filter-designs/) - Technical details of CEM3320 chip used in Oberheim filters

### State Variable Filter Implementation
- [The digital state variable filter | EarLevel Engineering](http://www.earlevel.com/main/2003/03/02/the-digital-state-variable-filter/) - Core SVF DSP algorithms
- [State Variable Filters](https://sound-au.com/articles/state-variable.htm) - Analog and digital SVF theory

---

*Architecture research for: CIPHER · OB v0.60b milestone*
*Researched: 2026-02-03*
*Confidence: HIGH (Cascading approach well-documented, VCV Rack patterns established, existing SVFilter proven stable)*
