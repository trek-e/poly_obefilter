# M001 Context: v0.60b OB-X Filter

Migrated from `.planning` v0.60b milestone (phases 1–9).

**Upstream:** v0.50b shipped 2026-02-03 (SEM-style 12dB multimode filter, polyphony, CV control, drive).

**This milestone:** Added OB-X style 24dB/oct cascaded SVF filter with panel switch, character tuning, LP-only output in 24dB mode, and click-free mode switching.

**Note:** Phase 9 Task 2 (human verification) had a pending blocker at migration time — resonance knob appeared non-functional due to stale binary. A fresh build was deployed plus noise injection dither was added (uncommitted). User had not yet retested. See `.planning/phases/09-character-output/.continue-here.md` for full debugging context.
