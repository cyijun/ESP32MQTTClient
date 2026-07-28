# AGENTS.md

Guidance for AI coding agents working on this repository. Assumes no prior knowledge of the project.

## Project Overview

**ESP32MQTTClient** is a C++ MQTT client library for ESP32, built directly on the official ESP-IDF `esp-mqtt` component (`mqtt_client.h`) rather than PubSubClient. It works in two environments:

- **Arduino ESP32** (`arduino-esp32` v2/v3+), distributed as an Arduino library (`library.properties`, currently version 1.1.5, `architectures=esp32`).
- **Native ESP-IDF** (v4.x and v5.x), registered as an ESP-IDF component via the top-level `CMakeLists.txt` / `component.mk`.

Key characteristics:

- **Non-blocking connection startup**: `loopStart()` returns immediately; connection and event handling run in a background FreeRTOS task managed by esp-mqtt. No `loop()` polling call is required in user code. `publish()` uses `esp_mqtt_client_publish()` and may block on network access or message fragmentation.
- Uses standard C++ `std::string` everywhere — **never** Arduino `String`.
- Logging uses ESP-IDF `ESP_LOGX` macros with tag `"ESP32MQTTClient"`.
- Interfaces are inspired by [EspMQTTClient](https://github.com/plapointe6/EspMQTTClient).
- TLS/SSL support via `setCaCert()` / `setClientCert()` / `setKey()`; `setURL()` automatically selects the `mqtts://` scheme when port is 8883.
- Supports per-topic subscription callbacks, a global catch-all callback (`setOnMessageCallback`), MQTT wildcards (`#`, `+`) matched by `mqttTopicMatch()`, and reassembly of fragmented incoming messages (`MQTT_EVENT_DATA` chunks buffered in `_incomingTopic`/`_incomingPayload` until complete).
- `publish()` / `subscribe()` / `unsubscribe()` return whether esp-mqtt accepted the local request, not whether the broker sent PUBACK / SUBACK / UNSUBACK.
- Incoming fragmented messages are reassembled up to 16 KiB by default. Calling `setMaxPacketSize()` with a larger value also raises that reassembly limit without adding a separate public setting.

## Repository Layout

```
src/ESP32MQTTClient.h        # Public API + class definition (only library header)
src/ESP32MQTTClient.cpp      # Full implementation
CMakeLists.txt               # ESP-IDF component registration (idf_component_register, REQUIRES mqtt)
component.mk                 # Legacy make-based ESP-IDF component file
library.properties           # Arduino Library Manager metadata
keywords.txt                 # Arduino IDE syntax highlighting
examples/HelloToMyself/      # Arduino sketch example (.ino)
examples/CppEspIdf/          # Native ESP-IDF example project
  components/ESP32MQTTClient/CMakeLists.txt  # Thin wrapper that compiles ../../../../src directly
.github/workflows/           # CI (see Testing / CI below)
tests/                       # Host behavior tests with lightweight ESP-IDF/esp-mqtt fakes
README.md                    # User-facing documentation and API reference
```

The entire library is a single class (`ESP32MQTTClient`) in one header/source pair. Keep it that way unless there is a strong reason to split.

## Build and Test Commands

The repository has lightweight host behavior tests plus compile checks for both
supported environments.

### Host behavior tests

```bash
bash tests/run_host_tests.sh
```

The script compiles `tests/host_tests.cpp` with a C++11 host compiler and fake
ESP-IDF/esp-mqtt headers for both the IDF 4.4.6 and IDF 5.3 compatibility
branches. It exercises binary publishing, topic matching, subscription rollback,
and fragmented-message reassembly without ESP32 hardware.

### Arduino ESP32 (CI)

`.github/workflows/ci4main.yml` uses `arduino-cli` to compile the Arduino example
against arduino-esp32 2.0.17 and 3.3.11. Locally, an equivalent check is:

```bash
# With arduino-cli (ESP32 core installed):
arduino-cli compile --fqbn esp32:esp32:esp32 --library . examples/HelloToMyself
```

### Native ESP-IDF

```bash
# Set up the ESP-IDF environment first (CI versions: v4.4.6, v5.3, and v5.5.3):
export IDF_PATH=/path/to/esp-idf
. $IDF_PATH/export.sh

cd examples/CppEspIdf
idf.py build                  # build
idf.py flash monitor          # flash and watch serial output
```

Before building the example, edit `examples/CppEspIdf/main/main.cpp` to set real `WIFI_SSID`/`WIFI_PASS` (and broker URI) values — the committed values are placeholders.

### CI matrix

`.github/workflows/esp_idf_ci.yml` builds `examples/CppEspIdf` with
`espressif/esp-idf-ci-action` for target `esp32` against **ESP-IDF v4.4.6, v5.3,
and v5.5.3**. Any change to `src/` must compile under all three. The IDF CI
compiles `src/` through the thin wrapper in
`examples/CppEspIdf/components/ESP32MQTTClient/`; the top-level `CMakeLists.txt`
component-registration path is not directly covered by CI.

## Code Style Guidelines

### Naming
- Classes: `PascalCase` (`ESP32MQTTClient`, `TopicSubscriptionRecord`)
- Methods / locals: `camelCase` (`loopStart`, `payloadStr`)
- Private members: leading underscore (`_mqtt_client`, `_mqttConnected`, `_topicSubscriptionList`)
- Constants / macros: `UPPER_SNAKE_CASE` (`WIFI_SSID`, `TAG`, `DEFAULT_PACKET_SIZE`)
- Typedefs: `PascalCase` (`MessageReceivedCallback`, `MessageReceivedCallbackWithTopic`)

### Types and strings
- `std::string` only; pass as `const std::string &`. Convert Arduino `String` at call sites with `.c_str()`.
- `uint16_t` / `uint8_t` for sizes and QoS, `int` for esp-mqtt return codes, `bool` for success/failure.
- `publish()` passes the full `std::string` length to esp-mqtt, so binary payloads with embedded `\0` are preserved — keep it that way.
- `publish()` / `subscribe()` / `unsubscribe()` return `true` when the request was submitted to esp-mqtt; they do not wait for a broker acknowledgement.
- Inline trivial getters/setters in the header (see `isConnected()`, `setURI()`, `setURL()`).

### Compilation constraints
- **No RTTI**: both `CMakeLists.txt` and `component.mk` compile with `-fno-rtti`. Do not use `dynamic_cast` or `typeid`. Note that `component.mk` sets this via `CXXFLAGS`, which leaks globally to the entire project under the legacy make build system (a known behavior of that build system; the make-based build is deprecated by ESP-IDF).
- C-style casts are used for ESP-IDF C API interop (e.g. `(char *)topic`); `static_cast` is fine in pure C++ code.
- Public API first in the class, private members after. `#pragma once` in headers. Include order: standard library, then ESP-IDF/FreeRTOS headers, then local headers. Include `esp_idf_version.h` for version checks.

### ESP-IDF v4.x / v5.x dual compatibility (CRITICAL)

`esp_mqtt_client_config_t` changed layout between IDF 4.x (flat fields) and 5.x (nested structs). **Never touch `_mqtt_config` fields directly outside the dedicated helper methods.** All version-dependent access goes through the `setConfig*()` private helpers (`setConfigUri`, `setConfigClientId`, `setConfigKeepAlive`, `setConfigLwt`, `setConfigSessionSettings`, etc.), each of which contains:

```cpp
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    _mqtt_config.uri = uri;                    // IDF 4.x flat field
#else
    _mqtt_config.broker.address.uri = uri;     // IDF 5.x nested field
#endif
```

Event registration differs too: IDF 4.x uses `_mqtt_config.event_handle = handleMQTT;`, IDF 5.x uses `esp_mqtt_client_register_event(_mqtt_client, MQTT_EVENT_ANY, handleMQTT, this);` (see `loopStart()`).

### Global callbacks (user-facing contract)

Users of the library **must** define two global functions — not class methods, not lambdas assigned as the handler itself (lambdas *inside* `subscribe()` calls are fine):

```cpp
void onMqttConnect(esp_mqtt_client_handle_t client);

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
esp_err_t handleMQTT(esp_mqtt_event_handle_t event);
#else
void handleMQTT(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
#endif
```

These are declared in `ESP32MQTTClient.h` and called from `onEventCallback()` / registered with esp-mqtt. Their signatures must not change without a major-version break, and both examples must be updated in lockstep.

### Threading and memory
- The subscription list (`_topicSubscriptionList`) and the global message callback are guarded by `_subscriptionListMutex` (a FreeRTOS mutex created in the constructor). MQTT events arrive on the esp-mqtt task while `publish`/`subscribe`/`unsubscribe` may be called from any user task — preserve this locking.
- `onMessageReceivedCallback()` deliberately collects matching callbacks **under** the mutex, then invokes them **after releasing it**, so callbacks may safely re-enter `subscribe()`/`unsubscribe()`. Do not "simplify" this back into holding the lock during dispatch.
- `setURL()` malloc's `_mqttUriBuffer` (freed on re-call and in the destructor); the destructor also destroys the esp-mqtt client and deletes the mutex. Keep every `malloc` paired with a `free`; prefer stack allocation elsewhere.
- `enableDrasticResetOnConnectionFailures()` calls `esp_restart()` on disconnect — intentional behavior (issue #59), not a bug.

### Error handling and logging
- Return `bool` for success/failure; guard debug logs with `if (_enableSerialLogs)`.
- Use `ESP_LOGI`/`ESP_LOGW`/`ESP_LOGE` with the file-scope `TAG`; `printError()` decodes `esp_mqtt_error_codes_t`.
- Null-check pointers before use (`if (_mqttUri != nullptr)` etc.).

## Testing Instructions

- Run `bash tests/run_host_tests.sh` for behavior covered by the host fakes.
- Host tests do not replace platform builds. The honest compile verification for any `src/` change is:
  1. `idf.py build` of `examples/CppEspIdf` under an IDF 4.x and IDF 5.x toolchain (CI does v4.4.6, v5.3, and v5.5.3).
  2. An Arduino compile of `examples/HelloToMyself` with arduino-esp32 v2 and v3.
- If you change the public API, update both examples and the README API reference so CI exercises the new signatures.

## Deployment / Release Process

- Arduino releases are picked up by the Arduino Library Manager from git tags; bump `version=` in `library.properties` when releasing.
- The library is also usable as an ESP-IDF component by adding this repo to a project's `components/` directory (top-level `CMakeLists.txt` handles registration).

## Security Considerations

- Examples contain placeholder credentials (`ssid`/`passwd`); never commit real Wi-Fi or broker credentials.
- `setURL()` builds the broker URI into a heap buffer sized as `strlen(url) + 16`; if you change the scheme/port formatting, re-check that bound.
- TLS is opt-in (`setCaCert()`); without it the client connects in plaintext. Keep the README's TLS example accurate when touching certificate APIs.
- `handleMQTT` (IDF 5.x) receives `this` as handler args; the library trusts the event loop to deliver valid `event_data` — validate before dereferencing if you extend event handling.
