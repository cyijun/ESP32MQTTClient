#include "Arduino.h"
#include <WiFi.h>
#include "ESP32MQTTClient.h"
#include "esp_idf_version.h" // check IDF version
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
    mqttClient.setOnMessageCallback([](const std::string &topic, const std::string &payload) {
        log_i("Global callback: %s: %s", topic.c_str(), payload.c_str());
    });
    WiFi.begin(ssid, pass);
    WiFi.setHostname("c3test");
    mqttClient.loopStart();
}

int pubCount = 0;

void loop()
{

    std::string msg = "Hello: " + std::to_string(pubCount++);
    mqttClient.publish(publishTopic, msg, 0, false);
    delay(2000);
}

void onMqttConnect(esp_mqtt_client_handle_t client)
{
    if (mqttClient.isMyTurn(client)) // can be omitted if only one client
    {
        mqttClient.subscribe(subscribeTopic, [](const std::string &payload)
                             { log_i("%s: %s", subscribeTopic, payload.c_str()); });

        mqttClient.subscribe("bar/#", [](const std::string &topic, const std::string &payload)
                             { log_i("%s: %s", topic.c_str(), payload.c_str()); });
    }
}

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
esp_err_t handleMQTT(esp_mqtt_event_handle_t event)
{
    mqttClient.onEventCallback(event);
    return ESP_OK;
}
#else  // IDF CHECK
void handleMQTT(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    auto *event = static_cast<esp_mqtt_event_handle_t>(event_data);
    mqttClient.onEventCallback(event);
}
#endif // // IDF CHECK