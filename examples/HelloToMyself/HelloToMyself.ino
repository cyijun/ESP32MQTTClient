#include "Arduino.h"
#include <WiFi.h>
#include "ESP32MQTTClient.h"
const char *ssid = "ssid";
const char *pass = "passwd";

// Test Mosquitto server, see: https://test.mosquitto.org
char *server = "mqtt://foo.bar:1883";

char *subscribeTopic = "foo";
char *publishTopic = "bar/bar";

ESP32MQTTClient mqttClient; // all params are set later

void setup()
{
    // Serial.begin(115200);
    log_i();
    log_i("setup, ESP.getSdkVersion(): ");
    log_i("%s", ESP.getSdkVersion());

    mqttClient.enableDebuggingMessages();

    mqttClient.setURI(server);
    mqttClient.enableLastWillMessage("lwt", "I am going offline");
    mqttClient.setKeepAlive(30);
    WiFi.begin(ssid, pass);
    WiFi.setHostname("c3test");
    mqttClient.loopStart();
}

int pubCount = 0;

void loop()
{

    String msg = "Hello: " + String(pubCount++);
    mqttClient.publish(publishTopic, msg, 0, false);
    delay(2000);
}

void onConnectionEstablishedCallback(esp_mqtt_client_handle_t client)
{
    if (mqttClient.isMyTurn(client)) // can be omitted if only one client
    {
        mqttClient.subscribe(subscribeTopic, [](const String &payload)
                             { log_i("%s: %s", subscribeTopic, payload.c_str()); });

        mqttClient.subscribe("bar/#", [](const String &topic, const String &payload)
                             { log_i("%s: %s", topic, payload.c_str()); });
    }
}

esp_err_t handleMQTT(esp_mqtt_event_handle_t event)
{
    mqttClient.onEventCallback(event);
    return ESP_OK;
}
