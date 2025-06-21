# ESP32MQTTClient

[![Arduino CI](https://github.com/cyijun/ESP32MQTTClient/actions/workflows/ci4main.yml/badge.svg?branch=main)](https://github.com/cyijun/ESP32MQTTClient/actions/workflows/ci4main.yml)
[![ESP-IDF CI](https://github.com/cyijun/ESP32MQTTClient/actions/workflows/esp_idf_ci.yml/badge.svg)](https://github.com/cyijun/ESP32MQTTClient/actions/workflows/esp_idf_ci.yml)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/cyijun/ESP32MQTTClient)

A thread-safe MQTT client for native ESP-IDF or Arduino ESP32. This library is compatible with `arduino-esp32` v2/v3+ and `ESP-IDF` v4.x/v5.x C++.

## Features🦄

- Encapsulated, thread-safe MQTT client based on the official `esp-mqtt` component.
- Uses standard C++ `std::string` instead of Arduino `String`.
- Logging is performed using the standard ESP-IDF `ESP_LOGX` macros.
- Provides both specific topic subscriptions and a global "catch-all" message callback.
- Interfaces inspired by [EspMQTTClient](https://github.com/plapointe6/EspMQTTClient).
- CA cert support by [dwolshin](https://github.com/dwolshin).
- Arduino-esp32 v3+ support by [dzungpv](https://github.com/dzungpv).

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
