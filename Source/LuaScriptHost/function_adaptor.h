// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <cstdint>
#include <functional>

struct lua_State;
struct ArgHolder;

struct FunctionInfo {
    std::vector<std::function<void (lua_State *L, int32_t idx, ArgHolder& holder)>> Args;
    void *(*FnPtr)(void);
    std::function<void (lua_State *L, void*)> ReturnHandler;
};

int FunctionWrapper(lua_State *L);
