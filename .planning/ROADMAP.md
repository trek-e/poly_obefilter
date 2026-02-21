# Roadmap: HydraQuartet VCF-OB

## Milestones

- ✅ **v0.50b Initial Beta** - Phases 1-7 (shipped 2026-02-03)
- 🚧 **v0.60b OB-X Filter** - Phases 8-9 (in progress)

## Phases

<details>
<summary>✅ v0.50b Initial Beta (Phases 1-7) - SHIPPED 2026-02-03</summary>

**Milestone Goal:** Polyphonic Oberheim-style multimode filter with SEM 12dB topology, four simultaneous outputs, and comprehensive CV control

### Phase 1: Project Foundation
**Goal**: VCV Rack plugin infrastructure with panel and build system
**Plans**: 2 plans

Plans:
- [x] 01-01: Plugin scaffold and build system
- [x] 01-02: Panel design and assets

### Phase 2: Core Filter DSP
**Goal**: SEM-style 12dB state-variable filter implementation
**Plans**: 2 plans

Plans:
- [x] 02-01: SVF implementation with trapezoidal integration
- [x] 02-02: Filter mode outputs (LP, HP, BP, Notch)

### Phase 3: Polyphony & I/O
**Goal**: 16-voice polyphonic processing
**Plans**: 1 plan

Plans:
- [x] 03-01: Polyphonic audio input/output routing

### Phase 4: Cutoff Control
**Goal**: Cutoff frequency parameter with CV control
**Plans**: 2 plans

Plans:
- [x] 04-01: Cutoff knob and CV input
- [x] 04-02: V/Oct tracking

### Phase 5: Resonance Control
**Goal**: Resonance parameter with self-oscillation
**Plans**: 1 plan

Plans:
- [x] 05-01: Resonance knob and CV input

### Phase 6: Drive & Saturation
**Goal**: Drive control with Oberheim character
**Plans**: 2 plans

Plans:
- [x] 06-01: Blended saturation algorithm
- [x] 06-02: Output-specific drive scaling

### Phase 7: Final Verification
**Goal**: Build verification and documentation
**Plans**: 1 plan

Plans:
- [x] 07-01: Build testing and README

</details>

### 🚧 v0.60b OB-X Filter (In Progress)

**Milestone Goal:** Add OB-X style 24dB/oct filter with type switching between 12dB SEM and 24dB OB-X modes

#### Phase 8: Core 24dB Cascade
**Goal**: Stable cascaded SVF filter with panel switch and correct frequency response
**Depends on**: Phase 7 (v0.50b shipped)
**Requirements**: FILT-06, FILT-07, TYPE-01, TYPE-02
**Success Criteria** (what must be TRUE):
  1. User can select between 12dB SEM and 24dB OB-X modes via panel switch
  2. 24dB mode produces steeper lowpass rolloff (-24dB/octave) than 12dB mode (-12dB/octave)
  3. Cutoff frequency parameter produces same cutoff frequency in both modes (1kHz stays 1kHz)
  4. Filter self-oscillates at maximum resonance in 24dB mode
  5. 24dB mode remains stable at all resonance settings without crashes or NaN
**Plans**: 1 plan

Plans:
- [ ] 08-01-PLAN.md — Cascaded SVF filter with panel switch, click-free switching, and stable self-oscillation

#### Phase 9: Character & Output
**Goal**: OB-X sonic character, lowpass-only output routing, and click-free mode switching
**Depends on**: Phase 8
**Requirements**: FILT-08, FILT-09, TYPE-03
**Success Criteria** (what must be TRUE):
  1. 24dB mode sounds bright/edgy compared to 12dB mode's warm/smooth character
  2. Only lowpass output is active in 24dB mode (HP/BP/Notch outputs silent, authentic OB-Xa)
  3. Switching between 12dB and 24dB modes produces no audible clicks or pops
  4. Drive control produces musical saturation in both 12dB and 24dB modes
**Plans**: TBD

Plans:
- [ ] 09-01: TBD

## Progress

**Execution Order:**
Phases execute in numeric order: 8 → 9

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 1. Project Foundation | v0.50b | 2/2 | Complete | 2026-01-30 |
| 2. Core Filter DSP | v0.50b | 2/2 | Complete | 2026-01-31 |
| 3. Polyphony & I/O | v0.50b | 1/1 | Complete | 2026-01-31 |
| 4. Cutoff Control | v0.50b | 2/2 | Complete | 2026-02-01 |
| 5. Resonance Control | v0.50b | 1/1 | Complete | 2026-02-03 |
| 6. Drive & Saturation | v0.50b | 2/2 | Complete | 2026-02-03 |
| 7. Final Verification | v0.50b | 1/1 | Complete | 2026-02-03 |
| 8. Core 24dB Cascade | 1/1 | Complete   | 2026-02-21 | - |
| 9. Character & Output | v0.60b | 0/? | Not started | - |
