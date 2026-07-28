#pragma once

#include <mutex>

#include "freertos/FreeRTOS.h"

struct FakeSemaphore
{
    std::recursive_mutex mutex;
};

using SemaphoreHandle_t = FakeSemaphore *;

inline SemaphoreHandle_t xSemaphoreCreateMutex()
{
    return new FakeSemaphore();
}

inline int xSemaphoreTake(SemaphoreHandle_t semaphore, TickType_t)
{
    if (semaphore != nullptr)
        semaphore->mutex.lock();
    return 1;
}

inline int xSemaphoreGive(SemaphoreHandle_t semaphore)
{
    if (semaphore != nullptr)
        semaphore->mutex.unlock();
    return 1;
}

inline void vSemaphoreDelete(SemaphoreHandle_t semaphore)
{
    delete semaphore;
}
