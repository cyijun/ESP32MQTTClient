idf_component_register(SRCS "../../../../src/ESP32MQTTClient.cpp"
                    INCLUDE_DIRS "../../../../src"
                    REQUIRES mqtt)

target_compile_options(${COMPONENT_TARGET} PRIVATE -fno-rtti)