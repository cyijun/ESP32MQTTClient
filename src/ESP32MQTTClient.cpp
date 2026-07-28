#include <algorithm>
#include <limits>

#include "esp_system.h"
#include "esp_timer.h"

#include "ESP32MQTTClient.h"

static const char *TAG = "ESP32MQTTClient";

ESP32MQTTClient::ESP32MQTTClient(/* args */)
{
    memset(&_mqtt_config, 0, sizeof(_mqtt_config));
    _subscriptionListMutex = xSemaphoreCreateMutex();
    if (_subscriptionListMutex == nullptr)
        ESP_LOGE(TAG, "Failed to create subscription list mutex");
}

ESP32MQTTClient::~ESP32MQTTClient()
{
    if (_mqtt_client != nullptr) {
        esp_mqtt_client_destroy(_mqtt_client);
        _mqtt_client = nullptr;
    }
    if (_mqttUriBuffer != nullptr) {
        free(_mqttUriBuffer);
        _mqttUriBuffer = nullptr;
    }
    if (_subscriptionListMutex != nullptr) {
        vSemaphoreDelete(_subscriptionListMutex);
        _subscriptionListMutex = nullptr;
    }
}

// =============== Configuration functions, most of them must be called before loopStart() ==============

void ESP32MQTTClient::enableDebuggingMessages(const bool enabled)
{
    _enableSerialLogs.store(enabled, std::memory_order_relaxed);
}

void ESP32MQTTClient::disablePersistence()
{
    // clean_session = 1: the broker discards the session on disconnect (esp-mqtt default)
    _disableMQTTCleanSession = 0;
}

void ESP32MQTTClient::enablePersistence()
{
    // clean_session = 0: request a persistent session from the broker
    _disableMQTTCleanSession = 1;
}

void ESP32MQTTClient::enableLastWillMessage(const char *topic, const char *message, const bool retain, int qos)
{
    if (topic == nullptr || !isValidPublishTopic(topic))
    {
        ESP_LOGE(TAG, "Last will topic is invalid, ignoring");
        return;
    }

    if (message == nullptr)
    {
        ESP_LOGE(TAG, "Last will message is null, ignoring");
        return;
    }

    if (qos < 0 || qos > 2)
    {
        ESP_LOGE(TAG, "Last will QoS must be between 0 and 2, ignoring");
        return;
    }

    _mqttLastWillTopic = topic;
    _mqttLastWillMessage = message;
    _mqttLastWillRetain = retain;
    _mqttLastWillQos = qos;
}

void ESP32MQTTClient::disableAutoReconnect()
{
    setConfigAutoReconnect(true);
}

void ESP32MQTTClient::setTaskPrio(int prio)
{
    setConfigTaskPrio(prio);
}

void ESP32MQTTClient::setClientCert(const char *clientCert)
{
    setConfigClientCert(clientCert);
}

void ESP32MQTTClient::setCaCert(const char *caCert)
{
    setConfigCaCert(caCert);
}

void ESP32MQTTClient::setKey(const char *clientKey)
{
    setConfigClientKey(clientKey);
}
// =============== Public functions for interaction with thus lib =================

void ESP32MQTTClient::setOnMessageCallback(MessageReceivedCallbackWithTopic callback)
{
    if (_subscriptionListMutex != nullptr)
        xSemaphoreTake(_subscriptionListMutex, portMAX_DELAY);

    _globalMessageReceivedCallback = callback;

    if (_subscriptionListMutex != nullptr)
        xSemaphoreGive(_subscriptionListMutex);
}

void ESP32MQTTClient::setConnectionState(bool state)
{
    _mqttConnected = state;
}

void ESP32MQTTClient::setAutoReconnect(bool choice)
{
    setConfigAutoReconnect(!choice);
}

bool ESP32MQTTClient::setMaxOutPacketSize(const uint16_t size)
{
    if (size == 0)
        return false;

    _mqttMaxOutPacketSize = size;
    return true;
}

bool ESP32MQTTClient::setMaxPacketSize(const uint16_t size)
{
    if (size == 0)
        return false;

    _mqttMaxInPacketSize = size;
    _mqttMaxOutPacketSize = _mqttMaxInPacketSize;
    _mqttMaxIncomingMessageSize = std::max<std::size_t>(
        16 * DEFAULT_PACKET_SIZE, static_cast<std::size_t>(size));

    return true;
}

bool ESP32MQTTClient::publish(const std::string &topic, const std::string &payload, int qos, bool retain)
{
    return publish(topic, reinterpret_cast<const uint8_t *>(payload.data()), payload.length(), qos, retain);
}

bool ESP32MQTTClient::publish(const std::string &topic, const uint8_t *payload, size_t payloadLength, int qos, bool retain)
{
    if (!isValidPublishTopic(topic))
    {
        if (_enableSerialLogs)
            ESP_LOGW(TAG, "Publish topic is invalid");
        return false;
    }

    if (qos < 0 || qos > 2)
    {
        if (_enableSerialLogs)
            ESP_LOGW(TAG, "Publish QoS must be between 0 and 2");
        return false;
    }

    if ((payload == nullptr && payloadLength > 0) ||
        payloadLength > static_cast<size_t>(std::numeric_limits<int>::max()))
    {
        if (_enableSerialLogs)
            ESP_LOGW(TAG, "Publish payload pointer or length is invalid");
        return false;
    }

    // Do not try to publish if MQTT is not connected.
    if (!isConnected())
    {
        if (_enableSerialLogs)
            ESP_LOGI(TAG, "Trying to publish when disconnected, skipping.");

        return false;
    }

    // esp_mqtt_client_publish() interprets len == 0 with non-null data as a
    // null-terminated string. Pass nullptr for an explicitly empty binary
    // payload so the raw-buffer overload never reads past the supplied range.
    const char *payloadData =
        payloadLength == 0 ? nullptr : reinterpret_cast<const char *>(payload);
    const int messageId = esp_mqtt_client_publish(
        _mqtt_client, topic.c_str(), payloadData,
        static_cast<int>(payloadLength), qos, retain);
    const bool success = messageId >= 0;

    if (_enableSerialLogs)
    {
        if (success)
            ESP_LOGI(TAG, "MQTT << [%s] (%u bytes)", topic.c_str(), (unsigned int)payloadLength);
        else
            ESP_LOGW(TAG, "Publish failed (error %d)", messageId);
    }

    return success;
}

bool ESP32MQTTClient::subscribe(const std::string &topic, MessageReceivedCallback messageReceivedCallback, uint8_t qos)
{
    return subscribeInternal(topic, messageReceivedCallback, nullptr, qos);
}

bool ESP32MQTTClient::subscribe(const std::string &topic, MessageReceivedCallbackWithTopic messageReceivedCallback, uint8_t qos)
{
    return subscribeInternal(topic, nullptr, messageReceivedCallback, qos);
}

bool ESP32MQTTClient::subscribeInternal(const std::string &topic, MessageReceivedCallback callback,
                                        MessageReceivedCallbackWithTopic callbackWithTopic, uint8_t qos)
{
    if (_mqtt_client == nullptr || !isConnected())
    {
        if (_enableSerialLogs)
            ESP_LOGW(TAG, "Trying to subscribe when disconnected, skipping.");
        return false;
    }

    if (!isValidTopicFilter(topic) || qos > 2)
    {
        if (_enableSerialLogs)
            ESP_LOGW(TAG, "Subscription topic filter or QoS is invalid");
        return false;
    }

    if (_subscriptionListMutex == nullptr)
    {
        ESP_LOGE(TAG, "Subscription list mutex is not available");
        return false;
    }

    TopicSubscriptionRecord previousRecord{};
    bool hadPreviousRecord = false;
    uint32_t operationGeneration = 0;

    xSemaphoreTake(_subscriptionListMutex, portMAX_DELAY);

    ++_subscriptionGeneration;
    if (_subscriptionGeneration == 0)
        ++_subscriptionGeneration;
    operationGeneration = _subscriptionGeneration;

    bool found = false;
    for (auto &record : _topicSubscriptionList)
    {
        if (record.topic == topic)
        {
            previousRecord = record;
            hadPreviousRecord = true;
            found = true;
            if (callback != nullptr)
                record.callback = callback;
            if (callbackWithTopic != nullptr)
                record.callbackWithTopic = callbackWithTopic;
            record.generation = operationGeneration;
            break;
        }
    }

    if (!found)
        _topicSubscriptionList.push_back(
            {topic, callback, callbackWithTopic, operationGeneration});

    xSemaphoreGive(_subscriptionListMutex);

    // Install the local callback before sending SUBSCRIBE so a retained
    // message cannot race ahead of callback registration when called from a
    // user task. Roll it back if the request cannot be sent.
    const int messageId = esp_mqtt_client_subscribe(_mqtt_client, topic.c_str(), qos);
    const bool success = messageId >= 0;

    if (!success)
    {
        xSemaphoreTake(_subscriptionListMutex, portMAX_DELAY);

        for (auto it = _topicSubscriptionList.begin();
             it != _topicSubscriptionList.end(); ++it)
        {
            if (it->topic == topic && it->generation == operationGeneration)
            {
                if (hadPreviousRecord)
                    *it = previousRecord;
                else
                    _topicSubscriptionList.erase(it);
                break;
            }
        }

        xSemaphoreGive(_subscriptionListMutex);
    }

    if (_enableSerialLogs)
    {
        if (success)
            ESP_LOGI(TAG, "MQTT: Subscribed to [%s]", topic.c_str());
        else
            ESP_LOGW(TAG, "MQTT! subscribe failed (error %d)", messageId);
    }

    return success;
}

bool ESP32MQTTClient::unsubscribe(const std::string &topic)
{
    // Do not try to unsubscribe if MQTT is not connected.
    if (!isConnected())
    {
        if (_enableSerialLogs)
            ESP_LOGW(TAG, "Trying to unsubscribe when disconnected, skipping.");

        return false;
    }

    if (_subscriptionListMutex == nullptr)
    {
        ESP_LOGE(TAG, "Subscription list mutex is not available");
        return false;
    }

    bool found = false;
    uint32_t recordGeneration = 0;

    xSemaphoreTake(_subscriptionListMutex, portMAX_DELAY);

    for (const auto &record : _topicSubscriptionList)
    {
        if (record.topic == topic)
        {
            found = true;
            recordGeneration = record.generation;
            break;
        }
    }

    xSemaphoreGive(_subscriptionListMutex);

    if (!found)
        return false;

    // Only remove the local callback once the UNSUBSCRIBE request has been
    // sent successfully. A failed request leaves the observable local state
    // unchanged.
    const int messageId = esp_mqtt_client_unsubscribe(_mqtt_client, topic.c_str());
    if (messageId >= 0)
    {
        xSemaphoreTake(_subscriptionListMutex, portMAX_DELAY);

        for (auto it = _topicSubscriptionList.begin();
             it != _topicSubscriptionList.end(); ++it)
        {
            if (it->topic == topic && it->generation == recordGeneration)
            {
                _topicSubscriptionList.erase(it);
                break;
            }
        }

        xSemaphoreGive(_subscriptionListMutex);

        if (_enableSerialLogs)
            ESP_LOGI(TAG, "MQTT: Unsubscribed from %s", topic.c_str());

        return true;
    }

    if (_enableSerialLogs)
        ESP_LOGW(TAG, "MQTT! unsubscribe failed (error %d)", messageId);

    return false;
}

void ESP32MQTTClient::setKeepAlive(uint16_t keepAliveSeconds)
{
    setConfigKeepAlive(keepAliveSeconds);
}

// ================== Private functions ====================-

void ESP32MQTTClient::printError(esp_mqtt_error_codes_t *error_handle)
{
    if (error_handle == nullptr)
        return;

    const char *error_type_str = "UNKNOWN";
    switch (error_handle->error_type)
    {
    case MQTT_ERROR_TYPE_NONE:
        error_type_str = "NONE";
        break;
    case MQTT_ERROR_TYPE_TCP_TRANSPORT:
        error_type_str = "TCP_TRANSPORT";
        break;
    case MQTT_ERROR_TYPE_CONNECTION_REFUSED:
        error_type_str = "CONNECTION_REFUSED";
        break;
    default:
        break;
    }

    ESP_LOGE(TAG, "Error type: %s (%d)", error_type_str, (int)error_handle->error_type);

    if (error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
    {
        ESP_LOGE(TAG, "ESP transport socket errno: %d", error_handle->esp_transport_sock_errno);
    }
    else if (error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED)
    {
        ESP_LOGE(TAG, "MQTT connect return code: %d", (int)error_handle->connect_return_code);
    }
}

// ================== ESP-IDF Version Adaptation Helper Methods ====================

void ESP32MQTTClient::setConfigUri(const char *uri)
{
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    _mqtt_config.uri = uri;
#else
    _mqtt_config.broker.address.uri = uri;
#endif
}

void ESP32MQTTClient::setConfigClientId(const char *clientId)
{
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    _mqtt_config.client_id = clientId;
#else
    _mqtt_config.credentials.client_id = clientId;
#endif
}

void ESP32MQTTClient::setConfigUsername(const char *username)
{
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    _mqtt_config.username = username;
#else
    _mqtt_config.credentials.username = username;
#endif
}

void ESP32MQTTClient::setConfigPassword(const char *password)
{
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    _mqtt_config.password = password;
#else
    _mqtt_config.credentials.authentication.password = password;
#endif
}

void ESP32MQTTClient::setConfigAutoReconnect(bool disable)
{
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    _mqtt_config.disable_auto_reconnect = disable;
#else
    _mqtt_config.network.disable_auto_reconnect = disable;
#endif
}

void ESP32MQTTClient::setConfigTaskPrio(int prio)
{
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    _mqtt_config.task_prio = prio;
#else
    _mqtt_config.task.priority = prio;
#endif
}

void ESP32MQTTClient::setConfigClientCert(const char *cert)
{
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    _mqtt_config.client_cert_pem = cert;
#else
    _mqtt_config.credentials.authentication.certificate = cert;
#endif
}

void ESP32MQTTClient::setConfigCaCert(const char *cert)
{
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    _mqtt_config.cert_pem = cert;
#else
    _mqtt_config.broker.verification.certificate = cert;
#endif
}

void ESP32MQTTClient::setConfigClientKey(const char *key)
{
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    _mqtt_config.client_key_pem = key;
#else
    _mqtt_config.credentials.authentication.key = key;
#endif
}

void ESP32MQTTClient::setConfigKeepAlive(uint16_t seconds)
{
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    _mqtt_config.keepalive = seconds;
#else
    _mqtt_config.session.keepalive = seconds;
#endif
}

void ESP32MQTTClient::setConfigLwt(const char *topic, const char *msg, int qos, bool retain)
{
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    _mqtt_config.lwt_topic = topic;
    _mqtt_config.lwt_msg = msg;
    _mqtt_config.lwt_qos = qos;
    _mqtt_config.lwt_retain = retain;
    _mqtt_config.lwt_msg_len = strlen(msg);
#else
    _mqtt_config.session.last_will.topic = topic;
    _mqtt_config.session.last_will.msg = msg;
    _mqtt_config.session.last_will.qos = qos;
    _mqtt_config.session.last_will.retain = retain;
    _mqtt_config.session.last_will.msg_len = strlen(msg);
#endif
}

void ESP32MQTTClient::setConfigSessionSettings()
{
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    _mqtt_config.disable_clean_session = _disableMQTTCleanSession;
    _mqtt_config.out_buffer_size = _mqttMaxOutPacketSize;
    _mqtt_config.buffer_size = _mqttMaxInPacketSize;
#else
    _mqtt_config.session.disable_clean_session = _disableMQTTCleanSession;
    _mqtt_config.buffer.out_size = _mqttMaxOutPacketSize;
    _mqtt_config.buffer.size = _mqttMaxInPacketSize;
#endif
}

// Start the MQTT client and return true if esp_mqtt_client_start() succeeded.
// Non-blocking: the connection itself is established asynchronously by the esp-mqtt task.
bool ESP32MQTTClient::loopStart()
{
    if (_mqtt_client != nullptr)
    {
        if (_enableSerialLogs)
            ESP_LOGW(TAG, "loopStart() called while MQTT client is already initialized");
        return false;
    }

    bool success = false;
    esp_err_t err = ESP_OK;

    if (_mqttUri != nullptr)
    {
        if (_enableSerialLogs)
        {
            if (_mqttUsername)
                ESP_LOGW(TAG, "Connecting to broker %s with client name %s and username %s ... (%lus)", _mqttUri, (_mqttClientName ? _mqttClientName : ""), _mqttUsername, (unsigned long)(esp_timer_get_time() / 1000000));
            else
                ESP_LOGW(TAG, "Connecting to broker %s with client name %s ... (%lus)", _mqttUri, (_mqttClientName ? _mqttClientName : ""), (unsigned long)(esp_timer_get_time() / 1000000));
        }

        // explicitly set the server/port here in case they were not provided in the constructor
        // Use helper methods to set configuration
        setConfigUri(_mqttUri);
        setConfigClientId(_mqttClientName);
        setConfigUsername(_mqttUsername);
        setConfigPassword(_mqttPassword);
        
        if (_mqttLastWillTopic != nullptr)
        {
            setConfigLwt(_mqttLastWillTopic, _mqttLastWillMessage, 
                         _mqttLastWillQos, _mqttLastWillRetain);
        }
        
        setConfigSessionSettings();

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
        // IDF 4.x
        _mqtt_config.event_handle = handleMQTT;
        _mqtt_client = esp_mqtt_client_init(&_mqtt_config);
#else  // IDF CHECK
        // IDF 5.x
        _mqtt_client = esp_mqtt_client_init(&_mqtt_config);
        if (_mqtt_client != nullptr)
            err = esp_mqtt_client_register_event(_mqtt_client, MQTT_EVENT_ANY, handleMQTT, this);
#endif // IDF CHECK
        if (_mqtt_client == nullptr)
            err = ESP_FAIL;

        if (_mqtt_client != nullptr && err == ESP_OK)
        {
            err = esp_mqtt_client_start(_mqtt_client);
            success = (err == ESP_OK);
        }
        else
        {
            success = false;
        }

        if (!success && _mqtt_client != nullptr)
        {
            // Destroy the client so a later loopStart() call can retry from a clean state
            esp_mqtt_client_destroy(_mqtt_client);
            _mqtt_client = nullptr;
        }
    }
    else
    {
        if (_enableSerialLogs)
            ESP_LOGW(TAG, "Broker server ip is not set, not connecting (%lus)", (unsigned long)(esp_timer_get_time() / 1000000));
        success = false;
    }

    if (_enableSerialLogs)
    {
        if (success)
            ESP_LOGI(TAG, "Connection ok. (%lus)", (unsigned long)(esp_timer_get_time() / 1000000));
        else
        {

            ESP_LOGE(TAG, "Connection failed, error code: %d", err);
        }
    }

    return success;
}

bool ESP32MQTTClient::isValidPublishTopic(const std::string &topic) const
{
    return !topic.empty() &&
           topic.size() <= std::numeric_limits<uint16_t>::max() &&
           topic.find('\0') == std::string::npos &&
           topic.find('#') == std::string::npos &&
           topic.find('+') == std::string::npos;
}

bool ESP32MQTTClient::isValidTopicFilter(const std::string &topicFilter) const
{
    if (topicFilter.empty() ||
        topicFilter.size() > std::numeric_limits<uint16_t>::max() ||
        topicFilter.find('\0') != std::string::npos)
    {
        return false;
    }

    size_t position = 0;
    while (position <= topicFilter.size())
    {
        const size_t end = topicFilter.find('/', position);
        const std::string level = topicFilter.substr(
            position, end == std::string::npos
                          ? std::string::npos
                          : end - position);

        if (level.find('#') != std::string::npos &&
            (level != "#" || end != std::string::npos))
        {
            return false;
        }

        if (level.find('+') != std::string::npos && level != "+")
            return false;

        if (end == std::string::npos)
            break;

        position = end + 1;
    }

    return true;
}

/**
 * Match an MQTT topic filter against a concrete topic, level by level ('/' separated).
 *
 * '#' matches the parent level itself plus any number of remaining levels
 * (e.g. "sport/#" matches "sport", "sport/x" and "sport/x/y").
 * '+' matches exactly one level, which may be empty (e.g. "sport/+" matches "sport/"
 * but not "sport" nor "sport/x/y").
 *
 * @param topic1 is the topic filter, may contain wildcards
 * @param topic2 must not contain wildcards
 * @return true on MQTT topic match, false otherwise
 */
bool ESP32MQTTClient::mqttTopicMatch(const std::string &topic1, const std::string &topic2)
{
    if (!isValidTopicFilter(topic1) || !isValidPublishTopic(topic2))
        return false;

    // MQTT-4.7.2-1: a filter beginning with a wildcard must not match
    // topic names beginning with '$' (for example, '#' must not match
    // '$SYS/broker/uptime').
    if (topic2[0] == '$' && (topic1[0] == '#' || topic1[0] == '+'))
        return false;

    size_t pos1 = 0;
    size_t pos2 = 0;

    while (pos1 <= topic1.size())
    {
        size_t end1 = topic1.find('/', pos1);
        std::string level1 = topic1.substr(pos1, end1 == std::string::npos ? std::string::npos : end1 - pos1);

        // '#' matches the parent level itself and any remaining levels
        if (level1 == "#")
            return true;

        // The filter still expects a level but the topic has none left
        if (pos2 > topic2.size())
            return false;

        size_t end2 = topic2.find('/', pos2);
        std::string level2 = topic2.substr(pos2, end2 == std::string::npos ? std::string::npos : end2 - pos2);

        // '+' matches exactly one level, anything else must be equal
        if (level1 != "+" && level1 != level2)
            return false;

        if (end1 == std::string::npos)
            // Last filter level consumed: match only if the topic has no extra levels
            return end2 == std::string::npos;

        pos1 = end1 + 1;
        pos2 = (end2 == std::string::npos) ? topic2.size() + 1 : end2 + 1;
    }

    return true;
}

void ESP32MQTTClient::onMessageReceivedCallback(const char *topic, const char *payload, unsigned int length)
{
    // Create a copy of the payload, don't modify the original buffer
    std::string payloadStr;
    if (payload != nullptr && length > 0)
        payloadStr.assign(payload, length);

    std::string topicStr(topic ? topic : "");

    // Logging
    if (_enableSerialLogs)
        ESP_LOGI(TAG, "MQTT >> [%s] %s", topicStr.c_str(), payloadStr.c_str());

    // Collect callbacks under mutex protection, then invoke without the lock
    // to avoid deadlocks if a callback re-enters subscribe/unsubscribe.
    MessageReceivedCallbackWithTopic globalCallback = nullptr;
    std::vector<TopicSubscriptionRecord> matchedSubscriptions;

    if (_subscriptionListMutex != nullptr)
        xSemaphoreTake(_subscriptionListMutex, portMAX_DELAY);

    globalCallback = _globalMessageReceivedCallback;

    for (const auto &record : _topicSubscriptionList)
    {
        if (mqttTopicMatch(record.topic, topicStr))
            matchedSubscriptions.push_back(record);
    }

    if (_subscriptionListMutex != nullptr)
        xSemaphoreGive(_subscriptionListMutex);

    // Call global callback
    if (globalCallback != nullptr)
        globalCallback(topicStr, payloadStr);

    // Send the message to subscribers
    for (const auto &record : matchedSubscriptions)
    {
        if (record.callback != nullptr)
            record.callback(payloadStr);
        if (record.callbackWithTopic != nullptr)
            record.callbackWithTopic(topicStr, payloadStr);
    }
}

void ESP32MQTTClient::onEventCallback(esp_mqtt_event_handle_t event)
{
    if (event == nullptr)
    {
        ESP_LOGE(TAG, "MQTT event is null");
        return;
    }

    if (event->client == _mqtt_client)
    {
        switch (event->event_id)
        {
        case MQTT_EVENT_CONNECTED:
            if (_enableSerialLogs)
                ESP_LOGI(TAG, "MQTT -->> onMqttConnect");
            setConnectionState(true);
            onMqttConnect(_mqtt_client);
            break;
        case MQTT_EVENT_DATA:
            if (_enableSerialLogs)
                ESP_LOGI(TAG, "MQTT -->> onMqttEventData");
            {
                // total_data_len is present in every fragment event: reject oversized
                // messages up front and drop all their fragments.
                if (event->total_data_len < 0 ||
                    static_cast<std::size_t>(event->total_data_len) >
                        _mqttMaxIncomingMessageSize)
                {
                    _incomingTopic.clear();
                    _incomingPayload.clear();
                    ESP_LOGW(TAG,
                             "MQTT message size is invalid or too large "
                             "(%d bytes, max %u), dropping it",
                             event->total_data_len,
                             static_cast<unsigned int>(
                                 _mqttMaxIncomingMessageSize));
                    break;
                }

                const bool invalidFragment =
                    event->current_data_offset < 0 ||
                    event->data_len < 0 ||
                    event->topic_len < 0 ||
                    event->current_data_offset > event->total_data_len ||
                    event->data_len >
                        event->total_data_len - event->current_data_offset ||
                    (event->data_len > 0 && event->data == nullptr) ||
                    (event->topic_len > 0 && event->topic == nullptr);

                if (invalidFragment)
                {
                    _incomingTopic.clear();
                    _incomingPayload.clear();
                    ESP_LOGW(TAG, "MQTT message fragment metadata is invalid, dropping it");
                    break;
                }

                // Start of a new message: first fragment, topic is available
                if (event->current_data_offset == 0)
                {
                    if (event->topic != nullptr && event->topic_len > 0)
                        _incomingTopic.assign(event->topic, event->topic_len);
                    else
                        _incomingTopic.clear();

                    _incomingPayload.clear();
                    if (event->total_data_len > 0)
                        _incomingPayload.reserve(event->total_data_len);
                }
                else if (_incomingPayload.size() !=
                         static_cast<std::size_t>(event->current_data_offset))
                {
                    _incomingTopic.clear();
                    _incomingPayload.clear();
                    ESP_LOGW(TAG, "MQTT message fragments are out of order, dropping them");
                    break;
                }

                if (event->data != nullptr && event->data_len > 0)
                    _incomingPayload.append(event->data, event->data_len);

                // Last fragment received: dispatch the complete message
                if (event->current_data_offset + event->data_len >= event->total_data_len)
                {
                    onMessageReceivedCallback(_incomingTopic.c_str(),
                                              _incomingPayload.data(),
                                              static_cast<unsigned int>(_incomingPayload.size()));
                    _incomingTopic.clear();
                    _incomingPayload.clear();
                }
            }
            break;
        case MQTT_EVENT_DISCONNECTED:
            if (_enableSerialLogs)
                ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            setConnectionState(false);
            if (_enableSerialLogs)
                ESP_LOGW(TAG, "MQTT -->> %s disconnected (%lus)", _mqttUri, (unsigned long)(esp_timer_get_time() / 1000000));

            if (_drasticResetOnConnectionFailures.load(std::memory_order_relaxed)) {
                ESP_LOGW(TAG, "Drastic reset triggered due to connection failure");
                esp_restart();
            }
            break;
        case MQTT_EVENT_ERROR:
            if (_enableSerialLogs)
                ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            printError(event->error_handle);
            break;
        default:
            break;
        }
    }
}
