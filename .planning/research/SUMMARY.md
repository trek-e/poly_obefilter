# Project Research Summary

**Project:** HydraQuartet VCF-OB (VCV Rack Module)
**Domain:** VCV Rack Polyphonic Filter Module Development
**Researched:** 2026-01-29
**Confidence:** HIGH

## Executive Summary

HydraQuartet VCF-OB is a polyphonic multimode filter module for VCV Rack 2.x, inspired by classic Oberheim SEM (12dB) and OB-X (24dB) filters. VCV Rack modules are built using the official VCV Rack SDK 2.6.6+ with C++11, following a well-established architecture pattern: Module class for DSP/state management, ModuleWidget for UI, and configuration in constructor using config*() methods. The recommended approach uses state-variable filter topology to provide simultaneous lowpass/highpass/bandpass/notch outputs, polyphonic processing with SIMD optimization (rack::simd::float_4), and standard VCV voltage conventions.

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
