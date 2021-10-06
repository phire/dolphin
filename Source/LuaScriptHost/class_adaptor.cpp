// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "class_adaptor.h"
#include "function_adaptor.h"
#include "type_adaptor.h"
#include <APIDiscovery.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <functional>
#include <string>
#include <vector>

struct MethodInfo {
    std::vector<std::function<void (lua_State *L, uint32_t idx, ArgHolder& holder)>> Args;
    void *(*FnPtr)(void);
    std::function<void (lua_State *L, void*)> ReturnHandler;
};

struct MemberInfo {
    TypeInfo* Type;
    uint32_t Offset;
};

struct ObjectUserData {
    uint8_t* Ptr;
    size_t NameHash;
};

// This function is called when the lua code indexes into our object
static int ClassIndex(lua_State *L) {
    // The stack contains the index key at -1 and our object (a uservalue) at -2

    // we want to redirect that lookup to the object's class attribute table

    lua_getiuservalue(L, -2, 1); // grab the attribute table out of uservalue storage
    lua_pushvalue(L, -2); // Duplicate the key
    lua_rawget(L, -2); // lookup key in uservalue

    // The value from the class attribute table is now at the top of the stack

    if (lua_isuserdata(L, -1)) {
        // a userdata in our class attribute means only one thing, this is a member that code can access
        // read the member info
        auto Member = reinterpret_cast<MemberInfo*>(lua_touserdata(L, -1));
        // Grab our object pointer
        auto Ptr = reinterpret_cast<ObjectUserData*>(lua_touserdata(L, -2))->Ptr;
        // Load the pointer
        void* Data = *reinterpret_cast<void**>(Ptr + Member->Offset);

        // Run it though the Type access function to convert and push it onto the stack
        Member->Type->AccessHandler(L, Data);
    }

    return 1;
}

static bool HaveClassMetaTable = false;
static void createClassMetatable(lua_State *L) {
    constexpr luaL_Reg classMetaFunctions[] = {
        {"__index", &ClassIndex },
        { nullptr, nullptr }
    };

    luaL_newmetatable(L, "class_metatable");
    luaL_setfuncs(L, classMetaFunctions, 0);

    lua_setfield(L, LUA_REGISTRYINDEX, "class_metatable");

    HaveClassMetaTable = true;
}

static void InstanceClass(lua_State *L, size_t NameHash, void* object);

// Preregister the Class' name with the typesystem
void RegisterClass(Class& ClassDesc)
{
    std::string Name = ClassDesc.ClassName;
    size_t NameHash = std::hash<std::string>{}(Name);

    auto ClassArgHandler = [NameHash, Name] (lua_State *LL, uint32_t idx, ArgHolder& holder) {
        if(!lua_isuserdata(LL, idx)) {
            luaL_error(LL, "Expected and object at argument %i", idx);
        }
        auto objData = reinterpret_cast<ObjectUserData*>(lua_touserdata(LL, idx));

        // Check the object's class pointer points to the correct class
        if (lua_rawlen(LL, idx) != sizeof(ObjectUserData) || objData->NameHash != NameHash) {
            // If the it's not the correct class, when we have no idea what the script has passed
            // us and we shouldn't even try to de-reference the Class pointer
            luaL_error(LL, "Expected class %s at argument %i", Name.c_str(), idx);
        }

        holder.val = objData->Ptr;
    };

    auto ClassAccessHandler = [NameHash] (lua_State *LL, void* val) {
        InstanceClass(LL, NameHash, val);
    };

    RegisterType(ClassDesc.ClassName, { ClassArgHandler, ClassAccessHandler });
}

// Actually set up the class 
// We need to do this after preregistring all classes and registering the callbacks
// so that methods types can reference other classes/callbacks
void AddClass(lua_State *L, Class& ClassDesc) {
    if (!HaveClassMetaTable)
        createClassMetatable(L);

    lua_newtable(L);

    for (uint32_t i = 0; i < ClassDesc.MethodCount; i++) {
        auto& Method = ClassDesc.Methods[i];
        auto Info = reinterpret_cast<MethodInfo*>(lua_newuserdatauv(L, sizeof(MethodInfo), 0));
        new (Info) MethodInfo();

        Info->FnPtr = Method.FnPtr;

        // Add an explicit self pointer
        Info->Args.push_back(LookupType(ClassDesc.ClassName).ArgHandler);

        for (uint32_t k = 0; k < Method.ArgumentCount; k++) {
            Argument& Arg = Method.Arguments[k];

            auto TypeInfo = LookupType(Arg.Type);
            Info->Args.push_back(TypeInfo.ArgHandler);
        }
        Info->ReturnHandler = LookupType(Method.ReturnType).AccessHandler;

        lua_pushcclosure(L, &FunctionWrapper, 1);
        lua_setfield(L, -2, Method.FunctionName);
    }

    // TODO: members

    size_t NameHash = std::hash<std::string>{}(ClassDesc.ClassName);
    lua_seti(L, LUA_REGISTRYINDEX, NameHash);
}

static void InstanceClass(lua_State *L, size_t NameHash, void* object) {
    auto userdata = reinterpret_cast<ObjectUserData*>(lua_newuserdatauv(L, sizeof(ObjectUserData), 1));
    userdata->Ptr = reinterpret_cast<uint8_t*>(object);
    userdata->NameHash = NameHash;

    lua_geti(L, LUA_REGISTRYINDEX, NameHash);
    if (!lua_istable(L, -1)) {
        luaL_error(L, "Trying to Instance unknown class %i", NameHash);
    }
    lua_setiuservalue(L, -2, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, "class_metatable");
    lua_setmetatable(L, -2);
}
