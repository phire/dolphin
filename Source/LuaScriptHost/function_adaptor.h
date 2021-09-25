// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <cstdint>

struct ArgHolder
{
    void* val; // The actual value which will based passed to the called function
    uint8_t extra_data_storage[32]; // A bit of storage in case an arg type requires building a struct
};

extern std::array<void*(*)(void*(*)(void), ArgHolder *), 21> CallWithArgs;