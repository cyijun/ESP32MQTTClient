---
name: doc-maintenance
description: Create, initialize, restructure, or review this project’s documentation using explicit fact ownership, audience-first hierarchy, current-state prose, and source-backed verification.
---

# Project documentation maintenance

Make each page searchable, newcomer-readable, and exact enough for agents and maintainers. Read applicable `AGENTS.md` files, `.agents/docs-config.json`, and the [documentation standard](references/standard.md). Package contracts stay beside code; architecture, procedures, decisions, and incidents each keep their own kind of truth.

## Load in this turn

A copied skill may be absent from the current skill catalog. Open this file directly and read [prose-standard](../prose-standard/SKILL.md), its [examples](../prose-standard/references/examples.md), and [trim-session-residue](../trim-session-residue/SKILL.md) before authoring initialization outputs. Read [archive-agent-notes](../archive-agent-notes/SKILL.md) when handling decisions. Do not treat a path listing, copy operation, hash receipt, or truncated output as reading the rules. Continue in this turn; never defer quality review until discovery refreshes.

## Workflow

1. Derive scope from the request. Read the target documents, source/tests, owning navigation, and applicable instructions. A review reports findings; an authorized write applies scoped changes.
2. Classify each page by one primary job and reader. Use the [hierarchy reference](references/structure-hierarchy.md); set scope by its subject and position before choosing tutorial or reference.
3. Preserve existing owners. Update existing pages where possible, link to detail instead of duplicating it, and preserve public routes during authorized moves.
4. Define the reader’s starting state, observable outcome, likely failure, recovery path, and next useful depth before writing details.
5. Verify claims against current code, tests, metadata, or an active decision owner. Run safe and authorized documented operations when feasible. Otherwise distinguish configured/static evidence from execution, and state what remains unverified. Never run deployment or destructive commands merely to verify prose.
6. Write actual project content. Do not infer historical rationale from code, fabricate alternatives, invent test results, or populate templates with empty generic sections. Use the project’s language; translations and metadata wrappers are optional unless already required.
7. Review the complete text using [review criteria](references/review.md) and prose-standard. Apply session-residue checks semantically; preserve facts, modality, failures, and exceptions.
8. Run the configured checker and existing relevant tests. Inspect the diff for ownership, links, exclusions, and preserved user edits. Report only checks run and material limitations.

## Quality criteria

Use these definitions in review. Each section opens with a short orienting paragraph before subsections or exhaustive detail.

- **Brief:** the common path contains only facts needed for its outcome; exhaustive truth remains one direct link or detail layer away.
- **Intuitive:** prerequisites precede dependent concepts, one next action is obvious, and headings use terms readers search for.
- **Friendly:** readers can recognize success, understand risk before acting, recover from likely failure, and choose whether to continue deeper.
- **Accurate:** each durable claim has one owner and a verification path proportionate to its risk.
- **Agent-readable:** metadata, stable headings, anchors, terminology, ownership, and current/proposed status support targeted retrieval without loading the corpus.
- **Newcomer-complete:** a professional engineer with no repository context can reconstruct the relevant architecture or feature through three to five linked pages.

Do not apply a universal word limit to exhaustive references. Measure entry-path length, unrelated material scanned for one lookup, largest section, heading count, and page size; split by an existing domain owner when retrieval cost is high.


## Ongoing changes

Update affected documentation with the implementation. New decisions trigger a supersession search; use the archive workflow for classification. Change owner sources before generated derivatives. A site, if present, projects canonical documents and is not a second editable copy. Do not create a site pipeline, translation process, or new dependency for ordinary documentation work.
