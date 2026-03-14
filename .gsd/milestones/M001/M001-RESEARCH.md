# Project Research Summary

**Project:** CIPHER · OB (VCV Rack Module)
**Domain:** VCV Rack Polyphonic Filter Module Development
**Researched:** 2026-01-29
**Confidence:** HIGH

## Executive Summary

CIPHER · OB is a polyphonic multimode filter module for VCV Rack 2.x, inspired by classic Oberheim SEM (12dB) and OB-X (24dB) filters. VCV Rack modules are built using the official VCV Rack SDK 2.6.6+ with C++11, following a well-established architecture pattern: Module class for DSP/state management, ModuleWidget for UI, and configuration in constructor using config*() methods. The recommended approach uses state-variable filter topology to provide simultaneous lowpass/highpass/bandpass/notch outputs, polyphonic processing with SIMD optimization (rack::simd::float_4), and standard VCV voltage conventions.

The critical success factors are: (1) implementing resonance stability controls from the start to prevent filter blow-up and NaN propagation, (2) correct polyphonic channel handling with setChannels() and proper CV summing patterns, and (3) following VCV voltage standards without hard-clipping outputs. The main risk is that a basic state-variable filter implementation will sound "clinical" compared to analog Oberheim hardware - this requires accepting character loss in MVP and planning iterative refinement for analog modeling post-launch. Performance target is <5% CPU for 16 voices, achievable with SIMD optimization and efficient coefficient caching.

## Key Findings

### Recommended Stack

VCV Rack modules follow an official SDK-based development workflow with mature tooling and clear patterns. The stack is non-negotiable: VCV Rack SDK 2.6.6+ provides the framework, build system (Makefile), and all necessary DSP/UI libraries. C++11 is required by SDK compile flags, though the built-in rack::dsp library includes state-variable filters, SIMD types (float_4), and optimization utilities that eliminate the need for external DSP libraries.

**Core technologies:**
- **VCV Rack SDK 2.6.6+**: Official framework and API - provides headers, build system, component library, latest version includes MidiParser class and UTF-32/UTF-8 support
- **C++11**: Required by SDK compile flags - sufficient for DSP and UI code, ensures broadest compatibility
- **rack::dsp library**: Built-in DSP utilities - BiquadFilter, IIRFilter, SIMD optimization (float_4), ClockDivider for expensive operations
- **NanoVG**: Built-in vector graphics - renders all UI elements, custom widgets, dynamic displays
- **Inkscape**: SVG panel creation - official recommendation for panel design, free and cross-platform

**Critical version requirements:**
- Plugin version MAJOR must match Rack MAJOR (e.g., plugin 2.x.x for Rack 2.x)
- Optimization flags: -march=nehalem requires SSE4.2 and POPCNT (2008+ CPUs)
- No spaces allowed in file paths (Makefile limitation)

### Expected Features

Filter module users have strong expectations shaped by decades of hardware and software precedent. Missing table stakes features makes the product feel incomplete, while the right differentiators create competitive advantage in a crowded VCV Library ecosystem.

**Must have (table stakes):**
- Cutoff and resonance controls with CV inputs + attenuverters - universal filter parameters, VCV Rack standard for modulation
- 1V/Oct tracking - critical for musical/tonal filtering, users expect filters to track keyboard pitch
- Polyphonic support (16 channels) - expected for modern VCV modules, much better CPU than 8 mono modules
- Self-oscillation at high resonance - expected behavior, generates sine-like tone at cutoff frequency
- Low-pass output with 24dB slope - most common filter type, standard for Oberheim emulation
- Standard I/O layout - inputs top/left, outputs bottom/right follows VCV conventions

**Should have (competitive):**
- Simultaneous mode outputs (LP/HP/BP/Notch) - state-variable architecture enables this, key differentiator vs switchable filters
- Dual filter types (12dB SEM + 24dB OB-X) - two classic Oberheim characters in one module, 2-pole vs 4-pole
- Drive/saturation control - analog warmth and harmonic richness, complements Oberheim character
- Compact HP size (12-14 HP) - space efficiency in patches, typical full-featured filters are 12-16 HP
- Visual feedback (frequency response) - high value but complex to implement, helps users understand filter behavior

**Defer (v2+):**
- FM input with attenuverter - enables complex modulation but not critical for basic filtering
- Mixed output - convenience feature, users can patch external mixer
- Per-voice outputs - advanced feature for sophisticated users, defer to gauge demand
- Stereo I/O - adds complexity, most VCV synth voices are mono

### Architecture Approach

VCV Rack modules follow a layered architecture with clear separation: UI layer (ModuleWidget + SVG panel), Module processing layer (process() method executed per audio sample), and DSP state layer (filter coefficients, integrators, buffers). The state-variable filter topology is ideal for multimode filters as it produces LP/BP/HP/Notch outputs simultaneously from a single structure using two integrators and a feedback path with resonance control.

**Major components:**
1. **Module class** - inherits from rack::Module, implements process(const ProcessArgs& args) for per-sample DSP, configures params/inputs/outputs in constructor using config*() methods
2. **Polyphonic channel iteration** - determines channel count from input cables (getChannels()), processes each voice independently, maintains separate state per channel (float state[16] or simd::float_4 state[4])
3. **State-variable filter core** - two integrators (z1, z2) for filter state, Topology Preserving Transform (TPT) for stability at high cutoffs, simultaneous LP/BP/HP/Notch outputs from single structure
4. **SIMD optimization** - process 4 channels simultaneously using simd::float_4, Structure-of-Arrays layout for state, 4x performance improvement over scalar code
5. **UI widget placement** - ModuleWidget inherits from rack::ModuleWidget, places params/ports/lights using createParam()/createInput()/createOutput() helpers, SVG panel rendered automatically

**Critical patterns:**
- **Configuration in constructor**: All configParam(), configInput(), configOutput() calls happen in constructor, not process()
- **ClockDivider for expensive operations**: Update lights/UI at divided rate (e.g., every 512 samples), not per-sample
- **Resonance limiting and soft clipping**: Cap resonance below stability threshold, use tanh() in feedback path to prevent blow-up
- **Output channel setting**: Must call outputs[ID].setChannels(channels) to propagate polyphony, otherwise defaults to 1

### Critical Pitfalls

Research identified 16 pitfalls across critical/moderate/minor categories. The top pitfalls can crash the module, produce silent output, or cause VCV Library rejection if not addressed from the start.

1. **Filter resonance instability and blow-up** - State values spiral to infinity when resonance exceeds stability threshold, producing NaN/silent output. Prevention: implement resonance limiting, add tanh() nonlinearity in feedback path, check for NaN every sample and reset to 0.0f, use 2x oversampling minimum. Address in Phase 1 (Core DSP).

2. **Polyphonic channel handling errors** - Incorrectly summing CV inputs, failing to sum audio inputs, or not propagating channel counts breaks polyphonic patches. Prevention: use getVoltageSum() for monophonic audio, getVoltage(0) for CV (never sum), always call output.setChannels(channels), use getPolyVoltage(c) for per-channel modulation. Address in Phase 1 (Core DSP).

3. **DSP thread blocking and audio hiccups** - Accessing files, allocating memory, or performing expensive operations in process() causes crackling. Prevention: never access files in process(), cache expensive calculations, use FramebufferWidget for complex widgets, ensure <5% CPU target. Address in Phase 2 (Performance Optimization).

4. **Voltage standard violations** - Hard-clipping outputs at ±12V, incorrect V/oct scaling, or wrong trigger thresholds breaks interoperability. Prevention: allow voltages outside ±12V (let downstream modules attenuate), use soft saturation for gain >1x, follow V/oct standard (C4 at 0V), implement Schmitt triggers with hysteresis. Address in Phase 1 (Core DSP).

5. **Oberheim filter character loss** - Basic SVF sounds "clinical" without OTA nonlinearities, diode damping modeling, and DC offset/saturation behavior. Prevention: accept character loss in MVP, document as "Oberheim-inspired" not "clone", study SPICE simulations, plan iterative refinement post-launch. Address expectations in Phase 1, refine in Phase 4+ (post-MVP).

## Implications for Roadmap

Based on research, the recommended phase structure follows VCV Rack's natural build order: module shell before DSP, monophonic before polyphonic, scalar before SIMD, core filter before variants. This sequence enables incremental testing and avoids critical path dependencies.

### Phase 1: Foundation (Module Shell + Basic I/O)
**Rationale:** Must establish plugin infrastructure and I/O plumbing before implementing DSP. VCV Rack requires configuration in constructor using config*() methods, and testing DSP requires working inputs/outputs. Dependencies chain: can't test DSP without I/O infrastructure.

**Delivers:**
- Plugin structure (plugin.hpp/cpp with model registration)
- Module class with enums for params/inputs/outputs
- Configuration of all controls (cutoff, resonance, drive, filter type)
- Basic bypass routing using configBypass()
- Passthrough audio (no DSP yet, just infrastructure)

**Addresses:**
- Initialization lifecycle pitfall (configure in constructor, not elsewhere)
- Establishes correct polyphonic channel handling patterns from start
- plugin.json manifest with required metadata

**Avoids:**
- Pitfall 6 (incorrect parameter initialization lifecycle) - configure all params in constructor
- Pitfall 8 (plugin.json manifest errors) - set up correct metadata early

**Research flags:** Standard VCV Rack module structure, well-documented in official Plugin Development Tutorial. Skip additional research.

---

### Phase 2: Core DSP (Single Filter Type, Monophonic)
**Rationale:** Implement one filter type in simplest form (scalar, monophonic) to validate DSP algorithm before scaling to polyphony and SIMD. State-variable filter topology recommended by research for simultaneous outputs. Debugging DSP in simple case (1 voice, scalar code) much easier than 16 voices with SIMD.

**Delivers:**
- SEM-style 12dB lowpass state-variable filter (scalar implementation)
- Cutoff and resonance parameter processing with CV inputs
- Self-oscillation at high resonance with stability controls
- Resonance limiting and tanh() soft clipping in feedback path
- 1V/Oct tracking on cutoff frequency
- NaN/infinity checking and state reset

**Addresses:**
- **Must-have features**: Cutoff control, resonance control, 1V/Oct tracking, self-oscillation
- **Pitfall 1 (resonance instability)**: Implement resonance cap and soft clipping from start
- **Pitfall 4 (voltage standards)**: Follow ±5V audio output, V/oct standard (C4 at 0V)
- **Pitfall 7 (character loss)**: Set expectations - MVP is Oberheim-inspired, not clone

**Avoids:**
- Filter blow-up from unbounded resonance
- NaN propagation to downstream modules
- Voltage standard violations

**Research flags:** State-variable filter implementation is well-documented in DSP resources (EarLevel Engineering, MusicDSP.org). Standard patterns exist. Skip phase-specific research - use research findings from ARCHITECTURE.md and PITFALLS.md.

---

### Phase 3: Polyphonic Extension (Multi-Voice, Scalar)
**Rationale:** Extend working monophonic DSP to polyphonic (16 channels) using scalar code before SIMD optimization. This validates polyphonic channel handling patterns and per-voice state management without SIMD complexity. Dependencies: requires working monophonic DSP to test against.

**Delivers:**
- 16-channel polyphonic processing (scalar, not SIMD yet)
- Channel count detection from input cables using getChannels()
- Per-channel state management (float z1[16], z2[16])
- Correct output channel propagation using setChannels()
- Polyphonic CV modulation with getPolyVoltage(c)
- Testing with 1/2/8/16 channel polyphonic cables

**Addresses:**
- **Must-have features**: Polyphonic support (16 channels)
- **Pitfall 2 (polyphonic channel handling)**: Implement correct summing/channel propagation patterns
- getVoltageSum() for monophonic audio, getVoltage(0) for CV, setChannels() for outputs

**Avoids:**
- Incorrect CV summing (never sum pitch/modulation CV)
- Missing channel count propagation (silent voices)
- Polyphonic feedback loops

**Research flags:** Standard VCV Rack polyphony patterns, documented in official Polyphony Manual and community tutorials. Skip phase-specific research.

---

### Phase 4: Multiple Filter Modes and Types
**Rationale:** Add remaining filter modes (HP/BP/Notch) and second filter type (24dB OB-X) now that single filter works in polyphonic context. State-variable architecture naturally provides simultaneous outputs. Cascaded 2-pole sections for 24dB filter (simpler than true 4-pole, more stable).

**Delivers:**
- Simultaneous mode outputs: LP, HP, BP, Notch from state-variable topology
- Second filter type: 24dB (4-pole) OB-X by cascading two 12dB SVF stages
- Filter type parameter with mode switching
- Resonance compensation across cascaded stages for consistent gain
- All four outputs active simultaneously (key differentiator)

**Addresses:**
- **Must-have features**: LP/HP/BP/Notch modes, 12dB + 24dB filter types
- **Differentiator features**: Simultaneous outputs, dual Oberheim characters
- State-variable topology enables all modes from single structure

**Avoids:**
- Switchable-only outputs (less flexible than simultaneous)
- Unstable 4-pole implementation (cascade is more stable)

**Research flags:** Cascaded filter stages and mode extraction from SVF are standard DSP techniques. Documented in architecture research. Skip phase-specific research.

---

### Phase 5: SIMD Optimization (float_4)
**Rationale:** Refactor to SIMD (rack::simd::float_4) to achieve <5% CPU target for 16 voices. Polyphonic modules should use SIMD for 4x performance improvement (VCV Manual). Structure-of-Arrays layout required for SIMD efficiency. Dependencies: requires working scalar polyphonic implementation to test against.

**Delivers:**
- SIMD vectorization using simd::float_4 (process 4 channels at once)
- Structure-of-Arrays state layout (simd::float_4 z1[4], z2[4])
- Coefficient caching to avoid per-sample expensive math
- ClockDivider for light/meter updates (every 512 samples)
- Performance target: <5% CPU for 16 voices

**Addresses:**
- **Pitfall 3 (DSP thread blocking)**: Cache expensive calculations, use ClockDivider for UI updates
- Performance optimization for polyphonic processing
- CPU efficiency for multiple instances in patches

**Avoids:**
- Excessive CPU usage (>5% per instance)
- Per-sample expensive math (tan, exp, pow)
- High-rate UI updates causing performance hits

**Research flags:** SIMD optimization patterns documented in VCV Rack Plugin API Guide and rack::simd namespace. Standard patterns exist. Skip phase-specific research.

---

### Phase 6: Quality Polish (Drive, Anti-Aliasing, Panel)
**Rationale:** Add drive/saturation control and anti-aliasing to improve sound quality, finalize panel design. Non-blocking work items (panel design) can happen in parallel with earlier DSP phases. Anti-aliasing requires oversampling (CPU cost), make optional via right-click menu.

**Delivers:**
- Drive/saturation control with soft clipping
- Optional 2x oversampling for anti-aliasing (right-click menu toggle)
- SVG panel design (text converted to paths, simple gradients only)
- LED/meter visual feedback for filter activity
- Parameter tooltips and descriptions
- Panel layout following VCV guidelines (space between controls, readable text)

**Addresses:**
- **Differentiator features**: Drive control for analog warmth
- **Pitfall 5 (panel SVG errors)**: Export with text-to-paths, test incrementally
- **Pitfall 11 (missing anti-aliasing)**: Add oversampling for nonlinear processes
- **Pitfall 9 (zipper noise)**: Parameter smoothing for cutoff/resonance

**Avoids:**
- Harsh digital artifacts from saturation without anti-aliasing
- Panel rendering issues from SVG export errors
- Zipper noise from unsmoothed parameters

**Research flags:**
- Panel design: Inkscape SVG workflow well-documented in Panel Guide. Standard process.
- Anti-aliasing: Oversampling patterns documented in DSP Manual. Standard implementation.
- Skip phase-specific research.

---

### Phase 7: Library Submission Prep
**Rationale:** Final verification and testing before VCV Library submission. Checklist-driven phase ensures all requirements met and common pitfalls avoided.

**Delivers:**
- Complete plugin.json metadata (descriptions, tags, URLs)
- LICENSE file matching plugin.json license field
- Multi-platform build verification (Linux/Mac/Windows)
- Stress testing (hours-long operation, parameter automation)
- DAW plugin mode testing (VST/AU with window close/reopen)
- Final CPU profiling (<5% target verification)

**Addresses:**
- **Pitfall 8 (plugin.json errors)**: Verify all required metadata, correct version format
- **Pitfall 15 (missing metadata)**: Module descriptions, tags, URLs complete
- **Pitfall 13 (font/image loading)**: Test DAW plugin mode compatibility
- VCV Library submission requirements checklist

**Avoids:**
- Library rejection for missing metadata
- Build failures on other platforms
- Crashes in DAW plugin mode
- Performance issues under stress

**Research flags:** VCV Library submission process documented in Plugin Development Tutorial and Library repository. Standard checklist. Skip phase-specific research.

---

### Phase Ordering Rationale

This phase structure follows critical path dependencies discovered in architecture research:

1. **Module shell before DSP**: Can't test DSP without I/O infrastructure (configParam, inputs, outputs)
2. **Monophonic before polyphonic**: Debug DSP in simple case (1 voice) before scaling to 16 voices
3. **Scalar before SIMD**: SIMD optimization requires working scalar implementation to test against
4. **Core filter before variants**: Establish one filter type fully before adding modes and second type
5. **Optimization before polish**: Performance must be acceptable before adding CPU-intensive features (oversampling)

This grouping follows VCV Rack architectural patterns:
- **Phase 1-2**: Module and DSP foundation layers
- **Phase 3-4**: Polyphonic processing layer (core feature)
- **Phase 5**: Performance optimization layer (SIMD)
- **Phase 6**: UI/quality layer (panel, visual feedback)
- **Phase 7**: Integration layer (library submission)

This order avoids critical pitfalls:
- **Resonance stability** (Pitfall 1) addressed in Phase 2 before polyphonic scaling
- **Polyphonic handling** (Pitfall 2) addressed in Phase 3 with correct patterns from start
- **Performance** (Pitfall 3) addressed in Phase 5 before adding CPU-intensive features
- **Panel SVG** (Pitfall 5) deferred to Phase 6 when DSP stable (can happen in parallel)

### Research Flags

**Phases with standard patterns (skip additional research):**
- **Phase 1 (Module Shell)**: Well-documented in Plugin Development Tutorial, standard config*() patterns
- **Phase 2 (Core DSP)**: State-variable filter documented in architecture research, standard TPT implementation
- **Phase 3 (Polyphony)**: Polyphonic patterns documented in Polyphony Manual, standard channel handling
- **Phase 4 (Multiple Modes)**: SVF mode extraction standard DSP, cascaded stages well-documented
- **Phase 5 (SIMD)**: SIMD patterns documented in Plugin API Guide, standard float_4 usage
- **Phase 6 (Polish)**: Panel design in Panel Guide, oversampling in DSP Manual
- **Phase 7 (Library Prep)**: Submission process in Library repository, standard checklist

**No phases require `/gsd:research-phase`** - all patterns are well-documented in existing research files (STACK.md, ARCHITECTURE.md, PITFALLS.md) and official VCV Rack documentation. Research findings provide sufficient detail for implementation.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | Official VCV Rack SDK documentation, clear version requirements, no alternatives |
| Features | MEDIUM | Based on VCV Library ecosystem, community discussions, competitor analysis. User preferences validated but MVP feature set needs market validation |
| Architecture | HIGH | Official Plugin API Guide, multiple open-source reference implementations, standard patterns well-established |
| Pitfalls | MEDIUM-HIGH | Mix of official documentation (HIGH) and community discussions (MEDIUM). Critical pitfalls verified with multiple sources |

**Overall confidence:** HIGH

Research is comprehensive across all four areas. Stack and architecture have official documentation backing. Features and pitfalls combine official sources with community knowledge. No major gaps prevent roadmap creation.

### Gaps to Address

**Character modeling depth** - Research identifies that basic SVF will sound "clinical" without OTA nonlinearity modeling, but specific implementation details for Oberheim character are sparse. Mitigation: Accept as known limitation for MVP, document as "Oberheim-inspired" not "clone", plan iterative refinement post-launch with user feedback and A/B testing against hardware recordings.

**Resonance stability thresholds** - Research provides general guidance (cap below stability limit, use tanh()) but exact resonance parameter ranges need empirical testing. Mitigation: Plan testing phase during Phase 2 implementation with resonance sweep 0-100%, monitor for NaN, adjust caps based on results.

**SIMD vectorization patterns** - Research confirms SIMD is standard and provides float_4 patterns, but filter-specific SIMD implementation details may need experimentation. Mitigation: Implement scalar version first (Phase 3), use as reference for SIMD refactor (Phase 5), profile to verify 4x improvement.

**Performance targets** - Research suggests <5% CPU for 16 voices but exact measurements depend on implementation efficiency. Mitigation: Profile early and often, use CPU meter throughout development, have fallback optimization strategies (reduce oversampling, simplify coefficient calculation).

All gaps are implementation-level details, not architectural unknowns. Roadmap can proceed with planned validation/testing phases.

## Sources

### Primary (HIGH confidence)
- **VCV Rack Official Documentation**: Plugin Development Tutorial, Plugin API Guide, Polyphony Manual, DSP Manual, Voltage Standards, Panel Guide, Manifest spec - comprehensive coverage of all development aspects
- **VCV Rack API Reference**: rack::dsp namespace, rack::simd namespace, componentlibrary.hpp - official API documentation for SDK 2.6.6
- **VCV Rack Changelog v2**: Version 2.6.6 current as of Nov 2025, confirms latest SDK features

### Secondary (MEDIUM confidence)
- **VCV Community Development Forum**: Best practices discussions, polyphony patterns, common issues, optimization techniques - validated by multiple community members and official responses
- **Open Source Module Implementations**: Bogaudio (VCF, LVCF), Valley Audio (filters), StudioSixPlusOne (ladder filters) - real-world implementations demonstrating patterns
- **VCV Library**: Filter module ecosystem analysis for feature comparison and user expectations

### Tertiary (MEDIUM confidence, domain-specific)
- **DSP Resources**: EarLevel Engineering (SVF fundamentals), MusicDSP.org (implementation variants), KVR Forums (self-oscillation, Oberheim character) - technical DSP knowledge
- **Vintage Synth Explorer**: Oberheim SEM historical context for filter characteristics
- **Vult DSP Blog**: SEM filter modeling approaches and analog character techniques

---
*Research completed: 2026-01-29*
*Ready for roadmap: yes*

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
├── CipherOB.cpp     # Main module implementation
│   ├── struct CipherOB : Module
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
│   └── COLOSSUS · 16Widget.cpp   # Panel layout and widget placement
└── res/                    # Resources
    ├── COLOSSUS · 16.svg         # Panel design
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
struct CipherOB : Module {
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

    CipherOB() {
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
struct CipherOB : Module {
    dsp::ClockDivider lightDivider;

    CipherOB() {
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

**Recommendation for COLOSSUS · 16:**
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

**Budget Breakdown for COLOSSUS · 16 (16 voices):**
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
*Architecture research for: CIPHER · OB — 8-voice polyphonic Oberheim-style filter module*
*Researched: 2026-01-29*
*Confidence: HIGH (verified with official VCV Rack documentation and community implementations)*

# Technology Stack

**Project:** CIPHER · OB (VCV Rack Module)
**Researched:** 2026-01-29
**Confidence:** HIGH

## Recommended Stack

### Core Framework

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| VCV Rack SDK | 2.6.6+ | Module framework and API | Official SDK for VCV Rack 2.x with stable API, includes all headers, build system, and component library. Latest version (2.6.6 released Nov 2025) includes UTF-32/UTF-8 support and MidiParser class. |
| C++11 | std=c++11 | Programming language | Required by VCV Rack SDK compile flags. While individual plugins can use C++17, the SDK builds with C++11 standard. Provides sufficient features for DSP and UI code. |
| Makefile | Standard | Build system | Official VCV Rack build system. Uses `make` to compile, `make dist` to package, `make install` to deploy to user folder. No spaces allowed in paths. |
| helper.py | Bundled with SDK | Scaffolding tool | Python script included in SDK for creating plugin templates and generating modules from SVG panels. Automatically sets up plugin.json, source files, and directory structure. |

### DSP Libraries (Built into SDK)

| Library | Purpose | When to Use |
|---------|---------|-------------|
| rack::dsp | Digital signal processing utilities | Always - provides BiquadFilter, IIRFilter, RCFilter, ExponentialFilter, SlewLimiter, PeakFilter for filter modules |
| rack::simd | SIMD optimization (float_4) | For polyphonic processing - process 4 channels simultaneously with simd::pow(), simd::sin(), simd::trunc() |
| pffft | Fast Fourier Transform | If frequency-domain processing needed (not required for state-variable filters) |
| dsp::approx | Fast math approximations | For performance-critical DSP - dsp::exp2_taylor5() and similar functions |
| dsp::ode | ODE solvers (Euler, RK2, RK4) | For modeling analog circuits - useful for accurate filter emulation |
| dsp::Trigger | Trigger detection (SchmittTrigger, BooleanTrigger) | For CV gate/trigger inputs and button handling |

### Graphics & UI

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| NanoVG | Bundled with SDK | Vector graphics rendering | Built into VCV Rack - renders all UI elements, custom widgets, and dynamic displays. Use nvg* functions for custom drawing in Widget::draw() override. |
| NanoSVG | Bundled with SDK | SVG parsing | Automatically renders SVG panels. Limited gradient support - stick to two-color linear gradients. |
| Component Library | componentlibrary.hpp | UI widgets | Rack's built-in knobs, ports, switches, lights, screws. Use createParam(), createInput(), createOutput() helpers instead of building from scratch. |
| Framebuffer Widget | FramebufferWidget | GPU caching | Wrap expensive custom widgets to cache rendering. Only redraws when dirty flag set - critical for performance. |

### Panel Design Tools

| Tool | Purpose | Notes |
|------|---------|-------|
| Inkscape | SVG panel creation (RECOMMENDED) | Official recommendation. Free, cross-platform. Export with "Path > Object to Path" to convert all text. Set document to millimeters. Height = 128.5mm, width = multiple of 5.08mm (1 HP). |
| Adobe Illustrator | SVG panel creation (alternative) | Commercial option. Export with "Export As..." not "Save As...". Settings: Inline Style, Convert to Outlines, Embed, Layer Name, disable Minify/Responsive. |
| Affinity Designer | SVG panel creation (alternative) | Low-cost alternative (~$50). Export SVG with "Export Text as Curves" and "Use relative coordinates" enabled. |

### Development Tools

| Tool | Purpose | Notes |
|------|---------|-------|
| GDB | Debugging | Included with MinGW64 on Windows, standard on Linux/Mac. Use with `-d` flag to launch Rack in dev mode (uses terminal for logging, disables library to prevent plugin overwrites). |
| VS Code | IDE/Editor | Community standard. Configure tasks.json and launch.json for integrated debugging with GDB. |
| Git | Version control | Tag releases with `git tag vX.Y.Z` and `git push --tags`. Use commit hashes (not branch names) when submitting to VCV Library. |
| GitHub Actions | CI/CD (optional) | Community workflows available for automated multi-platform builds using VCV Rack Plugin Toolchain Docker image v14. |

## Build Environment Requirements

### Windows (MSYS2 MinGW 64-bit)

```bash
# Required packages (run in MinGW 64-bit shell, NOT default MSYS shell)
pacman -Syu git wget make tar unzip zip \
  mingw-w64-x86_64-gcc \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-gdb \
  mingw-w64-x86_64-jq \
  mingw-w64-x86_64-pkgconf \
  autoconf automake libtool \
  python zstd
```

### macOS (Homebrew)

```bash
brew install git wget cmake autoconf automake libtool jq python zstd pkg-config
```

### Linux (Ubuntu 16.04+)

```bash
sudo apt-get install git wget cmake autoconf automake libtool \
  libx11-dev libglu1-mesa-dev \
  libasound2-dev libjack-jackd2-dev \
  libgl1-mesa-dev libglu1-mesa-dev \
  jq python3 zstd pkg-config
```

### Linux (Arch)

```bash
sudo pacman -Syu git wget make tar unzip zip \
  gcc cmake gdb jq python zstd pkgconf \
  autoconf automake libtool \
  mesa glu alsa-lib jack
```

## Installation

### Initial Setup

```bash
# Download VCV Rack SDK from https://vcvrack.com/downloads/
# Extract SDK (available for Windows x64, Mac x64, Mac ARM64, Linux x64)

# Set environment variable
export RACK_DIR=/path/to/Rack-SDK

# Create plugin template
$RACK_DIR/helper.py createplugin YourPluginSlug

# Create module from SVG panel
$RACK_DIR/helper.py createmodule ModuleName res/ModuleName.svg src/ModuleName.cpp
```

### Development Workflow

```bash
# Build plugin
make

# Install to Rack user folder
make install

# Create distributable package
make dist

# Clean build artifacts
make clean
```

## Alternatives Considered

| Category | Recommended | Alternative | Why Not Alternative |
|----------|-------------|-------------|---------------------|
| Build System | Makefile | CMake | Makefile is the official standard. CMake support exists (unofficial Rack-SDK project) but adds complexity and isn't VCV Library compatible. |
| C++ Standard | C++11 | C++17 | SDK compiles with C++11. While individual plugins CAN use C++17 by overriding flags, C++11 is sufficient and ensures broadest compatibility. |
| Panel Design | Inkscape | Programmatic (code-gen) | Code-generated panels lack artistic flexibility. Most successful modules use SVG workflow for professional appearance. |
| DSP Library | rack::dsp | External (JUCE, etc.) | rack::dsp is optimized for VCV Rack, includes SIMD support, and is already linked. External libraries add binary size and linking complexity. |
| Graphics API | NanoVG | Direct OpenGL | NanoVG is the only supported API. Direct OpenGL would break compatibility and require reimplementing all UI infrastructure. |

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| Spaces in file paths | Breaks Makefile-based build system | Use underscores or hyphens in directory names |
| Master branch name | Rack repo uses version branches (v2, etc.) | Use `main` or version-specific branch names |
| Building Rack from source | Unnecessary for plugin development, slow | Download precompiled Rack SDK |
| Complex SVG gradients | Not fully supported by NanoSVG | Stick to simple two-color linear gradients |
| CSS styling in SVG | Not implemented in NanoSVG | Use inline styles, convert text to paths |
| Symbols in Illustrator SVG | Cause rendering issues in Rack | Flatten or expand symbols before export |
| `git add -A` for commits | Risk accidentally committing secrets | Add files explicitly by name |
| Branch names for Library submission | VCV requires commit hashes | Use `git rev-parse HEAD` to get hash |
| v prefix in plugin.json version | Version format is MAJOR.MINOR.REVISION | Use "2.0.1" not "v2.0.1" |

## Stack Patterns by Module Type

**For State-Variable Filter Module (This Project):**
- Use `rack::dsp::BiquadFilter` or `rack::dsp::IIRFilter` as foundation
- Implement `rack::simd::float_4` for 8-voice polyphony (process 2x float_4 vectors per sample)
- Use `configParam()` with attenuverters for CV control
- Set output channels with `setChannels()` based on input polyphony
- Override `process()` method for per-sample DSP
- Use `SchmittTrigger` for any mode switching buttons

**For Oscillator Modules:**
- Use `dsp::MinBLEP` for anti-aliased waveforms
- Use `dsp::PulseGenerator` for gate outputs
- Use `dsp::Resampler` if variable sample rate needed

**For Effect Modules:**
- Use `dsp::RingBuffer` for delay lines
- Use `dsp::RealFFT` for frequency-domain effects
- Use `FramebufferWidget` for spectrum analyzers to cache expensive visualization

## Version Compatibility

| Rack Version | SDK Version | Plugin MAJOR Version | Compiler | SSE Requirements |
|--------------|-------------|----------------------|----------|------------------|
| 2.6.6 | 2.6.6 | 2.x.x | GCC 12+ / Clang 14+ | SSE4.2 + POPCNT (nehalem) |
| 2.6.x | 2.6.x | 2.x.x | GCC 12+ / Clang 14+ | SSE4.2 + POPCNT (nehalem) |
| 2.5.x | 2.5.x | 2.x.x | GCC 11+ / Clang 13+ | SSE4.2 + POPCNT (nehalem) |
| 2.0.x | 2.0.x | 2.x.x | GCC 11+ / Clang 13+ | SSE4.2 + POPCNT (nehalem) |

**Important:** Plugin version MAJOR number must match Rack MAJOR version (e.g., plugin version 2.4.2 means compatible with Rack 2.x).

## Optimization Flags

VCV Rack SDK automatically applies:
```makefile
CXXFLAGS += -std=c++11
CXXFLAGS += -O3 -march=nehalem -funsafe-math-optimizations
```

**-march=nehalem** requires: SSE4.2 and POPCNT instruction sets (2008+ CPUs)

## Plugin Manifest (plugin.json)

### Required Fields
```json
{
  "slug": "YourPluginSlug",
  "name": "Your Plugin Name",
  "version": "2.0.0",
  "license": "GPL-3.0-or-later",
  "author": "Your Name"
}
```

### Recommended Optional Fields
```json
{
  "brand": "Synth-etic Intelligence",
  "description": "Polyphonic multimode filter inspired by Oberheim designs",
  "authorEmail": "support@example.com",
  "sourceUrl": "https://github.com/yourusername/yourplugin",
  "manualUrl": "https://yourplugin.com/manual",
  "changelogUrl": "https://github.com/yourusername/yourplugin/blob/main/CHANGELOG.md"
}
```

### Module Entry
```json
{
  "modules": [
    {
      "slug": "CipherOB",
      "name": "CIPHER · OB",
      "description": "8-voice polyphonic multimode filter (SEM 12dB, OB-X 24dB)",
      "tags": ["Filter", "Polyphonic"],
      "keywords": ["oberheim", "sem", "obx", "state variable", "multimode"]
    }
  ]
}
```

## Library Submission Process

1. **Prepare Repository:**
   - Host on GitHub with public access
   - Include LICENSE file matching plugin.json license field
   - Tag releases: `git tag v2.0.0 && git push --tags`

2. **Submit to VCV Library:**
   - Create issue at https://github.com/VCVRack/library/issues
   - Title = plugin slug (exact match)
   - Provide source code URL
   - Post commit hash (from `git rev-parse HEAD`)

3. **Push Updates:**
   - Increment version in plugin.json (2.0.0 → 2.0.1)
   - Commit and push
   - Post new commit hash in library issue thread

## Sources

**HIGH Confidence (Official Documentation):**
- [VCV Rack Plugin Development Tutorial](https://vcvrack.com/manual/PluginDevelopmentTutorial) - Core workflow, requirements, C++11 standard
- [VCV Rack Building Guide](https://vcvrack.com/manual/Building) - Build dependencies, compiler setup, environment requirements
- [VCV Rack Plugin API Guide](https://vcvrack.com/manual/PluginGuide) - Advanced features, polyphony, expanders, data persistence
- [VCV Rack Module Panel Guide](https://vcvrack.com/manual/Panel) - SVG specifications, component placement, design guidelines
- [VCV Rack Plugin Manifest](https://vcvrack.com/manual/Manifest) - plugin.json requirements and fields
- [VCV Rack DSP Manual](https://vcvrack.com/manual/DSP) - DSP theory and FFT library
- [VCV Rack Polyphony Manual](https://vcvrack.com/manual/Polyphony) - Polyphonic cable system (16 channels)
- [VCV Rack API Reference - rack::dsp](https://vcvrack.com/docs-v2/namespacerack_1_1dsp) - DSP classes and functions
- [VCV Rack API Reference - rack::simd](https://vcvrack.com/docs-v2/namespacerack_1_1simd) - SIMD optimization
- [VCV Rack API Reference - componentlibrary.hpp](https://vcvrack.com/docs-v2/componentlibrary_8hpp) - UI widget library
- [VCV Rack Changelog v2](https://github.com/VCVRack/Rack/blob/v2/CHANGELOG.md) - Version 2.6.6 current as of Nov 2025

**MEDIUM Confidence (VCV Community - Official Forum):**
- [VCV Community Development Forum](https://community.vcvrack.com/c/development/8) - Best practices, workflows
- [C++ Standard Discussion](https://community.vcvrack.com/t/newest-c-standard-allowed-for-inclusion-in-plugin-library/5970) - C++11 requirement confirmed
- [VS Code Debugging Setup](https://medium.com/@tonetechnician/how-to-setup-your-windows-vs-code-environment-for-vcv-rack-plugin-development-and-debugging-6e76c5a5f115) - IDE configuration
- [GitHub Actions Workflow](https://community.vcvrack.com/t/automated-building-and-releasing-plugins-on-github-with-github-actions/11364) - CI/CD automation

**MEDIUM Confidence (Open Source Examples):**
- [ValleyAudio/ValleyRackFree](https://github.com/ValleyAudio/ValleyRackFree) - GPL-3.0 plugin with filters, reverb, oscillators
- [VCV Rack GitHub Organization](https://github.com/vcvrack) - Official repositories
- [VCV Library](https://library.vcvrack.com/) - Published plugin examples

---
*Stack research for: VCV Rack 2.x polyphonic filter module*
*Researched: 2026-01-29*

# Feature Research

**Domain:** VCV Rack Polyphonic Multimode Filter Modules
**Researched:** 2026-01-29
**Confidence:** MEDIUM

## Feature Landscape

### Table Stakes (Users Expect These)

Features users assume exist. Missing these = product feels incomplete.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Cutoff frequency control | Core filter parameter, universal | LOW | Standard knob + CV input with attenuverter |
| Resonance/Q control | Essential for character & self-oscillation | LOW | High resonance should enable self-oscillation |
| CV inputs with attenuverters | VCV Rack standard for modulation | MEDIUM | At minimum: cutoff, resonance. Attenuverters required for flexible patching |
| 1V/Oct tracking | Users expect filters to track keyboard pitch | MEDIUM | Critical for musical filtering. Bogaudio noted for "very accurate V/OCT tracking" |
| Polyphonic support | 16-channel polyphonic processing | MEDIUM | Must auto-poly when receiving poly input. Much better CPU than 8 mono modules |
| Low-pass output | Most common filter type | LOW | 24dB slope (4-pole) is standard for Oberheim emulation |
| Self-oscillation | Expected at high resonance values | MEDIUM | Generates sine-like tone at cutoff frequency. VCV VCF self-oscillates around 70-80% resonance |
| Standard I/O layout | Inputs top/left, outputs bottom/right | LOW | Follows VCV Rack conventions for intuitive patching |

### Differentiators (Competitive Advantage)

Features that set the product apart. Not required, but valuable.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| Simultaneous mode outputs | All filter types available at once (LP/HP/BP/Notch) | MEDIUM | State-variable architecture enables this. Vult Stabile and AAS SVF provide simultaneous outputs |
| Dual filter types (12dB SEM + 24dB OB-X) | Two classic Oberheim characters in one module | HIGH | 12dB = 2-pole SEM, 24dB = 4-pole OB-X. Each pole = 6dB/octave |
| Drive/Saturation control | Analog warmth & harmonic richness | MEDIUM | VCV VCF's DRIVE "adds gain...saturated by filter circuit model, creating dirtier, grungier sound" |
| FM input with attenuverter | Audio-rate modulation for complex timbres | MEDIUM | Bogaudio implements by converting cutoff to V/Oct, adding FM, converting back |
| Visual feedback (frequency response) | Real-time display of filter curve | HIGH | Surge XT shows "filter response via chirp sweep transformation" - helps users understand what filter is doing |
| Mixed output | Pre-mixed combination of selected modes | LOW | Convenience for common use cases, saves patching a mixer |
| Individual voice outputs | Access to each poly channel separately | HIGH | Rare feature. Enables advanced routing/processing per voice |
| Compact HP size (8-14 HP) | Space efficiency in patches | LOW | Typical filters: 4-8 HP minimal, 12-16 HP full-featured. 12-14 HP reasonable for feature set |
| Analog modeling character | Oberheim-specific circuit emulation | HIGH | Cytomic CF100 praised for "highly detailed analog model." Vult filters valued for "analog modelling sound" |

### Anti-Features (Commonly Requested, Often Problematic)

Features that seem good but create problems.

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| Separate resonance per mode | "More control over each output" | Complex UI, confusing patch behavior when switching modes | Single resonance affects all modes consistently - simpler mental model |
| Unlimited resonance/instability | "Extreme feedback effects" | Produces NaN/infinite values that crash downstream modules | Cap resonance at self-oscillation point. Use std::isfinite() output checking |
| Audio-rate cutoff modulation without oversampling | "Fast FM effects" | Severe aliasing. Surge XT docs: "filter FM...easily falls apart in high registers" | Either implement oversampling (CPU cost) or document FM limitations |
| Switchable slope per mode | "Variable steepness" | UI bloat, doesn't match hardware inspiration (Oberheim has fixed slopes) | Offer two distinct filter types (12dB vs 24dB) as separate models |
| Built-in envelope generator | "All-in-one voice module" | Scope creep, defeats modular philosophy | Keep filter focused. Users can patch external envelopes |
| Morphing between filter types | "Smooth transitions" | Doesn't match hardware behavior, complex DSP | Discrete switching via CV input matches Oberheim design |

## Feature Dependencies

```
Polyphonic Support
    └──requires──> CV Input Processing (polyphonic CV handling)
    └──requires──> SIMD Optimization (CPU efficiency)

Self-Oscillation
    └──requires──> Resonance Control
    └──enhances──> 1V/Oct Tracking (tuned oscillator when self-oscillating)

Simultaneous Mode Outputs
    └──requires──> State-Variable Filter Topology
    └──conflicts──> Ladder Filter Topology (only LP output)

Drive/Saturation
    └──requires──> Anti-aliasing (oversampling or bandlimiting)
    └──enhances──> Analog Modeling Character

FM Input
    └──requires──> Exponential frequency control
    └──requires──> Anti-aliasing for audio-rate modulation
    └──enhances──> 1V/Oct Tracking (uses same pitch conversion)

Visual Feedback
    └──requires──> Filter coefficient calculation
    └──independent──> Real-time audio processing (can be lower sample rate)
```

### Dependency Notes

- **Polyphonic Support requires SIMD**: "Polyphonic modules are usually written with SIMD instructions" (VCV Manual). Performance advantage often 4x vs monophonic modules.
- **Simultaneous Outputs require State-Variable Topology**: Ladder filters (Moog-style) naturally only produce LP output. State-variable filters produce LP/HP/BP inherently.
- **Drive requires Anti-aliasing**: "Anti-aliasing is required for...distortion, saturation, and typically all nonlinear processes" (VCV Manual DSP section).
- **Self-Oscillation enhances 1V/Oct**: When filter self-oscillates, accurate pitch tracking turns it into a sine oscillator. Bogaudio filters noted for "very accurate V/OCT tracking."

## MVP Definition

### Launch With (v1)

Minimum viable product — what's needed to validate the concept.

- [x] **Cutoff control with CV input + attenuverter** — Core filter parameter, universal expectation
- [x] **Resonance control with CV input + attenuverter** — Essential for character & self-oscillation
- [x] **1V/Oct tracking on cutoff** — Critical for musical/tonal filtering
- [x] **Polyphonic support (16 channels)** — Target feature, expected for modern VCV modules
- [x] **Self-oscillation at high resonance** — Expected behavior, generates tones
- [x] **Four filter modes: LP/HP/BP/Notch** — Multimode is core value proposition
- [x] **Simultaneous mode outputs** — Key differentiator vs switchable filters
- [x] **12dB and 24dB filter types** — Dual Oberheim character (SEM + OB-X)
- [x] **Drive control** — Adds analog warmth, complements Oberheim character

### Add After Validation (v1.x)

Features to add once core is working.

- [ ] **FM input with attenuverter** — Enables complex modulation, but not critical for basic filtering
- [ ] **Visual frequency response display** — High value for users, but complex to implement
- [ ] **Mixed output** — Convenience feature, low priority (users can patch external mixer)
- [ ] **Per-voice outputs** — Advanced feature for sophisticated users, defer to gauge demand

### Future Consideration (v2+)

Features to defer until product-market fit is established.

- [ ] **Stereo input/output** — Adds complexity, most VCV synth voices are mono
- [ ] **Multiple filter topologies** — Beyond SEM/OB-X (e.g., Moog, Korg MS-20). Focus on Oberheim identity first.
- [ ] **Morphing/crossfade between 12dB/24dB** — Nice-to-have, but discrete switching is more authentic
- [ ] **Built-in VCA on outputs** — Convenient but scope creep. Users can patch external VCA.

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| Cutoff + CV | HIGH | LOW | P1 |
| Resonance + CV | HIGH | LOW | P1 |
| 1V/Oct tracking | HIGH | MEDIUM | P1 |
| Polyphonic (16ch) | HIGH | MEDIUM | P1 |
| Self-oscillation | HIGH | MEDIUM | P1 |
| LP/HP/BP/Notch modes | HIGH | MEDIUM | P1 |
| Simultaneous outputs | MEDIUM | MEDIUM | P1 |
| 12dB + 24dB types | MEDIUM | HIGH | P1 |
| Drive/Saturation | MEDIUM | MEDIUM | P1 |
| FM input | MEDIUM | MEDIUM | P2 |
| Visual response | MEDIUM | HIGH | P2 |
| Mixed output | LOW | LOW | P2 |
| Per-voice outputs | LOW | MEDIUM | P2 |
| Stereo I/O | LOW | MEDIUM | P3 |
| Filter morphing | LOW | HIGH | P3 |

**Priority key:**
- P1: Must have for launch (MVP)
- P2: Should have, add when possible (post-validation)
- P3: Nice to have, future consideration (v2+)

## Competitor Feature Analysis

| Feature | Surge XT VCF | Bogaudio Filter | Vult Stabile | Our Approach |
|---------|--------------|-----------------|--------------|--------------|
| Polyphonic | Yes, stereo | Yes | Yes | Yes, 16 channels |
| Filter modes | Many types/subtypes | LP/HP/BP/Notch | LP/HP/BP/SEM | LP/HP/BP/Notch (SEM-style blend) |
| Simultaneous outputs | No (switchable) | No (switchable) | Yes | Yes (key differentiator) |
| 1V/Oct tracking | Workaround (mod input) | "Very accurate" | Yes (with attenuverter) | Yes, dedicated input |
| Self-oscillation | Yes | No | Yes | Yes |
| Drive/Gain | Pre/post gain | No | Yes (Drive control) | Yes (Drive control) |
| Analog modeling | Yes (OB-Xd model) | No (pure DSP) | Yes (analog character) | Yes (Oberheim SEM + OB-X) |
| Filter slopes | Varies by type | 1-12 poles variable | Fixed (state-variable) | 12dB (SEM) + 24dB (OB-X) |
| Visual feedback | Yes (response curve) | No | No | Defer to v1.x |
| HP size | ~12-14 HP (estimate) | ~6-8 HP | ~6 HP | 12-14 HP |

## Sources

### Official Documentation (HIGH confidence)
- [VCV Rack Polyphony Manual](https://vcvrack.com/manual/Polyphony) - Polyphonic cable behavior, SIMD performance
- [VCV Rack DSP Manual](https://vcvrack.com/manual/DSP) - Anti-aliasing requirements
- [VCV Free Modules - VCF](https://vcvrack.com/Free) - VCV VCF features (drive, self-oscillation)
- [Surge XT Manual](https://surge-synthesizer.github.io/rack_xt_manual/) - OB-Xd filter, polyphonic stereo, FM limitations
- [Vult Stabile Documentation](https://modlfo.github.io/VultModules/stabile/) - State-variable filter, simultaneous outputs

### Community Discussions (MEDIUM confidence)
- [VCV Community: Favourite Filters](https://community.vcvrack.com/t/favourite-filters/10872) - User preferences, Vult praised for analog sound
- [VCV Community: Self-Oscillating SVF Questions](https://community.vcvrack.com/t/self-oscillating-svf-questions/17896) - Self-oscillation implementation
- [VCV Community: Compact Multimode Filter](https://community.vcvrack.com/t/compact-multi-mode-and-multi-slope-filter-module-exist/20268) - Multi-mode + multi-slope requests
- [VCV Community: Common Issues for Plugin Developers](https://community.vcvrack.com/t/help-wanted-for-reporting-common-issues-to-rack-plugin-developers/11031) - NaN/infinite values, anti-aliasing

### Library & Product Pages (MEDIUM confidence)
- [VCV Library - Filter Tag](https://library.vcvrack.com/?tag=Filter) - Filter module ecosystem overview
- [Cytomic VCV Rack Modules](https://cytomic.com/vcv-rack-modules/) - CF100 analog modeling, drive control
- [AAS Multiphonics CV-1 State Variable Filter](https://www.applied-acoustics.com/multiphonics-cv-1/manual/state-variable-filter/) - SVF simultaneous outputs, slopes

### GitHub & Technical Resources (MEDIUM confidence)
- [Bogaudio GitHub](https://github.com/bogaudio/BogaudioModules) - Filter implementation details, V/Oct tracking

---
*Feature research for: VCV Rack Polyphonic Multimode Filter Modules*
*Researched: 2026-01-29*

# Domain Pitfalls: VCV Rack Module Development

**Domain:** VCV Rack Polyphonic Filter Module Development
**Researched:** 2026-01-29
**Confidence:** MEDIUM-HIGH

## Critical Pitfalls

### Pitfall 1: Filter Resonance Instability and Blow-Up

**What goes wrong:**
Filter state values spiral to infinity when resonance is set too high, causing the filter to "blow up" and produce silent output or extreme distortion. This is especially problematic for self-oscillating state variable filters where the feedback gain can overwhelm the filter creating a positive-amplification feedback loop.

**Why it happens:**
- Resonance parameter exceeds stability threshold (often around k=8/3 for basic SVF implementations)
- Cutoff frequency becomes negative (ωc < 0), moving poles to right semiplane
- High resonance with insufficient nonlinearity in feedback path to limit amplitude
- Digital state variable filters become unstable at frequencies above f=1 (one-sixth of sample rate - 8kHz at 48kHz)

**Consequences:**
- Module produces NaN or infinity values that propagate through the patch
- Silent output when denormal protection clamps to zero
- Extreme CPU spikes from denormal number processing
- Poor user experience - users lose trust in the module

**Prevention:**
1. **Implement resonance limiting:** Cap resonance feedback gain below theoretical stability limit
2. **Add nonlinearity in feedback path:** Use tanh() or other soft saturation to control amplitude at self-oscillation
3. **Check for NaN/infinity:** Test state variables every sample and reset to 0.0f when detected
4. **Use 2x oversampling minimum:** Extends usable frequency range and improves stability
5. **Implement proper denormal handling:** Add DC offset or use flush-to-zero flags

**Warning signs:**
- Filter produces intermittent clicks or silence
- CPU meter shows spikes when filter is at high resonance
- Filter behavior changes dramatically at specific cutoff frequencies
- Self-oscillation amplitude grows unbounded instead of stabilizing

**Phase to address:**
Phase 1 (Core DSP Implementation) - Must implement resonance limiting and stability checks before building UI

**Sources:**
- [Self-oscillating SVF Questions - VCV Community](https://community.vcvrack.com/t/self-oscillating-svf-questions/17896)
- [How do you know what resonance levels blow up your filter? - KVR](https://www.kvraudio.com/forum/viewtopic.php?t=504548)
- [State Variable Filter (Double Sampled, Stable) - Musicdsp.org](https://www.musicdsp.org/en/latest/Filters/92-state-variable-filter-double-sampled-stable.html)

---

### Pitfall 2: Polyphonic Channel Handling Errors

**What goes wrong:**
Module incorrectly sums CV inputs that shouldn't be summed, fails to sum audio inputs that should be summed, or doesn't propagate channel counts correctly. This breaks polyphonic patches and creates user confusion.

**Why it happens:**
- Using `getVoltage()` instead of `getVoltageSum()` for monophonic audio processing
- Summing CV/pitch inputs when only first channel should be used
- Not calling `setChannels()` on outputs to match input channel count
- Forgetting polyphonic parameter modulation with `getPolyVoltage(c)`
- Polyphony feedback loops causing channel count to increase but never decrease

**Consequences:**
- Audio sounds wrong or is silent when polyphonic cables connected
- Modulation doesn't work as expected across multiple voices
- Lights show incorrect polyphonic state
- Module rejected from VCV Library for incorrect polyphonic behavior

**Prevention:**
1. **Audio inputs (monophonic modules):** Use `getVoltageSum()` to sum all channels
2. **CV/pitch inputs:** Use `getVoltage(0)` to read only first channel - never sum CV
3. **Set output channels:** Always call `output.setChannels(channels)` in process()
4. **Parameter modulation:** Use `getPolyVoltage(c)` to apply modulation per-channel
5. **Display polyphonic state:** Use root-mean-square formula for lights: √(x₀² + x₁² + ... + xₙ²)
6. **Test with polyphonic sources:** Verify behavior with 1, 2, 8, and 16 channel inputs

**Warning signs:**
- Polyphonic signals sound quieter than expected
- Only first voice audible when using polyphonic cables
- Modulation affects all voices identically when it shouldn't
- Channel count displayed incorrectly

**Phase to address:**
Phase 1 (Core DSP Implementation) - Polyphonic architecture must be correct from the start

**Sources:**
- [Polyphony reminders for plugin developers - VCV Community](https://community.vcvrack.com/t/polyphony-reminders-for-plugin-developers/9572)
- [VCV Rack Manual - Polyphony](https://vcvrack.com/manual/Polyphony)
- [Plugin API Guide - VCV Rack Manual](https://vcvrack.com/manual/PluginGuide)

---

### Pitfall 3: DSP Thread Blocking and Audio Hiccups

**What goes wrong:**
Audio output crackles or stutters because the process() method blocks the DSP thread by accessing files, allocating memory, or performing expensive operations during real-time processing.

**Why it happens:**
- Accessing patch storage files in process() instead of onAdd()/onSave()
- Loading fonts/images during process() instead of draw()
- Performing expensive calculations without caching results
- Not using FramebufferWidget to cache complex widget rendering
- Setting buffer size too small relative to DSP processing load

**Consequences:**
- Audible clicks, pops, or crackling in output
- VCV Library rejection for poor performance
- Bad user experience with audio dropouts
- Module unusable in real-time performance contexts

**Prevention:**
1. **Never access files in process():** Use onAdd() to load, onSave() to save
2. **Cache expensive calculations:** Store filter coefficients when parameters change
3. **Use FramebufferWidget:** Mark dirty only when UI state changes, not every frame
4. **Profile first, optimize second:** Use perf/Valgrind to identify actual bottlenecks
5. **Test with CPU meter:** Ensure module uses <5% CPU for single instance
6. **Optimize with SIMD:** Use -march=nehalem compiler flags and vectorizable code

**Warning signs:**
- CPU meter shows spikes when adjusting parameters
- Audio clicks when moving knobs
- Performance degrades over time
- Module works fine alone but causes issues in larger patches

**Phase to address:**
Phase 2 (Performance Optimization) - After core DSP works, before Library submission

**Sources:**
- [VCV Rack Manual - Digital Signal Processing](https://vcvrack.com/manual/DSP)
- [VCV Rack Manual - Plugin API Guide](https://vcvrack.com/manual/PluginGuide)
- [Performance Issue: Audio Hiccups - VCV Community](https://community.vcvrack.com/t/performance-issue-audio-hiccups/1696)

---

### Pitfall 4: Voltage Standard Violations

**What goes wrong:**
Module produces voltages outside expected ranges, hard-clips outputs at ±12V, or uses incorrect V/oct scaling. This breaks interoperability with other modules and causes unexpected behavior.

**Why it happens:**
- Using clamp() to enforce ±12V limits on outputs (explicitly discouraged)
- Not implementing soft saturation when applying >1x gain
- Using wrong pitch standard (Hz/oct, Hz/V) instead of V/oct
- Incorrect base frequency for oscillators (not C4 at 0V)
- Trigger thresholds without hysteresis causing false retriggering

**Consequences:**
- Module sounds harsh or distorts incorrectly
- Pitch tracking doesn't work with other modules
- Triggers fire multiple times from single gate
- VCV Library rejection for violating voltage standards
- User complaints about "broken" behavior

**Prevention:**
1. **Never hard-clip outputs:** Allow voltages outside ±12V range, let downstream modules attenuate
2. **Use soft saturation:** Apply tanh() or similar when gain >1x to prevent harsh clipping
3. **Follow V/oct standard:** 1V = one octave doubling, C4 (261.6256Hz) at 0V
4. **Implement Schmitt triggers:** Low threshold ~0.1V, high threshold ~1-2V with hysteresis
5. **Standard voltage ranges:**
   - Audio outputs: ±5V (before bandlimiting)
   - Unipolar CV: 0-10V
   - Bipolar CV: ±5V
   - Gates/triggers: 10V output
6. **Handle NaN/infinity:** Check and return 0 when unstable outputs occur

**Warning signs:**
- Output sounds harsh when driven hard
- Pitch doesn't match other oscillators
- Module behaves differently than hardware counterpart
- Gates trigger inconsistently

**Phase to address:**
Phase 1 (Core DSP Implementation) - Must follow standards from beginning

**Sources:**
- [VCV Rack Manual - Voltage Standards](https://vcvrack.com/manual/VoltageStandards)
- [Plugin API Guide - VCV Rack Manual](https://vcvrack.com/manual/PluginGuide)

---

### Pitfall 5: Panel SVG Export Errors

**What goes wrong:**
Module panel doesn't display correctly in VCV Rack - text is missing, gradients don't render, or entire panel is blank. This blocks Library submission and wastes development time.

**Why it happens:**
- Text not converted to paths/outlines before export
- Using complex gradients beyond simple two-color linear gradients
- Using CSS styling features with limited SVG support
- Exporting with incorrect settings from design software
- Including unsupported SVG features

**Consequences:**
- Module can't be submitted to Library without working panel
- Time wasted debugging visual issues instead of DSP
- Unprofessional appearance if workarounds used
- Need to redo panel design work

**Prevention:**
1. **Convert all text to paths:** In Inkscape: Path > Object to Path
2. **Use simple gradients only:** Stick to two-color linear gradients
3. **Export settings (Affinity Designer):**
   - Use "Export As..." not "Save As..."
   - Enable: Inline Style, Convert to Outlines, Embed, Layer Name
   - More settings: Export Text as Curves, Use relative coordinates
4. **Export settings (Inkscape):**
   - Use batch conversion script for text to paths
   - Export as Plain SVG
5. **Test incrementally:** Start with tutorial SVG, add elements one at a time, export and test each time
6. **Follow panel guidelines:**
   - Enough space between controls for fingers
   - Text readable at 100% on non-high-DPI monitor
   - Inverted background for output ports

**Warning signs:**
- Panel displays correctly in Inkscape but not VCV Rack
- Text visible in export preview but missing in Rack
- Gradients appear as solid colors
- Panel is blank or shows only some elements

**Phase to address:**
Phase 3 (UI/Panel Design) - Before creating final panel artwork

**Sources:**
- [Module Panel Guide - VCV Rack Manual](https://vcvrack.com/manual/Panel)
- [svg text not showing - VCV Community](https://community.vcvrack.com/t/svg-text-not-showing/19987)

---

### Pitfall 6: Incorrect Parameter Initialization Lifecycle

**What goes wrong:**
Module crashes on load, parameters don't persist correctly, or patch storage operations fail because initialization happens in wrong lifecycle methods.

**Why it happens:**
- Calling configParam() outside constructor
- Accessing patch storage in constructor before module added to engine
- Loading state in constructor instead of onAdd()
- Not understanding constructor vs. onAdd() vs. onReset() vs. dataFromJson()

**Consequences:**
- Crashes when loading patches
- User settings lost between sessions
- VCV Library rejection for instability
- Difficult-to-debug lifecycle issues

**Prevention:**
1. **Constructor:** Use ONLY for config() calls:
   - configParam(), configInput(), configOutput(), configLight()
   - Setting up parameter ranges and defaults
   - Allocating member variables
2. **onAdd():** Use for:
   - Loading patch storage files
   - Initialization requiring module in engine
   - First-time setup operations
3. **onReset():** Use for:
   - Resetting module to default state
   - Called when user clicks "Initialize" in context menu
4. **dataFromJson():** Use for:
   - Loading custom instance variables from patch
   - Deserializing state not covered by parameters
5. **dataToJson():** Use for:
   - Saving custom instance variables to patch
   - Large data (>100kB) should use patch storage instead

**Warning signs:**
- Crashes when opening patches with your module
- Parameters reset to defaults unexpectedly
- State doesn't persist between Rack sessions
- Null pointer exceptions in constructor

**Phase to address:**
Phase 1 (Core DSP Implementation) - Establish correct lifecycle patterns early

**Sources:**
- [Confusion about onAdd() and onReset() - VCV Community](https://community.vcvrack.com/t/confusion-about-onadd-and-onreset/11855)
- [Plugin API Guide - VCV Rack Manual](https://vcvrack.com/manual/PluginGuide)
- [VCV Rack API - Module Reference](https://vcvrack.com/docs-v2/structrack_1_1engine_1_1Module)

---

### Pitfall 7: Oberheim Filter Character Loss in Digital Modeling

**What goes wrong:**
Digital implementation sounds "clinical" or "sterile" compared to hardware Oberheim SEM/OB-X filters. The characteristic "beautifully buzzing high-end" and musical filter modulation response is missing despite getting basic frequency response correct.

**Why it happens:**
- Using basic SVF topology without modeling OTA (Operational Transconductance Amplifier) nonlinearities
- Missing diode damping path characteristics
- Not modeling DC offset issues and buffer saturation behavior
- Ignoring "bonk out" phenomenon (pendulum-like state variable behavior)
- Insufficient modeling of voltage-controlled resonance complexity
- Standard TPT (Topology Preserving Transform) with Newton-Raphson not capturing character

**Consequences:**
- Filter sounds correct on paper but lacks musical character
- Users complain it doesn't sound like "real" Oberheim
- Module fails to differentiate from generic SVF filters
- Marketing claims about Oberheim-inspired sound ring hollow

**Prevention:**
1. **Accept character loss in MVP:** Document as "Oberheim-inspired" not "Oberheim clone"
2. **Study SPICE simulations:** Analyze circuit-level behavior, not just transfer function
3. **Model nonlinearities:** Focus on OTA current-dumping behavior and saturation
4. **Reference existing implementations:** Study Vult, Arturia SEM filter approaches
5. **Use oversampling:** Minimum 2x to capture high-frequency character
6. **Test with musical context:** A/B against hardware recordings, not just sine sweeps
7. **Plan iterative refinement:** Version 1.0 = functional, future versions = character refinement

**Warning signs:**
- Filter sounds too "clean" compared to references
- High resonance lacks "buzz" or "grit"
- FM/modulation response sounds digital, not organic
- Beta testers say "it works but doesn't sound right"

**Phase to address:**
Phase 1 (Core DSP Implementation) - Set realistic expectations for V1.0
Phase 4 (Character Refinement) - Post-MVP iteration for improved modeling

**Sources:**
- [What makes SEM filter so special - KVR](https://www.kvraudio.com/forum/viewtopic.php?t=497961)
- [Exploring Chorus and SEM filter features - Vult DSP](https://www.vult-dsp.com/post/exploring-the-new-chorus-and-sem-filter-features-in-the-freak-firmware-update)

---

## Moderate Pitfalls

### Pitfall 8: Plugin.json Manifest Errors

**What goes wrong:**
Plugin fails to load, VCV Library rejects submission, or versioning is incorrect due to malformed plugin.json manifest.

**Prevention:**
1. **Required fields:** slug, name, version, license, author, modules[].slug, modules[].name
2. **Version format:** MAJOR.MINOR.REVISION where MAJOR matches Rack version (2.x.x for Rack 2)
3. **Slug rules:** Never change after release (breaks patch compatibility), only alphanumeric/hyphen/underscore
4. **License:** Use SPDX identifiers (MIT, GPL-3.0-or-later) or "proprietary"
5. **Module tags:** Must match predefined case-insensitive tags
6. **Version updates:** Increment version field and push commit with hash (not just branch name)

**Sources:**
- [Plugin Manifest - VCV Rack Manual](https://vcvrack.com/manual/Manifest)
- [Plugin.json version common errors - VCV Community](https://community.vcvrack.com/t/v2-issue-kind-of-with-plugin-json/14254)

---

### Pitfall 9: Zipper Noise from Unsmoothed Parameters

**What goes wrong:**
Audible crackling or stepping artifacts when modulating filter parameters, especially cutoff frequency at audio rate.

**Prevention:**
1. **Implement parameter smoothing:** Use one-pole lowpass filter for cutoff/resonance changes
2. **Smooth over multiple samples:** 10-100 samples depending on parameter
3. **Don't smooth audio-rate modulation inputs:** Only smooth user-controlled parameters
4. **Use exponential smoothing:** `smoothed += (target - smoothed) * coefficient`
5. **Test with fast LFO modulation:** Sweep parameters quickly to expose zipper noise

**Sources:**
- [State Variable Filter zipper noise - KVR](https://www.kvraudio.com/forum/viewtopic.php?t=58555)
- [Digital Signal Processing - VCV Rack Manual](https://vcvrack.com/manual/DSP)

---

### Pitfall 10: Incorrect Bypass Routing Configuration

**What goes wrong:**
Module bypasses incorrectly - either routes signals that shouldn't be bypassed, or users expect muting but get unity mixing.

**Prevention:**
1. **Only bypass effect modules:** If module applies effect to input → output, configure bypass
2. **Don't bypass:** Pitch CV to audio, mixer channels, oscillators, generators
3. **A mixer is not an effect:** Users expect muting, not unity-mixing all channels
4. **Test bypass behavior:** Ask "what would user expect when bypassing?" - least surprising behavior wins

**Sources:**
- [Plugin API Guide - VCV Rack Manual](https://vcvrack.com/manual/PluginGuide)

---

### Pitfall 11: Missing Anti-Aliasing in Nonlinear Processes

**What goes wrong:**
Filter produces harsh digital artifacts during waveshaping, saturation, or distortion from lack of anti-aliasing.

**Prevention:**
1. **Required for:** Waveform generation, waveshaping, distortion, saturation, all nonlinear processes
2. **Use oversampling:** 2x minimum, 4x-8x for high-quality
3. **Choose quality vs. CPU:** Context menu option for oversampling quality settings
4. **Test with high frequencies:** Sweep filter at high resonance near Nyquist to expose aliasing
5. **Disable for testing:** Some cases (chaotic/low-frequency output) may not need oversampling

**Sources:**
- [Digital Signal Processing - VCV Rack Manual](https://vcvrack.com/manual/DSP)
- [Tricks for using CPU meters effectively - VCV Community](https://community.vcvrack.com/t/tricks-for-using-the-cpu-meters-effectively/13992)

---

### Pitfall 12: Thread Safety Violations with Expander Modules

**What goes wrong:**
Accessing expander module instance variables directly causes race conditions, corrupt reads, or inconsistent latency.

**Prevention:**
1. **Never access instance variables directly:** Always use expander message system
2. **Allocate both message buffers:** Identical blocks of memory (structs/arrays)
3. **Check model before writing:** Verify expander is correct type
4. **Set messageFlipRequested:** After writing message, set to true for double-buffer swap
5. **Write-only producer, read-only consumer:** Don't mix read/write on same buffer
6. **Alternative:** Base module can access expander inputs/outputs directly (no sample delay)

**Sources:**
- [Simple Expander example/tutorial - VCV Community](https://community.vcvrack.com/t/simple-expander-example-tutorial/17989)
- [Expander Thread safe? - VCV Community](https://community.vcvrack.com/t/expander-thread-safe/11029)
- [Plugin API Guide - VCV Rack Manual](https://vcvrack.com/manual/PluginGuide)

---

### Pitfall 13: Font/Image Loading in Widget Constructor (DAW Plugin Mode)

**What goes wrong:**
In DAW plugin mode, when editor window closes and reopens, Font/Image references become invalid because OpenGL context changes.

**Prevention:**
1. **Don't store Font/Image in constructor:** Only store file path
2. **Fetch every frame in draw():** Use cached path to reload from APP->window
3. **Only affects DAW plugin mode:** Standalone works fine, but Library submission requires DAW compatibility
4. **Test in DAW:** Load as VST/AU in DAW, close/reopen editor window to verify

**Sources:**
- [Plugin API Guide - VCV Rack Manual](https://vcvrack.com/manual/PluginGuide)
- [Migrating v1 Plugins to v2 - VCV Rack Manual](https://vcvrack.com/manual/Migrate2)

---

## Minor Pitfalls

### Pitfall 14: Build Environment Path Spaces

**What goes wrong:**
Makefile-based build system breaks if absolute path contains spaces.

**Prevention:**
1. **Use paths without spaces:** Place project in /home/user/vcv/ not /home/user/My Projects/vcv/
2. **Disable antivirus during build:** Can interfere with build process or make builds slow
3. **Test build early:** Run `make` in plugin directory to verify empty plugin builds

**Sources:**
- [Building - VCV Rack Manual](https://vcvrack.com/manual/Building)

---

### Pitfall 15: Missing Module Metadata for Library Submission

**What goes wrong:**
VCV Library rejects plugin due to missing descriptions, incorrect tags, or missing URLs.

**Prevention:**
1. **Required metadata:** Module descriptions, module tags, URLs (source, manual, homepage)
2. **Describe what module does:** Clear explanation or hardware reference
3. **Use correct tags:** Match predefined tag list (Filter, Polyphonic, etc.)
4. **Review before submission:** Check plugin.json for spelling, capitalization, completeness
5. **Hardware clones:** Must attribute original manufacturer in module name

**Sources:**
- [Help wanted for reporting common issues - VCV Community](https://community.vcvrack.com/t/help-wanted-for-reporting-common-issues-to-rack-plugin-developers/11031)
- [Plugin Manifest - VCV Rack Manual](https://vcvrack.com/manual/Manifest)

---

### Pitfall 16: CPU Meter Left Enabled

**What goes wrong:**
Module appears to use more CPU than it actually does because CPU meter itself consumes engine CPU time.

**Prevention:**
1. **Disable when not profiling:** Turn off CPU meter after optimization work
2. **Don't leave on by default:** Wastes CPU in user patches
3. **Profile module without meter running:** For accurate measurements

**Sources:**
- [Tricks for using CPU meters effectively - VCV Community](https://community.vcvrack.com/t/tricks-for-using-the-cpu-meters-effectively/13992)

---

## Technical Debt Patterns

Shortcuts that seem reasonable but create long-term problems.

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Skip oversampling | 50-90% less CPU usage | Harsh aliasing artifacts in nonlinear processing | Only if output is chaotic/low-frequency and aliasing inaudible |
| Hard-clip outputs at ±12V | Simple code, "safe" voltage range | Breaks interoperability, harsh distortion | Never - explicitly discouraged by VCV |
| Use basic SVF without nonlinearities | Stable, predictable behavior | Lacks character of analog filters | Acceptable for MVP if documented as limitation |
| Skip parameter smoothing | Instant response to changes | Zipper noise when modulating | Never - always smooth control-rate parameters |
| Direct expander variable access | No sample delay | Race conditions, corrupt data | If using base-module-does-all-work pattern |
| Single-threaded testing only | Faster development iteration | Crashes in user patches with threading | Never - must test multi-threaded scenarios |
| Copy tutorial code without understanding | Quick start | Subtle bugs from misunderstanding lifecycle | For learning, but refactor before release |
| Skip unit tests | Faster initial development | Hard to debug DSP issues later | Acceptable for MVP, add before 1.0 release |

---

## Performance Traps

Patterns that work at small scale but fail as usage grows.

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| No coefficient caching | CPU spikes when moving controls | Calculate coefficients only when parameters change | Multiple instances or fast modulation |
| Accessing SIMD elements with a[i] | High CPU despite SIMD code | Use SIMD operations, don't index individual elements | Any SIMD usage - defeats vectorization |
| Complex widget drawing every frame | Laggy UI, high CPU in editor | Use FramebufferWidget, mark dirty only on changes | Multiple instances visible on screen |
| Oversampling at 8x-16x by default | Sounds great initially | Can't use multiple instances before hitting CPU limit | 3+ instances in patch |
| Missing denormal protection | Gradual CPU increase over time | Flush-to-zero or add DC offset to feedback paths | After filter runs for several minutes at high resonance |
| File I/O in process() | Works fine with small files | Audio hiccups with large preset files | Larger patch storage data |

---

## Integration Gotchas

Common mistakes when connecting to VCV Rack ecosystem.

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| Polyphonic cables | Using getVoltage() for audio inputs | Use getVoltageSum() in monophonic audio modules |
| V/oct pitch CV | Using 440Hz at 0V or Hz/oct scaling | Use C4 (261.6256Hz) at 0V, 1V/oct standard |
| Reset/clock sync | Missing triggers due to 1-sample cable delay | Ignore CLOCK for 1ms after RESET received |
| Gate/trigger inputs | No hysteresis causes double-triggering | Schmitt trigger: low ~0.1V, high ~1-2V |
| NaN/infinity propagation | Assuming other modules handle it | Check outputs and return 0 when unstable |
| Patch storage | Accessing in constructor | Load in onAdd(), save in onSave() |
| Expander communication | Direct instance variable access | Use double-buffered message system |

---

## "Looks Done But Isn't" Checklist

Things that appear complete but are missing critical pieces.

- [ ] **Filter DSP:** Sounds correct with sine wave test — verify with noise, square wave, and self-oscillation stress tests
- [ ] **Polyphony:** Works with monophonic cables — verify with 1, 2, 8, 16 channel polyphonic cables
- [ ] **Panel design:** Looks good in Inkscape — verify text converted to paths and renders in VCV Rack
- [ ] **Build system:** Compiles on your machine — verify clean build on Linux/Mac/Windows with no path spaces
- [ ] **Performance:** Single instance runs fine — verify 8+ instances don't exceed 50% CPU
- [ ] **Stability:** Works in test patch — verify hours-long stress test with parameter automation
- [ ] **DAW mode:** Works standalone — verify in VST/AU with window close/reopen cycles
- [ ] **Library submission:** Plugin.json complete — verify all required metadata, tags, URLs present
- [ ] **Voltage standards:** Produces expected voltage ranges — verify with oscilloscope module monitoring outputs
- [ ] **Save/load:** State persists — verify dataToJson/dataFromJson handles all custom state
- [ ] **Parameter ranges:** Knobs turn smoothly — verify no sudden jumps, discontinuities, or zipper noise
- [ ] **Thread safety:** No crashes in your tests — verify with multiple random patches loading/unloading

---

## Recovery Strategies

When pitfalls occur despite prevention, how to recover.

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Filter resonance blow-up | LOW | Add NaN check in process(), clamp state variables, test resonance sweep 0-100% |
| Polyphonic channel handling wrong | MEDIUM | Audit all input.getVoltage() calls, add channel count logic, test with poly cables |
| DSP thread blocking | LOW | Move file I/O to onAdd/onSave, profile with perf, cache expensive calculations |
| Voltage standard violations | LOW-MEDIUM | Remove hard clipping, add soft saturation, verify ranges with scope module |
| Panel SVG errors | LOW | Rebuild SVG from scratch following export guidelines, test incrementally |
| Wrong initialization lifecycle | MEDIUM | Refactor constructor/onAdd/dataFromJson, test save/load cycles thoroughly |
| Character loss in filter modeling | HIGH | Requires DSP research, SPICE analysis, iterative refinement - plan for V2.0 |
| Plugin.json errors | LOW | Review manifest against spec, fix required fields, test load in Rack |
| Zipper noise | LOW | Add parameter smoothing with one-pole filter, test with fast modulation |
| Bypass routing incorrect | LOW | Review bypass logic against guidelines, test bypass in various patch contexts |
| Missing anti-aliasing | MEDIUM | Add oversampling infrastructure, test with high-frequency sweeps |
| Thread safety violations | MEDIUM | Switch to expander messages, add double-buffering, test with rapid add/remove |
| Font/image loading issues | LOW | Save path only, fetch in draw(), test in DAW plugin mode |

---

## Pitfall-to-Phase Mapping

How roadmap phases should address these pitfalls.

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| Filter resonance instability | Phase 1: Core DSP | Stress test: resonance at 100% with noise input for 10 minutes, verify no NaN |
| Polyphonic channel handling | Phase 1: Core DSP | Test: 1/2/8/16 channel inputs, verify correct summing/channel propagation |
| DSP thread blocking | Phase 2: Performance | CPU meter: single instance <5%, watch for spikes during parameter changes |
| Voltage standard violations | Phase 1: Core DSP | Scope module: verify ±5V audio, proper V/oct tracking, Schmitt trigger thresholds |
| Panel SVG errors | Phase 3: UI/Panel Design | Visual check: all text/graphics render correctly in VCV Rack |
| Initialization lifecycle | Phase 1: Core DSP | Test: save/load patch, initialize module, verify state persists correctly |
| Oberheim character loss | Phase 4: Character Refinement (post-MVP) | A/B test: compare against hardware recordings in musical context |
| Plugin.json manifest errors | Phase 5: Library Submission Prep | Validation: load in clean Rack install, check Library submission requirements |
| Zipper noise | Phase 2: Performance | Audio test: fast LFO on cutoff, verify smooth transitions no clicks |
| Bypass routing | Phase 3: UI/Panel Design | User test: bypass module in patch, verify expected behavior |
| Missing anti-aliasing | Phase 2: Performance | Spectrum analyzer: high-resonance sweep near Nyquist, verify no harsh aliasing |
| Thread safety violations | Phase 1: Core DSP (if using expanders) | Stress test: rapid add/remove cycles, load large patches repeatedly |
| Font/image loading (DAW mode) | Phase 5: Library Submission Prep | DAW test: load as VST/AU, close/reopen editor window multiple times |

---

## Sources

### Official VCV Rack Documentation
- [VCV Rack Manual - Digital Signal Processing](https://vcvrack.com/manual/DSP)
- [VCV Rack Manual - Plugin API Guide](https://vcvrack.com/manual/PluginGuide)
- [VCV Rack Manual - Voltage Standards](https://vcvrack.com/manual/VoltageStandards)
- [VCV Rack Manual - Plugin Manifest](https://vcvrack.com/manual/Manifest)
- [VCV Rack Manual - Module Panel Guide](https://vcvrack.com/manual/Panel)
- [VCV Rack Manual - Polyphony](https://vcvrack.com/manual/Polyphony)
- [VCV Rack Manual - Building](https://vcvrack.com/manual/Building)
- [VCV Rack Manual - Migrating v1 to v2](https://vcvrack.com/manual/Migrate2)

### Community Discussions
- [Self-oscillating SVF Questions - VCV Community](https://community.vcvrack.com/t/self-oscillating-svf-questions/17896)
- [Polyphony reminders for plugin developers - VCV Community](https://community.vcvrack.com/t/polyphony-reminders-for-plugin-developers/9572)
- [Help wanted for reporting common issues - VCV Community](https://community.vcvrack.com/t/help-wanted-for-reporting-common-issues-to-rack-plugin-developers/11031)
- [Tricks for using CPU meters effectively - VCV Community](https://community.vcvrack.com/t/tricks-for-using-the-cpu-meters-effectively/13992)
- [Confusion about onAdd() and onReset() - VCV Community](https://community.vcvrack.com/t/confusion-about-onadd-and-onreset/11855)
- [Simple Expander example/tutorial - VCV Community](https://community.vcvrack.com/t/simple-expander-example-tutorial/17989)
- [svg text not showing - VCV Community](https://community.vcvrack.com/t/svg-text-not-showing/19987)

### DSP Resources
- [State Variable Filter (Double Sampled, Stable) - Musicdsp.org](https://www.musicdsp.org/en/latest/Filters/92-state-variable-filter-double-sampled-stable.html)
- [The digital state variable filter - EarLevel Engineering](http://www.earlevel.com/main/2003/03/02/the-digital-state-variable-filter/)
- [What makes SEM filter so special - KVR Audio](https://www.kvraudio.com/forum/viewtopic.php?t=497961)
- [How do you know what resonance levels blow up your filter? - KVR Audio](https://www.kvraudio.com/forum/viewtopic.php?t=504548)

### VCV Rack Ecosystem
- [VCV Library Repository](https://github.com/VCVRack/library)
- [Exploring Chorus and SEM filter features - Vult DSP](https://www.vult-dsp.com/post/exploring-the-new-chorus-and-sem-filter-features-in-the-freak-firmware-update)

---

*Pitfalls research for: VCV Rack Polyphonic Filter Module Development*
*Researched: 2026-01-29*
*Confidence: MEDIUM-HIGH (Official docs = HIGH, Community discussions = MEDIUM, DSP specifics = MEDIUM)*