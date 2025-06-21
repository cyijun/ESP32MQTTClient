idf_component_register(SRCS "main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES ESP32MQTTClient esp_wifi esp_event nvs_flash) 