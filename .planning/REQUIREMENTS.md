# Requirements: HydraQuartet VCF-OB

**Defined:** 2026-02-03
**Core Value:** The distinctive Oberheim filter sound in a polyphonic VCV Rack module

## v0.60b Requirements

Requirements for OB-X 24dB filter addition. Each maps to roadmap phases.

### Filter Core

- [ ] **FILT-06**: 24dB/oct OB-X style lowpass filter via cascaded SVF topology
- [ ] **FILT-07**: Self-oscillation at high resonance in 24dB mode
- [ ] **FILT-08**: OB-X character tuning (bright/edgy saturation distinct from SEM warm/smooth)
- [ ] **FILT-09**: Lowpass-only output active in 24dB mode (HP/BP/Notch silent, authentic OB-Xa)

### Type Switching

- [ ] **TYPE-01**: Panel switch for 12dB SEM / 24dB OB-X filter type selection
- [ ] **TYPE-02**: Cutoff frequency remains consistent between modes (1kHz stays 1kHz)
- [ ] **TYPE-03**: Click-free mode switching without pops or glitches

## Future Requirements

Deferred to later milestones. Tracked but not in current roadmap.

### v0.70b — CV Control

- **CTRL-06**: CV input for filter type switching
- **CTRL-07**: CV input for mode selection
- **CTRL-08**: FM input with attenuverter
- **CTRL-09**: 1V/Oct pitch tracking input

### v0.80b — Per-Voice Control

- **POLY-02**: Per-voice cutoff control option
- **POLY-03**: Per-voice resonance control option

### v0.90b — Polish

- **IO-02**: Mixed/selected output jack
- **STAB-02**: CPU optimization (SIMD for polyphony)

### v1.0 — Release

- **QUAL-01**: VCV Library submission requirements met
- **QUAL-02**: Cross-platform builds verified
- **QUAL-03**: Complete user documentation

## Out of Scope

Explicitly excluded. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| HP/BP/Notch outputs in 24dB mode | Authentic OB-Xa was LP-only; cascaded outputs would be confusing |
| True 4th-order state-variable | Cascaded 2-pole is simpler, more stable, close enough to OB-X |
| Per-stage parameter control | Confusing UX, not how original hardware worked |
| CV control of filter type | Deferred to v0.70b |
| SIMD optimization | Deferred to v0.90b |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| FILT-06 | Phase 8 | Pending |
| FILT-07 | Phase 8 | Pending |
| FILT-08 | Phase 9 | Pending |
| FILT-09 | Phase 9 | Pending |
| TYPE-01 | Phase 8 | Pending |
| TYPE-02 | Phase 8 | Pending |
| TYPE-03 | Phase 9 | Pending |

**Coverage:**
- v0.60b requirements: 7 total
- Mapped to phases: 7 (100%)
- Unmapped: 0

---
*Requirements defined: 2026-02-03*
*Last updated: 2026-02-03 after roadmap creation*
