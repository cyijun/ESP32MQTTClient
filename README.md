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

- **Non-blocking operation** - MQTT runs in background FreeRTOS task, no `loop()` calls needed
- **Thread-safe** MQTT client based on the official `esp-mqtt` component
- **TLS/SSL support** for secure MQTT connections (port 8883)
- Uses standard C++ `std::string` instead of Arduino `String`
- Logging is performed using the standard ESP-IDF `ESP_LOGX` macros
- Provides both specific topic subscriptions and a global "catch-all" message callback
- Interfaces inspired by [EspMQTTClient](https://github.com/plapointe6/EspMQTTClient)
- CA cert support by [dwolshin](https://github.com/dwolshin)
- Arduino-esp32 v3+ support by [dzungpv](https://github.com/dzungpv)

## Non-Blocking Architecture

Unlike blocking MQTT libraries, `loopStart()` returns immediately and MQTT operations run in a background FreeRTOS task. This means:

- No watchdog timeouts during connection
- Safe for single-core devices (e.g., ESP32-C3)
- Main loop stays responsive
- Connection typically completes in ~1 second vs 8+ seconds with blocking clients

## Quick Start

### Arduino Setup

```cpp
#include <WiFi.h>
#include "ESP32MQTTClient.h"

ESP32MQTTClient mqttClient;

void setup() {
  Serial.begin(115200);
  WiFi.begin("SSID", "PASSWORD");
  
  mqttClient.setURI("mqtt://broker.hivemq.com:1883");
  mqttClient.setMqttClientName("ESP32_Client");
  mqttClient.loopStart(); // Non-blocking!
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
  auto *event = static_cast<esp_mqtt_event_handle_t>(event_data);
  mqttClient.onEventCallback(event);
}
#endif
```

## Migration Guide from PubSubClient

If you're migrating from the popular PubSubClient library:

| PubSubClient | ESP32MQTTClient | Notes |
|-------------|-----------------|-------|
| `client.connect()` | `mqttClient.loopStart()` | Non-blocking, returns immediately |
| `client.loop()` | **Not needed** | Runs automatically in background |
| `client.connected()` | `mqttClient.isConnected()` | Check connection status |
| `client.setServer()` + `setCallback()` | `mqttClient.setURL()` + callbacks | Similar setup pattern |
| `client.setBufferSize()` | `mqttClient.setMaxPacketSize()` | Adjust packet size limits |
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
  mqttClient.loopStart(); // Returns immediately!
}

void loop() {
  // No mqttClient.loop() needed!
}
```

## API Reference

### Configuration Methods
- `setURL(url, port, username, password)` - Set broker connection details
- `setURI(uri, username, password)` - Set complete MQTT URI
- `setMqttClientName(name)` - Set client ID
- `setCaCert(caCert)` - Enable TLS with CA certificate
- `setClientCert(clientCert)` - Set client certificate
- `setKey(clientKey)` - Set client private key
- `setMaxPacketSize(size)` - Set maximum packet size (default: 1024)
- `setKeepAlive(seconds)` - Change keepalive interval (default: 15s)
- `enableLastWillMessage(topic, message, retain)` - Set last will message
- `setAutoReconnect(choice)` - Enable/disable auto-reconnect
- `disableAutoReconnect()` - Disable auto-reconnect
- `enableDebuggingMessages(enabled)` - Enable debug logging

### Lifecycle Methods
- `loopStart()` - Start non-blocking MQTT connection
- `isConnected()` - Check connection status
- `isMyTurn(client)` - Check if event is for this client

### Pub/Sub Methods
- `publish(topic, payload, qos, retain)` → `bool` - Publish message
- `subscribe(topic, callback, qos)` → `bool` - Subscribe with payload callback
- `subscribe(topic, callbackWithTopic, qos)` → `bool` - Subscribe with topic+payload callback
- `unsubscribe(topic)` → `bool` - Unsubscribe from topic
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

The library includes a native ESP-IDF example in the `examples/CppEspIdf` directory. To build it:

1.  **Set up ESP-IDF:** Ensure you have the ESP-IDF environment installed and configured.
2.  **Configure Wi-Fi:** Open `examples/CppEspIdf/main/main.cpp` and set your Wi-Fi SSID and password.
3.  **Build the project:**
    ```bash
    cd examples/CppEspIdf
    idf.py build
    ```
