# Development and documentation maintenance

**English** | [简体中文](development_CN.md)

Use the [README](../README.md) for installation, API behavior, and working examples. The root [agent instructions](../AGENTS.md) own the source layout and implementation constraints. This guide owns the local validation and documentation workflow.

## Validate a change

From the repository root, run the host behavior suite with a C++11 compiler:

```sh
sh tests/run_host_tests.sh
```

The [runner](../tests/run_host_tests.sh) uses `CXX` when set and otherwise `c++`. It builds with warnings treated as errors, RTTI disabled, and pthread support. Binaries go under `${TMPDIR:-/tmp}/esp32mqttclient-host-tests`. The same five cases run against the ESP-IDF 4.4.6 and 5.3 configuration branches: publish validation and return codes, topic matching, subscribe rollback and retained-message ordering, unsubscribe rollback, and fragment reassembly with size limits. The [host fakes](../tests/fakes/mqtt_client.h) replace platform services; a passing suite does not establish device, network, or TLS behavior.

With Arduino CLI and an ESP32 core installed, compile the Arduino example:

```sh
arduino-cli compile --fqbn esp32:esp32:esp32 --library . examples/HelloToMyself
```

This command uses the installed core. The [Arduino workflow](../.github/workflows/ci4main.yml) defines the supported CI matrix; one local compile does not cover every matrix entry. The [ESP-IDF workflow](../.github/workflows/esp_idf_ci.yml) builds `examples/CppEspIdf` with its configured toolchains. Follow the README's ESP-IDF build instructions and the root agent instructions for platform coverage required by source changes. Compile success does not replace a board and broker test.

## Maintain the documentation

Keep API contracts and examples in both existing README files. Keep implementation rules in the root agent instructions, and keep the English and Chinese versions of this guide aligned when their procedures change. Link to those owners instead of copying their API tables or platform matrix into another page. `.agents/docs-config.json` includes `README_CN.md` explicitly because its filename is outside the checker's default root README names.

Before writing initialization output, run these commands separately and read every printed instruction and reference in the current session:

```sh
python3 .agents/doc-tools/docs.py activate --root . --part docs
python3 .agents/doc-tools/docs.py activate --root . --part prose
python3 .agents/doc-tools/docs.py activate --root . --part trim
python3 .agents/doc-tools/docs.py activate --root . --part archive
```

Copied skills can be absent from the agent's skill catalog. Open omitted files directly if output is truncated. Delivery receipts record file hashes; they do not prove that an agent read or applied the rules. Re-read affected instructions after changing them.

Review the complete documents against their source files, then run:

```sh
python3 .agents/doc-tools/docs.py check --root .
```

The checker covers local file targets and managed decision structure, configured budgets, and archive seals. Review link anchors, translation meaning, technical accuracy, and command results separately. For initialization, write `.agents/doc-tools/.local/review.json` with `outputs` entries containing a document `path` and existing `sources` paths, `reviews` entries containing `principle`, output `path`, and a specific `finding`, and a `verification` list of actual outcomes and unverified limits. Cover all four principles: `fact-ownership`, `complete-propositions`, `session-independent`, and `existing-content-preserved`. The local [validator](../.agents/doc-tools/docs.py) owns the report schema. Then run:

```sh
python3 .agents/doc-tools/docs.py finish --root . --report .agents/doc-tools/.local/review.json
```

The ignored `.local` files are working evidence. Record enduring design rationale only when it meets the [decision policy](../.agents/skills/doc-maintenance/references/note-policy.md); code alone does not establish historical alternatives or dates. The [notes entry point](../.agents/notes/README.md) owns decision lifecycle navigation. Frozen archives are historical evidence and are excluded from ordinary searches by `.rgignore`.
