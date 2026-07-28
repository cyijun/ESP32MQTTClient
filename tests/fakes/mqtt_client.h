#pragma once

#include <cstdint>

#include "esp_idf_version.h"

using esp_err_t = int;
using esp_event_base_t = const char *;

constexpr esp_err_t ESP_OK = 0;
constexpr esp_err_t ESP_FAIL = -1;
constexpr int MQTT_EVENT_ANY = -1;

enum esp_mqtt_event_id_t
{
    MQTT_EVENT_CONNECTED = 0,
    MQTT_EVENT_DISCONNECTED,
    MQTT_EVENT_SUBSCRIBED,
    MQTT_EVENT_UNSUBSCRIBED,
    MQTT_EVENT_PUBLISHED,
    MQTT_EVENT_DATA,
    MQTT_EVENT_ERROR
};

enum esp_mqtt_error_type_t
{
    MQTT_ERROR_TYPE_NONE = 0,
    MQTT_ERROR_TYPE_TCP_TRANSPORT,
    MQTT_ERROR_TYPE_CONNECTION_REFUSED
};

struct esp_mqtt_error_codes_t
{
    esp_mqtt_error_type_t error_type = MQTT_ERROR_TYPE_NONE;
    int esp_transport_sock_errno = 0;
    int connect_return_code = 0;
};

struct esp_mqtt_client
{
    int unused;
};

using esp_mqtt_client_handle_t = esp_mqtt_client *;

struct esp_mqtt_event_t
{
    esp_mqtt_event_id_t event_id = MQTT_EVENT_CONNECTED;
    esp_mqtt_client_handle_t client = nullptr;
    char *topic = nullptr;
    int topic_len = 0;
    char *data = nullptr;
    int data_len = 0;
    int current_data_offset = 0;
    int total_data_len = 0;
    esp_mqtt_error_codes_t *error_handle = nullptr;
};

using esp_mqtt_event_handle_t = esp_mqtt_event_t *;
using esp_event_handler_t = void (*)(void *, esp_event_base_t, std::int32_t, void *);

struct esp_mqtt_client_config_t
{
    const char *uri = nullptr;
    const char *client_id = nullptr;
    const char *username = nullptr;
    const char *password = nullptr;
    bool disable_auto_reconnect = false;
    int task_prio = 0;
    const char *client_cert_pem = nullptr;
    const char *cert_pem = nullptr;
    const char *client_key_pem = nullptr;
    std::uint16_t keepalive = 0;
    const char *lwt_topic = nullptr;
    const char *lwt_msg = nullptr;
    int lwt_qos = 0;
    bool lwt_retain = false;
    int lwt_msg_len = 0;
    int disable_clean_session = 0;
    int out_buffer_size = 0;
    int buffer_size = 0;
    esp_err_t (*event_handle)(esp_mqtt_event_handle_t) = nullptr;

    struct
    {
        struct
        {
            const char *uri = nullptr;
        } address;
        struct
        {
            const char *certificate = nullptr;
        } verification;
    } broker;

    struct
    {
        const char *client_id = nullptr;
        const char *username = nullptr;
        struct
        {
            const char *password = nullptr;
            const char *certificate = nullptr;
            const char *key = nullptr;
        } authentication;
    } credentials;

    struct
    {
        bool disable_auto_reconnect = false;
    } network;

    struct
    {
        int priority = 0;
    } task;

    struct
    {
        std::uint16_t keepalive = 0;
        int disable_clean_session = 0;
        struct
        {
            const char *topic = nullptr;
            const char *msg = nullptr;
            int qos = 0;
            bool retain = false;
            int msg_len = 0;
        } last_will;
    } session;

    struct
    {
        int out_size = 0;
        int size = 0;
    } buffer;
};

esp_mqtt_client_handle_t esp_mqtt_client_init(const esp_mqtt_client_config_t *config);
esp_err_t esp_mqtt_client_register_event(esp_mqtt_client_handle_t client,
                                         int event,
                                         esp_event_handler_t handler,
                                         void *handler_args);
esp_err_t esp_mqtt_client_start(esp_mqtt_client_handle_t client);
esp_err_t esp_mqtt_client_destroy(esp_mqtt_client_handle_t client);
int esp_mqtt_client_publish(esp_mqtt_client_handle_t client,
                            const char *topic,
                            const char *data,
                            int len,
                            int qos,
                            int retain);
int esp_mqtt_client_subscribe(esp_mqtt_client_handle_t client, const char *topic, int qos);
int esp_mqtt_client_unsubscribe(esp_mqtt_client_handle_t client, const char *topic);
