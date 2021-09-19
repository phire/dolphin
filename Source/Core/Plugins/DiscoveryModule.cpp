// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// This is an exported module that exists just so plugins can find other modules.
//
// This is the one module that is always expected to exist. Plugins are expected
// to query it blind to find out about

#include <APIDiscovery.h>

#include "Plugins/BasicTypes.h"
#include "Plugins/ModuleManager.h"

#include <memory>
#include <vector>
#include <map>

extern "C" {
EXPORTED Array<ModuleInfo>* ListAllModules();
}

// TODO: In the future, hope to generate these structures automatically though reflection.
//       but until we get a good idea why they should look like, they are hand-written

static Function ListAllModules_Info = {
    .FunctionName = "ListAllModules",
    .ReturnType = "Array*",
    .ReturnOwnership = false,
    .ArgumentCount = 0,
    .Arguments = nullptr,
    .Symbol = SYMBOL(ListAllModules),
    .FnPtr = reinterpret_cast<void* (*)(void)>(&ListAllModules)
};

static Member ModuleInfo_Members[] = {
    { .Name = "Name",
      .Type = "String",
      .Offset = offsetof(ModuleInfo, Name) },
    { .Name = "Description",
      .Type = "String",
      .Offset = offsetof(ModuleInfo, Description) },
    { .Name = "StableVersion",
      .Type = "VersionInfo",
      .Offset = offsetof(ModuleInfo, StableVersion) },
    { .Name = "OtherVersions",
      .Type = "Array<VersionInfo>",
      .Offset = offsetof(ModuleInfo, OtherVersions) }
};

static Class ModuleInfo_Info = {
    .ClassName = "ModuleInfo",
    .MethodCount = 0,
    .MemberCount = 2,
    .Methods = nullptr,
    .Members = ModuleInfo_Members
};

static Module Discovery = {
    .Version = 1,
    .GlobalFunctionsCount = 1,
    .CallbackCount = 0,
    .ClassCount = 1,
    .GlobalFunctions = {
        &ListAllModules_Info,
    },
    .Callbacks = {},
    .Classes = { &ModuleInfo_Info }
};


EXPORTED Array<ModuleInfo>* ListAllModules() {
    return &GetAllModules();
}

void InitDiscoveryModule() {
    RegisterModuleDefintion(&Discovery, ModuleInfo {
        String("Discovery"),
        String("This module provide tools to discover other modules"),
        VersionInfo{1, 1}
    });
}