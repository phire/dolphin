// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later#pragma once

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <plugin.h>

#define SYMBOL(x) #x

struct Argument {
    const char* Name;
    const char* Type;
    bool Out;
};

struct Function {
    const char* FunctionName;
    const char* ReturnType;
    bool ReturnOwnership; // false:
    const uint32_t ArgumentCount;
    struct Argument* Arguments;
    const char* Symbol; // unused for callbacks
    void* (*FnPtr)(void*); // unused for callbacks
};

struct Member {
    const char* Name;
    const char* Type;
    uint32_t Offset;
};

struct Class {
    const char* ClassName;
    uint32_t MethodCount;
    uint32_t MemberCount;
    struct Function* Methods;
    struct Member* Members;
};

struct Module
{
    uint32_t Version;
    uint32_t GlobalFunctionsCount;
    uint32_t CallbackCount;
    uint32_t ClassCount;
    struct Function* GlobalFunctions;
    struct Function* Callbacks;
    struct Class* Classes;
};

__attribute__ ((visibility("default"))) struct Module* GetModuleDefintion(const char* ModuleName, uint32_t Version);

#ifdef __cplusplus
} // extern "C"
#endif