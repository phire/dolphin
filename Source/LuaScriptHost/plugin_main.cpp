// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "test_discovery.h"
#include "discovery.h"
#include <APIDiscovery.h>
#include "function_adaptor.h"
#include "type_adaptor.h"

#include <plugin.h>

#include <stdio.h>

// We have lua compiled as c++, so this works
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <vector>

static uint64_t mod_id;


static void RunLua() {
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    lua_pushinteger(L, mod_id);
    lua_setglobal(L, "ModID");

    RegisterHardcodedTypes();

    auto modules = GetModuleList();
    for (uint32_t i = 0; i < modules->Count; i++) {
        ModuleInfo& Info = modules->data[i];

        Module* Mod = GetModuleDefintion(Info.Name.c_str, Info.StableVersion.Version);

        // Create a table for the module
        lua_newtable(L);

        //printf("Module: %s\n", Info.Name.c_str);

        // Attach all global functions to it
        for (uint32_t j = 0; j < Mod->GlobalFunctionsCount; j++) {
            Function& Func = Mod->GlobalFunctions[j];

            //printf("    %s\n", Func.FunctionName);

            auto ptr = reinterpret_cast<FunctionInfo*>(lua_newuserdatauv(L, sizeof(FunctionInfo), 0));
            new (ptr) FunctionInfo();

            ptr->FnPtr = Func.FnPtr;
            for (uint32_t k = 0; k < Func.ArgumentCount; k++) {
                Argument& Arg = Func.Arguments[k];

                //printf("        %s %s\n", Arg.Type, Arg.Name);

                auto TypeInfo = LookupType(Arg.Type);
                ptr->Args.push_back(TypeInfo.ArgHandler);
            }
            ptr->ReturnHandler = LookupType(Func.ReturnType).AccessHandler;

                lua_pushcclosure(L, &FunctionWrapper, 1);
                lua_setfield(L, -2, Func.FunctionName);
        }

        lua_setglobal(L, Info.Name.c_str);
    }

    if (luaL_dofile(L, "script.lua") == LUA_OK) {
        lua_pop(L, lua_gettop(L));
    } else {
        puts(lua_tostring(L, lua_gettop(L)));
        printf("Error loading/running script.lua\n");
    }

    lua_close(L);
}

extern "C" {

EXPORTED void plugin_init(uint64_t id) {
    mod_id = id;
    TestDiscovery();

    RunLua();

    printf("LuaScriptHost initialized\n");
}

}