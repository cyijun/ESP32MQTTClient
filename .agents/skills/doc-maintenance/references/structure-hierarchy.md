# Structure and hierarchy

Use existing document owners and navigation first. Package contracts remain beside code. Cross-package material belongs under audience and purpose, but do not impose a new hierarchy on a working documentation site. Do not create empty sections, mandatory YAML, a table of contents, or a Dev Note when the page does not need them.

## Section progression

Open every substantive H2 with a short orienting paragraph before tables, code, or H3 subsections. Explain the section's subject and decision-relevant point; do not repeat its complete contents.

Within a page, order content by reader depth:

1. Basic use: when to choose the feature, required inputs, shortest safe example, observable success, and likely recovery.
2. Advanced use: configuration choices, limits, operations, and integration behavior.
3. Developer detail: ownership, lifecycle, data model, failures, performance, security, and extension points worth maintaining.

Use progressive detail or folds only when they help the reader and the renderer supports them.

Folded developer detail is concept-level by requirement: the overall design concept, the architecture of the main components, and hand-waving dataflow — enough to understand how the package works — plus source-map tables and links to code for exact detail. It never becomes an exhaustive catalog: no full API inventories, column lists, event-payload enumerations, or JSDoc restatement. Keep hypotheses in explicitly non-authoritative scratch notes or the project’s existing issue system; current-state prose remains verified.

Keep exhaustive generated types, schemas, or catalogs behind a compact entry paragraph and stable index. Split them by an existing domain owner when one lookup requires scanning unrelated groups.


## Scope and placement

Give an independently searchable rule or decision its own file when it has its own owner, change cadence, inbound links, or validation. Keep tightly coupled obligations together. An ordinary folder entry page explains purpose and links direct children without restating them. Decision records use their path/status and search; do not generate a second central status index.

Retain existing ADR/RFC naming and dates. New managed records use status/class/date-topic paths. Create a directory when real content needs it. Scratch notes that survive handoff name an owner, expiry, and promotion target; disposable notes remain ignored.
