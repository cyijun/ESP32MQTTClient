#pragma once

inline void fakeEspLog(const char *, const char *, ...)
{
}

#define ESP_LOGE(...) fakeEspLog(__VA_ARGS__)
#define ESP_LOGW(...) fakeEspLog(__VA_ARGS__)
#define ESP_LOGI(...) fakeEspLog(__VA_ARGS__)
