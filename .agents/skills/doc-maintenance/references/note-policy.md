# Decision records

Write only lasting decision rationale that code, tests, and existing documentation cannot carry. Update an existing owner instead of duplicating it. A code pattern is evidence of implementation, not proof of historical motivation. Never invent alternatives or first-proposed dates; when importing incomplete history, retain its original format and clearly name missing evidence.

Managed records use `{lifecycle}/{class}/yyyy-mm-dd-topic.md`. Lifecycles are proposed, implemented, rejected; classes are feature, bug-fix, simplification, architecture, process, testing. The filename date is the actual first proposal date when known. New proposals use the current user-local date. An existing ADR tree retains its own format and is checked semantically unless its project validator is available.

Managed records start with `# Agent Note: title`, a blank line, `Status: proposed|implemented|rejected — reason`, and a blank line. English structural tokens are stable parser keys even when body prose is Chinese; explain this choice rather than translating keys silently.

- Proposed: Problem, Proposal, Alternatives considered, Acceptance criteria, Risks.
- Implemented: Problem, Decision, Alternatives considered, Consequences. Optional Testing/Verification retains actual evidence and gaps. Update factual realization in place; a reversal requires a new decision and cross-links.
- Rejected: retain the proposal’s Problem, Proposal and Alternatives considered; status carries the rejection reason. Keep only while it prevents a plausible meaningful mistake.

Alternatives are real options and why they lost, never invented to fill a section. A new record triggers a search for superseded decisions. Preserve and cross-link partial supersessions. Consolidate a fully superseded record only after its unique rationale, alternatives, consequences, verification and named gaps survive elsewhere, and repair inbound links. Feature removal is complete only when production behavior, configuration, durable/wire formats, compatibility handling and supported tests no longer retain it.

Delete purely mechanical/local UI records only within an authorized cleanup. Keep useful architectural, security, ownership, durability and reintroduction rationale regardless of age or size. Archive only completed implemented decisions with historical value but little future guidance. Frozen content is historical context, never current authority. Archives preserve body content, add the archival date, and seal all actually maintained counterpart files; no translations are required by default.
