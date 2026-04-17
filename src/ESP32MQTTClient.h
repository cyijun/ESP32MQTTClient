#pragma once

#include <vector>
#include <string>
#include <mqtt_client.h>
#include <functional>
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
    esp_mqtt_client_handle_t _mqtt_client;
    MessageReceivedCallbackWithTopic _globalMessageReceivedCallback = nullptr;
	

    // MQTT related
    bool _mqttConnected;
    const char *_mqttUri;
    const char *_mqttUsername;
    const char *_mqttPassword;
    const char *_mqttClientName;
    int _disableMQTTCleanSession;
    char *_mqttLastWillTopic;
    char *_mqttLastWillMessage;
    int _mqttLastWillQos;
    bool _mqttLastWillRetain;

    int _mqttMaxInPacketSize;
    int _mqttMaxOutPacketSize;
    char *_mqttUriBuffer;  // Buffer for setURL allocated memory

    struct TopicSubscriptionRecord
    {
        std::string topic;
        MessageReceivedCallback callback;
        MessageReceivedCallbackWithTopic callbackWithTopic;
    };
    std::vector<TopicSubscriptionRecord> _topicSubscriptionList;

    // General behaviour related
    bool _enableSerialLogs;
    bool _drasticResetOnConnectionFailures;

public:
    // Constants
    static constexpr uint16_t DEFAULT_PACKET_SIZE = 1024;

    ESP32MQTTClient(/* args */);
    ~ESP32MQTTClient();

    // Optional functionality
    void enableDebuggingMessages(const bool enabled = true);                                       // Allow to display useful debugging messages. Can be set to false to disable them during program execution
    void disablePersistence();                                                                 // Do not request a persistent connection. Connections are persistent by default. Must be called before the first loop() execution
    void enableLastWillMessage(const char *topic, const char *message, const bool retain = false); // Must be set before the first loop() call.
    void enableDrasticResetOnConnectionFailures() { _drasticResetOnConnectionFailures = true; }    // Can be usefull in special cases where the ESP board hang and need resetting (#59)

    void disableAutoReconnect();
    void setTaskPrio(int prio);

    /// Main loop, to call at each sketch loop()
    //void loop();

    // MQTT related
	void setClientCert(const char * clientCert);
	void setCaCert(const char * caCert);
	void setKey(const char * clientKey);
    void setOnMessageCallback(MessageReceivedCallbackWithTopic callback);
    void setConnectionState(bool state);
    void setAutoReconnect(bool choice);
    bool setMaxOutPacketSize(const uint16_t size);
    bool setMaxPacketSize(const uint16_t size); // override the default value of 1024
    bool publish(const std::string &topic, const std::string &payload, int qos = 0, bool retain = false);
    bool subscribe(const std::string &topic, MessageReceivedCallback messageReceivedCallback, uint8_t qos = 0);
    bool subscribe(const std::string &topic, MessageReceivedCallbackWithTopic messageReceivedCallback, uint8_t qos = 0);
    bool unsubscribe(const std::string &topic);                                       // Unsubscribes from the topic, if it exists, and removes it from the CallbackList.
    void setKeepAlive(uint16_t keepAliveSeconds);                                // Change the keepalive interval (15 seconds by default)
    inline void setMqttClientName(const char *name) { _mqttClientName = name; }; // Allow to set client name manually (must be done in setup(), else it will not work.)
    inline void setURI(const char *uri, const char *username = "", const char *password = "")
    { // Allow setting the MQTT info manually (must be done in setup())
        _mqttUri = uri;
        _mqttUsername = username;
        _mqttPassword = password;
    };

    inline void setURL(const char *url, const uint16_t port, const char *username = "", const char *password = "")
    { // Allow setting the MQTT info manually (must be done in setup())
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
        snprintf(_mqttUriBuffer, needed, "%s://%s:%u", scheme, url, port);

        if (_enableSerialLogs)
        {
            ESP_LOGI("ESP32MQTTClient", "MQTT uri %s", _mqttUriBuffer);
        }
        _mqttUri = _mqttUriBuffer;
        _mqttUsername = username;
        _mqttPassword = password;
    };

    inline bool isConnected() const { return _mqttConnected; };    
    inline bool isMyTurn(esp_mqtt_client_handle_t client) const { return _mqtt_client==client; }; // Return true if mqtt is connected

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

    void onMessageReceivedCallback(const char *topic, char *payload, unsigned int length);
    bool mqttTopicMatch(const std::string &topic1, const std::string &topic2);
};
