// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

struct Function;
struct lua_State;

void AddCallback(lua_State *L, Function& CallbackDesc);
