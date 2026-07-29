# ESP32MQTTClient

[English](README.md) | **简体中文**

[![Arduino CI](https://github.com/cyijun/ESP32MQTTClient/actions/workflows/ci4main.yml/badge.svg?branch=main)](https://github.com/cyijun/ESP32MQTTClient/actions/workflows/ci4main.yml)
[![ESP-IDF CI](https://github.com/cyijun/ESP32MQTTClient/actions/workflows/esp_idf_ci.yml/badge.svg)](https://github.com/cyijun/ESP32MQTTClient/actions/workflows/esp_idf_ci.yml)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/cyijun/ESP32MQTTClient)

一个适用于原生 ESP-IDF 和 Arduino ESP32 的线程安全 MQTT 客户端。支持 `arduino-esp32` v2/v3+ 以及 `ESP-IDF` v4.x/v5.x C++ 环境。

## 目录

- [功能特性](#功能特性)
- [非阻塞架构](#非阻塞架构)
- [快速开始](#快速开始)
  - [Arduino 配置](#arduino-配置)
  - [TLS/SSL 示例](#tlsssl-示例)
- [字符串处理](#字符串处理)
- [必须提供的全局回调](#必须提供的全局回调)
- [从 PubSubClient 迁移](#从-pubsubclient-迁移)
- [API 参考](#api-参考)
  - [配置方法](#配置方法)
  - [生命周期方法](#生命周期方法)
  - [发布与订阅方法](#发布与订阅方法)
- [新增功能](#新增功能)
- [构建 ESP-IDF 示例](#构建-esp-idf-示例)

## 功能特性

- **后台连接与事件处理**：`loopStart()` 立即返回，无需调用 `loop()` 轮询
- **线程安全**：基于官方 `esp-mqtt` 组件实现
- **TLS/SSL 支持**：可连接使用 8883 端口的安全 MQTT 服务
- 使用标准 C++ `std::string`，不依赖 Arduino `String`
- 使用标准 ESP-IDF `ESP_LOGX` 宏输出日志
- 同时支持按主题订阅回调和全局消息回调
- 接口设计参考 [EspMQTTClient](https://github.com/plapointe6/EspMQTTClient)
- CA 证书支持由 [dwolshin](https://github.com/dwolshin) 贡献
- Arduino-esp32 v3+ 支持由 [dzungpv](https://github.com/dzungpv) 贡献

## 非阻塞架构

`loopStart()` 会立即返回。连接、重连和 MQTT 事件处理随后由 esp-mqtt 管理的后台任务执行，因此不需要调用 `loop()` 轮询。

这并不表示所有 API 都是非阻塞的。特别是 `publish()` 使用 `esp_mqtt_client_publish()`，等待网络访问或发送分片消息时可能阻塞。消息回调也运行在 MQTT 事件任务中，因此应尽快执行完毕。

## 快速开始

### Arduino 配置

```cpp
#include <WiFi.h>
#include "ESP32MQTTClient.h"

ESP32MQTTClient mqttClient;

void onMqttConnect(esp_mqtt_client_handle_t client) {
  if (mqttClient.isMyTurn(client)) {
    mqttClient.subscribe("test/topic", [](const std::string &payload) {
      Serial.printf("收到消息：%s\n", payload.c_str());
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
  mqttClient.loopStart(); // 立即返回
}

void loop() {
  // 在这里编写其他逻辑，无需调用 mqttClient.loop()
  delay(1000);
}
```

### TLS/SSL 示例

```cpp
// 适用于 HiveMQ Cloud 等 TLS MQTT 服务
const char* caCert = "-----BEGIN CERTIFICATE-----\n...";

void setup() {
  // ... Wi-Fi 配置 ...

  mqttClient.setURL("broker.hivemq.cloud", 8883, "username", "password");
  mqttClient.setCaCert(caCert);  // 启用 TLS 证书校验
  mqttClient.loopStart();
}
```

## 字符串处理

本库使用 `std::string`，而不是 Arduino `String`。两者可以按以下方式转换：

```cpp
// ✅ 正确：直接使用 std::string
std::string topic = "mytopic";
std::string payload = "Hello World";
mqttClient.publish(topic, payload, 0, false);

// ✅ 将 Arduino String 转换为 C 字符串
String arduinoTopic = "mytopic";
String arduinoPayload = "Hello World";
mqttClient.publish(arduinoTopic.c_str(), arduinoPayload.c_str(), 0, false);

// ❌ 这会导致编译错误
// String arduinoTopic = "mytopic";
// mqttClient.publish(arduinoTopic, payload, 0, false);
```

## 必须提供的全局回调

**重要：** 以下回调必须定义为全局函数，不能是类成员方法，也不能直接用 lambda 替代。`subscribe()` 内部的回调仍然可以使用 lambda。

```cpp
// 必须提供：MQTT 连接成功回调
void onMqttConnect(esp_mqtt_client_handle_t client) {
  if (mqttClient.isMyTurn(client)) {
    // 在这里订阅主题
    mqttClient.subscribe("test/topic", [](const std::string &payload) {
      Serial.printf("收到消息：%s\n", payload.c_str());
    });
  }
}

// 必须提供：根据 ESP-IDF 版本选择对应的事件处理函数
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

## 从 PubSubClient 迁移

如果你正在从常用的 PubSubClient 库迁移，可以参考以下对应关系：

| PubSubClient | ESP32MQTTClient | 说明 |
|-------------|-----------------|------|
| `client.connect()` | `mqttClient.loopStart()` | 连接启动后立即返回 |
| `client.loop()` | **不需要** | 自动在后台运行 |
| `client.connected()` | `mqttClient.isConnected()` | 检查连接状态 |
| `client.setServer()` + `setCallback()` | `mqttClient.setURL()` + 回调 | 配置方式相近 |
| `client.setBufferSize()` | `mqttClient.setMaxPacketSize()` | 配置 esp-mqtt 缓冲区大小 |
| Arduino `String` | `std::string` | 使用 `.c_str()` 转换 |

### 迁移示例

**迁移前（PubSubClient）：**

```cpp
PubSubClient client;

void setup() {
  client.setServer("broker", 1883);
  client.setCallback(messageCallback);
}

void loop() {
  if (!client.connected()) {
    client.connect("client"); // 可能阻塞 8 秒以上
  }
  client.loop(); // 必须在 loop() 中调用
}
```

**迁移后（ESP32MQTTClient）：**

```cpp
ESP32MQTTClient mqttClient;

void setup() {
  mqttClient.setURI("mqtt://broker:1883");
  mqttClient.loopStart(); // 连接启动后立即返回
}

void loop() {
  // 不再需要 mqttClient.loop()
}
```

## API 参考

### 配置方法

除非另有说明，配置方法都必须在 `loopStart()` 之前调用。

- `setURL(url, port, username, password)`：设置 MQTT 服务地址、端口和认证信息
- `setURI(uri, username, password)`：设置完整的 MQTT URI
- `setMqttClientName(name)`：设置客户端 ID
- `setCaCert(caCert)`：使用 CA 证书启用 TLS 校验
- `setClientCert(clientCert)`：设置客户端证书
- `setKey(clientKey)`：设置客户端私钥
- `setMaxPacketSize(size)` → `bool`：设置 esp-mqtt 接收和发送缓冲区大小，默认为 1024 字节。完整入站消息默认最多重组 16 KiB；设置值大于 16 KiB 时，重组上限也会提升到指定大小
- `setMaxOutPacketSize(size)` → `bool`：设置 esp-mqtt 发送缓冲区大小，默认为 1024 字节
- `setKeepAlive(seconds)`：设置心跳间隔，默认为 esp-mqtt 的 120 秒，本库不会主动覆盖默认值
- `setTaskPrio(prio)`：设置 MQTT 后台任务优先级
- `enablePersistence()`：请求持久会话，即 `clean_session = 0`
- `disablePersistence()`：禁用持久会话，恢复默认的非持久会话行为
- `enableLastWillMessage(topic, message, retain = false, qos = 0)`：设置遗嘱消息
- `setAutoReconnect(choice)`：启用或禁用自动重连
- `disableAutoReconnect()`：禁用自动重连
- `enableDrasticResetOnConnectionFailures()`：MQTT 连接断开时重启 ESP32（#59）
- `enableDebuggingMessages(enabled)`：启用或禁用调试日志
- `DEFAULT_PACKET_SIZE`：默认缓冲区大小常量，值为 1024 字节，供 `setMaxPacketSize()` 和 `setMaxOutPacketSize()` 使用

### 生命周期方法

- `loopStart()`：启动后台 MQTT 连接流程并立即返回
- `isConnected()`：检查 MQTT 连接状态
- `isMyTurn(client)`：检查事件是否属于当前客户端
- `getClientName()` → `const char *`：获取已配置的客户端名称
- `getURI()` → `const char *`：获取已配置的 MQTT URI
- `printError(error_handle)`：解析并记录 `esp_mqtt_error_codes_t` 错误

### 发布与订阅方法

以下方法返回的 `bool` 表示请求是否已被本地 esp-mqtt 客户端接受，并不表示已经收到 Broker 的 PUBACK、SUBACK 或 UNSUBACK。

如果订阅或退订请求无法提交，本地回调注册状态会恢复到调用前的状态。

- `publish(topic, payload, qos, retain)` → `bool`：提交发布请求。调用可能阻塞；传递完整的 `std::string` 长度，因此能保留载荷中嵌入的 `\0`
- `publish(topic, buffer, length, qos, retain)` → `bool`：使用显式长度提交原始 `uint8_t` 缓冲区，适用于 Protocol Buffers 等二进制载荷，无需先转换为 `std::string`。调用可能阻塞
- `subscribe(topic, callback, qos)` → `bool`：提交订阅请求，并注册仅接收载荷的回调
- `subscribe(topic, callbackWithTopic, qos)` → `bool`：提交订阅请求，并注册同时接收主题和载荷的回调
- `unsubscribe(topic)` → `bool`：提交退订请求
- `setOnMessageCallback(callback)`：设置全局消息处理回调

## 新增功能

### `setOnMessageCallback(MessageReceivedCallbackWithTopic callback)`

设置一个全局回调。无论消息属于哪个主题，收到消息时都会调用该回调，适合集中记录或统一处理全部 MQTT 消息。

**示例：**

```cpp
mqttClient.setOnMessageCallback([](const std::string &topic, const std::string &payload) {
    ESP_LOGI("MAIN", "全局回调：%s: %s", topic.c_str(), payload.c_str());
});
```

### `setAutoReconnect(bool choice)`

启用或禁用底层 ESP-IDF MQTT 客户端的自动重连功能。默认启用自动重连。

**示例：**

```cpp
// 禁用自动重连
mqttClient.setAutoReconnect(false);
```

## 构建 ESP-IDF 示例

本库在 `examples/CppEspIdf` 目录中提供了原生 ESP-IDF 示例。顶层 `CMakeLists.txt` 使用 `idf_component_register()` 将本库注册为 ESP-IDF 组件。

1. **配置 ESP-IDF：** 确保 ESP-IDF 环境已经安装并完成初始化。
2. **配置 Wi-Fi：** 打开 `examples/CppEspIdf/main/main.cpp`，填写 Wi-Fi SSID 和密码。
3. **构建项目：**

   ```bash
   cd examples/CppEspIdf
   idf.py build
   ```
