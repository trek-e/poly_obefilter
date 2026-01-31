# Roadmap: HydraQuartet VCF-OB

## Overview

This roadmap delivers the v0.50b initial beta release of HydraQuartet VCF-OB, a polyphonic Oberheim-style filter module for VCV Rack 2.x. The journey starts with project foundation and builds incrementally through DSP implementation, polyphonic extension, and control integration, culminating in a fully functional 8-voice multimode filter with comprehensive CV control.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [x] **Phase 1: Foundation** - Plugin scaffold, panel design, build infrastructure
- [x] **Phase 2: Core Filter DSP** - SEM-style 12dB state-variable filter with lowpass mode
- [x] **Phase 3: Filter Modes** - Add highpass, bandpass, and notch outputs
- [x] **Phase 4: Polyphonic Extension** - 8-voice polyphonic audio processing
- [x] **Phase 5: Cutoff Control** - Cutoff frequency parameter with CV input and attenuverter
- [ ] **Phase 6: Resonance Control** - Resonance parameter with CV input
- [ ] **Phase 7: Drive Control** - Drive/saturation for filter character

## Phase Details

### Phase 1: Foundation
**Goal**: Establish plugin infrastructure and panel design
**Depends on**: Nothing (first phase)
**Requirements**: FOUND-01, FOUND-02, FOUND-03
**Success Criteria** (what must be TRUE):
  1. Plugin compiles and loads in VCV Rack without errors
  2. Module appears in library browser with correct branding
  3. Panel renders with all control positions marked
  4. README contains build instructions and usage documentation
**Plans**: 1 plan

Plans:
- [x] 01-01-PLAN.md — Plugin infrastructure with panel design, module code, build system, and documentation

### Phase 2: Core Filter DSP
**Goal**: Working SEM-style 12dB state-variable filter with lowpass output
**Depends on**: Phase 1
**Requirements**: FILT-01, FILT-02
**Success Criteria** (what must be TRUE):
  1. Audio passes through module with lowpass filtering applied
  2. Filter processes input signal and produces audible lowpass output
  3. Filter remains stable (no blow-up, NaN, or crashes)
  4. Self-oscillation produces audible tone at high resonance
**Plans**: 1 plan

Plans:
- [x] 02-01-PLAN.md — Trapezoidal SVF implementation with lowpass output and cutoff/resonance controls

### Phase 3: Filter Modes
**Goal**: Complete multimode filter with all four outputs
**Depends on**: Phase 2
**Requirements**: FILT-03, FILT-04, FILT-05
**Success Criteria** (what must be TRUE):
  1. Highpass output produces audible high-frequency content
  2. Bandpass output produces audible band-limited signal
  3. Notch output produces audible notch-filtered signal
  4. All four outputs work simultaneously from same input
**Plans**: 1 plan

Plans:
- [x] 03-01-PLAN.md — Expose HP, BP, and Notch outputs from SVF topology

### Phase 4: Polyphonic Extension
**Goal**: 16-voice polyphonic audio processing with per-voice CV modulation
**Depends on**: Phase 3
**Requirements**: POLY-01, IO-01
**Success Criteria** (what must be TRUE):
  1. Module accepts polyphonic cable input (1-16 channels)
  2. Each voice processes independently with separate filter state
  3. Filter remains stable (no blow-up, NaN, or crashes)
  4. Polyphonic patch produces distinct filtered voices
**Plans**: 1 plan

Plans:
- [x] 04-01-PLAN.md — Transform module to polyphonic with filter array and per-voice CV

### Phase 5: Cutoff Control
**Goal**: Cutoff frequency control with CV modulation
**Depends on**: Phase 4
**Requirements**: CTRL-01, CTRL-02
**Success Criteria** (what must be TRUE):
  1. Cutoff knob sweeps filter frequency across audible range
  2. CV input modulates cutoff frequency
  3. Attenuverter scales CV input range (positive and negative)
  4. Cutoff control responds smoothly without zipper noise
**Plans**: 1 plan

Plans:
- [x] 05-01-PLAN.md — Refine cutoff default to fully open with verified V/Oct CV modulation

### Phase 6: Resonance Control
**Goal**: Resonance control with CV modulation
**Depends on**: Phase 5
**Requirements**: CTRL-03, CTRL-04
**Success Criteria** (what must be TRUE):
  1. Resonance knob increases filter emphasis from subtle to self-oscillation
  2. CV input modulates resonance amount
  3. Filter remains stable at all resonance settings
  4. Resonance control responds smoothly without zipper noise
**Plans**: TBD

Plans:
- [ ] 06-01: TBD

### Phase 7: Drive Control
**Goal**: Drive/saturation for filter character
**Depends on**: Phase 6
**Requirements**: CTRL-05
**Success Criteria** (what must be TRUE):
  1. Drive knob adds harmonic saturation to filtered signal
  2. Drive control affects tone character without instability
  3. Full drive range produces musical saturation (not harsh clipping)
**Plans**: TBD

Plans:
- [ ] 07-01: TBD

## Progress

**Execution Order:**
Phases execute in numeric order: 1 → 2 → 3 → 4 → 5 → 6 → 7

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Foundation | 1/1 | Complete | 2026-01-30 |
| 2. Core Filter DSP | 1/1 | Complete | 2026-01-31 |
| 3. Filter Modes | 1/1 | Complete | 2026-01-31 |
| 4. Polyphonic Extension | 1/1 | Complete | 2026-01-31 |
| 5. Cutoff Control | 1/1 | Complete | 2026-01-31 |
| 6. Resonance Control | 0/1 | Not started | - |
| 7. Drive Control | 0/1 | Not started | - |

---
*Roadmap created: 2026-01-29*
*Milestone: v0.50b (initial beta)*
