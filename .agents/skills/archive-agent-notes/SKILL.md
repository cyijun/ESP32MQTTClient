---
name: archive-agent-notes
description: Use when adding, auditing, pruning, archiving, restoring, or reviewing Agent Notes in this project; checks every new note for superseded active records, deletes small UI and purely mechanical records, classifies other implemented notes by future decision value, deletes rejected notes that no longer prevent a tempting fallacy, and applies the frozen archived/{kind} document set and manifest rules.
---

# Archive project decision records

Reduce the active decision corpus without erasing history that can still guide work. Judge every note semantically; word count and age are discovery aids, never archive criteria.

## Read the contracts

Read `.agents/docs-config.json`, the configured decision root’s existing README and archive instructions, and applicable `AGENTS.md` files. If the existing ADR convention has no policy owner, use [the portable decision policy](../doc-maintenance/references/note-policy.md) for judgment without changing its established file grammar. Use current code, configuration, docs, newer decisions, and inbound links to establish present ownership. With an existing ADR convention, retain its format and use semantic review rather than imposing the managed-note parser.

## Check supersession when adding a note

Before drafting, apply the creation criteria in the configured decision root’s README. Every new Agent Note triggers a scoped audit of active notes covering the same decision, mechanism, or rejected alternative. Classify each full or partial supersession while writing the new note: archive qualifying implemented document sets in the same change, retain and cross-link partial supersessions or independently useful rationale, reject obsolete proposals, and delete rejected notes that no longer prevent a plausible mistake. Apply the Agent Note consolidation rule when the new owner absorbs every unique proposition; do not defer a known match to a later corpus audit.

## Classify by future value

Apply these lifecycle-specific outcomes:

- **Implemented — delete:** delete complete document sets that only describe small UI adjustments or purely mechanical changes, and repair or remove inbound links. Local bug fixes, performance changes, new capabilities, and substantive behavior or ownership decisions do not qualify merely because they are implemented. Apply this criterion before keep/archive classification.
- **Implemented — keep active:** retain a note when its rationale, alternatives, negative guarantees, durable/wire semantics, ownership boundary, security rule, or reintroduction condition is likely to guide a future change. Classify the decision, not the mechanics of its implementation: a mechanical rename or type extraction can still record lasting naming, compatibility, or ownership rules. Length does not matter.
- **Implemented — archive:** archive a substantive historical decision when it is complete and unlikely to guide future work, but its historical rationale still warrants preservation. Do not archive records that meet the direct-deletion criterion.
- **Proposed — never archive:** keep a live proposal active; if it is no longer worth pursuing, reject it with an honest reason and satisfy the rejected lifecycle format.
- **Rejected — keep only as a guardrail:** retain a rejection only when the losing proposal remains a tempting, meaningful mistake and the note explains why it loses.
- **Rejected — delete:** delete the whole document set when the rejected idea is obsolete, superseded, no longer plausible, or unlikely to prevent re-litigation. Repair or delete inbound links.

Do not archive toward a quota. Inspect every note in scope, classify analogous groups under one principle, use best judgment for close cases, and record genuinely borderline decisions for the handoff.

## Calibrated examples

These examples set the bar; the word counts demonstrate that size is not the test.

Delete implemented notes such as:

- collapsed sidebar control rail — 533 words: closed, minor UI behavior;
- issue-policy module ownership — completed function relocation and import rewiring with unchanged behavior.

Keep implemented notes such as:

- event-sourced sessions — 248 words: foundational authority and durability boundary;
- single application-home resolver — 596 words: cross-product ownership rule;
- project session directories — 628 words: durable storage and identity policy;
- parallel pre-push gates — 400 words: borderline, but still guides gate scheduling and resource tuning;
- dropped image content block — 334 words: keep until multimodal support lands, because it states the coordinated reintroduction condition.

For rejected notes:

- keep folding the compaction package split — 426 words: the temptation to merge the packages remains meaningful;
- delete streaming workflow progress through tool calls — 972 words: its transport/UI premise is obsolete;
- delete dropping terminal metadata — 362 words: the later automation-only transport decision resolved the question.

## Archive one implemented record

1. Identify the complete document set actually owned by the record: one file for a single-language project; all existing counterparts and consistency records when the project maintains them. Never invent translations to satisfy archival.
2. Archive only a completed implemented decision. Move it to the configured archive by class. Preserve the body, insert `Archived: YYYY-MM-DD` immediately after the status, and keep its implemented status. For an existing ADR format, use its equivalent metadata and documented procedure.
3. Repair inbound links in active documents; point to current authority unless history is intentionally cited. Never repair outbound links inside the frozen record.
4. Run the project checker’s `seal` command to append new artifact hashes. It must verify existing seals first. Run `check` afterward. The trusted Git baseline option additionally checks that the manifest itself has not been rewritten; local hashes alone cannot provide that guarantee.

After sealing, never edit, move, translate, reformat, or delete the historical record. Future facts and decisions belong in active documentation. Default searches exclude the archive, while explicit historical queries may enter it.

## Validate and report

Run the project’s documented checks and inspect the diff. Report records kept, consolidated, deleted, archived, or rejected, including genuinely borderline cases and the rationale for each group. Report checks actually run. Do not claim archived outbound links are valid: frozen sources are intentionally excluded from that check.
