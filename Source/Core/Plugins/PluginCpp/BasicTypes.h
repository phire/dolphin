// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later#pragma once

#pragma once

#include <Common/BitUtils.h>
#include <string>

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

    std::string to_string() { return std::string(Data, Len); }
};

struct WrappedDouble {
    uint64_t Value = 0;

    WrappedDouble() {};
    WrappedDouble(double d) {
        Value = Common::BitCast<uint64_t, double>(d);
    }

    operator double() {
        return Common::BitCast<double, uint64_t>(Value);
    }
};

template<typename ReturnType, typename... ArgTypes>
class Functor {
    struct FuntorData {
        ReturnType (*FnPtr) (void*, ArgTypes...);
        void* Owner; // We will use this for something later

        // The owner of this function is allowed to extend this structure with extra state
    };

    FuntorData* Data;

    static void _check() {
        static_assert(sizeof(Functor) == sizeof(void*));
    }

public:
    ReturnType operator()(ArgTypes... args) {
        return Data->FnPtr(reinterpret_cast<void*>(Data), args...);
    }
};
