// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "type_adaptor.h"
#include "discovery.h"

#include <map>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <cstring>

static std::map<std::string, TypeInfo> Types;

void RegisterType(std::string TypeName, TypeInfo typeinfo) {
    Types[TypeName] = typeinfo;
}

static TypeInfo ErrorType = {
    [] (lua_State *L, int32_t idx, ArgHolder& holder) {
        luaL_error(L, "Unimplemented type at call idx %d", idx);
    },
    [] (lua_State *L, void* ret) {
        luaL_error(L, "Unimplemented type");
    }
};

TypeInfo& LookupType(std::string TypeName) {
    auto it = Types.find(TypeName);

    if (it == Types.end())
        return ErrorType;

    return Types[TypeName];
}

void RegisterHardcodedTypes() {
    RegisterType("uint64_t", TypeInfo {
        [] (lua_State *L, int32_t idx, ArgHolder& holder) {
            uint64_t val = luaL_checkinteger(L, idx);
            holder.val = reinterpret_cast<void*>(val);
        },
        [] (lua_State *L, void* ret) {
            uint64_t val = reinterpret_cast<uint64_t>(ret);
            lua_pushinteger(L, val);
        }
    });

    RegisterType("uint32_t", TypeInfo {
        [] (lua_State *L, int32_t idx, ArgHolder& holder) {
            uint32_t val = static_cast<uint32_t>(luaL_checkinteger(L, idx));
            holder.val = reinterpret_cast<void*>(val);
        },
        [] (lua_State *L, void* ret) {
            uint32_t val = static_cast<uint32_t>(reinterpret_cast<uint64_t>(ret));
            lua_pushinteger(L, val);
        }
    });

    RegisterType("WrappedFloat", TypeInfo {
        [] (lua_State *L, int32_t idx, ArgHolder& holder) {
            float val = static_cast<float>(luaL_checknumber(L, idx));
            uint32_t punned;

            std::memcpy(&punned, &val, sizeof(float));
            holder.val = reinterpret_cast<void*>(static_cast<uint64_t>(punned));
        },
        [] (lua_State *L, void* ret) {
            float val;
            std::memcpy(&val, &ret, sizeof(float));
            lua_pushnumber(L, static_cast<double>(val));
        }
    });

    RegisterType("WrappedDouble", TypeInfo {
        [] (lua_State *L, int32_t idx, ArgHolder& holder) {
            double val = luaL_checknumber(L, idx);
            uint64_t punned;

            std::memcpy(&punned, &val, sizeof(double));
            holder.val = reinterpret_cast<void*>(punned);
        },
        [] (lua_State *L, void* ret) {
            double val;
            std::memcpy(&val, &ret, sizeof(double));
            lua_pushnumber(L, val);
        }
    });

    RegisterType("String", TypeInfo {
        [] (lua_State *L, int32_t idx, ArgHolder& holder) {
            auto str = reinterpret_cast<String*>(&holder.extra_data_storage[0]);
            size_t string_length = 0;
            str->c_str = luaL_checklstring(L, idx, &string_length);
            str->Len = string_length;
            holder.val = reinterpret_cast<void*>(str);
        },
        [] (lua_State *L, void* val) {
            auto str = reinterpret_cast<String*>(val);
            lua_pushlstring(L, str->c_str, str->Len);
        }
    });

    // TODO: figure out the best way to represent enums in lua, probably tables
    //       For now I'm using strings
    RegisterType("LogLevel", TypeInfo {
        [] (lua_State *L, int32_t idx, ArgHolder& holder) {
            std::string enum_val = luaL_checklstring(L, idx, nullptr);
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
        },
        nullptr
    });

    RegisterType("void", TypeInfo { nullptr, nullptr });
}
