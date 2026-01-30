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
