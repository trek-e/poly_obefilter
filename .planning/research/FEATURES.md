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
