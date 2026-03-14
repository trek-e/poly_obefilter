---
estimated_steps: 5
estimated_files: 1
---

# T02: Add FM, FM attenuverter, and V/Oct visual elements to panel SVG

**Slice:** S02 — FM Input and 1V/Oct Pitch Tracking
**Milestone:** M002

## Description

Adds the visual panel elements for the three new components introduced in T01: FM input jack, FM depth attenuverter knob, and V/Oct input jack. Each needs a dot indicator (colored circle), a pixel-art label, and a component layer entry matching the widget coordinates from T01. The INPUT section header must also be widened to span the new jacks.

## Steps

1. **Widen INPUT section header** — Change `width="30"` to `width="65.12"` on the INPUT section header rect at Y=80. This extends coverage from the audio input's X range to include FM (X≈34) and V/Oct (X≈58) jacks. The `65.12` value spans `3` to `68.12` (full 14 HP usable area).

2. **Add FM input dot indicator and label** — Add a circle at (34, 90) matching the blue input section color with opacity 0.4, following the established pattern from the audio input dot indicator. Add "FM" label in pixel-art rectangle-stroke style near (34, 87).

3. **Add FM attenuverter dot indicator and label** — Add a circle at (42, 84) for the knob position. Use appropriate section color. Add a small label or reuse the "FM" association since the knob is visually grouped with the FM input jack.

4. **Add V/Oct input dot indicator and label** — Add a circle at (58, 90) matching input section color. Add "V/O" label in pixel-art style near (58, 87). Research noted "V/OCT" may be too wide — "V/O" is the compact alternative.

5. **Add component layer entries** — In the `<g id="components">` group, add:
   - `<circle style="fill:#00ff00" cx="34" cy="90" r="3.5" id="fm-cv-input"/>`
   - `<circle style="fill:#00ff00" cx="42" cy="84" r="3.5" id="fm-atten"/>`
   - `<circle style="fill:#00ff00" cx="58" cy="90" r="3.5" id="voct-input"/>`
   Coordinates must match T01's widget `mm2px(Vec(...))` values exactly.

## Must-Haves

- [ ] INPUT section header `width` covers all input jacks (including FM and V/Oct)
- [ ] Dot indicators for FM input, FM attenuverter, and V/Oct input at correct coordinates
- [ ] Pixel-art labels "FM" and "V/O" in established style
- [ ] Component layer entries with IDs `fm-cv-input`, `fm-atten`, `voct-input`
- [ ] Component layer coordinates match T01's widget positions exactly
- [ ] Valid SVG (xmllint passes)

## Verification

- `xmllint --noout res/CipherOB.svg` — exit 0
- `grep -c 'fm-cv-input\|fm-atten\|voct-input' res/CipherOB.svg` — returns 3
- `grep 'width="65.12"' res/CipherOB.svg` — matches the widened INPUT header
- Visual inspection: coordinates (34,90), (42,84), (58,90) match T01 widget code

## Inputs

- `res/CipherOB.svg` — current panel with INPUT section header at Y=80, existing dot indicator and component patterns
- T01 widget positions: FM input at `Vec(34.0, 90.0)`, FM atten at `Vec(42.0, 84.0)`, V/Oct at `Vec(58.0, 90.0)`
- S01 SVG patterns: dot indicator circle style, pixel-art label approach, component layer entry format

## Observability Impact

- **SVG structural validation:** `xmllint --noout res/CipherOB.svg` confirms valid XML — catches unclosed tags, malformed attributes from manual edits.
- **Component layer completeness:** `grep -c 'fm-cv-input\|fm-atten\|voct-input' res/CipherOB.svg` returning 3 confirms all component IDs are present. A future agent can verify widget-to-SVG coordinate alignment by comparing `mm2px(Vec(...))` values in `src/CipherOB.cpp` against `cx`/`cy` attributes in the component layer.
- **Failure mode:** If component layer IDs are missing or misspelled, VCV Rack's panel helper will silently skip them — the module loads but widgets won't align to panel art. Grep for the exact IDs to catch this.
- **Visual verification:** Dot indicator colors (`#8040a0` for FM/V/Oct modulation inputs vs `#3060a0` for audio inputs) distinguish modulation from signal path visually. A future agent can verify color consistency by grepping the INPUT section for fill colors.

## Expected Output

- `res/CipherOB.svg` — widened INPUT header, three dot indicators, two pixel-art labels, three component layer entries
