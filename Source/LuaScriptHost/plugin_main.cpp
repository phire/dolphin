// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "test_discovery.h"
#include "discovery.h"
#include <APIDiscovery.h>
#include "function_adaptor.h"

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
    std::vector<std::function<void (lua_State *L, uint32_t idx, ArgHolder& holder)>> Args;
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

            bool valid = true;

            auto ptr = GlobalFunctions.emplace_back(std::make_unique<GlobalFunction>()).get();
            ptr->FnPtr = Func.FnPtr;
            for (uint32_t k = 0; k < Func.ArgumentCount; k++) {
                Argument& Arg = Func.Arguments[k];
                std::string Type = Arg.Type;

                //printf("        %s %s\n", Arg.Type, Arg.Name);

                // Ignore the proper type system for now, just hardcode the types we need for testing

                if (Type == "uint64_t") {
                    ptr->Args.emplace_back([] (lua_State *LS, uint32_t idx, ArgHolder& holder) {
                        uint64_t val = luaL_checkinteger(LS, idx);
                        holder.val = reinterpret_cast<void*>(val);
                    });
                } else if (Type == "String") {
                    ptr->Args.emplace_back([] (lua_State *LS, uint32_t idx, ArgHolder& holder) {
                        auto str = reinterpret_cast<String*>(&holder.extra_data_storage[0]);
                        size_t string_length = 0;
                        str->c_str = luaL_checklstring(LS, idx, &string_length);
                        str->Len = string_length;
                        holder.val = reinterpret_cast<void*>(str);
                    });
                } else if (Type == "LogLevel") {
                    // TODO: figure out the best way to represent enums in lua, probably tables
                    //       For now I'm using strings
                    // TODO: Also it shouldn't need to be hardcoded
                    ptr->Args.emplace_back([] (lua_State *LS, uint32_t idx, ArgHolder& holder) {
                        std::string enum_val = luaL_checklstring(LS, idx, nullptr);
                        if (enum_val == "Notice")
                            holder.val = (void*) 1;
                        else if (enum_val == "Error")
                            holder.val = (void*) 2;
                        else if (enum_val == "Warning")
                            holder.val = (void*) 3;
                        else if (enum_val == "Info")
                            holder.val = (void*) 4;
                        else if (enum_val == "Debug")
                            holder.val = (void*) 5;
                    });
                } else {
                    printf("LuaScriptHost: Unhandled argument type %s for function %s; Skipped function\n", Type.c_str(), Func.FunctionName);
                    valid = false;
                }
            }

            std::string ReturnType = Func.ReturnType;
            if (ReturnType == "uint64_t") {
                ptr->ReturnHandler = [](lua_State *LS, void* ret) {
                    uint64_t val = reinterpret_cast<uint64_t>(ret);
                    lua_pushinteger(LS, val);
                };
            } else if (ReturnType != "void") {
                printf("LuaScriptHost: Unhandled return type %s for function %s; Skipped function\n", ReturnType.c_str(), Func.FunctionName);
                valid = false;
            }

            if (valid) {
                lua_pushlightuserdata(L, ptr);
                lua_pushcclosure(L, &FunctionWrapper, 1);
                lua_setfield(L, -2, Func.FunctionName);
            }
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