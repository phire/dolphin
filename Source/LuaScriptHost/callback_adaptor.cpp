// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "callback_adaptor.h"
#include "type_adaptor.h"
#include <APIDiscovery.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <cstdarg>
#include <cstdint>
#include <string>

// FIXME: we shouldn't be using global state
static int64_t CallbackId = 1;

// Reverse of a Function
struct CallbackInfo
{
    std::string CallbackType;
    std::vector<std::function<void (lua_State *L, void*)>> Args;
    std::function<void (lua_State *L, int32_t idx, ArgHolder& holder)> ReturnHandler;
};

struct LuaCallbackFunctor : public RawFunctor
{
    lua_State* L;
    int64_t Id;
    CallbackInfo Info;
};

static int traceback(lua_State *L) {
    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "traceback");
    lua_pushvalue(L, 1);
    lua_pushinteger(L, 2);
    lua_call(L, 2, 1);
    printf("%s\n", lua_tostring(L, -1));
    return 1;
}

static void* ExecuteCallback(RawFunctor* functor, ...) {
    // Most of the information we need is packed inside the functor
    auto lua_functor = reinterpret_cast<LuaCallbackFunctor*>(functor);
    lua_State* L = lua_functor->L;

    lua_pushcfunction(L, traceback);
    int errorfn = lua_gettop(L);

    // Except for the actual lua function, which we must extract from the Lua state
    // It's kept there for garbage collector reasons
    lua_getfield(L, LUA_REGISTRYINDEX, "active_callbacks");
    lua_geti(L, -1, lua_functor->Id);
    lua_getiuservalue(L, -1, 1);

    // our lua callback function is now at the top of the stack

    // Process our args. Each argFn will push args onto the stack
    int arg_count = 0;
    va_list args;
    va_start(args, functor);
    for (auto argFn : lua_functor->Info.Args) {
        void* data  = va_arg(args, void*);
        argFn(L, data);
        arg_count++;
    }
    va_end(args);

    // calculate how many return stack slots we are expecting
    int result_count = 0;
    if (lua_functor->Info.ReturnHandler)
        result_count++;

    // call into lua
    if (lua_pcall(L, arg_count, result_count, errorfn) != LUA_OK) {
        puts(lua_tostring(L, lua_gettop(L)));

        return nullptr;
    }

    // Extract the result from lua
    ArgHolder result{ .val = nullptr };
    if (lua_functor->Info.ReturnHandler) {
        lua_functor->Info.ReturnHandler(L, -1, result);
    }

    return result.val;
}

void AddCallback(lua_State *L, Function& Callback) {
    std::string FullType = std::string(Callback.ReturnType) + " (";

    CallbackInfo CBInfo;
    for (uint32_t k = 0; k < Callback.ArgumentCount; k++) {
        Argument& Arg = Callback.Arguments[k];

        auto TypeInfo = LookupType(Arg.Type);
        CBInfo.Args.push_back(TypeInfo.AccessHandler);
        FullType += Arg.Type;
        if (k + 1 != Callback.ArgumentCount) {
            FullType += ", ";
        }
    }
    CBInfo.ReturnHandler = LookupType(Callback.ReturnType).ArgHandler;

    FullType += ")";
    CBInfo.CallbackType = FullType;

    printf("registering callback %s\n", FullType.c_str());
    RegisterType(FullType, TypeInfo { [CBInfo] (lua_State *LL, uint32_t idx, ArgHolder& holder) {
        // We need to prevent function from being garbage collected

        // We have a table of all active callbacks in the global registry.
        // fetch it
        lua_getfield(LL, LUA_REGISTRYINDEX, "active_callbacks");

        // Create a userdata
        auto userdata = reinterpret_cast<LuaCallbackFunctor*>(lua_newuserdatauv(LL, sizeof(LuaCallbackFunctor), 1));
        new (userdata) LuaCallbackFunctor();

        // attach the Lua callback function to userdata as a uservalue

        lua_pushvalue(LL, idx);
        lua_setiuservalue(LL, -2, 1);

        int64_t Id = CallbackId++;

        // active_callbacks[Id] = Userdata
        lua_seti(LL, -2, Id);

        userdata->FnPtr = &ExecuteCallback;
        userdata->Id = Id;
        userdata->L = LL; // TODO: is it safe to store the lua state like this?
        userdata->Info = CBInfo;

        holder.val = reinterpret_cast<void*>(userdata);
    }, nullptr /* TODO: Support callbacks in the reverse direction */});
}
