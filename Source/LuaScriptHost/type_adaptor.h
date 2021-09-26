// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>
#include <string>

#include "function_adaptor.h"

struct lua_State;

struct ArgHolder
{
    void* val; // The actual value which will based passed to the called function
    uint8_t extra_data_storage[32]; // A bit of storage in case an arg type requires building a struct
};

struct TypeInfo {
    std::function<void (lua_State *L, int32_t idx, ArgHolder& holder)> ArgHandler;
    std::function<void (lua_State *L, void*)> AccessHandler;
};

void RegisterType(std::string TypeName, TypeInfo typeinfo);
TypeInfo& LookupType(std::string TypeName);

void RegisterHardcodedTypes();
