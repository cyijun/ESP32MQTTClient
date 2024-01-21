[![arduino-library-badge](https://www.ardu-badge.com/badge/ESP32MQTTClient.svg?)](https://www.ardu-badge.com/ESP32MQTTClient)
[![Arduino Library CI](https://github.com/cyijun/ESP32MQTTClient/actions/workflows/arduinoci.yml/badge.svg)](https://github.com/cyijun/ESP32MQTTClient/actions/workflows/arduinoci.yml)

# ESP32MQTTClient

A thread-safe MQTT client for Arduino ESP32xx.

## About *next* branch

Part of mqtt component has been changed in ESP-IDF v5.1. The encapsulation needs to be modified, otherwise it cannot be compiled. Refer to [compiling error](https://github.com/cyijun/ESP32MQTTClient/actions/runs/7595202860/job/20687511391) and latest [mqtt_client.h](https://github.com/espressif/esp-mqtt/blob/master/include/mqtt_client.h).

## Features🦄

- Encapsulated thread-safe MQTT client of ESP-IDF.
- Interfaces design inspired by [EspMQTTClient](https://github.com/plapointe6/EspMQTTClient).
- CA cert support by [dwolshin](https://github.com/dwolshin).
