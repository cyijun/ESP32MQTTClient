#pragma once

#include <vector>
#include <string>
#include <atomic>
#include <mqtt_client.h>
#include <functional>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_idf_version.h" // check IDF version

void onMqttConnect(esp_mqtt_client_handle_t client);
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
esp_err_t handleMQTT(esp_mqtt_event_handle_t event);
#else  // IDF CHECK
/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
void handleMQTT(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
#endif // // IDF CHECK


typedef std::function<void(const std::string &message)> MessageReceivedCallback;
typedef std::function<void(const std::string &topicStr, const std::string &message)> MessageReceivedCallbackWithTopic;

class ESP32MQTTClient
{
private:
    esp_mqtt_client_config_t _mqtt_config; // C so different naming
    esp_mqtt_client_handle_t _mqtt_client = nullptr;
    MessageReceivedCallbackWithTopic _globalMessageReceivedCallback = nullptr;

    // MQTT related
    std::atomic<bool> _mqttConnected{false};
    const char *_mqttUri = nullptr;
    const char *_mqttUsername = nullptr;
    const char *_mqttPassword = nullptr;
    const char *_mqttClientName = nullptr;
    int _disableMQTTCleanSession = 0;
    const char *_mqttLastWillTopic = nullptr;
    const char *_mqttLastWillMessage = nullptr;
    int _mqttLastWillQos = 0;
    bool _mqttLastWillRetain = false;

    int _mqttMaxInPacketSize = DEFAULT_PACKET_SIZE;
    int _mqttMaxOutPacketSize = DEFAULT_PACKET_SIZE;
    char *_mqttUriBuffer = nullptr;  // Buffer for setURL allocated memory

    struct TopicSubscriptionRecord
    {
        std::string topic;
        MessageReceivedCallback callback;
        MessageReceivedCallbackWithTopic callbackWithTopic;
    };
    std::vector<TopicSubscriptionRecord> _topicSubscriptionList;

    // Incoming message fragmentation buffers
    std::string _incomingTopic;
    std::string _incomingPayload;

    // General behaviour related
    bool _enableSerialLogs = false;
    bool _drasticResetOnConnectionFailures = false;

    SemaphoreHandle_t _subscriptionListMutex = nullptr;

public:
    // Constants
    static constexpr uint16_t DEFAULT_PACKET_SIZE = 1024;

    ESP32MQTTClient(/* args */);
    ~ESP32MQTTClient();

    // Non-copyable: the class owns a malloc'd buffer, a mutex and the esp-mqtt handle
    ESP32MQTTClient(const ESP32MQTTClient &) = delete;
    ESP32MQTTClient &operator=(const ESP32MQTTClient &) = delete;

    // Optional functionality
    void enableDebuggingMessages(const bool enabled = true);                                       // Allow to display useful debugging messages. Can be set to false to disable them during program execution
    void disablePersistence();                                                                     // Do not request a persistent session (clean_session = 1, the esp-mqtt default). Must be called before loopStart()
    void enablePersistence();                                                                      // Request a persistent session from the broker (clean_session = 0). Must be called before loopStart()
    void enableLastWillMessage(const char *topic, const char *message, const bool retain = false, int qos = 0); // Must be called before loopStart().
    void enableDrasticResetOnConnectionFailures() { _drasticResetOnConnectionFailures = true; }    // Can be usefull in special cases where the ESP board hang and need resetting (#59)

    void disableAutoReconnect();
    void setTaskPrio(int prio);

    // MQTT runs in a background task once loopStart() has been called, no loop() polling is required

    // MQTT related
	void setClientCert(const char * clientCert);  // Must be called before loopStart()
	void setCaCert(const char * caCert);          // Must be called before loopStart()
	void setKey(const char * clientKey);          // Must be called before loopStart()
    void setOnMessageCallback(MessageReceivedCallbackWithTopic callback);
    void setAutoReconnect(bool choice);
    bool setMaxOutPacketSize(const uint16_t size);
    bool setMaxPacketSize(const uint16_t size); // override the default value of 1024. Must be called before loopStart()
    bool publish(const std::string &topic, const std::string &payload, int qos = 0, bool retain = false);
    // Publish a raw buffer with explicit length, for binary payloads (e.g. Protocol Buffers)
    // without converting to std::string first.
    bool publish(const std::string &topic, const uint8_t *payload, size_t payloadLength, int qos = 0, bool retain = false);
    // Subscribe to a topic. Should be called once the connection is established (i.e. from onMqttConnect);
    // calling it before loopStart() fails because the esp-mqtt client does not exist yet.
    bool subscribe(const std::string &topic, MessageReceivedCallback messageReceivedCallback, uint8_t qos = 0);
    bool subscribe(const std::string &topic, MessageReceivedCallbackWithTopic messageReceivedCallback, uint8_t qos = 0);
    bool unsubscribe(const std::string &topic);                                       // Unsubscribes from the topic, if it exists, and removes it from the CallbackList.
    void setKeepAlive(uint16_t keepAliveSeconds);                                // Change the keepalive interval (default is 120 seconds, the esp-mqtt default). Must be called before loopStart()
    inline void setMqttClientName(const char *name) { _mqttClientName = name; }; // Allow to set client name manually. Must be called before loopStart()
    inline void setURI(const char *uri, const char *username = "", const char *password = "")
    { // Allow setting the MQTT info manually. Must be called before loopStart()
        _mqttUri = uri;
        _mqttUsername = username;
        _mqttPassword = password;
    };

    inline void setURL(const char *url, const uint16_t port, const char *username = "", const char *password = "")
    { // Allow setting the MQTT info manually. Must be called before loopStart()
        // Free previous buffer if exists
        if (_mqttUriBuffer != nullptr) {
            free(_mqttUriBuffer);
            _mqttUriBuffer = nullptr;
            _mqttUri = nullptr;
        }

        if (url == nullptr) {
            if (_enableSerialLogs) {
                ESP_LOGE("ESP32MQTTClient", "URL is null");
            }
            return;
        }

        // Dynamically calculate required space: mqtt(s):// + url + :65535 + \0
        size_t urlLen = strlen(url);
        size_t needed = urlLen + 16;  // Enough for scheme + port + terminator

        _mqttUriBuffer = (char *)malloc(needed);
        if (_mqttUriBuffer == nullptr) {
            if (_enableSerialLogs) {
                ESP_LOGE("ESP32MQTTClient", "Failed to allocate memory for MQTT URI");
            }
            return;
        }

        const char* scheme = (port == 8883) ? "mqtts" : "mqtt";
        snprintf(_mqttUriBuffer, needed, "%s://%s:%u", scheme, url, static_cast<unsigned>(port));

        if (_enableSerialLogs)
        {
            ESP_LOGI("ESP32MQTTClient", "MQTT uri %s", _mqttUriBuffer);
        }
        _mqttUri = _mqttUriBuffer;
        _mqttUsername = username;
        _mqttPassword = password;
    };

    inline bool isConnected() const { return _mqttConnected.load(); };
    inline bool isMyTurn(esp_mqtt_client_handle_t client) const { return _mqtt_client==client; }; // Return true if the given handle is this client's esp-mqtt handle

    inline const char *getClientName() { return _mqttClientName; };
    inline const char *getURI() { return _mqttUri; };

    void printError(esp_mqtt_error_codes_t *error_handle);
    
    bool loopStart();

    void onEventCallback(esp_mqtt_event_handle_t event);
    
private:
    // ESP-IDF 版本适配辅助方法
    void setConfigUri(const char *uri);
    void setConfigClientId(const char *clientId);
    void setConfigUsername(const char *username);
    void setConfigPassword(const char *password);
    void setConfigAutoReconnect(bool disable);
    void setConfigTaskPrio(int prio);
    void setConfigClientCert(const char *cert);
    void setConfigCaCert(const char *cert);
    void setConfigClientKey(const char *key);
    void setConfigKeepAlive(uint16_t seconds);
    void setConfigLwt(const char *topic, const char *msg, int qos, bool retain);
    void setConfigSessionSettings();

    void setConnectionState(bool state);

    bool subscribeInternal(const std::string &topic, MessageReceivedCallback callback,
                           MessageReceivedCallbackWithTopic callbackWithTopic, uint8_t qos);

    void onMessageReceivedCallback(const char *topic, const char *payload, unsigned int length);
    bool mqttTopicMatch(const std::string &topic1, const std::string &topic2);
};
