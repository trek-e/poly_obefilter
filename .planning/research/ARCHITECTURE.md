# Architecture Research

**Domain:** VCV Rack Polyphonic Filter Module
**Researched:** 2026-01-29
**Confidence:** HIGH

## Standard VCV Rack Module Architecture

### System Overview

```
┌─────────────────────────────────────────────────────────────┐
│                      User Interface Layer                    │
│                    (ModuleWidget + SVG)                      │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐        │
│  │ Params  │  │ Inputs  │  │ Outputs │  │ Lights  │        │
│  │(Knobs)  │  │ (Ports) │  │ (Ports) │  │ (LEDs)  │        │
│  └────┬────┘  └────┬────┘  └────┬────┘  └────┬────┘        │
│       │            │            │            │              │
├───────┴────────────┴────────────┴────────────┴──────────────┤
│                   Module Processing Layer                     │
│                   (Module::process())                         │
│  ┌──────────────────────────────────────────────────────┐    │
│  │  - Read params/inputs (up to 16 poly channels)       │    │
│  │  - Execute DSP per channel (SIMD optimized)          │    │
│  │  - Write outputs/lights per channel                  │    │
│  └──────────────────────────────────────────────────────┘    │
├─────────────────────────────────────────────────────────────┤
│                       DSP State Layer                         │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐                   │
│  │ Filter   │  │ Envelope │  │ Internal │                   │
│  │ States   │  │ Followers│  │ Buffers  │                   │
│  └──────────┘  └──────────┘  └──────────┘                   │
└─────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

| Component | Responsibility | Typical Implementation |
|-----------|----------------|------------------------|
| **Module** | Core DSP logic, state management, I/O handling | C++ class inheriting from `rack::Module`, implements `process(const ProcessArgs& args)` |
| **ModuleWidget** | UI layout, panel graphics, widget placement | C++ class inheriting from `rack::ModuleWidget`, places params/ports/lights |
| **Params** | User-adjustable controls (knobs, switches, buttons) | Configured via `configParam()`, read with `params[ID].getValue()` |
| **Inputs** | External signal reception (audio, CV, gates) | Read with `inputs[ID].getVoltage(channel)` or `getPolyVoltage(channel)` |
| **Outputs** | Processed signal transmission | Written with `outputs[ID].setVoltage(voltage, channel)` |
| **Lights** | Visual feedback (LEDs, meters) | Written with `lights[ID].setBrightness(brightness)` |
| **DSP State** | Per-channel filter coefficients, phase accumulators, buffers | Member variables, often arrays for polyphony: `float state[16]` or `simd::float_4 state[4]` |

## Recommended Project Structure for Filter Module

```
src/
├── plugin.hpp              # Plugin metadata, model declarations
├── plugin.cpp              # Plugin initialization, model registration
├── HydraQuartetVCF.cpp     # Main module implementation
│   ├── struct HydraQuartetVCF : Module
│   ├── enum ParamId (knobs, switches)
│   ├── enum InputId (audio in, CV ins)
│   ├── enum OutputId (filter outputs)
│   ├── enum LightId (LEDs)
│   └── process() method
├── dsp/                    # DSP algorithm implementations
│   ├── StateVariableFilter.hpp  # SVF core implementation
│   ├── SEMFilter.hpp            # SEM 12dB filter topology
│   ├── OBXFilter.hpp            # OB-X 24dB filter topology
│   └── ResonanceProcessor.hpp   # Self-oscillation handling
├── ui/                     # User interface components
│   └── HydraQuartetWidget.cpp   # Panel layout and widget placement
└── res/                    # Resources
    ├── HydraQuartet.svg         # Panel design
    └── components/              # Custom UI components
```

### Structure Rationale

- **plugin.hpp/cpp:** Central registration point keeps module discovery clean. All models declared here and added to plugin in `init()`.
- **Module implementation:** Single file per module keeps related code together. For complex modules, can split into `.hpp` and `.cpp`.
- **dsp/ directory:** Separates DSP algorithms from Rack API integration. Makes code testable, reusable, and easier to optimize.
- **ui/ directory:** Isolates UI layout from logic. Panel designs can evolve independently of DSP.
- **res/ directory:** Graphics and assets. SVG panels are standard for VCV Rack.

## Architectural Patterns

### Pattern 1: Polyphonic Channel Iteration

**What:** Process each voice independently in the main audio loop. Determine channel count from input cables, iterate per-channel, maintain separate state per voice.

**When to use:** All polyphonic modules (which is the standard for modern VCV Rack development)

**Trade-offs:**
- Pro: Natural fit for voice-based instruments, clean separation of concerns
- Pro: Users expect this behavior (cable channel count drives processing)
- Con: Requires duplicating state for all channels (memory overhead)
- Con: Without SIMD, can be CPU-intensive for 16 channels

**Example:**
```cpp
void process(const ProcessArgs& args) override {
    // Determine channel count from a "primary" input
    int channels = std::max(1, inputs[AUDIO_INPUT].getChannels());

    // Process each channel independently
    for (int c = 0; c < channels; c++) {
        float input = inputs[AUDIO_INPUT].getPolyVoltage(c);
        float cutoff = params[CUTOFF_PARAM].getValue() + inputs[CUTOFF_INPUT].getPolyVoltage(c);

        // Apply DSP per channel
        float output = filters[c].process(input, cutoff, resonance);

        outputs[AUDIO_OUTPUT].setVoltage(output, c);
    }

    // Set output channel count
    outputs[AUDIO_OUTPUT].setChannels(channels);
}
```

### Pattern 2: SIMD Optimization (float_4)

**What:** Process 4 channels simultaneously using SIMD vector types (`simd::float_4`). Reduces loop iterations from 16 to 4, leveraging CPU vector instructions for 4x speedup.

**When to use:** Performance-critical modules, filters with expensive per-sample computations

**Trade-offs:**
- Pro: 4x performance improvement with proper implementation
- Pro: Automatic vectorization by compiler for math operations
- Pro: Rack provides `simd::float_4` abstraction (SSE, NEON, etc.)
- Con: Requires thinking in vectors (less intuitive than scalar code)
- Con: Some operations (conditionals, memory access) harder to vectorize
- Con: Initial development takes longer

**Example:**
```cpp
// State stored as 4 vectors of 4 channels each (total 16 voices)
simd::float_4 filterState[4] = {};

void process(const ProcessArgs& args) override {
    int channels = std::max(1, inputs[AUDIO_INPUT].getChannels());

    // Process 4 channels at a time
    for (int c = 0; c < channels; c += 4) {
        // Load 4 channels into a vector
        simd::float_4 input = inputs[AUDIO_INPUT].getVoltageSimd<simd::float_4>(c);
        simd::float_4 cutoffCV = inputs[CUTOFF_INPUT].getPolyVoltageSimd<simd::float_4>(c);

        // SIMD math operations process all 4 channels in parallel
        simd::float_4 cutoff = params[CUTOFF_PARAM].getValue() + cutoffCV;
        simd::float_4 output = processFilterSimd(input, cutoff, c / 4);

        // Write 4 channels back
        outputs[AUDIO_OUTPUT].setVoltageSimd(output, c);
    }

    outputs[AUDIO_OUTPUT].setChannels(channels);
}
```

### Pattern 3: State Variable Filter Topology

**What:** Implement multimode filters using state-variable (SVF) architecture. SVF provides simultaneous lowpass, highpass, bandpass outputs from a single structure using integrators and feedback.

**When to use:** Multimode filters, especially when multiple outputs or smooth mode morphing needed

**Trade-offs:**
- Pro: Multiple filter types from one structure (LP/HP/BP/Notch)
- Pro: Stable across wide cutoff range (better than ladder filters at high frequencies)
- Pro: Smooth parameter changes without zipper noise
- Pro: Self-oscillation behavior relatively easy to implement
- Con: Can become unstable at cutoff frequencies above ~fs/6
- Con: Nonlinear behavior (OTA-style) requires careful modeling
- Con: Resonance compensation needed for consistent volume

**Example:**
```cpp
class StateVariableFilter {
    float z1 = 0.f, z2 = 0.f;  // Integrator states

    struct Outputs { float lp, bp, hp, notch; };

    Outputs process(float input, float cutoff, float resonance, float sampleRate) {
        // Convert cutoff frequency to filter coefficient
        float g = std::tan(M_PI * cutoff / sampleRate);
        float k = 2.f - 2.f * resonance;  // Resonance (k=2 for no resonance, k=0 for self-osc)

        // TPT (Topology Preserving Transform) structure
        float v0 = input;
        float v1 = (z1 + g * (v0 - k * z1 - z2)) / (1.f + g * (g + k));
        float v2 = z2 + g * v1;

        z1 = 2.f * v1 - z1;  // Update integrator states
        z2 = 2.f * v2 - z2;

        // Return all filter modes
        return {
            .lp = v2,                    // Lowpass
            .bp = v1,                    // Bandpass
            .hp = v0 - k * v1 - v2,      // Highpass
            .notch = v0 - k * v1         // Notch
        };
    }
};
```

### Pattern 4: Configuration in Constructor

**What:** Set up all module parameters, inputs, outputs, and lights in the Module constructor using `config*()` methods. This registers components with the Rack engine and provides metadata for tooltips/automation.

**When to use:** Always (required pattern for VCV Rack modules)

**Trade-offs:**
- Pro: Declarative, self-documenting configuration
- Pro: Tooltips and right-click menus automatically generated
- Pro: Type-safe parameter ranges prevent invalid values
- Con: Boilerplate for every param/port/light
- Con: C++ enum verbosity

**Example:**
```cpp
struct HydraQuartetVCF : Module {
    enum ParamId {
        CUTOFF_PARAM,
        RESONANCE_PARAM,
        FILTER_TYPE_PARAM,
        PARAMS_LEN
    };

    enum InputId {
        AUDIO_INPUT,
        CUTOFF_INPUT,
        RESONANCE_INPUT,
        INPUTS_LEN
    };

    HydraQuartetVCF() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        // Configure parameters with ranges, defaults, labels, units
        configParam(CUTOFF_PARAM, 0.f, 1.f, 0.5f, "Cutoff", " Hz", 2.f, dsp::FREQ_C4);
        configParam(RESONANCE_PARAM, 0.f, 1.f, 0.f, "Resonance", "%", 0.f, 100.f);
        configSwitch(FILTER_TYPE_PARAM, 0.f, 3.f, 0.f, "Filter Type",
            {"SEM LP 12dB", "SEM BP 12dB", "OB-X LP 24dB", "OB-X BP 24dB"});

        // Configure inputs/outputs with labels
        configInput(AUDIO_INPUT, "Audio");
        configInput(CUTOFF_INPUT, "Cutoff CV");
        configOutput(LOWPASS_OUTPUT, "Lowpass");
        configOutput(BANDPASS_OUTPUT, "Bandpass");

        // Bypass routing: when bypassed, audio input → outputs
        configBypass(AUDIO_INPUT, LOWPASS_OUTPUT);
    }
};
```

### Pattern 5: ClockDivider for Expensive Operations

**What:** Use `dsp::ClockDivider` to run expensive computations (parameter smoothing, UI updates, coefficient recalculation) less frequently than audio rate.

**When to use:** Operations that don't need per-sample updates (e.g., LED updates, expensive math)

**Trade-offs:**
- Pro: Significant CPU savings for expensive operations
- Pro: Built-in utility, no manual counter management
- Con: Slight delay in visual feedback (negligible for humans)
- Con: Can cause parameter stepping artifacts if overdone

**Example:**
```cpp
struct HydraQuartetVCF : Module {
    dsp::ClockDivider lightDivider;

    HydraQuartetVCF() {
        // ... config ...
        lightDivider.setDivision(512);  // Update lights every 512 samples (~10ms at 48kHz)
    }

    void process(const ProcessArgs& args) override {
        // Audio processing every sample
        // ... DSP code ...

        // Update lights occasionally (not critical for audio)
        if (lightDivider.process()) {
            lights[CLIPPING_LIGHT].setBrightness(clippingLevel);
        }
    }
};
```

### Pattern 6: JSON Serialization for Custom State

**What:** Override `dataToJson()` and `dataFromJson()` to save/load custom module state beyond parameters. Rack auto-saves params, but custom state (filter type selection, calibration data) requires explicit handling.

**When to use:** Modules with state not representable as simple parameters (e.g., wavetables, learned calibration)

**Trade-offs:**
- Pro: Flexible schema for complex state
- Pro: Can version state format for backward compatibility
- Con: Manual serialization code (error-prone)
- Con: Large state can slow patch save/load

**Example:**
```cpp
json_t* dataToJson() override {
    json_t* rootJ = json_object();
    json_object_set_new(rootJ, "filterMode", json_integer(currentFilterMode));
    json_object_set_new(rootJ, "oversampleFactor", json_integer(oversampleFactor));
    return rootJ;
}

void dataFromJson(json_t* rootJ) override {
    json_t* filterModeJ = json_object_get(rootJ, "filterMode");
    if (filterModeJ)
        currentFilterMode = json_integer_value(filterModeJ);

    json_t* oversampleJ = json_object_get(rootJ, "oversampleFactor");
    if (oversampleJ)
        oversampleFactor = json_integer_value(oversampleJ);
}
```

## Data Flow

### Audio Processing Flow

```
User Patch Cable (up to 16 channels)
    ↓
Input Port (inputs[AUDIO_INPUT])
    ↓
Module::process() reads getPolyVoltage(channel)
    ↓
Per-Channel DSP Processing Loop
    ├─→ Read Parameters (params[].getValue())
    ├─→ Read CV Inputs (inputs[CV].getPolyVoltage(channel))
    ├─→ Calculate Filter Coefficients
    ├─→ Apply Filter DSP (state variable, ladder, etc.)
    ├─→ Apply Resonance/Self-Oscillation
    └─→ Soft Clipping / Saturation (if applicable)
    ↓
outputs[OUTPUT].setVoltage(result, channel)
    ↓
Output Port (polyphonic cable, 16 channels)
    ↓
User Patch Cable (next module in chain)
```

### Parameter Modulation Flow

```
User Knob Turn (Param)           CV Cable (Input Port)
    ↓                                   ↓
params[CUTOFF].getValue()      inputs[CUTOFF_CV].getPolyVoltage(c)
    ↓                                   ↓
    └──────────────┬────────────────────┘
                   ↓
        Combined Modulation Value
                   ↓
        Clamp to Valid Range
                   ↓
        Convert to Filter Coefficient (Hz → g parameter)
                   ↓
        Apply to Filter DSP
```

### State Persistence Flow

```
User Saves Patch
    ↓
Rack Engine calls Module::dataToJson()
    ↓
Module serializes custom state to JSON
    ↓
Rack merges with auto-saved params/cables
    ↓
JSON written to .vcv patch file
    ↓
(Later) User Loads Patch
    ↓
Rack Engine calls Module::dataFromJson()
    ↓
Module deserializes custom state
    ↓
Parameters restored (automatic)
    ↓
Module resumes with full state
```

### Key Data Flow Insights

1. **Channel count determination:** Usually derived from a "primary" input port using `getChannels()`. All processing scales to this count.

2. **CV modulation pattern:** Base value from param + CV input value. CV typically ±5V range, scaled to param range via attenuverter or direct summing.

3. **Filter coefficient calculation:** Done per-sample or per-block (with smoothing). Expensive operations (tan, exp) can be approximated or table-driven.

4. **State updates:** DSP state (integrators, delays) updated in-place during `process()`. No separate "update" phase.

5. **Output channel setting:** Must explicitly call `setChannels()` to inform Rack of output polyphony. Otherwise defaults to 1.

## Filter-Specific Architecture Considerations

### State Variable Filter Implementation

**Core Structure:**
- Two integrators (z1, z2) store filter state per channel
- Feedback path with resonance control (k parameter)
- Topology Preserving Transform (TPT) for stability at high cutoffs
- Simultaneous outputs: LP, BP, HP, Notch from single structure

**Self-Oscillation:**
- Achieved by reducing damping (k parameter) below critical value
- Requires noise injection in digital domain (analog has inherent noise)
- Soft clipping in feedback path prevents runaway (tanh or polynomial)
- At resonance = 100%, k ≈ 0, filter oscillates at cutoff frequency

**Per-Voice State:**
```cpp
// Scalar approach (16 voices)
float z1[16], z2[16];

// SIMD approach (4 vectors of 4 voices)
simd::float_4 z1[4], z2[4];
```

### Oberheim SEM Filter Characteristics

**12dB/octave multimode filter:**
- Based on OTA (Operational Transconductance Amplifier) design
- State variable topology with unique notch mode (LP+HP mix)
- Characteristic sound: warm, musical, smooth resonance
- Nonlinear behavior important for authenticity (requires modeling)

**Implementation Approach:**
- Start with linear SVF as foundation
- Add soft clipping at critical stages (input, integrators, feedback)
- Model OTA nonlinearity (tanh approximation common)
- Special notch mode: blend LP and HP outputs

### Oberheim OB-X Filter Characteristics

**24dB/octave (4-pole) ladder-style filter:**
- Cascaded 2-pole sections OR true 4-pole state variable
- Steeper rolloff than SEM (24dB vs 12dB)
- Resonant peak more pronounced
- Can self-oscillate into sine wave territory

**Implementation Approach:**
- Cascade two 12dB SVF stages (simpler, more stable)
- OR implement 4-pole state variable (more authentic, harder to stabilize)
- Resonance compensation across stages to maintain consistent gain
- Oversampling recommended for reducing aliasing at high resonance

### Anti-Aliasing Strategy

**Why Needed:**
Nonlinear processes (saturation, resonance clipping) generate harmonics above Nyquist, causing aliasing artifacts (harsh, digital sound).

**Techniques:**
1. **Oversampling:** Process at 2x/4x/8x sample rate, then downsample. Simple but CPU-intensive.
2. **Soft Clipping:** Use smooth functions (tanh, polynomial) instead of hard clipping. Reduces harmonic energy.
3. **Bandlimited Algorithms:** For oscillators/waveforms (not directly applicable to filters).
4. **Pre/Post Filtering:** Lowpass before nonlinear stage, lowpass after. Adds latency.

**Recommendation for HydraQuartet:**
- Start with soft clipping (tanh) in feedback path (low CPU cost, effective)
- Add optional 2x oversampling if CPU budget allows (right-click menu toggle)
- Monitor for aliasing during testing with spectrum analyzer

### Polyphonic Optimization Strategy

**Memory Layout:**
```cpp
// Option 1: Array-of-Structures (AoS)
struct Voice {
    StateVariableFilter filter;
    float phase;
};
Voice voices[16];  // Poor cache locality for SIMD

// Option 2: Structure-of-Arrays (SoA) - PREFERRED
simd::float_4 z1[4], z2[4];  // All state packed for SIMD
simd::float_4 phase[4];
```

**Processing Strategy:**
1. **Determine channel count** from primary input
2. **Loop in chunks of 4** (for SIMD) or 1 (for simplicity)
3. **Process DSP** per voice/vector
4. **Write outputs** per voice/vector
5. **Update lights/UI** at divided rate (not per-sample)

**Performance Target:**
- **Good:** Processes 16 voices at <5% CPU on modern system
- **Better:** <3% with SIMD optimization
- **Best:** <2% with SIMD + efficient algorithms

## Anti-Patterns

### Anti-Pattern 1: Per-Sample Expensive Math

**What people do:** Call `std::tan()`, `std::exp()`, or other transcendental functions per-sample per-voice in the audio loop.

**Why it's wrong:** These functions are slow (10-100+ cycles). At 48kHz × 16 voices = 768k calls/sec, this dominates CPU time.

**Do this instead:**
- **Approximate:** Use polynomial or rational approximations (Rack provides `simd::exp_taylor5()`, etc.)
- **Table Lookup:** Pre-compute expensive functions, interpolate from table
- **Reduce Rate:** Calculate coefficients at control rate (e.g., every 64 samples) with smoothing
- **SIMD:** Process 4 values at once to amortize overhead

### Anti-Pattern 2: Allocating Memory in process()

**What people do:** Create temporary buffers, vectors, or objects inside the `process()` method.

**Why it's wrong:** Memory allocation is non-realtime-safe (can block, cause jitter). Audio dropouts result. VCV Rack runs on audio thread.

**Do this instead:**
- **Pre-allocate:** Create buffers as member variables in constructor
- **Static Size:** Use fixed-size arrays (e.g., `float buffer[MAX_CHANNELS]`)
- **Stack Allocation:** Small temporary arrays on stack are fine (`float temp[4]`)
- **RAII:** Use existing buffer, reset/reuse each frame

### Anti-Pattern 3: Ignoring Sample Rate Changes

**What people do:** Calculate filter coefficients once in constructor, assume 44.1kHz or 48kHz.

**Why it's wrong:** VCV Rack supports arbitrary sample rates (44.1k, 48k, 96k, etc.). Users can change engine sample rate. Filter becomes unstable or wrong frequency at other rates.

**Do this instead:**
- **Read `args.sampleRate`** in `process()` method
- **Recalculate coefficients** when sample rate changes (cache previous rate, compare)
- **Override `onSampleRateChange()`** event handler for expensive recalculations
- **Scale cutoff frequency** by sample rate (`cutoffHz / sampleRate`)

### Anti-Pattern 4: Not Setting Output Channels

**What people do:** Write to output ports with `setVoltage()` but forget to call `setChannels()`.

**Why it's wrong:** Rack defaults outputs to 1 channel. Even if you write to channels 0-15, only channel 0 propagates. Result: lost voices, monophonic output from polyphonic module.

**Do this instead:**
```cpp
// At end of process() method
outputs[AUDIO_OUTPUT].setChannels(channels);
```

### Anti-Pattern 5: Direct-Form IIR at High Cutoff

**What people do:** Implement filters using direct-form biquad structure at high cutoff frequencies (>fs/4).

**Why it's wrong:** Direct-form biquads become numerically unstable at high cutoffs due to coefficient quantization. Noise, oscillation, or NaN results.

**Do this instead:**
- **Use TPT (Topology Preserving Transform)** structure for state variable filters
- **Use transposed direct-form II** for biquads (better numerical properties)
- **Limit cutoff frequency** to fs/6 or fs/4 for stability
- **Add oversampling** if high cutoff needed (process at 2x, downsample)

### Anti-Pattern 6: Global State Between Instances

**What people do:** Use static or global variables to share state between multiple module instances.

**Why it's wrong:** Users often patch multiple copies of the same module. Shared state causes crosstalk, making modules interfere with each other.

**Do this instead:**
- **All state in Module instance:** Make state member variables of the Module class
- **Per-instance DSP objects:** Each module has its own filter instances
- **Expanders for communication:** Use official expander mechanism for inter-module communication

## Scaling Considerations

| Scale | Architecture Adjustments |
|-------|--------------------------|
| **1-4 voices** | Simple scalar code fine. Focus on correctness and sound quality. Per-voice iteration, no SIMD needed. |
| **5-8 voices** | Consider SIMD optimization if CPU usage high. Use `simd::float_4` to process 4 voices at once. Profile first. |
| **9-16 voices** | SIMD optimization strongly recommended. Structure-of-Arrays layout for state. Use `dsp::ClockDivider` for UI updates. Consider oversampling toggle (off by default). |
| **16+ voices (future)** | Requires advanced optimization: SIMD, efficient algorithms (biquad over SVF), minimal per-sample operations, consider GPU acceleration (non-standard for VCV). |

### Scaling Priorities

1. **First bottleneck (4-8 voices):** Per-sample expensive math (tan, exp, pow). Mitigation: approximate, reduce rate, or table lookup.

2. **Second bottleneck (8-16 voices):** Memory bandwidth from scalar processing. Mitigation: SIMD vectorization with `float_4`.

3. **Third bottleneck (oversampling):** 2x/4x oversampling for anti-aliasing multiplies CPU cost. Mitigation: make optional (right-click menu), use efficient upsampler (Rack provides `dsp::Upsampler`).

### Performance Budget Guideline

VCV Rack community expectation: modules should use <5% CPU per instance on mid-range system (16 polyphonic voices).

**Budget Breakdown for HydraQuartet (16 voices):**
- **Input/Output handling:** <0.5% (trivial operations)
- **Parameter processing:** <0.5% (reading params, CV, clamping)
- **Filter DSP (per voice):** ~3.0% (main cost, optimize here)
- **UI updates (lights, meters):** <0.5% (clock-divided)
- **Anti-aliasing (if enabled):** +1-3% (oversampling overhead)

**Total target:** <5% without oversampling, <8% with 2x oversampling

## Build Order Implications for Roadmap

### Dependency Chain

The architecture suggests this build order:

```
1. Module Shell + Basic I/O
   ↓
2. Single-Voice DSP (monophonic)
   ↓
3. Polyphonic Extension (scalar)
   ↓
4. SIMD Optimization (float_4)
   ↓
5. Anti-Aliasing (oversampling)
   ↓
6. UI Polish (lights, meters, tooltips)
```

### Phase Structure Recommendations

**Phase 1: Foundation (Module Shell)**
- Set up plugin structure (plugin.hpp/cpp)
- Create module with params/inputs/outputs
- Implement basic bypass routing
- No DSP yet, just infrastructure

**Phase 2: Core DSP (Single Filter Type)**
- Implement one filter type (e.g., SEM 12dB LP) in scalar code
- Get basic sound working (monophonic)
- Test cutoff CV, resonance CV
- Verify self-oscillation behavior

**Phase 3: Polyphony (Multi-Voice)**
- Extend to polyphonic processing (16 voices)
- Per-channel state management
- Channel count detection from inputs
- Test with polyphonic MIDI-CV

**Phase 4: Multiple Filter Types**
- Add remaining filter types (SEM BP, OB-X LP/BP)
- Mode switching via parameter
- Multiple simultaneous outputs

**Phase 5: Optimization (SIMD)**
- Refactor to SIMD (float_4) processing
- Profile and optimize hot paths
- Ensure <5% CPU target met

**Phase 6: Quality (Anti-Aliasing + Polish)**
- Add optional oversampling
- Soft clipping for anti-aliasing
- LED/metering, UI refinements

### Critical Path Items

These must be built in order (strong dependencies):

1. **Module shell before DSP:** Can't test DSP without I/O infrastructure
2. **Monophonic before polyphonic:** Debug DSP in simple case before scaling to 16 voices
3. **Scalar before SIMD:** SIMD optimization requires working scalar implementation to test against
4. **Core filter before variants:** Establish one filter type fully before adding modes

### Parallelizable Work

These can be developed concurrently:

- **Panel design (SVG)** while building module shell
- **DSP algorithms** (can prototype in separate test harness) while building module infrastructure
- **Documentation/presets** after core DSP working

## Sources

### Official VCV Rack Documentation
- [Plugin Development Tutorial](https://vcvrack.com/manual/PluginDevelopmentTutorial) - Core module structure and component overview
- [Plugin API Guide](https://vcvrack.com/manual/PluginGuide) - Advanced patterns, polyphony, SIMD, configuration methods
- [Polyphony Manual](https://vcvrack.com/manual/Polyphony) - Polyphonic cable architecture and implementation patterns
- [DSP Manual](https://vcvrack.com/manual/DSP) - Filter implementation approaches, anti-aliasing, optimization strategies
- [rack::dsp Namespace API](https://vcvrack.com/docs-v2/namespacerack_1_1dsp) - Built-in DSP utilities and SIMD types

### Community Resources
- [VCV Community Development Forum](https://community.vcvrack.com/c/development/8) - Best practices discussions
- [Making Your Monophonic Module Polyphonic](https://community.vcvrack.com/t/making-your-monophonic-module-polyphonic/6926) - Polyphony implementation tutorial
- [Simple LPF Code Discussion](https://community.vcvrack.com/t/simple-lpf-code/18633) - Filter implementation examples

### Open Source Module Implementations
- [Bogaudio Modules](https://github.com/bogaudio/BogaudioModules) - VCF, LVCF filter examples with polyphony
- [StudioSixPlusOne Modules](https://github.com/StudioSixPlusOne/rack-modules) - Ladder filter implementation based on Pirkle's book
- [Valley Audio Modules](https://github.com/ValleyAudio/ValleyRackFree) - Various filter implementations

### DSP Implementation Resources
- [Digital State Variable Filter (EarLevel Engineering)](http://www.earlevel.com/main/2003/03/02/the-digital-state-variable-filter/) - SVF fundamentals
- [State Variable Filter (MusicDSP)](https://www.musicdsp.org/en/latest/Filters/23-state-variable.html) - Implementation variants
- [Self-Oscillating State Variable Filter (KVR Forum)](https://www.kvraudio.com/forum/viewtopic.php?t=333894) - Resonance and self-oscillation techniques
- [Oberheim SEM Filter Discussion (KVR Forum)](https://www.kvraudio.com/forum/viewtopic.php?t=497961) - SEM filter characteristics and modeling approaches

### Domain-Specific Knowledge
- [Oberheim SEM (Vintage Synth Explorer)](https://www.vintagesynth.com/oberheim/sem) - Historical context for SEM filter characteristics
- [State Space Filters (DSP Related)](https://www.dsprelated.com/freebooks/filters/State_Space_Filters.html) - Advanced filter theory

---
*Architecture research for: HydraQuartet VCF-OB — 8-voice polyphonic Oberheim-style filter module*
*Researched: 2026-01-29*
*Confidence: HIGH (verified with official VCV Rack documentation and community implementations)*
