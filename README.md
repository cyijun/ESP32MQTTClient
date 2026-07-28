# ESP32MQTTClient

[![Arduino CI](https://github.com/cyijun/ESP32MQTTClient/actions/workflows/ci4main.yml/badge.svg?branch=main)](https://github.com/cyijun/ESP32MQTTClient/actions/workflows/ci4main.yml)
[![ESP-IDF CI](https://github.com/cyijun/ESP32MQTTClient/actions/workflows/esp_idf_ci.yml/badge.svg)](https://github.com/cyijun/ESP32MQTTClient/actions/workflows/esp_idf_ci.yml)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/cyijun/ESP32MQTTClient)

A thread-safe MQTT client for native ESP-IDF or Arduino ESP32. This library is compatible with `arduino-esp32` v2/v3+ and `ESP-IDF` v4.x/v5.x C++.

## Table of Contents

- [Features](#features)
- [Non-Blocking Architecture](#non-blocking-architecture)
- [Quick Start](#quick-start)
  - [Arduino Setup](#arduino-setup)
  - [TLS/SSL Example](#tlsssl-example)
- [String Handling](#string-handling)
- [Required Global Callbacks](#required-global-callbacks)
- [Migration Guide from PubSubClient](#migration-guide-from-pubsubclient)
- [API Reference](#api-reference)
  - [Configuration Methods](#configuration-methods)
  - [Lifecycle Methods](#lifecycle-methods)
  - [Pub/Sub Methods](#pubsub-methods)
- [New Functions](#new-functions)
- [Building the ESP-IDF Example](#building-the-esp-idf-example)

## Features

- **Background connection and event handling** - `loopStart()` returns immediately and no `loop()` polling call is needed
- **Thread-safe** MQTT client based on the official `esp-mqtt` component
- **TLS/SSL support** for secure MQTT connections (port 8883)
- Uses standard C++ `std::string` instead of Arduino `String`
- Logging is performed using the standard ESP-IDF `ESP_LOGX` macros
- Provides both specific topic subscriptions and a global "catch-all" message callback
- Interfaces inspired by [EspMQTTClient](https://github.com/plapointe6/EspMQTTClient)
- CA cert support by [dwolshin](https://github.com/dwolshin)
- Arduino-esp32 v3+ support by [dzungpv](https://github.com/dzungpv)

## Non-Blocking Architecture

`loopStart()` returns immediately. Connection, reconnection, and MQTT event handling
then run in the background task managed by esp-mqtt, so no `loop()` polling call is
required.

This does not mean that every API call is non-blocking. In particular, `publish()`
uses `esp_mqtt_client_publish()`, which can block while waiting for network access or
while sending a fragmented message. Message callbacks also run from the MQTT event
task and should therefore finish quickly.

## Quick Start

### Arduino Setup

```cpp
#include <WiFi.h>
#include "ESP32MQTTClient.h"

ESP32MQTTClient mqttClient;

void onMqttConnect(esp_mqtt_client_handle_t client) {
  if (mqttClient.isMyTurn(client)) {
    mqttClient.subscribe("test/topic", [](const std::string &payload) {
      Serial.printf("Received: %s\n", payload.c_str());
    });
  }
}

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
esp_err_t handleMQTT(esp_mqtt_event_handle_t event) {
  mqttClient.onEventCallback(event);
  return ESP_OK;
}
#else
void handleMQTT(void *handler_args, esp_event_base_t base,
                int32_t event_id, void *event_data) {
  (void)base;
  (void)event_id;
  auto *client = static_cast<ESP32MQTTClient *>(handler_args);
  auto *event = static_cast<esp_mqtt_event_handle_t>(event_data);
  client->onEventCallback(event);
}
#endif

void setup() {
  Serial.begin(115200);

  mqttClient.setURI("mqtt://broker.hivemq.com:1883");
  mqttClient.setMqttClientName("ESP32_Client");
  WiFi.begin("SSID", "PASSWORD");
  mqttClient.loopStart(); // Returns immediately.
}

void loop() {
  // Your code here - no need for mqttClient.loop()
  delay(1000);
}
```

### TLS/SSL Example

```cpp
// For TLS brokers like HiveMQ Cloud
const char* caCert = "-----BEGIN CERTIFICATE-----\n...";

void setup() {
  // ... WiFi setup ...
  
  mqttClient.setURL("broker.hivemq.cloud", 8883, "username", "password");
  mqttClient.setCaCert(caCert);  // Enable TLS verification
  mqttClient.loopStart();
}
```

## String Handling

This library uses `std::string` instead of Arduino `String`. Here's how to handle conversions:

```cpp
// ✅ Correct - use std::string directly
std::string topic = "mytopic";
std::string payload = "Hello World";
mqttClient.publish(topic, payload, 0, false);

// ✅ Convert from Arduino String
String arduinoTopic = "mytopic";
String arduinoPayload = "Hello World";
mqttClient.publish(arduinoTopic.c_str(), arduinoPayload.c_str(), 0, false);

// ❌ This will cause compilation error
// String arduinoTopic = "mytopic";
// mqttClient.publish(arduinoTopic, payload, 0, false);
```

## Required Global Callbacks

**Important:** These callback functions must be global (not class methods, not lambdas):

```cpp
// Required global callback for connection events
void onMqttConnect(esp_mqtt_client_handle_t client) {
  if (mqttClient.isMyTurn(client)) {
    // Subscribe to topics here
    mqttClient.subscribe("test/topic", [](const std::string &payload) {
      Serial.printf("Received: %s\n", payload.c_str());
    });
  }
}

// Required global event handler - ESP-IDF version dependent
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
esp_err_t handleMQTT(esp_mqtt_event_handle_t event) {
  mqttClient.onEventCallback(event);
  return ESP_OK;
}
#else
void handleMQTT(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
  (void)base;
  (void)event_id;
  auto *client = static_cast<ESP32MQTTClient *>(handler_args);
  auto *event = static_cast<esp_mqtt_event_handle_t>(event_data);
  client->onEventCallback(event);
}
#endif
```

## Migration Guide from PubSubClient

If you're migrating from the popular PubSubClient library:

| PubSubClient | ESP32MQTTClient | Notes |
|-------------|-----------------|-------|
| `client.connect()` | `mqttClient.loopStart()` | Connection startup returns immediately |
| `client.loop()` | **Not needed** | Runs automatically in background |
| `client.connected()` | `mqttClient.isConnected()` | Check connection status |
| `client.setServer()` + `setCallback()` | `mqttClient.setURL()` + callbacks | Similar setup pattern |
| `client.setBufferSize()` | `mqttClient.setMaxPacketSize()` | Configure esp-mqtt buffer sizes |
| Arduino `String` | `std::string` | Use `.c_str()` for conversion |

### Example Migration

**Before (PubSubClient):**
```cpp
PubSubClient client;

void setup() {
  client.setServer("broker", 1883);
  client.setCallback(messageCallback);
}

void loop() {
  if (!client.connected()) {
    client.connect("client"); // Blocks for 8+ seconds!
  }
  client.loop(); // Required in every loop
}
```

**After (ESP32MQTTClient):**
```cpp
ESP32MQTTClient mqttClient;

void setup() {
  mqttClient.setURI("mqtt://broker:1883");
  mqttClient.loopStart(); // Connection startup returns immediately.
}

void loop() {
  // No mqttClient.loop() needed!
}
```

## API Reference

### Configuration Methods

Unless otherwise noted, configuration setters must be called before `loopStart()`.

- `setURL(url, port, username, password)` - Set broker connection details
- `setURI(uri, username, password)` - Set complete MQTT URI
- `setMqttClientName(name)` - Set client ID
- `setCaCert(caCert)` - Enable TLS with CA certificate
- `setClientCert(clientCert)` - Set client certificate
- `setKey(clientKey)` - Set client private key
- `setMaxPacketSize(size)` → `bool` - Set the esp-mqtt incoming and outgoing buffer sizes (default: 1024 bytes). Complete incoming messages are reassembled up to 16 KiB by default; setting this above 16 KiB raises that reassembly limit to the requested size.
- `setMaxOutPacketSize(size)` → `bool` - Set the esp-mqtt outgoing buffer size (default: 1024 bytes)
- `setKeepAlive(seconds)` - Change keepalive interval (default: 120 seconds, the esp-mqtt default — the library does not set one itself)
- `setTaskPrio(prio)` - Set the priority of the MQTT background task
- `enablePersistence()` - Request a persistent session from the broker (clean_session = 0)
- `disablePersistence()` - Do not request a persistent session (restores the default non-persistent behavior)
- `enableLastWillMessage(topic, message, retain = false, qos = 0)` - Set last will message
- `setAutoReconnect(choice)` - Enable/disable auto-reconnect
- `disableAutoReconnect()` - Disable auto-reconnect
- `enableDrasticResetOnConnectionFailures()` - Restart the ESP32 when the MQTT connection is lost (#59)
- `enableDebuggingMessages(enabled)` - Enable debug logging
- `DEFAULT_PACKET_SIZE` - Constant, the default buffer size in bytes (1024), used by `setMaxPacketSize()` / `setMaxOutPacketSize()`

### Lifecycle Methods
- `loopStart()` - Start the background MQTT connection process and return immediately
- `isConnected()` - Check connection status
- `isMyTurn(client)` - Check if event is for this client
- `getClientName()` → `const char *` - Get the configured client name
- `getURI()` → `const char *` - Get the configured broker URI
- `printError(error_handle)` - Log a decoded `esp_mqtt_error_codes_t` error

### Pub/Sub Methods

The `bool` returned by the methods below reports whether the request was accepted
by the local esp-mqtt client. It does not report a broker acknowledgement such as
PUBACK, SUBACK, or UNSUBACK.
If a subscribe or unsubscribe request cannot be submitted, the local callback
registration remains as it was before the call.

- `publish(topic, payload, qos, retain)` → `bool` - Submit a message for publishing. The call may block; the full `std::string` length is used, so binary payloads containing embedded `\0` bytes are preserved.
- `publish(topic, buffer, length, qos, retain)` → `bool` - Submit a raw `uint8_t` buffer with explicit length, for binary payloads (e.g. Protocol Buffers) without converting to `std::string` first. The call may block.
- `subscribe(topic, callback, qos)` → `bool` - Submit a subscription request with a payload callback
- `subscribe(topic, callbackWithTopic, qos)` → `bool` - Submit a subscription request with a topic+payload callback
- `unsubscribe(topic)` → `bool` - Submit an unsubscribe request
- `setOnMessageCallback(callback)` - Set global message handler

## New Functions

### `setOnMessageCallback(MessageReceivedCallbackWithTopic callback)`

Sets a global callback function that is invoked for any incoming message, regardless of the topic. This is useful for centralized logging or handling all messages in one place.

**Example:**
```cpp
mqttClient.setOnMessageCallback([](const std::string &topic, const std::string &payload) {
    ESP_LOGI("MAIN", "Global handler: %s: %s", topic.c_str(), payload.c_str());
});
```

### `setAutoReconnect(bool choice)`

Enables or disables the automatic reconnection feature of the underlying ESP-IDF MQTT client. By default, auto-reconnect is enabled.

**Example:**
```cpp
// Disable automatic reconnection
mqttClient.setAutoReconnect(false);
```

## Building the ESP-IDF Example

The library includes a native ESP-IDF example in the `examples/CppEspIdf` directory. The top-level `CMakeLists.txt` registers the library as an ESP-IDF component using `idf_component_register()`.

1.  **Set up ESP-IDF:** Ensure you have the ESP-IDF environment installed and configured.
2.  **Configure Wi-Fi:** Open `examples/CppEspIdf/main/main.cpp` and set your Wi-Fi SSID and password.
3.  **Build the project:**
    ```bash
    cd examples/CppEspIdf
    idf.py build
    ```
