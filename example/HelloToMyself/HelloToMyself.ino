#include "Arduino.h"
#include <WiFi.h>
#include "ESP32MQTTClient.h"
const char *ssid = "ssid";
const char *pass = "passwd";

// Test Mosquitto server, see: https://test.mosquitto.org
char *server = "mqtt://foo.bar:1883";

char *subscribeTopic = "foo";
char *publishTopic = "bar/bar";

ESP32MQTTClient mqttClient; //all params are set later

void setup()
{
    // Serial.begin(115200);
    log_i();
    log_i("setup, ESP.getSdkVersion(): ");
    log_i("%s", ESP.getSdkVersion());

    mqttClient.enableDebuggingMessages();

    mqttClient.setMqttUri(server);
    mqttClient.enableLastWillMessage("lwt", "I am going offline");
    mqttClient.setKeepAlive(30);
    WiFi.begin(ssid, pass);
    WiFi.setHostname("c3test");
    mqttClient.connectToMqttBroker();
}

int pubCount = 0;

void loop()
{

    String msg = "Hello: " + String(pubCount++);
    mqttClient.publish(publishTopic, msg, 0, false);
    delay(2000);
}

void onMqttConnect(esp_mqtt_client_handle_t client)
{
    log_i("onMqttConnect");
    mqttClient.setMQTTState(true);

    mqttClient.subscribe(subscribeTopic, [](const String &payload)
                         { log_i("%s: %s", subscribeTopic, payload.c_str()); });

    mqttClient.subscribe("bar/#", [](const String &topic, const String &payload)
                         { log_i("%s: %s", topic, payload.c_str()); });
}

esp_err_t handleMQTT(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t mqtt_client = event->client;
    esp_mqtt_error_codes_t *error_handle = event->error_handle;

    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        onMqttConnect(mqtt_client);
        break;
    case MQTT_EVENT_DATA:
        mqttClient.mqttMessageReceivedCallback(String(event->topic).substring(0, event->topic_len).c_str(), event->data, event->data_len);
        break;
    case MQTT_EVENT_DISCONNECTED:
        mqttClient.setMQTTState(false);
        log_i("disconnected (%fs)", millis() / 1000.0);
        break;
    case MQTT_EVENT_ERROR:
        mqttClient.printError(error_handle);
        break;
    default:
        break;
    }
    return ESP_OK;
}
