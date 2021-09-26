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

struct GlobalFunction {
    std::vector<std::function<void (lua_State *L, int32_t idx, ArgHolder& holder)>> Args;
    void *(*FnPtr)(void);
    std::function<void (lua_State *L, void*)> ReturnHandler;
};
std::vector<std::unique_ptr<GlobalFunction>> GlobalFunctions;

// This is a generic function that wraps any c function from the API
static int FunctionWrapper(lua_State *L) {
    // First, we need to work out what function was actually called

    auto upvalue = lua_upvalueindex(1);
    if (!lua_islightuserdata(L, upvalue)) {
        // todo: error handling?
        return -1;
    }

    auto FnInfo = reinterpret_cast<GlobalFunction*>(lua_touserdata(L, upvalue));

    std::array<ArgHolder, 20> args;
    uint32_t arg_count = 0;

    for (auto& arg_fn : FnInfo->Args) {
        arg_fn(L, arg_count+1, args[arg_count]);
        arg_count++;
    }

    void* result = CallWithArgs[arg_count](FnInfo->FnPtr, args.data());

    if (FnInfo->ReturnHandler) {
        FnInfo->ReturnHandler(L, result);
    }

    return 1;
}

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

            auto ptr = GlobalFunctions.emplace_back(std::make_unique<GlobalFunction>()).get();
            ptr->FnPtr = Func.FnPtr;
            for (uint32_t k = 0; k < Func.ArgumentCount; k++) {
                Argument& Arg = Func.Arguments[k];

                //printf("        %s %s\n", Arg.Type, Arg.Name);

                auto TypeInfo = LookupType(Arg.Type);
                ptr->Args.push_back(TypeInfo.ArgHandler);
            }
            ptr->ReturnHandler = LookupType(Func.ReturnType).AccessHandler;

                lua_pushlightuserdata(L, ptr);
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