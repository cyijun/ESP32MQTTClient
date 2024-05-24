#include "ESP32MQTTClient.h"

ESP32MQTTClient::ESP32MQTTClient(/* args */)
{
    _mqttConnected = false;
    _mqttMaxInPacketSize = 1024;
    _mqttMaxOutPacketSize = _mqttMaxInPacketSize;
    _mqtt_config.event_handle = handleMQTT;
}

ESP32MQTTClient::~ESP32MQTTClient()
{
    esp_mqtt_client_destroy(_mqtt_client);
}

// =============== Configuration functions, most of them must be called before the first loop() call ==============

void ESP32MQTTClient::enableDebuggingMessages(const bool enabled)
{
    _enableSerialLogs = enabled;
}

void ESP32MQTTClient::disablePersistence()
{
    _disableMQTTCleanSession = 1;
}

void ESP32MQTTClient::enableLastWillMessage(const char *topic, const char *message, const bool retain)
{
    _mqttLastWillTopic = (char *)topic;
    _mqttLastWillMessage = (char *)message;
    _mqttLastWillRetain = retain;
}

void ESP32MQTTClient::disableAutoReconnect()
{
    _mqtt_config.disable_auto_reconnect = true;
}

void ESP32MQTTClient::setTaskPrio(int prio)
{
    _mqtt_config.task_prio = prio;
}

void ESP32MQTTClient::setClientCert(const char *clientCert)
{
    _mqtt_config.client_cert_pem = clientCert;
}

void ESP32MQTTClient::setCaCert(const char *caCert)
{
    _mqtt_config.cert_pem = caCert;
}

void ESP32MQTTClient::setKey(const char *clientKey)
{
    _mqtt_config.client_key_pem = clientKey;
}
// =============== Public functions for interaction with thus lib =================

void ESP32MQTTClient::setConnectionState(bool state)
{
    _mqttConnected = state;
}

bool ESP32MQTTClient::setMaxOutPacketSize(const uint16_t size)
{

    _mqttMaxOutPacketSize = size;
    return true;
}

bool ESP32MQTTClient::setMaxPacketSize(const uint16_t size)
{
    _mqttMaxInPacketSize = size;
    _mqttMaxOutPacketSize = _mqttMaxInPacketSize;

    return true;
}

bool ESP32MQTTClient::publish(const char* topic, const char* payload, uint8_t qos, bool retain)
{
    // Do not try to publish if MQTT is not connected.
    if (!isConnected()) //! isConnected())
    {
        if (_enableSerialLogs)
            log_i("MQTT! Trying to publish when disconnected, skipping.");

        return false;
    }

    bool success = false;
    if (esp_mqtt_client_publish(_mqtt_client, topic, payload, 0, qos, retain) != -1)
    {
        success = true;
    }

    if (_enableSerialLogs)
    {
        if (success)
            log_i("MQTT << [%s] %s\n", topic, payload);
        else
            log_i("MQTT! publish failed, is the message too long ? (see setMaxPacketSize())"); // This can occurs if the message is too long according to the maximum defined in PubsubClient.h
    }

    return success;
}

bool ESP32MQTTClient::subscribe(const char* topic, MessageReceivedCallback messageReceivedCallback, uint8_t qos)
{
    bool success = false;
    if (esp_mqtt_client_subscribe(_mqtt_client, topic, qos) != -1)
    {
        success = true;
    }

    if (success)
    {
        // Add the record to the subscription list only if it does not exists.
        bool found = false;
        for (std::size_t i = 0; i < _topicSubscriptionList.size() && !found; i++)
            found = !strcmp(_topicSubscriptionList[i].topic, topic);

        if (!found)
            _topicSubscriptionList.push_back({topic, messageReceivedCallback, NULL});
    }

    if (_enableSerialLogs)
    {
        if (success)
            log_i("MQTT: Subscribed to [%s]\n", topic);
        else
            log_i("MQTT! subscribe failed");
    }

    return success;
}

bool ESP32MQTTClient::subscribe(const char* topic, MessageReceivedCallbackWithTopic messageReceivedCallback, uint8_t qos)
{

    if (subscribe(topic, (MessageReceivedCallback)NULL, qos))
    {
        _topicSubscriptionList[_topicSubscriptionList.size() - 1].callbackWithTopic = messageReceivedCallback;
        return true;
    }

    return false;
}

bool ESP32MQTTClient::unsubscribe(const char* topic)
{

    // Do not try to unsubscribe if MQTT is not connected.
    if (!isConnected())
    {
        if (_enableSerialLogs)
            log_i("MQTT! Trying to unsubscribe when disconnected, skipping.");

        return false;
    }

    for (std::size_t i = 0; i < _topicSubscriptionList.size(); i++)
    {
        if (!strcmp(_topicSubscriptionList[i].topic, topic))
        {
            if (esp_mqtt_client_unsubscribe(_mqtt_client, topic) != -1)
            {
                _topicSubscriptionList.erase(_topicSubscriptionList.begin() + i);
                i--;

                if (_enableSerialLogs)
                    log_i("MQTT: Unsubscribed from %s\n", topic);
            }
            else
            {
                if (_enableSerialLogs)
                    log_i("MQTT! unsubscribe failed");

                return false;
            }
        }
    }

    return true;
}

void ESP32MQTTClient::setKeepAlive(uint16_t keepAliveSeconds)
{
    _mqtt_config.keepalive = keepAliveSeconds;
}

// ================== Private functions ====================-

void ESP32MQTTClient::printError(esp_mqtt_error_codes_t *error_handle)
{
    switch (error_handle->error_type)
    {
    case MQTT_ERROR_TYPE_NONE:
        log_i("ERROR TYPE: %s", "MQTT_ERROR_TYPE_NONE");
        break;
    case MQTT_ERROR_TYPE_TCP_TRANSPORT:
        log_i("ERROR TYPE: %s", "MQTT_ERROR_TYPE_TCP_TRANSPORT");
        switch (error_handle->esp_transport_sock_errno)
        {
        case MQTT_ERROR_TYPE_NONE:
            log_i("ERROR CODE: %s", "MQTT_ERROR_TYPE_NONE");
            break;
        case MQTT_ERROR_TYPE_TCP_TRANSPORT:
            log_i("ERROR CODE: %s", "MQTT_ERROR_TYPE_TCP_TRANSPORT");
            break;
        case MQTT_ERROR_TYPE_CONNECTION_REFUSED:
            log_i("ERROR CODE: %s", "MQTT_ERROR_TYPE_CONNECTION_REFUSED");
            break;

        default:
            log_i("ERROR CODE: %s", "MQTT_TRANSPORT_UNKONW_ERR");
            break;
        }

        break;
    case MQTT_ERROR_TYPE_CONNECTION_REFUSED:
        log_i("ERROR TYPE: %s", "MQTT_ERROR_TYPE_CONNECTION_REFUSED");
        switch (error_handle->connect_return_code)
        {
        case MQTT_CONNECTION_ACCEPTED:
            log_i("ERROR CODE: %s", "MQTT_CONNECTION_ACCEPTED");
            break;
        case MQTT_CONNECTION_REFUSE_PROTOCOL:
            log_i("ERROR CODE: %s", "MQTT_CONNECTION_REFUSE_PROTOCOL");
            break;
        case MQTT_CONNECTION_REFUSE_ID_REJECTED:
            log_i("ERROR CODE: %s", "MQTT_CONNECTION_REFUSE_ID_REJECTED");
            break;
        case MQTT_CONNECTION_REFUSE_SERVER_UNAVAILABLE:
            log_i("ERROR CODE: %s", "MQTT_CONNECTION_REFUSE_SERVER_UNAVAILABLE");
            break;
        case MQTT_CONNECTION_REFUSE_BAD_USERNAME:
            log_i("ERROR CODE: %s", "MQTT_CONNECTION_REFUSE_BAD_USERNAME");
            break;
        case MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED:
            log_i("ERROR CODE: %s", "MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED");
            break;

        default:
            log_i("ERROR CODE: %s", "MQTT_CONNECTION_UNKONW_ERR");
            break;
        }
        break;

    default:
        log_i("ERROR TYPE: %s", "MQTT_ERROR_TYPE_UNKOWN");
        break;
    }
}

// Try to connect to the MQTT broker and return True if the connection is successfull (blocking)
bool ESP32MQTTClient::loopStart()
{
    bool success = false;

    if (_mqttUri != nullptr)
    {
        if (_enableSerialLogs)
        {
            // if (_mqttUsername)
            //     log_i("MQTT: Connecting to broker \"%s\" with client name \"%s\" and username \"%s\" ... (%fs)", _mqttUri, _mqttClientName, _mqttUsername, millis() / 1000.0);
            // else
            //     log_i("MQTT: Connecting to broker \"%s\" with client name \"%s\" ... (%fs)", _mqttUri, _mqttClientName, millis() / 1000.0);
        }

        // explicitly set the server/port here in case they were not provided in the constructor

        _mqtt_config.uri = _mqttUri;
        _mqtt_config.client_id = _mqttClientName;
        _mqtt_config.username = _mqttUsername;
        _mqtt_config.password = _mqttPassword;
        if (_mqttLastWillTopic != nullptr)
        {
            _mqtt_config.lwt_topic = _mqttLastWillTopic;
            _mqtt_config.lwt_msg = _mqttLastWillMessage;
            _mqtt_config.lwt_qos = _mqttLastWillQos;
            _mqtt_config.lwt_retain = _mqttLastWillRetain;
            _mqtt_config.lwt_msg_len = strlen(_mqttLastWillMessage);
        }
        _mqtt_config.disable_clean_session = _disableMQTTCleanSession;
        _mqtt_config.out_buffer_size = _mqttMaxOutPacketSize;
        _mqtt_config.buffer_size = _mqttMaxInPacketSize;

        _mqtt_client = esp_mqtt_client_init(&_mqtt_config);
        esp_mqtt_client_start(_mqtt_client);
    }
    else
    {
        if (_enableSerialLogs)
            log_i("MQTT: Broker server ip is not set, not connecting (%fs)\n", millis() / 1000.0);
        success = false;
    }

    if (_enableSerialLogs)
    {
        if (success)
            log_i(" - ok. (%fs) \n", millis() / 1000.0);
        else
        {

            log_i("MQTT: Retrying to connect in %i seconds.\n", _mqttReconnectionAttemptDelay / 1000);
        }
    }

    return success;
}

/**
 * Matching MQTT topics, handling the eventual presence of a single wildcard character
 *
 * @param topic1 is the topic may contain a wildcard
 * @param topic2 must not contain wildcards
 * @return true on MQTT topic match, false otherwise
 */
bool ESP32MQTTClient::mqttTopicMatch(const char *topic1, const char *topic2)
{
    bool result = false;
    if (strchr(topic1, '#')) // using C string for speed
    {
        std::string topic1Str(topic1);
        std::string topic2Str(topic2);

        int loc1 = topic1Str.find("#", 0);
        std::string t1a = topic1Str.substr(0, loc1);
        std::string t1b = topic1Str.substr(loc1 + 1);
        if ((t1a.length() == 0 || starts_with(topic2Str, t1a)) &&
            (t1b.length() == 0 || ends_with(topic2Str, t1b)))
        {
            result = true;
        }
        // free memory
        delete topic1Str;
        delete topic2Str;
        delete t1a;
        delete t1b;
    }
    else if (strchr(topic1, '+')) // using C string for speed
    {
        std::string topic1Str(topic1);
        std::string topic2Str(topic2);

        int loc2 = topic1Str.find("+", 0);
        std::string t1a = topic1Str.substr(0, loc2);
        std::string t1b = topic1Str.substr(loc2 + 1);

        if ((t1a.length() == 0 || starts_with(topic2Str, t1a)) &&
            (t1b.length() == 0 || ends_with(topic2Str, t1b)))
        {
            std::string t2 = topic2Str.substr(t1a.length(), topic2Str.length() - t1b.length());
            int count = std::count(t2.begin(), t2.end(), '/');
            if (count == 1) // only 1 leve
            {
                result = true;
            }
        }
        // free memory
        delete topic1Str;
        delete topic2Str;
        delete t1a;
        delete t1b;
    }
    else
    {
        result = strcmp(topic1, topic2) == 0;
    }
    return result;
}

void ESP32MQTTClient::onMessageReceivedCallback(const char *topic, unsigned int topic_len, char *payload, unsigned int payload_len)
{
    // Copy payload into message buffer
    char *topicC = (char*)malloc(topic_len + 1);
    memcpy(topicC, topic, topic_len);
    topicC[topic_len] = '\0';
    ESP_LOGE("MQTT DATA", "MQTT Topic: %s", topicC);

       // Copy payload into message buffer
    char *message = (char*)malloc(payload_len + 1);
    memcpy(message, payload, payload_len);
    message[payload_len] = '\0';
    ESP_LOGI("MQTT DATA", "MQTT Msg: %s", message);

    if (payload)
    {
        // Send the message to subscribers
        for (std::size_t i = 0; i < _topicSubscriptionList.size(); i++)
        {
            if (mqttTopicMatch(_topicSubscriptionList[i].topic, topicC))
            {
                // Logging
                if (_enableSerialLogs)
                {
                    ESP_LOGE("MQTT DATA", "MQTT >> [%s]: %s", topicC, message);
                }

                if (_topicSubscriptionList[i].callback != NULL)
                    _topicSubscriptionList[i].callback(message, payload_len); // Call the callback
                if (_topicSubscriptionList[i].callbackWithTopic != NULL)
                    _topicSubscriptionList[i].callbackWithTopic(topicC, topic_len, message, payload_len); // Call the callback
            }
        }
    } else {
        if (_enableSerialLogs)
        {
            log_i("MQTT >> empty payload");
        }
    }
    free(topicC);
    free(message);
}

void ESP32MQTTClient::onEventCallback(esp_mqtt_event_handle_t event)
{
    //_event = &event;
    if (event->client == _mqtt_client)
    {
        switch (event->event_id)
        {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI("ESP32MQTTClient", "MQTT_EVENT_CONNECTED");
            log_i("onMqttConnect");
            setConnectionState(true);
            onMqttConnect(_mqtt_client);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGE("ESP32MQTTClient", "MQTT_EVENT_DATA");
            onMessageReceivedCallback(event->topic, event->topic_len, event->data, event->data_len);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI("ESP32MQTTClient", "MQTT_EVENT_DISCONNECTED");
            setConnectionState(false);
            log_i("%s disconnected (%fs)", _mqttUri, millis() / 1000.0);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI("ESP32MQTTClient", "MQTT_EVENT_ERROR");
            printError(event->error_handle);
            break;
        default:
            break;
        }
    }
}

bool ESP32MQTTClient::ends_with(const std::string & s, const std::string & suffix) {
     return s.rfind(suffix) == s.length() - suffix.length();
}

bool ESP32MQTTClient::starts_with(const std::string & s, const std::string & prefix) {
     return strncmp(s.c_str(), prefix.c_str(), prefix.size()) == 0;
}