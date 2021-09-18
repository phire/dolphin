// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "test_discovery.h"

#include <plugin.h>

#include <stdio.h>

// We have lua compiled as c++, so this works
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

static void RunLua() {
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    lua_close(L);
}

extern "C" {

EXPORTED void plugin_init() {
    TestDiscovery();

    RunLua();

    printf("LuaScriptHost initialized\n");
}

}