// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Common/CommonTypes.h"
#include "PluginCpp/BasicTypes.h"
#include <APIDiscovery.h>

#include "Plugins/ModuleManager.h"


typedef u64 BasicGuiHandle;

extern "C" {
EXPORTED u32 BasicGui_DrawText(u64 handle, String* text, u32 posX, u32 posY, u32 color);
EXPORTED u64 BasicGui_RegisterDrawHook(void (*Fn)(BasicGuiHandle));
}

// TODO: In the future, hope to generate these structures automatically though reflection.
//       but until we get a good idea why they should look like, they are hand-written

static Argument DrawText_Args[] = {
    {
        .Name = "text",
        .Type = "String"
    },
    {
        .Name = "posX",
        .Type = "uint32_t"
    },
    {
        .Name = "posY",
        .Type = "uint32_t"
    },
    {
        .Name = "color",
        .Type = "uint32_t" // TODO: Proper bitfield type for this
    }
};

static Function BasicGuiHandle_Methods[] = {
    {
        .FunctionName = "DrawText",
        .ReturnType = "uint32_t",
        .ReturnOwnership = false,
        .ArgumentCount = ARRAY_LEN(DrawText_Args),
        .Arguments = DrawText_Args,
        .Symbol = SYMBOL(BasicGui_DrawText),
        .FnPtr = reinterpret_cast<void* (*)(void)>(&BasicGui_DrawText)
    },
};

static Class BasicGuiHandle_Info = {
    .ClassName = "BasicGuiHandle",
    .MethodCount = ARRAY_LEN(BasicGuiHandle_Methods),
    .MemberCount = 0,
    .Methods = BasicGuiHandle_Methods,
    .Members = nullptr
};

static Argument DrawHookCallbackArgs[] = {
    {
        .Name = "Context",
        .Type = "BasicGuiHandle"
    },
};

static Function DrawHookCallback = {
    .FunctionName = "DrawCallback",
    .ReturnType = "void",
    .ReturnOwnership = false,
    .ArgumentCount = ARRAY_LEN(DrawHookCallbackArgs),
    .Arguments = DrawHookCallbackArgs,
    .Symbol = nullptr,
    .FnPtr = nullptr
};

static Argument RegisterDrawHook_Args[] = {
    {
        .Name = "CallbackFn",
        .Type = "void (BasicGuiHandle)"
    },
};

static Function BasicGuiHandle_Functions[] = {
    {
        .FunctionName = "RegisterDrawHook",
        .ReturnType = "uint64_t",
        .ReturnOwnership = false,
        .ArgumentCount = ARRAY_LEN(RegisterDrawHook_Args),
        .Arguments = RegisterDrawHook_Args,
        .Symbol = SYMBOL(BasicGui_RegisterDrawHook),
        .FnPtr = reinterpret_cast<void* (*)(void)>(&BasicGui_RegisterDrawHook)
    },
};

static Module BasicGui = {
    .Version = 1,
    .GlobalFunctionsCount = ARRAY_LEN(BasicGuiHandle_Functions),
    .CallbackCount = 1,
    .ClassCount = 1,
    .GlobalFunctions = BasicGuiHandle_Functions,
    .Callbacks = { &DrawHookCallback },
    .Classes = { &BasicGuiHandle_Info }
};

void InitBasicGuiModule() {
    RegisterModuleDefintion(&BasicGui, ModuleInfo {
        String("BasicGui"),
        String("The goal of this API is to meet the basic use-case of \"I'm a script and I want"
               "to overlay some debug text and lines on the screen\""),
        VersionInfo{1, 1}
    });
}