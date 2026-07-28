#include <algorithm>
#include <climits>
#include <cstdint>
#include <functional>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wkeyword-macro"
#endif
#define private public
#include "../src/ESP32MQTTClient.h"
#undef private
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace fake
{
int publishResult = 0;
int subscribeResult = 0;
int unsubscribeResult = 0;
int publishCalls = 0;
int subscribeCalls = 0;
int unsubscribeCalls = 0;
const char *lastPublishData = nullptr;
int lastPublishLength = -1;
std::string lastPublishTopic;
std::function<void()> duringSubscribe;
esp_mqtt_client clientStorage{};

void reset()
{
    publishResult = 0;
    subscribeResult = 0;
    unsubscribeResult = 0;
    publishCalls = 0;
    subscribeCalls = 0;
    unsubscribeCalls = 0;
    lastPublishData = nullptr;
    lastPublishLength = -1;
    lastPublishTopic.clear();
    duringSubscribe = nullptr;
}
} // namespace fake

esp_mqtt_client_handle_t esp_mqtt_client_init(const esp_mqtt_client_config_t *)
{
    return &fake::clientStorage;
}

esp_err_t esp_mqtt_client_register_event(esp_mqtt_client_handle_t,
                                         int,
                                         esp_event_handler_t,
                                         void *)
{
    return ESP_OK;
}

esp_err_t esp_mqtt_client_start(esp_mqtt_client_handle_t)
{
    return ESP_OK;
}

esp_err_t esp_mqtt_client_destroy(esp_mqtt_client_handle_t)
{
    return ESP_OK;
}

int esp_mqtt_client_publish(esp_mqtt_client_handle_t,
                            const char *topic,
                            const char *data,
                            int len,
                            int,
                            int)
{
    ++fake::publishCalls;
    fake::lastPublishTopic = topic == nullptr ? "" : topic;
    fake::lastPublishData = data;
    fake::lastPublishLength = len;
    return fake::publishResult;
}

int esp_mqtt_client_subscribe(esp_mqtt_client_handle_t, const char *, int)
{
    ++fake::subscribeCalls;
    if (fake::duringSubscribe)
        fake::duringSubscribe();
    return fake::subscribeResult;
}

int esp_mqtt_client_unsubscribe(esp_mqtt_client_handle_t, const char *)
{
    ++fake::unsubscribeCalls;
    return fake::unsubscribeResult;
}

void esp_restart()
{
}

void onMqttConnect(esp_mqtt_client_handle_t)
{
}

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
esp_err_t handleMQTT(esp_mqtt_event_handle_t)
{
    return ESP_OK;
}
#else
void handleMQTT(void *, esp_event_base_t, std::int32_t, void *)
{
}
#endif

#include "../src/ESP32MQTTClient.cpp"

namespace
{
struct AssertionFailure : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

#define EXPECT_TRUE(value)                                                                           \
    do                                                                                               \
    {                                                                                                \
        if (!(value))                                                                                \
            throw AssertionFailure(std::string("EXPECT_TRUE failed: ") + #value);                    \
    } while (false)

#define EXPECT_FALSE(value) EXPECT_TRUE(!(value))

#define EXPECT_EQ(actual, expected)                                                                  \
    do                                                                                               \
    {                                                                                                \
        const auto actualValue = (actual);                                                           \
        const auto expectedValue = (expected);                                                       \
        if (!(actualValue == expectedValue))                                                         \
            throw AssertionFailure(std::string("EXPECT_EQ failed: ") + #actual + " != " + #expected); \
    } while (false)

void connectClient(ESP32MQTTClient &client)
{
    client._mqtt_client = &fake::clientStorage;
    client.setConnectionState(true);
}

void preventDestructorFromDestroyingFakeClient(ESP32MQTTClient &client)
{
    client._mqtt_client = nullptr;
}

void testRawPublishValidationAndReturnCodes()
{
    fake::reset();
    ESP32MQTTClient client;
    connectClient(client);

    const std::uint8_t binary[] = {'A', 0, 'B'};
    fake::publishResult = 0;
    EXPECT_TRUE(client.publish("binary", binary, sizeof(binary)));
    EXPECT_EQ(fake::publishCalls, 1);
    EXPECT_EQ(fake::lastPublishLength, 3);
    EXPECT_TRUE(fake::lastPublishData == reinterpret_cast<const char *>(binary));

    fake::publishResult = -1;
    EXPECT_FALSE(client.publish("binary", binary, sizeof(binary)));
    fake::publishResult = -2;
    EXPECT_FALSE(client.publish("binary", binary, sizeof(binary)));

    fake::publishResult = 0;
    EXPECT_TRUE(client.publish("empty", binary, 0));
    EXPECT_EQ(fake::lastPublishLength, 0);
    EXPECT_TRUE(fake::lastPublishData == nullptr);

    EXPECT_TRUE(client.publish(
        "empty", static_cast<const std::uint8_t *>(nullptr), static_cast<std::size_t>(0)));
    EXPECT_EQ(fake::lastPublishLength, 0);
    EXPECT_TRUE(fake::lastPublishData == nullptr);

    const int callsBeforeNull = fake::publishCalls;
    EXPECT_FALSE(client.publish(
        "invalid", static_cast<const std::uint8_t *>(nullptr), static_cast<std::size_t>(1)));
    EXPECT_EQ(fake::publishCalls, callsBeforeNull);

    const int callsBeforeOversize = fake::publishCalls;
    EXPECT_FALSE(client.publish("too-large", binary, static_cast<std::size_t>(INT_MAX) + 1U));
    EXPECT_EQ(fake::publishCalls, callsBeforeOversize);

    preventDestructorFromDestroyingFakeClient(client);
}

void testTopicMatchingIncludingSystemTopics()
{
    ESP32MQTTClient client;

    EXPECT_TRUE(client.mqttTopicMatch("#", "sport"));
    EXPECT_TRUE(client.mqttTopicMatch("sport/#", "sport"));
    EXPECT_TRUE(client.mqttTopicMatch("sport/#", "sport/tennis/player"));
    EXPECT_TRUE(client.mqttTopicMatch("sport/+", "sport/tennis"));
    EXPECT_TRUE(client.mqttTopicMatch("sport/+", "sport/"));
    EXPECT_FALSE(client.mqttTopicMatch("sport/+", "sport"));
    EXPECT_FALSE(client.mqttTopicMatch("sport/+", "sport/a/b"));

    EXPECT_FALSE(client.mqttTopicMatch("#", "$SYS/broker/uptime"));
    EXPECT_FALSE(client.mqttTopicMatch("+/broker/uptime", "$SYS/broker/uptime"));
    EXPECT_TRUE(client.mqttTopicMatch("$SYS/#", "$SYS/broker/uptime"));
}

void testSubscribeFailureRollsBackAndRetainedMessageSeesNewCallback()
{
    fake::reset();
    ESP32MQTTClient client;
    connectClient(client);

    int oldCallbackCalls = 0;
    int newCallbackCalls = 0;

    fake::subscribeResult = 1;
    EXPECT_TRUE(client.subscribe("sensor/value", [&](const std::string &) { ++oldCallbackCalls; }));

    // The callback must already be visible while esp_mqtt_client_subscribe() is in
    // progress, otherwise a retained message can be lost.
    fake::duringSubscribe = [&]() {
        client.onMessageReceivedCallback("sensor/new", "retained", 8);
    };
    EXPECT_TRUE(client.subscribe("sensor/new", [&](const std::string &) { ++newCallbackCalls; }));
    EXPECT_EQ(newCallbackCalls, 1);
    fake::duringSubscribe = nullptr;

    // A failed replacement must restore the previous callback.
    fake::subscribeResult = -1;
    EXPECT_FALSE(client.subscribe("sensor/value", [&](const std::string &) { ++newCallbackCalls; }));
    client.onMessageReceivedCallback("sensor/value", "x", 1);
    EXPECT_EQ(oldCallbackCalls, 1);
    EXPECT_EQ(newCallbackCalls, 1);

    fake::subscribeResult = -2;
    EXPECT_FALSE(client.subscribe("sensor/value", [&](const std::string &) { ++newCallbackCalls; }));
    client.onMessageReceivedCallback("sensor/value", "x", 1);
    EXPECT_EQ(oldCallbackCalls, 2);
    EXPECT_EQ(newCallbackCalls, 1);

    preventDestructorFromDestroyingFakeClient(client);
}

void testUnsubscribeFailurePreservesCallback()
{
    fake::reset();
    ESP32MQTTClient client;
    connectClient(client);

    int callbackCalls = 0;
    fake::subscribeResult = 1;
    EXPECT_TRUE(client.subscribe("sensor/value", [&](const std::string &) { ++callbackCalls; }));

    fake::unsubscribeResult = -1;
    EXPECT_FALSE(client.unsubscribe("sensor/value"));
    client.onMessageReceivedCallback("sensor/value", "x", 1);
    EXPECT_EQ(callbackCalls, 1);

    fake::unsubscribeResult = -2;
    EXPECT_FALSE(client.unsubscribe("sensor/value"));
    client.onMessageReceivedCallback("sensor/value", "x", 1);
    EXPECT_EQ(callbackCalls, 2);

    fake::unsubscribeResult = 0;
    EXPECT_TRUE(client.unsubscribe("sensor/value"));
    client.onMessageReceivedCallback("sensor/value", "x", 1);
    EXPECT_EQ(callbackCalls, 2);

    preventDestructorFromDestroyingFakeClient(client);
}

void sendDataEvent(ESP32MQTTClient &client,
                   std::string &topic,
                   std::string &fragment,
                   int offset,
                   int totalLength)
{
    esp_mqtt_event_t event{};
    event.event_id = MQTT_EVENT_DATA;
    event.client = &fake::clientStorage;
    event.topic = topic.empty() ? nullptr : &topic[0];
    event.topic_len = static_cast<int>(topic.size());
    event.data = fragment.empty() ? nullptr : &fragment[0];
    event.data_len = static_cast<int>(fragment.size());
    event.current_data_offset = offset;
    event.total_data_len = totalLength;
    client.onEventCallback(&event);
}

void testFragmentReassemblyAndLimits()
{
    fake::reset();
    ESP32MQTTClient client;
    connectClient(client);

    std::vector<std::pair<std::string, std::string>> received;
    client.setOnMessageCallback(
        [&](const std::string &topic, const std::string &payload) {
            received.emplace_back(topic, payload);
        });

    std::string topic = "fragmented";
    std::string first("A\0B", 3);
    std::string second = "CD";
    sendDataEvent(client, topic, first, 0, 5);
    sendDataEvent(client, topic, second, 3, 5);
    EXPECT_EQ(received.size(), 1U);
    EXPECT_EQ(received[0].first, topic);
    EXPECT_EQ(received[0].second, std::string("A\0BCD", 5));

    std::string boundary(16 * ESP32MQTTClient::DEFAULT_PACKET_SIZE, 'x');
    sendDataEvent(client, topic, boundary, 0, static_cast<int>(boundary.size()));
    EXPECT_EQ(received.size(), 2U);
    EXPECT_EQ(received.back().second.size(), boundary.size());

    std::string overDefault(16 * ESP32MQTTClient::DEFAULT_PACKET_SIZE + 1, 'y');
    sendDataEvent(client, topic, overDefault, 0, static_cast<int>(overDefault.size()));
    EXPECT_EQ(received.size(), 2U);

    std::string invalidLength = "too long";
    sendDataEvent(client, topic, invalidLength, 0, 3);
    EXPECT_EQ(received.size(), 2U);

    std::string firstOutOfOrder = "ab";
    std::string secondOutOfOrder = "cd";
    sendDataEvent(client, topic, firstOutOfOrder, 0, 4);
    sendDataEvent(client, topic, secondOutOfOrder, 1, 4);
    EXPECT_EQ(received.size(), 2U);

    EXPECT_TRUE(client.setMaxPacketSize(32768));
    std::string raisedLimit(20 * 1024, 'z');
    sendDataEvent(client, topic, raisedLimit, 0, static_cast<int>(raisedLimit.size()));
    EXPECT_EQ(received.size(), 3U);
    EXPECT_EQ(received.back().second.size(), raisedLimit.size());

    preventDestructorFromDestroyingFakeClient(client);
}

using Test = std::pair<const char *, void (*)()>;

} // namespace

int main()
{
    const std::vector<Test> tests = {
        {"raw publish validation and return codes", testRawPublishValidationAndReturnCodes},
        {"topic matching including $SYS", testTopicMatchingIncludingSystemTopics},
        {"subscribe rollback and retained race", testSubscribeFailureRollsBackAndRetainedMessageSeesNewCallback},
        {"unsubscribe rollback", testUnsubscribeFailurePreservesCallback},
        {"fragment reassembly and limits", testFragmentReassemblyAndLimits},
    };

    int failures = 0;
    for (const auto &test : tests)
    {
        try
        {
            test.second();
            std::cout << "[PASS] " << test.first << '\n';
        }
        catch (const std::exception &error)
        {
            ++failures;
            std::cerr << "[FAIL] " << test.first << ": " << error.what() << '\n';
        }
    }

    std::cout << tests.size() - failures << "/" << tests.size() << " tests passed\n";
    return failures == 0 ? 0 : 1;
}
