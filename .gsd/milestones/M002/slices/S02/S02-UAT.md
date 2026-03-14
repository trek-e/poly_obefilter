# S02: FM Input and 1V/Oct Pitch Tracking — UAT

**Milestone:** M002
**Written:** 2026-03-13

## UAT Type

- UAT mode: live-runtime
- Why this mode is sufficient: FM audibility and pitch tracking accuracy can only be verified by ear in a running VCV Rack session. Build-time checks confirm wiring; listening confirms DSP behavior.

## Preconditions

- VCV Rack 2.x running on macOS
- CIPHER · OB module placed in a patch
- An audio output module connected to speakers/headphones
- A polyphonic oscillator (e.g., VCV VCO-2) available as FM source
- A keyboard/sequencer module (e.g., MIDI-CV or SEQ-3) available for V/Oct testing
- Module builds with zero warnings (`make -j4` clean)

## Smoke Test

Patch any oscillator into the FM CV input. Turn FM attenuverter knob clockwise past 12 o'clock. Feed audio into the filter's main input. If the filter's cutoff audibly wobbles at the oscillator's rate, FM is working.

## Test Cases

### 1. FM Modulation at Audio Rate

1. Patch a sine oscillator (e.g., 440 Hz) into the filter's audio input
2. Set cutoff to ~1kHz, resonance to ~50%
3. Patch a second oscillator (e.g., 100 Hz sine) into the FM CV input
4. Set the FM attenuverter knob to 12 o'clock (0 / center position)
5. **Expected:** No audible modulation — attenuverter at zero blocks FM
6. Turn the FM attenuverter clockwise toward +1
7. **Expected:** Filter cutoff modulates at 100 Hz — audible as a buzzy/growly timbral change. Modulation intensity increases as knob turns further clockwise.
8. Increase FM oscillator to 1kHz, then 5kHz
9. **Expected:** FM modulation remains audible at these rates — not killed by smoothing. Timbre changes character (metallic/clangorous at higher FM rates).

### 2. FM Attenuverter Bipolar Operation

1. Same patch as Test 1, FM oscillator at ~200 Hz, filter cutoff at ~2kHz
2. Sweep FM attenuverter slowly from full CCW (-1) through center (0) to full CW (+1)
3. **Expected:** At -1 and +1, FM depth is equal but modulation direction is inverted (pitch goes up where it went down). At center (0), modulation is silent. Transition through zero is smooth.

### 3. 1V/Oct Pitch Tracking

1. Patch a MIDI-CV or keyboard module's V/Oct output into both a VCO and the filter's V/Oct input
2. Set filter resonance high (~80-90%) so the resonant peak is clearly audible
3. Set cutoff knob to roughly match the VCO's base frequency
4. Play ascending notes: C2, C3, C4, C5
5. **Expected:** The filter's resonant peak tracks the VCO pitch — each octave up doubles the cutoff frequency. The resonant peak and VCO should stay roughly in tune across octaves.

### 4. V/Oct and Cutoff Knob Interaction

1. Same patch as Test 3 with resonance high
2. Play a held note (e.g., C3)
3. Sweep the cutoff knob up and down
4. **Expected:** Cutoff knob offsets the base frequency — the resonant peak moves up/down relative to the played note. V/Oct tracking remains accurate (each octave still doubles cutoff) regardless of knob position.

### 5. FM and V/Oct Simultaneous Operation

1. Patch V/Oct from a keyboard into the V/Oct input
2. Patch an LFO (slow, ~2 Hz) into the FM input, attenuverter at ~50%
3. Set resonance to ~60%, cutoff to ~1kHz
4. Play a melody on the keyboard
5. **Expected:** Filter tracks pitch from V/Oct while simultaneously wobbling from FM LFO. Both modulations are clearly audible and independent.

### 6. Polyphonic FM and V/Oct

1. Set up a polyphonic source (4+ voices) — e.g., MIDI-CV in poly mode
2. Patch poly V/Oct into filter's V/Oct input
3. Patch a mono LFO into FM input (will broadcast to all voices)
4. Play a chord (3-4 notes)
5. **Expected:** Each voice tracks its own pitch via V/Oct. FM modulation applies to all voices equally. No voice bleeding or cross-talk.
6. Now patch a polyphonic modulation source into FM input (different rate per voice)
7. **Expected:** Each voice gets its own FM modulation independently.

### 7. FM in 24dB OB-X Mode

1. Switch filter to 24dB mode (panel switch or filter type CV)
2. Repeat Test 1 (audio-rate FM)
3. **Expected:** FM modulation is equally audible in 24dB mode. Both cascade stages track FM identically — no pitch discrepancy between stages.

### 8. Disconnected Input Fast Path

1. Disconnect both FM and V/Oct inputs
2. Play audio through the filter normally
3. **Expected:** Filter behaves identically to before S02 — no change in sound, no CPU increase. The `fmOffsetVoct = 0.f` default means no multiplication overhead.

## Edge Cases

### Extreme FM Depth

1. Patch a fast oscillator (5kHz+) into FM input
2. Set attenuverter to maximum (+1)
3. **Expected:** Filter may produce extreme timbral results but should not crash, produce NaN, or go silent. The SVFilter's NaN/infinity reset catches instability.

### V/Oct with Extreme Cutoff

1. Set cutoff knob to maximum (20kHz)
2. Patch V/Oct and play high notes (C6, C7)
3. **Expected:** Cutoff will hit the normalization clamp (0.49 × Nyquist). Filter should remain stable — no oscillation artifacts or silence. Sound may become thin as cutoff exceeds audible range.

### Zero-Voltage V/Oct

1. Patch a cable carrying 0V into V/Oct input
2. **Expected:** No change from unpatched behavior — 0V means zero octave offset, so cutoff stays at knob position.

### FM with Filter Type Switching

1. Patch FM modulation (LFO or audio rate)
2. While FM is active, toggle filter type via CV or switch
3. **Expected:** FM continues working through the crossfade transition. No clicks or pops from the combination of FM modulation and type switching.

## Failure Signals

- FM modulation not audible at audio rates (>500 Hz) — likely FM is being killed by the cutoff smoother (post-smoothing bypass not working)
- V/Oct tracking inaccurate (octaves don't double cutoff) — likely V/Oct is applied as linear addition instead of logarithmic domain sum
- Module crashes or goes silent with FM patched — NaN/infinity propagation, check SVFilter reset logic
- CPU usage spikes when FM/V/Oct disconnected — hoisted `isConnected()` check not working, `pow(2, 0)` being computed unnecessarily
- FM sounds different between 12dB and 24dB modes — `fmOffsetVoct` not passed to one of the three `setParams()` call sites
- Polyphonic FM/V/Oct bleeds between voices — using `getVoltage()` instead of `getPolyVoltage(c)` in per-voice loop

## Requirements Proved By This UAT

- CTRL-08 — FM input with attenuverter: Tests 1, 2, 7 prove audio-rate FM with bipolar depth control
- CTRL-09 — 1V/Oct pitch tracking: Tests 3, 4 prove logarithmic pitch tracking
- CTRL-12 — FM bypasses smoothing: Test 1 (audio-rate FM audibility) proves smoothing is not killing high-frequency FM
- CTRL-13 — 1V/Oct logarithmic sum: Tests 3, 4 prove V/Oct sums correctly with cutoff in log domain

## Not Proven By This UAT

- Exact pitch tracking accuracy (measured in cents) — would require frequency analysis tools, not just ear verification
- CPU performance impact of FM/V/Oct processing — would require profiling, not listening
- Long-term stability under continuous extreme FM — short listening session may not catch slow divergence

## Notes for Tester

- For Test 3 (pitch tracking), getting the cutoff knob close to the VCO base frequency makes tracking easier to hear. If cutoff is way off, the resonant peak may be hard to distinguish from the VCO pitch.
- High resonance (80%+) makes pitch tracking much easier to verify — the resonant peak becomes a clear tone that should track the keyboard.
- FM sounds best with resonance around 40-70% — too high and self-oscillation competes with FM, too low and the effect is subtle.
- The filter's existing NaN reset will cause a brief silence if FM drives it into instability — this is working-as-designed, not a bug. It should recover within a few samples.
