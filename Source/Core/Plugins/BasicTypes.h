#pragma once

#include <string.h>

template <typename T>
struct Array {
    uint64_t Count = 0;
    T* elements = nullptr;
};

constexpr uint32_t c_strlen(const char* c_str) {
    uint32_t count = 0;
    while (c_str[count] != '\0')
        count++;

    return count;
}

struct String {
    uint32_t Len;
    const char* Data;

    constexpr String(const char c_str[]) : Len(c_strlen(c_str)), Data(c_str) {}
};