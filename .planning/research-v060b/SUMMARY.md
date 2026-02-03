# Research Summary: v0.60b OB-X 24dB Filter

**Project:** HydraQuartet VCF-OB (VCV Rack polyphonic filter module)
**Milestone:** v0.60b - Add OB-X 24dB filter alongside existing 12dB SEM filter
**Domain:** Audio DSP - Cascaded State Variable Filter Implementation
**Researched:** 2026-02-03
**Confidence:** HIGH

## Executive Summary

The v0.60b milestone extends the existing 12dB SEM state-variable filter by adding an OB-X style 24dB/oct mode through **cascaded SVF architecture**. Research confirms this requires zero new dependencies—the implementation cascades two instances of the existing SVFilter class with a manual panel switch for filter type selection.

The recommended approach is straightforward: add a second SVFilter array, implement conditional processing in the process() method, and apply split resonance (0.7x per stage) to prevent instability. The 24dB mode should provide lowpass-only output matching authentic OB-X behavior, while the 12dB mode retains all multimode outputs (LP/HP/BP/Notch) unchanged. Total implementation estimate: 30-40 lines of C++ code additions.

Key risks center on **resonance instability** (cascaded high-Q filters compound feedback unpredictably), **Q distribution** (naive duplication destroys Butterworth response), and **character mismatch** (drive/saturation designed for 12dB produces wrong sound when applied twice). These are mitigated through calculated per-stage Q values, inter-stage gain compensation, and dedicated saturation tuning for 24dB mode.

## Key Findings

### Recommended Stack

**NO new dependencies required.** The 24dB filter is implemented by cascading two instances of the existing SVFilter class in series. All required infrastructure exists in v0.50b codebase.

**Minimal additions:**
- **Second filter array:** `SVFilter filters24dB_stage2[PORT_MAX_CHANNELS]` alongside existing `filters[PORT_MAX_CHANNELS]`
- **Filter type parameter:** configSwitch for 12dB/24dB toggle (VCV Rack standard pattern)
- **Cascade logic:** Conditional routing where stage1 LP output feeds stage2 input in 24dB mode
- **Resonance scaling:** Multiply stage2 resonance by ~0.7x to prevent instability

**Estimated code addition:** 30-40 lines in HydraQuartetVCF.cpp

**Integration points:**
- Reuse existing SVFilter.hpp (no modifications)
- Reuse existing blendedSaturation() (may need tuning for 24dB character)
- Reuse existing parameter smoothing (TExponentialFilter in SVFilter)
- Reuse existing polyphonic channel iteration patterns

**What NOT to add:**
- No OBXFilter.hpp class (keep cascade logic in Module process() method)
- No external DSP libraries (existing trapezoidal SVF is sufficient)
- No true 4-pole state-variable filter (cascaded 2-pole matches OB-X topology)
- No SIMD optimization yet (defer to v0.90b milestone)

### Expected Features

The research identified clear sonic distinctions between 12dB SEM and 24dB OB-X filter modes that inform feature requirements.

**Must have (table stakes):**
- 24dB/octave lowpass response with steeper cutoff slope than 12dB mode
- Self-oscillation capability at maximum resonance (authentic CEM3320 behavior)
- V/Oct tracking when self-oscillating (already implemented in v0.50b)
- Lowpass-only output in 24dB mode (matches OB-X hardware)
- Panel switch for 12dB ↔ 24dB selection (configSwitch parameter)
- Consistent cutoff frequency parameter across modes (1kHz = 1kHz)
- Distinctive "edgy" character vs. 12dB "smooth" character
- Consistent polyphonic behavior across 16 voices

**Should have (differentiators):**
- Frequency-compensated resonance amplitude (addresses CEM3320 issue where low frequencies oscillate quieter than high—use 3.4x gain buffer)
- Independent saturation tuning for 24dB vs. 12dB (OB-X used Curtis chips with different clipping than discrete SEM)
- Smooth mode switching without clicks/pops (preserve filter state, consider 10ms crossfade)

**Defer (v2+):**
- Q-compensation option (uncertain if OB-X had this; research needed)
- CV control of filter type (planned for v0.70b)
- Per-voice polyphonic type selection (planned for v0.80b)

**Anti-features (avoid):**
- NO multimode outputs in 24dB mode (OB-X was LP-only; multimode was separate 12dB mode)
- NO morphing/crossfading between filter types (Oberheim used discrete switching)
- NO 2x 12dB cascade with identical Q (different from true 4-pole topology)

**User expectations when switching:**
- 12dB → 24dB: Sound becomes "tighter," less high-frequency haze, resonance more focused, bass punchier, character shifts from warm/smooth to bright/edgy
- 24dB → 12dB: Sound "opens up," more air, resonance spreads wider, filter sweeps gentler, high frequencies more present

### Architecture Approach

The integration strategy leverages existing validated infrastructure with minimal structural changes.

**Signal flow for 24dB mode:**
```
Input (per voice)
  ↓
SVFilter Stage 1 (filters[c])
  ├─ LP output → Stage 2 input
  └─ (HP/BP/Notch unused in cascade)
  ↓
SVFilter Stage 2 (filters24dB_stage2[c])
  ├─ LP output → 24dB LP (primary output)
  └─ (HP/BP/Notch outputs available but LP-cascade hybrids)
  ↓
blendedSaturation (drive applied)
  ↓
Output ports
```

**Key architectural decisions:**

1. **Cascaded SVF approach (not true 4-pole):**
   - Historically accurate (OB-X uses cascaded 2-pole sections)
   - Reuses proven SVFilter implementation
   - Independent per-stage stability via trapezoidal integration
   - Simpler than 4-pole state-space math

2. **Split resonance distribution:**
   - Stage 1: resonance × 0.7
   - Stage 2: resonance × 0.7
   - Prevents instability from excessive cascaded feedback
   - Combined resonance feels similar to original (0.7 × 0.7 ≈ 0.49)

3. **Identical cutoff frequency:**
   - Both stages use same cutoffHz parameter
   - Ensures 24dB filter resonates at expected frequency
   - Prevents detuning or octave shifts

4. **State management:**
   - Separate state arrays: `filters[16]` (always stage 1) and `filters24dB_stage2[16]` (24dB stage 2)
   - Preserve state when switching modes (allows smooth transitions)
   - Brief transient acceptable; adds "musical" quality to mode changes

5. **Filter type parameter:**
   - configSwitch(FILTER_TYPE_PARAM, 0.f, 1.f, 0.f, "Filter Type", {"12dB SEM", "24dB OB-X"})
   - Read in process(): `int filterType = (int)params[FILTER_TYPE_PARAM].getValue();`
   - Conditional processing: if (filterType == 0) { 12dB path } else { 24dB cascade path }

**Build order (10-18 hours total):**
1. Core structure (2-4h): Add FILTER_TYPE_PARAM, second filter array, compile verification
2. Conditional logic (2-3h): Implement if/else for 12dB vs. 24dB, test both paths
3. Resonance tuning (2-4h): Split resonance, test stability at 0%/50%/100%, verify self-oscillation
4. Output routing (1-2h): Route stage2 outputs to ports, test LP/HP/BP/Notch behavior
5. UI integration (1-2h): Update panel SVG, add switch widget, test parameter switching
6. Verification (2-3h): Test polyphony, CV modulation, CPU usage, NaN protection

### Critical Pitfalls

Research identified 10 pitfalls specific to cascaded filter implementation. Top 5 critical issues:

1. **Naive Q Duplication Destroys Filter Response** (CRITICAL)
   - **Risk:** Cascading two identical Butterworth filters (Q=0.707) produces -6dB at cutoff instead of -3dB, wrong frequency response
   - **Prevention:** Calculate per-stage Q from Butterworth poles: stage1_Q ≈ 0.54, stage2_Q ≈ 1.31 (or use split resonance approach: both stages at 0.7x user resonance)
   - **Phase:** Phase 1 (core implementation)—must be correct from start

2. **Explosive Resonance Instability at High Q** (CRITICAL)
   - **Risk:** 24dB filters become unstable "inconsistently and unpredictably" with cascaded resonance feedback compounding exponentially
   - **Prevention:** Reduce per-stage Q at high resonance (0.7x multiplier), increase saturation in stage 2 feedback, add inter-stage gain limiting
   - **Detection:** Loud clicks at resonance >70%, output clamps to ±10V, abrupt self-oscillation instead of smooth
   - **Phase:** Phase 1 (core implementation)

3. **Inter-Stage Clipping from Gain Staging** (CRITICAL)
   - **Risk:** First stage produces >±12V at resonance, overdriving second stage before its own processing begins
   - **Prevention:** Order stages by Q (low-Q first), add inter-stage gain compensation, clamp stage1 output to safe range
   - **Detection:** Harsh digital clipping artifacts, asymmetric distortion, inconsistent behavior across resonance settings
   - **Phase:** Phase 1 (gain staging must be designed from start)

4. **NaN Propagation Across Stages** (CRITICAL)
   - **Risk:** First stage NaN bypasses SVFilter's NaN check and contaminates second stage before detection
   - **Prevention:** Check for NaN at inter-stage boundary (after stage 1, before stage 2), reset both stages simultaneously if either produces NaN
   - **Detection:** Silent output that doesn't recover, flat line at 0V on scope
   - **Phase:** Phase 1 (critical safety feature)

5. **Drive/Saturation Character Mismatch** (CHARACTER)
   - **Risk:** Saturation designed for 12dB produces harsh over-saturated sound when applied twice in 24dB cascade
   - **Prevention:** Redesign saturation for 24dB mode (lighter in stage 1, heavier in stage 2 or global feedback), A/B test against OB-X recordings
   - **Detection:** 24dB sounds "fizzy" or "brittle," self-oscillation harsh instead of smooth, drive above 50% unusable
   - **Phase:** Phase 2 (refinement after basic 24dB works)

**Other notable pitfalls:**
- Output mode interaction: 24dB BP/HP/Notch are LP-cascade hybrids, not true 24dB multimode (document this clearly)
- Resonance bandwidth mismatch: Q-to-bandwidth relationship differs between 2-pole and 4-pole (may need different parameter curve in 24dB)
- State reuse breaks polyphony: Requires separate state arrays or smooth crossfading on mode switch
- Parameter smoothing insufficient: 1ms tau may be inadequate for 24dB cascade under fast modulation (consider increasing to 2ms)
- CPU usage doubles: Expected 180-200% of 12dB baseline; acceptable for v0.60b, optimize in v0.90b with SIMD

## Implications for Roadmap

Research suggests a focused 3-phase implementation strategy for v0.60b, prioritizing stability and correctness before character refinement.

### Phase 1: Core 24dB Cascade (Foundation)
**Rationale:** Establish basic cascaded filter with correct frequency response and stable operation before addressing sonic character. This phase addresses all CRITICAL pitfalls.

**Delivers:**
- Functional 24dB/oct lowpass mode alongside existing 12dB mode
- Panel switch for filter type selection
- Stable operation at all resonance settings (no crashes, no NaN)
- Correct frequency response (proper Q distribution, gain staging)

**Addresses features:**
- 24dB/octave lowpass response (table stakes)
- Self-oscillation capability (table stakes)
- Panel switch for 12dB ↔ 24dB (table stakes)
- Consistent cutoff frequency across modes (table stakes)

**Avoids pitfalls:**
- #1 Naive Q Duplication (use 0.7x split resonance or calculated Butterworth poles)
- #2 Explosive Resonance (Q capping, test thoroughly at 100% resonance)
- #3 Inter-Stage Clipping (gain compensation, order stages by Q)
- #4 NaN Propagation (inter-stage NaN check, simultaneous reset)

**Implementation tasks:**
- Add FILTER_TYPE_PARAM enum and configSwitch
- Add `SVFilter filters24dB_stage2[PORT_MAX_CHANNELS]` array
- Implement conditional cascade logic in process()
- Apply split resonance (0.7x per stage)
- Add inter-stage NaN check and gain limiting
- Test frequency response with sweep, verify -3dB at cutoff
- Test resonance stability (100% resonance + noise input for 10 minutes)
- Test polyphonic behavior (16 voices)

**Duration:** 8-12 hours

**Research needed:** NO—all patterns are well-documented. Stack research provides clear implementation guidance.

---

### Phase 2: Character Refinement (Sonic Quality)
**Rationale:** Once cascade is stable, tune sonic character to match OB-X expectations. This separates correctness (Phase 1) from musicality (Phase 2).

**Delivers:**
- Authentic OB-X "edgy, bright" character vs. SEM "warm, smooth"
- Proper saturation behavior in 24dB mode
- Smooth mode switching without artifacts
- Musical resonance response across full range

**Addresses features:**
- Distinctive "edgy" character (table stakes)
- Independent saturation tuning for 24dB (differentiator)
- Smooth mode switching (differentiator)

**Avoids pitfalls:**
- #5 Drive/Saturation Character Mismatch (redesign saturation for 24dB)
- #7 Resonance Bandwidth Mismatch (tune Q curve for musical response)

**Implementation tasks:**
- A/B test against OB-X hardware/emulation (Arturia OB-Xa, U-He Repro)
- Tune saturation: lighter in stage 1, heavier in stage 2
- Test drive control across 0-100% range in both modes
- Adjust resonance parameter mapping if needed (different curve for 24dB)
- Add mode switching crossfade (10ms) if clicks/pops occur
- Document output behavior: "24dB mode: LP only; HP/BP/Notch are stage 1 outputs"

**Duration:** 6-10 hours

**Research needed:** MAYBE—if A/B testing reveals unexpected character issues, may need targeted research on CEM3320 saturation curves or OB-X Q distribution. Otherwise, use empirical tuning.

---

### Phase 3: Polish & Verification (Quality Assurance)
**Rationale:** Final verification, edge case handling, and documentation before milestone completion.

**Delivers:**
- Verified stable operation across all parameter combinations
- Acceptable CPU performance
- Complete testing coverage
- Documentation for users

**Addresses features:**
- Consistent polyphonic behavior (table stakes)
- Frequency-compensated resonance amplitude (differentiator, if time permits)

**Avoids pitfalls:**
- #9 Parameter Smoothing Insufficient (test fast modulation, increase tau if needed)
- #10 CPU Usage Doubles (profile and verify <7% CPU for 16 voices)

**Implementation tasks:**
- Run full test suite (frequency response, resonance stability, self-oscillation, mode switching, polyphony)
- Profile CPU usage with VCV Rack CPU meter (target: <7% for 16-voice 24dB)
- Test edge cases: max resonance + max drive + rapid cutoff modulation
- Long-term stability test: 1 hour at high resonance
- Add frequency-compensated resonance amplitude (3.4x gain buffer) if time permits
- Update module documentation with 24dB mode behavior

**Duration:** 4-6 hours

**Research needed:** NO—standard verification patterns. If CPU profiling reveals unexpected performance issues, defer SIMD optimization to v0.90b.

---

### Phase Ordering Rationale

**Why Phase 1 first:**
- All CHARACTER pitfalls (#5, #7) are meaningless if CRITICAL pitfalls (#1-4) cause crashes or wrong frequency response
- Proper Q distribution and gain staging must be designed into architecture from the start
- Easier to debug sonic character when cascade is already stable

**Why Phase 2 second:**
- Character tuning requires A/B testing against references, which is time-consuming
- Saturation redesign is independent of stability concerns
- Users will tolerate "slightly wrong sound" but not crashes or instability

**Why Phase 3 last:**
- Verification only makes sense once feature is functionally complete
- CPU optimization can be deferred to v0.90b if needed (SIMD milestone)
- Documentation requires stable behavior to document

**Dependency chain:**
```
Phase 1 (stable cascade)
  ↓
Phase 2 (character tuning) ← requires stable platform
  ↓
Phase 3 (verification) ← requires finalized behavior
```

### Research Flags

**Phase 1: Core 24dB Cascade**
- **Research needed:** NO
- **Confidence:** HIGH—cascaded SVF patterns are well-documented, VCV Rack parameter configuration is standard
- **Rationale:** Stack and Architecture research provide complete implementation guidance

**Phase 2: Character Refinement**
- **Research needed:** MAYBE (conditional)
- **Confidence:** MEDIUM—OB-X character is documented but Curtis chip saturation specifics are sparse
- **Trigger:** If A/B testing reveals character doesn't match references after initial saturation tuning
- **Topic:** CEM3320 clipping curves, OB-X per-stage Q distribution (Butterworth vs. custom)

**Phase 3: Polish & Verification**
- **Research needed:** NO
- **Confidence:** HIGH—standard VCV Rack testing patterns, CPU profiling is straightforward
- **Rationale:** If CPU issues arise, defer SIMD optimization to v0.90b (already planned)

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| **Stack** | HIGH | No new dependencies required; reuses existing SVFilter.hpp. Implementation approach is well-documented in cascaded filter literature (EarLevel Engineering, VCV Rack patterns). |
| **Features** | MEDIUM | Table stakes features are clear (24dB/oct, self-oscillation, panel switch). Differentiators (frequency compensation, independent saturation) need empirical testing. User expectations documented via forum discussions and OB-Xa reviews. |
| **Architecture** | HIGH | Cascaded SVF approach is proven (Oberheim OB-X hardware uses this topology). VCV Rack configSwitch and polyphony patterns are established. Split resonance strategy is documented for preventing instability. |
| **Pitfalls** | MEDIUM-HIGH | Critical pitfalls (#1-4) are verified via multiple authoritative sources (EarLevel, DSPRelated). Character pitfalls (#5, #7) are based on community knowledge and require validation during implementation. |

**Overall confidence:** HIGH

Research provides clear actionable guidance for all three phases. The only medium-confidence area is sonic character tuning, which is inherently empirical (requires ear-testing and A/B comparison).

### Gaps to Address

**Low-confidence areas identified during research:**

1. **Exact per-stage Q values for OB-X character:**
   - Research found Butterworth 4-pole requires stage1_Q ≈ 0.54, stage2_Q ≈ 1.31
   - BUT: OB-X may use custom Q distribution for its specific character
   - **Resolution:** Start with 0.7x split resonance (simpler), measure frequency response, compare to OB-X reference. Adjust if needed.

2. **Optimal saturation curve for 24dB mode:**
   - SEM used discrete op-amps, OB-X used Curtis CEM3320 (different clipping)
   - Research didn't find specific CEM3320 saturation curves
   - **Resolution:** Empirical tuning in Phase 2 via A/B testing. If character still wrong, research CEM3320 datasheet transfer function.

3. **Q-compensation presence in OB-X:**
   - Roland System-100 (1975) had Q-compensation; unclear if Oberheim adopted it
   - Affects whether to implement gain restoration at high resonance
   - **Resolution:** Test without Q-compensation first (authentic). Add if users report unexpected volume drop at high Q.

4. **Output mode behavior in 24dB:**
   - Research confirms OB-X 24dB was lowpass-only
   - BUT: Module could expose stage 1 HP/BP/Notch for flexibility
   - **Resolution:** Document decision clearly. Recommend: 24dB mode outputs only LP (stage 2), all other modes available in 12dB mode only.

5. **Frequency-compensated resonance amplitude gain value:**
   - Sequential Pro-One used 3.4x gain buffer
   - CEM3320 may differ from Pro-One's circuit
   - **Resolution:** Start with 3.4x (documented value), measure oscillation amplitude across frequency range, adjust if needed.

**None of these gaps block implementation.** All can be resolved through empirical testing during Phase 2 (character refinement).

## Sources

### Primary Sources (HIGH confidence)

**Stack & Architecture:**
- [Cascading Filters | EarLevel Engineering](https://www.earlevel.com/main/2016/09/29/cascading-filters/) — Q distribution, Butterworth cascade issues
- [Design IIR Filters Using Cascaded Biquads - DSP Related](https://www.dsprelated.com/showarticle/1137.php) — Gain staging, coefficient sensitivity
- [VCV Rack Plugin API Guide](https://vcvrack.com/manual/PluginGuide) — configSwitch documentation and parameter patterns
- [The Digital State Variable Filter | EarLevel Engineering](http://www.earlevel.com/main/2003/03/02/the-digital-state-variable-filter/) — Core SVF DSP algorithms
- Existing codebase: SVFilter.hpp (validated trapezoidal SVF implementation)

**Features & OB-X Characteristics:**
- [CEM3320 Filter Designs — Electric Druid](https://electricdruid.net/cem3320-filter-designs/) — Technical implementation details for CEM3320 configurations
- [Oberheim OB-X8 — Sound on Sound](https://www.soundonsound.com/reviews/oberheim-ob-x8) — Modern Oberheim with both filter types
- [State Variable Filters](https://sound-au.com/articles/state-variable.htm) — Analog and digital SVF theory

**Pitfalls:**
- [VCV Rack Manual - Digital Signal Processing](https://vcvrack.com/manual/DSP) — Filter stability, implementation best practices
- [Self-oscillating State Variable Filter - KVR Audio](https://www.kvraudio.com/forum/viewtopic.php?t=333894) — Resonance instability, saturation approaches

### Secondary Sources (MEDIUM confidence)

- [State Variable Filter -24dB version? - KVR Audio](https://www.kvraudio.com/forum/viewtopic.php?t=263202) — Cascading two SVFs, resonance challenges
- [Multimode Filters, Part 2: Pole-Mixing Filters – Electric Druid](https://electricdruid.net/multimode-filters-part-2-pole-mixing-filters/) — Multi-pole filter architectures
- [AMSynths AM8325 OBIE-XA VCF — Synthanatomy](https://synthanatomy.com/2021/12/amsynths-am8325-obie-xa-vcf-the-oberheim-ob-xa-filters-for-eurorack.html) — Eurorack implementation details
- [Oberheim OB-Xa filter? - MOD WIGGLER](https://modwiggler.com/forum/viewtopic.php?t=10709) — OB-Xa dual filter architecture discussion
- [A Guide to Synth Filter Types — Reverb News](https://reverb.com/news/a-guide-to-synth-filter-types-ladders-steiner-parkers-and-more) — Filter topology overview

### Tertiary Sources (LOW confidence, requires validation)

- [24db filters easier to drive to resonance - KVR Audio](https://www.kvraudio.com/forum/viewtopic.php?t=230856) — Resonance bandwidth differences (anecdotal)
- [OB-X filter details - MOD WIGGLER](https://www.modwiggler.com/forum/viewtopic.php?t=243282) — OB-X vs SEM character differences (community discussion)
- [SVF filter modulation zipper noise - KVR Audio](https://www.kvraudio.com/forum/viewtopic.php?t=511314) — Parameter smoothing for modulation (single source)

---

*Research completed: 2026-02-03*
*Ready for roadmap: YES*
*Estimated total implementation time: 18-28 hours across 3 phases*
