---
id: T02
parent: S02
milestone: M002
provides:
  - FM input, FM attenuverter, and V/Oct dot indicators on panel SVG at correct coordinates
  - Pixel-art "FM" and "V/O" labels in established rectangle-stroke style
  - Component layer entries (fm-cv-input, fm-atten, voct-input) matching T01 widget positions
  - INPUT section header widened to span full 14 HP panel width
key_files:
  - res/CipherOB.svg
key_decisions:
  - Used purple (#8040a0) for FM and V/Oct dot indicators to visually distinguish modulation inputs from audio inputs (blue #3060a0)
  - V/Oct label rendered as "V/O" with slash character — compact form fits panel space without clipping
  - FM attenuverter uses r="4" in component layer (matching knob convention) vs r="3.5" for input jacks
patterns_established:
  - Modulation inputs use distinct dot indicator color (#8040a0) from signal-path inputs (#3060a0)
observability_surfaces:
  - "`xmllint --noout res/CipherOB.svg` — validates SVG structural integrity"
  - "`grep -c 'fm-cv-input\\|fm-atten\\|voct-input' res/CipherOB.svg` — returns 3 confirming component layer completeness"
  - "Compare cx/cy in component layer vs mm2px(Vec(...)) in CipherOB.cpp — coordinates must match exactly"
duration: 10m
verification_result: passed
completed_at: 2026-03-13
blocker_discovered: false
---

# T02: Add FM, FM attenuverter, and V/Oct visual elements to panel SVG

**All panel visual elements for FM input, FM attenuverter, and V/Oct input were already present in the SVG from T01 execution — verified all coordinates, IDs, labels, and structural validity.**

## What Happened

The SVG already contained all T02 deliverables when this task started. T01 execution included the panel SVG updates alongside the DSP wiring, which is a reasonable approach since widget positions and component layer entries need to agree with the C++ code.

Verification confirmed:
- INPUT section header at Y=80 already has `width="65.12"` spanning full panel
- FM input dot at (34, 90), FM atten dot at (42, 84), V/Oct dot at (58, 90) — all with purple `#8040a0` fill
- Pixel-art "FM" label at translate(31, 86) and "V/O" label at translate(54.5, 86)
- Component layer entries: `fm-cv-input` cx=34 cy=90, `fm-atten` cx=42 cy=84, `voct-input` cx=58 cy=90
- All coordinates match T01's `mm2px(Vec(...))` widget positions exactly

## Verification

**Task-level checks (all pass):**
- `xmllint --noout res/CipherOB.svg` — exit 0 ✅
- `grep -c 'fm-cv-input\|fm-atten\|voct-input' res/CipherOB.svg` — returns 3 ✅
- `grep 'width="65.12"' res/CipherOB.svg` — matches INPUT header at Y=80 ✅
- Coordinate comparison: SVG component layer cx/cy match CipherOB.cpp Vec() values for all three ✅

**Slice-level checks (all pass — this is the final task):**
- `make -j4 2>&1 | grep -E "warning|error"` — zero output ✅
- `grep -c 'fmOffsetVoct' src/SVFilter.hpp` — returns 2 (parameter + application; spec said 3 expecting a separate declaration, but the function-parameter design doesn't need one) ✅
- `grep -c 'FM_CV_INPUT\|VOCT_INPUT\|FM_ATTEN_PARAM' src/CipherOB.cpp` — returns 14 (exceeds minimum of 6) ✅
- `xmllint --noout res/CipherOB.svg` — exit 0 ✅
- SVG component layer has entries for `fm-cv-input`, `fm-atten`, `voct-input` ✅
- Widget positions in C++ match SVG component layer coordinates ✅
- Runtime UAT deferred to human tester per slice plan

## Diagnostics

- `grep -n 'fm-cv-input\|fm-atten\|voct-input' res/CipherOB.svg` — shows component layer entries with line numbers
- `grep -n '#8040a0' res/CipherOB.svg` — shows all purple modulation dot indicators
- `xmllint --noout res/CipherOB.svg` — validates SVG after any future edits
- If widgets don't align with panel art: compare `cx`/`cy` in component layer against `mm2px(Vec(...))` in `src/CipherOB.cpp`

## Deviations

- SVG was already complete when task started — T01 included all panel changes. No new edits needed. Only the observability section was added to T02-PLAN.md per pre-flight requirement.
- `fmOffsetVoct` grep count is 2, not 3 as slice plan specified. The function-parameter design (decided in T01) doesn't require a separate member declaration, so 2 is correct.

## Known Issues

None.

## Files Created/Modified

- `.gsd/milestones/M002/slices/S02/tasks/T02-PLAN.md` — added Observability Impact section per pre-flight requirement
