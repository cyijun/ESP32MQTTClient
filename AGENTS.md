# AGENTS.md

## Build Commands

### Arduino Platform (via PlatformIO or Arduino IDE)
```bash
# Build for Arduino ESP32 (tested via GitHub Actions)
# Requires Arduino CLI or PlatformIO
python3 ci/build_platform.py esp32  # From CI workflow
```

### Native ESP-IDF
```bash
# Build ESP-IDF example
cd examples/CppEspIdf
idf.py build

# Build for specific target
idf.py build --target esp32

# Flash and monitor
idf.py flash monitor

# Build with specific ESP-IDF version (tested: v4.4.6, v5.3)
export IDF_PATH=/path/to/esp-idf/v5.3
. $IDF_PATH/export.sh
idf.py build
```

### Running Single Test/Example
```bash
# Build and run specific example
cd examples/CppEspIdf
idf.py build flash monitor

# For Arduino example, compile with Arduino IDE or:
arduino-cli compile --fqbn esp32:esp32:esp32 examples/HelloToMyself
```

## Code Style Guidelines

### Naming Conventions
- **Classes**: `PascalCase` (e.g., `ESP32MQTTClient`)
- **Methods**: `camelCase` (e.g., `loopStart`, `setURI`, `publish`)
- **Private members**: `camelCase` with underscore prefix (e.g., `_mqttClient`, `_mqttConnected`)
- **Constants/macros**: `UPPER_SNAKE_CASE` (e.g., `WIFI_SSID`, `TAG`)
- **Local variables**: `camelCase` (e.g., `success`, `payloadStr`)
- **Typedefs**: `PascalCase` (e.g., `MessageReceivedCallback`)

### String Handling
- **MUST use `std::string` instead of Arduino `String`**
- Convert Arduino String to C-style string when interfacing:
  ```cpp
  String arduinoStr = "topic";
  mqttClient.publish(arduinoStr.c_str(), ...);
  ```
- Prefer `const std::string &` for function parameters to avoid copies
- Use `.c_str()` when passing to ESP-IDF C APIs

### Formatting & Types
- Use `const` references for string parameters: `const std::string &topic`
- Inline trivial getters in header file: `inline bool isConnected() const`
- Place public API first in class, then private members
- Use C-style casts for ESP-IDF compatibility: `(char *)topic`
- No RTTI enabled (compile flag: `-fno-rtti`)
- Use `uint16_t`, `uint8_t` for sizes, `int` for return codes

### Error Handling
- Return `bool` for success/failure operations
- Return `esp_err_t` for ESP-IDF operations, check against `ESP_OK`
- Use ESP-IDF logging macros:
  - `ESP_LOGI(TAG, "Info message")` for info
  - `ESP_LOGW(TAG, "Warning: %s", msg)` for warnings
  - `ESP_LOGE(TAG, "Error: %d", code)` for errors
- Validate pointer checks: `if (_mqttUri != nullptr)`
- Log debug messages conditionally: `if (config.enableSerialLogs)`

### ESP-IDF Version Compatibility
**CRITICAL**: Must support both ESP-IDF v4.x and v5.x

Use version guards for API differences:
```cpp
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    // IDF 4.x API
    _mqtt_config.uri = _mqttUri;
    _mqtt_config.event_handle = handleMQTT;
#else
    // IDF 5.x API
    _mqtt_config.broker.address.uri = _mqttUri;
    esp_mqtt_client_register_event(_mqtt_client, MQTT_EVENT_ANY, handleMQTT, this);
#endif
```

### Callback Requirements
**IMPORTANT**: Global callbacks are required (not class methods, not lambdas)

Required global functions:
```cpp
void onMqttConnect(esp_mqtt_client_handle_t client);

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
esp_err_t handleMQTT(esp_mqtt_event_handle_t event);
#else
void handleMQTT(void *handler_args, esp_event_base_t base,
                int32_t event_id, void *event_data);
#endif
```

Lambdas allowed inside `subscribe()` but event handlers must be global.

### Memory Management
- Be careful with malloc without matching free (see `setURL`)
- Prefer stack allocation when possible
- ESP-IDF handles MQTT client lifecycle
- Avoid dynamic allocation in hot paths

### Threading
- Non-blocking architecture: `loopStart()` returns immediately
- MQTT runs in background FreeRTOS task
- Member variables accessed from callbacks - no locks needed (ESP-IDF serializes)
- Use `isMyTurn(client)` when multiple MQTT instances possible

### Imports & Includes
- Use `#pragma once` in header files
- Order includes: standard library, ESP-IDF headers, local headers
  ```cpp
  #include <vector>
  #include <string>
  #include <mqtt_client.h>
  #include "esp_log.h"
  #include "ESP32MQTTClient.h"
  ```
- Include `esp_idf_version.h` for version checks

### Topic Patterns
- Support MQTT wildcards: `#` (multi-level) and `+` (single-level)
- Match via `mqttTopicMatch()` method
- Examples: `"foo/#"`, `"bar/+/baz"`

### File Organization
- Header files: `src/ESP32MQTTClient.h`
- Source files: `src/ESP32MQTTClient.cpp`
- Examples: `examples/CppEspIdf/`, `examples/HelloToMyself/`
- Library metadata: `library.properties`, `component.mk`

## Architecture Notes

### Non-Blocking Design
- Never block in main loop - MQTT runs in background
- `loopStart()` initializes and starts immediately
- No `loop()` calls needed in main application
- Safe for single-core devices (ESP32-C3)

### Connection Lifecycle
1. Configure in `setup()` with setters (`setURI`, `setCaCert`, etc.)
2. Call `loopStart()` - non-blocking
3. Global `onMqttConnect()` called when connected
4. Subscribe to topics in `onMqttConnect()`
5. Handle messages via callbacks
6. Auto-reconnect by default (toggle with `setAutoReconnect()`)

### Key Invariants
- `std::string` everywhere, no Arduino `String`
- Global callbacks for event handling
- ESP-IDF 4.x/5.x dual compatibility
- Thread-safe via ESP-IDF event loop serialization
