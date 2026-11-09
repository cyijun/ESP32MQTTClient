# Documentation standard

## One home per fact

Each fact has one home: the tier whose job it is; elsewhere, link there. Essential caller-visible conditions stay locally readable, while extended rationale has one owner.

| Location | Responsibility |
|---|---|
| Root AGENTS.md | Standing orders needed each session, concise rules with links |
| Subtree AGENTS.md | Only rules specific to that subtree |
| Architecture | Composition, component ownership, data flow, extension points |
| Package README / API reference | Configuration, semantics, failure, limitations, public contracts |
| Decision records | Durable rationale, alternatives, consequences, required verification |
| Procedures | Prerequisites, actions, observable verification, recovery |
| Incident reports | Evidence, chronology, causal chain, impact, prevention |
| Skills | Reusable workflows and specialized judgment |
| Generated references | Exhaustive information regenerated from owning sources |

A document’s subject and tree position fix its scope. Explain direct children by purpose and link to their owners for detail. A reference is exhaustive only about its own subject. Tutorials introduce prerequisites before dependent concepts and lead to an outcome; references support lookup. Separate substantial mixed forms.

## Writing rules

- Document current state. Keep history in decision records, incident reports, commits, or intentionally versioned references.
- Preserve actor, conditions, timing, modality, negative guarantees, exceptions, ownership, failures, and consequences. Link rationale; do not replace an essential local contract with a link.
- Record decisions only when lasting rationale is not already expressed by code, tests, or existing docs. Routine mechanical or local UI changes need no record.
- Implemented decisions keep factual realization current. A changed decision requires a new record; fully superseded records can be consolidated only after all unique rationale and verification obligations survive in the new owner.
- Keep documentation readable without private session transcripts, draft section numbers, or departed reviewers. Preserve valid issue references and measured evidence.
- Use stable terms and direct verbs. Headings and links should support targeted retrieval. Do not force English sentence-length or whitespace-word limits on other languages.
- Update translations only if the project maintains them. Keep generated sources and translated derivatives under their existing ownership.
- Prefer one physical line per paragraph in newly authored Markdown when compatible with local conventions; preserve established wrapping in unrelated text.

## Budgets

Apply explicit budgets to standing instructions when useful, not every reference page. Relocate misplaced detail first, condense without losing propositions second, and raise a ceiling when the content genuinely needs space. Budgets are guardrails, never deletion quotas. The checker counts each CJK character and each non-CJK word as a unit; this is a documented approximation, not a tokenizer.

## Review probes

Search for repeated rules, stale status annotations, hand-maintained catalogs, misplaced history, reasoning transcripts, repeated sibling rationale, paragraph walls, decorative emphasis, and proposal language in implemented records. Hits require semantic judgment. Preserve required verification and named coverage gaps when removing obsolete plans.

## Validation boundaries

Checks can establish file existence, local link targets, managed-note structure, configured budgets, and archive hashes. They cannot prove factual accuracy, translation equivalence, or that an agent understood delivered instructions. Existing ADR formats need their own checks and semantic review. Only a trusted baseline comparison can detect rewriting both an archive and its local seal.
