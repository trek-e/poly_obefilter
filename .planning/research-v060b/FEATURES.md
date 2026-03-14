# Features Research: v0.60b OB-X Filter

**Project:** CIPHER · OB
**Milestone:** v0.60b - Adding OB-X 24dB filter
**Researched:** 2026-02-03
**Overall confidence:** MEDIUM

## Executive Summary

The OB-X/OB-Xa 24dB filter represents a distinct sonic character from the already-implemented SEM 12dB state-variable filter. While both are Oberheim designs, they use different circuit topologies and exhibit different sonic behaviors. The 24dB filter uses a 4-pole cascade architecture (CEM3320 chip) providing steeper filtering and narrower resonance, contrasting with the 12dB state-variable's gentler slope and wider resonance band.

Users selecting "24dB" expect a more aggressive, focused filter character with steeper cutoff, better harmonic isolation at high resonance, and classic Oberheim "edge" — the brighter, more aggressive sound that defined the OB-Xa's identity versus the smoother SEM.

## OB-X 24dB Characteristics

### Circuit Topology

**Architecture:** 4-pole cascade lowpass using CEM3320 chip (or AS3320 clone)
- Four filter stages cascaded in sequence
- Standard configuration following CEM3320 datasheet design
- OTA (Operational Transconductance Amplifier) based
- Unlike the 12dB state-variable, this is a **single-mode lowpass only** configuration
- The OB-Xa also had a separate 12dB multimode mode using the same CEM3320 in state-variable configuration

**Key Technical Details:**
- 24dB/octave (4-pole) rolloff slope
- Voltage-controlled resonance from zero to oscillation
- Exponential (V/Oct) frequency control covering 10+ octaves
- Can self-oscillate at maximum resonance (unlike some Oberheim filters)
- Frequency tracking: ~18mV/octave for the CEM3320

### Sonic Character

**Sound Signature:**
- Edgy and bright character
- More aggressive filtering than the SEM 12dB
- Steeper cutoff creates more dramatic filter sweeps
- Narrower resonance bandwidth compared to 12dB filters
- Can "pick out" or "isolate" strong harmonics when swept with high resonance
- Classic association: "Moog bass, chords, brass stabs" territory

**What Makes It Sound Like OB-X:**
- Sharp, focused resonance peak (vs. the SEM's wider "purring" resonance)
- Aggressive character when pushed
- More precise timbral shaping
- The "bite" that characterized OB-Xa patches
- Better for tight, punchy sounds vs. smooth, warm tones

### Resonance Behavior

**Self-Oscillation:**
- The OB-Xa CEM3320 24dB filter CAN self-oscillate (important distinction)
- At max resonance, produces a sine-like tone
- When self-oscillating, should track V/Oct for use as an oscillator
- **Known issue:** Signal level when oscillating varies with frequency — much less at low frequencies than high
- **Compensation needed:** Output buffer gain (3.4x typical) for consistent oscillation amplitude across frequency range

**Resonance vs. 12dB:**
- Narrower bandwidth at high resonance (vs. 12dB's wider, "mushy but pleasant" sweep)
- More pronounced peak
- Can isolate individual harmonics more effectively
- May be louder/more dramatic depending on implementation

**Q-Compensation Consideration:**
- Filter resonance typically attenuates low-frequency content as Q increases
- Q-compensation amplifies pass-band to restore gain with increased resonance
- First implemented in Roland System-100 (1975), may or may not be present in OB-Xa
- Decision point: authentic (with volume drop at high Q) vs. compensated (even volume)

## Differences from SEM 12dB

### Architectural Differences

| Aspect | SEM 12dB (Current) | OB-X 24dB (New) |
|--------|-------------------|-----------------|
| **Topology** | State-variable (2-pole) | 4-pole cascade |
| **Slope** | 12dB/octave | 24dB/octave |
| **Outputs** | Simultaneous LP/HP/BP/Notch | Lowpass only |
| **Circuit** | Discrete components / integrator feedback | CEM3320 OTA cascade |
| **Pole count** | 2 poles | 4 poles |

### Sonic Differences

| Characteristic | SEM 12dB | OB-X 24dB |
|----------------|----------|-----------|
| **Overall sound** | Warm, smooth, "purring" | Bright, edgy, aggressive |
| **High-freq content** | More "fizz" and "air" retained | Tighter, more precise cutoff |
| **Resonance width** | Wider, "mushy but pleasant squelchy sweep" | Narrow, focused, can isolate harmonics |
| **Filter sweeps** | Gentle, gradual | Dramatic, pronounced |
| **Self-oscillation** | Yes (in our implementation) | Yes (authentic OB-Xa behavior) |
| **Character** | Musical, forgiving | Precise, surgical |

### Practical Sound Applications

**SEM 12dB is better for:**
- Quack lead synths
- Clav sounds
- High leads, plucky bass
- Pizzicato stabs
- Trombone, sync pads
- Sounds where brightness and "air" are desired

**OB-X 24dB is better for:**
- Moog-style bass (tight, punchy)
- Brass stabs, brass chords
- Tubas, trumpets
- Cluck leads, goose leads
- Organ sounds
- Dramatic filter sweeps with high resonance

### User Perception When Switching

**What users expect when switching from 12dB to 24dB:**
1. More aggressive filtering action
2. Tighter, more controlled sound
3. Less high-frequency "haze" above cutoff
4. Narrower resonance peak
5. More dramatic parameter response
6. Better bass tightness
7. Different harmonic content at high resonance

**Important:** Real-world users report the differences can be "mostly subtle" on actual hardware like the OB-X8 (which offers both filter types). The theoretical differences are clear, but in musical context, the distinction may be less dramatic than raw specs suggest.

## Table Stakes

Features users MUST have for authentic OB-X 24dB character.

### 1. 24dB/octave Lowpass Response
**Why expected:** This is the defining characteristic — 4-pole cascade topology
**Complexity:** Medium
**Notes:** Must implement true 4-pole cascade, not just doubled 2-pole

### 2. Steeper Cutoff Slope Than 12dB
**Why expected:** Users select 24dB specifically for tighter filtering
**Complexity:** Low (inherent in 4-pole design)
**Notes:** Should be audibly different from 12dB when A/B testing

### 3. Self-Oscillation Capability
**Why expected:** Authentic OB-Xa behavior, unlike some Oberheim filters
**Complexity:** Medium
**Notes:** CEM3320 does self-oscillate; this is table stakes for the 24dB mode

### 4. V/Oct Tracking When Self-Oscillating
**Why expected:** When used as an audio-rate sine oscillator
**Complexity:** Medium
**Notes:** Standard for self-oscillating filters in modular context

### 5. Lowpass-Only Output
**Why expected:** Authentic OB-Xa 24dB mode was lowpass-only (separate 12dB for multimode)
**Complexity:** Low
**Notes:** Unlike the state-variable which outputs all modes simultaneously

### 6. Resonance Range From Zero to Oscillation
**Why expected:** Voltage-controlled resonance is core to the CEM3320 design
**Complexity:** Low
**Notes:** Should cover the full range 0 to self-oscillation threshold

### 7. Distinctive "Edgy" Character
**Why expected:** OB-Xa's defining sonic signature vs. SEM smoothness
**Complexity:** Medium
**Notes:** Achieved through topology choice and possibly saturation tuning

### 8. Consistent Behavior Across Polyphonic Voices
**Why expected:** Already implemented for 12dB, users expect same for 24dB
**Complexity:** Low (architecture already supports this)
**Notes:** Each of 16 voices needs independent filter state

## Differentiators

Features that make a GREAT implementation, beyond authenticity.

### 1. Frequency-Compensated Resonance Amplitude
**Value proposition:** Consistent oscillation volume across frequency range
**Complexity:** Medium
**Notes:** Addresses known CEM3320 issue — low frequencies oscillate quieter than high
**Implementation:** Output buffer with ~3.4x gain as documented for Pro-One fix

### 2. Smooth Switching Between 12dB and 24dB Modes
**Value proposition:** No pops, clicks, or discontinuities when switching filter types
**Complexity:** Medium
**Notes:** Critical for live performance and modulation use cases
**Implementation:** May need crossfading or state interpolation

### 3. Independent Drive/Saturation Tuning for 24dB
**Value proposition:** Authentic OB-X character requires different saturation than SEM
**Complexity:** Low
**Notes:** 24dB may want more/less/different saturation curve than 12dB

### 4. Q-Compensation Option
**Value proposition:** Lets users choose authentic (volume drop at high Q) vs. compensated (even volume)
**Complexity:** Medium
**Notes:** Could be a panel switch or always-on design decision
**Modern preference:** Many modern implementations use Q-compensation for consistent loudness

### 5. Accurate CEM3320 Frequency Response Curve
**Value proposition:** Matches the specific resonance peak shape and rolloff of the original chip
**Complexity:** High
**Notes:** Requires careful DSP modeling or reference to CEM3320 datasheet frequency response
**Tradeoff:** Perfect accuracy vs. CPU efficiency

### 6. Polyphonic V/Oct Tracking Input
**Value proposition:** Each voice's filter can track its pitch independently
**Complexity:** Low (planned for v0.70b already)
**Notes:** Essential for keyboard tracking, already on roadmap

## Anti-Features

Things that BREAK the OB-X character and should be avoided.

### 1. Multimode Outputs in 24dB Mode
**Why bad:** OB-Xa's 24dB filter was lowpass-only; multimode was a separate 12dB mode
**Consequence:** Breaks authenticity, confuses the topology distinction
**Instead:** Keep 24dB as lowpass-only, use 12dB mode for HP/BP/Notch

### 2. Morphing/Crossfading Between Filter Types
**Why bad:** Oberheim used discrete switching, not continuous morphing
**Consequence:** CPU cost, design complexity, not authentic to hardware
**Instead:** Discrete switch matches original behavior, simpler implementation

### 3. Soft/Gentle Resonance Tuning
**Why bad:** OB-X is supposed to be "edgy and bright," not smooth
**Consequence:** Loses the defining character that distinguishes it from SEM
**Instead:** Keep resonance narrow and aggressive

### 4. Using 2x 12dB Cascade Instead of True 4-Pole
**Why bad:** Different frequency response curve, different resonance behavior
**Consequence:** Won't sound like OB-Xa, just sounds like doubled SEM
**Instead:** Implement proper 4-pole cascade topology

### 5. Making Both Filters Sound Too Similar
**Why bad:** Defeats the purpose of offering two filter types
**Consequence:** Users wonder why they bothered switching
**Instead:** Emphasize the sonic differences — bright vs. warm, narrow vs. wide resonance

### 6. No Self-Oscillation in 24dB Mode
**Why bad:** CEM3320 self-oscillates; this is expected behavior
**Consequence:** Limits musical possibilities, breaks expectations
**Instead:** Ensure full resonance range to oscillation

### 7. Inconsistent Filter Cutoff When Switching Types
**Why bad:** When switching 12dB ↔ 24dB, cutoff should stay musically equivalent
**Consequence:** Jarring jumps in frequency, breaks musical continuity
**Instead:** Map cutoff parameter so 1kHz stays 1kHz across both filter types

## Implementation Considerations for VCV Rack

### Switching Mechanism

**Panel Switch (v0.60b):**
- Manual switch between 12dB and 24dB modes
- Should be clear, immediate feedback
- Consider LED or visual indicator of active mode

**CV Control (v0.70b planned):**
- CV input for programmatic switching
- Decision: gates/triggers vs. voltage levels vs. polyphonic per-voice
- Smooth switching without clicks critical for modulation

### Output Strategy

**Option A: Reconfigure Outputs Based on Mode**
- 12dB mode: LP/HP/BP/Notch all active (current behavior)
- 24dB mode: Only LP output active, others silent or disconnected
- Pro: Authentic to hardware
- Con: May confuse users patching multiple outputs

**Option B: Always Provide LP Output**
- LP output works in both modes
- HP/BP/Notch only work in 12dB mode
- Pro: Clearer behavior
- Con: Same as Option A really

**Option C: Separate Output Jacks**
- "12dB Out" and "24dB Out" jacks
- Pro: No ambiguity
- Con: Panel space, more complex routing

**Recommendation:** Option A — reconfigure based on mode. Most authentic, matches user expectations.

### CPU Considerations

**12dB State-Variable (current):** ~3 integrators + feedback
**24dB Cascade:** ~4 cascaded stages

Both are similar computational cost. The 24dB shouldn't significantly impact CPU budget vs. current implementation.

### Polyphony Handling

Already solved for 12dB mode: 16 independent filter states per voice.
Same architecture extends to 24dB mode naturally.

## User Experience When Switching

### Expected Behavior

**When switching 12dB → 24dB at same cutoff/resonance:**
1. Sound becomes "tighter" — less high-frequency haze
2. Resonance peak becomes more focused, narrower
3. Filter sweeps become more dramatic
4. Bass becomes punchier, more defined
5. Overall character shifts from warm/smooth to bright/edgy

**When switching 24dB → 12dB at same cutoff/resonance:**
1. Sound "opens up" — more air and space
2. Resonance spreads wider, less surgical
3. Filter sweeps become gentler, more gradual
4. High frequencies remain more present above cutoff
5. Character shifts from precise/aggressive to musical/forgiving

### Parameter Mapping Consistency

Critical for good UX: cutoff and resonance parameters should "feel" musically equivalent across modes.

**Cutoff frequency:**
- 1kHz on 12dB mode should equal 1kHz on 24dB mode (same fc)
- Already handled by V/Oct CV input

**Resonance:**
- More challenging — 12dB Q of 5 doesn't equal 24dB Q of 5 perceptually
- May need to map resonance knob differently per mode for musical equivalence
- Or keep technically consistent (same Q) and let sonic differences emerge naturally

**Recommendation:** Keep cutoff frequency consistent (technical), consider perceptual mapping for resonance (musical).

## Feature Dependencies

```
24dB Filter Implementation
  ↓
  ├→ Lowpass-only output mode (table stakes)
  ├→ Self-oscillation (table stakes)
  ├→ 4-pole cascade topology (table stakes)
  │   ↓
  │   └→ Frequency-compensated resonance amplitude (differentiator)
  │
  ├→ Filter type switching (table stakes)
  │   ↓
  │   ├→ Panel switch (v0.60b)
  │   └→ CV input (v0.70b, depends on panel switch working)
  │
  └→ Independent saturation tuning (differentiator)
```

### Minimal Viable Implementation (v0.60b)

1. 4-pole cascade lowpass topology
2. Panel switch: 12dB ↔ 24dB
3. Self-oscillation capability
4. Consistent cutoff/resonance parameters
5. Output jack behavior: LP-only in 24dB mode

### Enhanced Implementation (future)

1. Frequency-compensated resonance amplitude
2. Q-compensation option
3. CV control of filter type
4. Polyphonic per-voice type selection
5. Independent drive/saturation per mode

## Open Questions & Research Gaps

### LOW Confidence Areas

**Resonance amplitude compensation:**
- Exact gain value for OB-Xa (Pro-One used 3.4x, but was that the same?)
- Whether OB-Xa originally had this or if it's a modern enhancement
- Whether to include it by default or make it optional

**Q-compensation in original:**
- Did the OB-Xa have Q-compensation or not?
- Sources suggest Roland System-100 (1975) had it first, but unclear if Oberheim adopted it
- If not present in original, should we add it for modern usability?

**Exact saturation characteristics:**
- How did the OB-Xa's saturation differ from the SEM?
- What was the saturation curve shape in the CEM3320?
- Should 24dB mode have more or less drive than 12dB?

### Areas Needing Phase-Specific Research

**Phase: Filter type switching implementation**
- Best practices for mode switching without audio artifacts
- Whether to use crossfading, zero-crossing detection, or instant switching
- How other VCV modules handle similar topology switches

**Phase: CV control of filter type**
- User expectations for voltage-controlled filter switching
- Whether to support per-voice polyphonic switching
- Performance implications of dynamic switching at audio rate

## Confidence Assessment

| Area | Confidence | Reason |
|------|------------|--------|
| **Basic 24dB characteristics** | HIGH | Well-documented CEM3320 behavior, multiple authoritative sources |
| **Sonic differences vs. 12dB** | MEDIUM | Verified by multiple sources, but "subtle" according to users |
| **Circuit topology** | HIGH | CEM3320 datasheet available, implementations documented |
| **Self-oscillation** | HIGH | Explicitly stated in multiple sources, CEM3320 spec |
| **Resonance amplitude issue** | MEDIUM | Pro-One fix documented, unclear if OB-Xa had same issue |
| **Q-compensation** | LOW | Conflicting/absent information on original OB-Xa implementation |
| **Saturation characteristics** | LOW | No specific documentation found on OB-Xa saturation curve |
| **User expectations** | MEDIUM | Forum discussions and user comments, not official sources |

## MVP Recommendation for v0.60b

Based on research, the MVP 24dB filter should include:

### Must-Have (Table Stakes)
1. True 4-pole cascade lowpass topology
2. 24dB/octave rolloff slope
3. Self-oscillation at max resonance
4. V/Oct tracking (cutoff frequency already implemented)
5. Lowpass-only output in 24dB mode
6. Panel switch for 12dB ↔ 24dB selection
7. Consistent cutoff frequency parameter across modes

### Should-Have (Differentiators)
1. Frequency-compensated resonance amplitude (use 3.4x gain buffer approach)
2. Independent saturation tuning for 24dB vs. 12dB
3. Smooth mode switching (minimize clicks/pops)

### Nice-to-Have (Defer to Later)
1. Q-compensation (decide: always on, always off, or switchable)
2. CV control of filter type (planned for v0.70b anyway)
3. Per-voice polyphonic type selection (planned for v0.80b)

### Implementation Priority

1. **Core DSP:** 4-pole cascade algorithm with proper frequency response
2. **Mode switching:** Panel switch that reconfigures topology
3. **Output routing:** LP-only in 24dB mode, all modes in 12dB mode
4. **Self-oscillation:** Ensure resonance range goes to full oscillation
5. **Resonance compensation:** Add output gain to even out oscillation amplitude
6. **Character tuning:** Adjust saturation to differentiate from 12dB

## Sources

### HIGH Confidence (Authoritative)
- [CEM3320 Filter designs — Electric Druid](https://electricdruid.net/cem3320-filter-designs/) — Technical implementation details for CEM3320 configurations
- [State Variable Filters — Sound-AU](https://sound-au.com/articles/state-variable.htm) — State variable filter architecture and behavior
- [Oberheim OB-X8 — Sound on Sound](https://www.soundonsound.com/reviews/oberheim-ob-x8) — Modern Oberheim with both filter types
- [Electronics Tutorials — State Variable Filter](https://www.electronics-tutorials.ws/filter/state-variable-filter.html) — SVF technical characteristics

### MEDIUM Confidence (Verified Community)
- [AMSynths AM8325 OBIE-XA VCF — Synthanatomy](https://synthanatomy.com/2021/12/amsynths-am8325-obie-xa-vcf-the-oberheim-ob-xa-filters-for-eurorack.html) — Eurorack implementation details
- [OB-Xa filter discussion — MOD WIGGLER](https://modwiggler.com/forum/viewtopic.php?t=10709) — Technical forum discussion
- [12dB vs 24dB filter comparison — Vintage Synth Explorer](https://forum.vintagesynth.com/viewtopic.php?t=69988) — User experiences with OB-X filters
- [A Guide to Synth Filter Types — Reverb News](https://reverb.com/news/a-guide-to-synth-filter-types-ladders-steiner-parkers-and-more) — Filter topology overview
- [Learning Synthesis: Filters — Perfect Circuit](https://www.perfectcircuit.com/signal/learning-synthesis-filters) — Filter behavior and applications

### MEDIUM Confidence (Community Knowledge)
- [12dB/oct or 24dB/oct filter: practical uses? — Vintage Synth Explorer](https://forum.vintagesynth.com/viewtopic.php?t=103067) — Practical sound design applications
- [12db per octave vs 24db per octave — KVR Audio](https://www.kvraudio.com/forum/viewtopic.php?t=417273) — Sonic differences discussion
- [SEM sound discussion — Gearspace](https://gearspace.com/board/electronic-music-instruments-and-electronic-music-production/1219528-sem-sound.html) — SEM vs. OB-Xa character
- [Oberheim vs. Oberheim — KVR Audio](https://www.kvraudio.com/forum/viewtopic.php?t=549407) — Filter comparisons across Oberheim models
- [A Beginners Guide to Synthesizer Filters — Molten Music](https://moltenmusictechnology.com/a-beginners-guide-to-synthesizer-filters/) — Filter slope explanations

### LOW Confidence (Unverified Single Source)
- Various forum posts on self-oscillation behavior
- User reports on "subtle differences" between modes
- Q-compensation implementation timing (Roland System-100 reference)

---

**Research complete.** Ready for roadmap creation and DSP implementation planning.
