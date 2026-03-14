---
estimated_steps: 5
estimated_files: 1
---

# T02: Panel SVG — add filter type CV jack

**Slice:** S01 — Filter Type CV with Per-Voice Crossfade
**Milestone:** M002

## Description

Add the filter type CV jack to the panel SVG so the new input is visible and patchable in VCV Rack. The jack must align with the `addInput` coordinates from T01 and fit within the 14 HP panel without overlapping existing components.

## Steps

1. Read `res/CipherOB.svg` to understand current layout: component positions, label styles, jack circle styling, and available space near the filter type switch.
2. Identify the exact coordinates used in T01's `addInput` call and convert to SVG coordinate space if needed (VCV uses mm2px — SVG should use the same mm-based coordinate system the existing jacks use).
3. Add a jack circle element for Filter Type CV matching the style of existing jack circles (stroke, fill, radius).
4. Add a text label ("TYPE" or "TYPE CV") near the jack, matching existing label font, size, and color.
5. Verify no overlap with the CKSS switch at (24.0, 22.0) or other nearby components. Ensure minimum ~8mm spacing from switch center to jack center.

## Must-Haves

- [ ] Jack circle added to SVG at coordinates matching T01's `addInput` position
- [ ] Label text added near the jack
- [ ] No overlap with existing panel components
- [ ] SVG is valid (module loads in VCV Rack)

## Verification

- `make -j4` succeeds (SVG is loaded at build time — invalid SVG causes load failure)
- Visual: module loads in VCV Rack with new jack visible below filter type switch
- Visual: jack is correctly sized and styled, label is readable

## Observability Impact

- **Visual panel inspection:** The new jack circle and "CV" label at (24, 32) are visible in VCV Rack's module browser and on the panel. A missing or mispositioned jack is immediately obvious.
- **SVG validity:** Invalid SVG causes module load failure at runtime — the module simply won't appear. `xmllint --noout res/CipherOB.svg` catches structural errors before launch.
- **Component layer registration:** The `id="filter-type-cv-input"` circle in the hidden components layer is used by helper scripts (e.g., `helper.py`) to map SVG positions to widget code. If this entry is missing, automated panel-to-code consistency checks will flag the discrepancy.
- **Coordinate alignment:** If the SVG dot indicator position doesn't match the `mm2px(Vec(24.0, 32.0))` in widget code, the jack ring will render offset from the panel art — visible as a misaligned circle on the panel.

## Inputs

- `res/CipherOB.svg` — current panel design
- T01's `addInput` coordinates — jack position must match
- Decision: "CKSS switch at (55mm, 28mm)" — but actual current position is (24.0, 22.0) per widget code; use widget code as ground truth

## Expected Output

- `res/CipherOB.svg` — updated with filter type CV jack circle and label
