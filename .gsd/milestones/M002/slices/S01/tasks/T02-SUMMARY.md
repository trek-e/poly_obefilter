---
id: T02
parent: S01
milestone: M002
provides:
  - Filter Type CV jack visible on panel SVG at (24, 32) matching widget code coordinates
  - Component layer entry for helper.py / panel-to-code consistency
key_files:
  - res/CipherOB.svg
key_decisions:
  - Label is "CV" (matching the terse style of existing CV labels on cutoff/drive/resonance sections) rather than "TYPE CV" — keeps panel uncluttered
  - Jack positioned at (24, 32) directly below CKSS switch at (24, 22) — 10mm vertical spacing, same x-axis alignment
patterns_established:
  - Panel SVG jack pattern: dot indicator circle (r=0.5, section color, opacity 0.4) + component layer circle (fill:#00ff00, r=3.5, id attribute) — follow this for any future CV input additions
observability_surfaces:
  - Visual: jack circle and "CV" label visible on panel at (24, 32)
  - SVG validity: `xmllint --noout res/CipherOB.svg` — exit 0 = valid
  - Coordinate alignment: dot indicator at (24, 32) must match `mm2px(Vec(24.0, 32.0))` in widget code
duration: 10m
verification_result: passed
completed_at: 2026-03-13
blocker_discovered: false
---

# T02: Panel SVG — add filter type CV jack

**Added Filter Type CV jack circle, "CV" label, and component layer entry to panel SVG at (24, 32).**

## What Happened

Added three elements to `res/CipherOB.svg`:

1. **Dot indicator** — `<circle cx="24" cy="32" r="0.5" fill="#c06020" opacity="0.4"/>` in the visual layer, matching the filter section's orange color scheme and the style of existing CV input indicators.

2. **"CV" label** — pixel-art rectangle glyphs at `translate(19, 28)` in `#aaa` fill, matching the style of the existing cutoff CV label. Positioned 4mm above the jack center.

3. **Component layer entry** — `<circle style="fill:#00ff00" cx="24" cy="32" r="3.5" id="filter-type-cv-input"/>` in the hidden `#components` group, matching the `fill:#00ff00` / `r=3.5` convention used by all other input jacks.

Coordinates (24, 32) match the `addInput(createInputCentered<PJ301MPort>(mm2px(Vec(24.0, 32.0)), ...))` from T01 exactly. Distance from CKSS switch center (24, 22) to jack center (24, 32) is 10mm — well above the 8mm minimum.

Also added the `## Observability Impact` section to T02-PLAN.md per pre-flight requirement.

## Verification

- `make -j4 2>&1 | grep -E "warning|error"` — zero output ✅
- `xmllint --noout res/CipherOB.svg` — exit 0, valid XML ✅
- Code-level invariant check: all `prevFilterType` and `crossfadeCounter` references are array declarations or `[c]`-indexed ✅
- Component layer has `id="filter-type-cv-input"` at correct coordinates ✅
- No overlap: nearest components are CKSS switch at (24, 22) [10mm away] and cutoff knob at (20, 43) [~12mm away] ✅

### Slice-level verification status (T02 is final task):
- ✅ `make -j4 2>&1 | grep -E "warning|error"` produces no output
- ⏳ Module loads in VCV Rack without crash — requires manual runtime verification
- ⏳ Patching a gate to Filter Type CV toggles filter type — requires manual runtime verification
- ⏳ Unplugging Filter Type CV cable → panel switch resumes control — requires manual runtime verification
- ⏳ Polyphonic gate cable → different voices can be in different filter modes simultaneously — requires manual runtime verification

Build-time and code-level checks all pass. Runtime/audio checks require human UAT in VCV Rack (as specified in slice plan's Proof Level section).

## Diagnostics

- **SVG validity:** `xmllint --noout res/CipherOB.svg`
- **Coordinate match:** `grep "24.*32" res/CipherOB.svg` should show the dot indicator, component circle, and the `addInput` line in .cpp
- **Visual mismatch:** If the jack ring appears offset from the panel art in VCV Rack, compare the SVG dot indicator coordinates with the `mm2px(Vec(...))` in widget code

## Deviations

- Used "CV" as the label instead of "TYPE" or "TYPE CV" — the existing panel already labels this section "FILTER" in the section header, so "CV" is sufficient and consistent with how cutoff/drive/resonance CV inputs are labeled.

## Known Issues

None.

## Files Created/Modified

- `res/CipherOB.svg` — added filter type CV jack dot indicator, "CV" label, and component layer entry
- `.gsd/milestones/M002/slices/S01/tasks/T02-PLAN.md` — added Observability Impact section per pre-flight requirement
