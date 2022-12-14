// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "test_discovery.h"

#include <plugin.h>
#include <APIDiscovery.h>

#include <stdio.h>
#include <string.h>

#include "discovery.h"

ModuleArray* (*ListAllModules)() = nullptr;

void TestDiscovery() {
    Module* Discovery = GetModuleDefintion("Discovery", 1);

    if (Discovery == nullptr) {
        printf("Error Couldn't find Discovery Module\n");
        return;
    }

    for (uint32_t i = 0; i < Discovery->GlobalFunctionsCount; i++) {
        Function* fn_info = &Discovery->GlobalFunctions[i];
        if (strcmp("ListAllModules", fn_info->FunctionName) == 0) {
            ListAllModules = (ModuleArray*(*)()) fn_info->FnPtr;
        }
    }

    if (ListAllModules == nullptr) {
        printf("Error Couldn't find ListAllModules Function\n");
        return;
    }

    ModuleArray* Modules = ListAllModules();
    if (Modules == nullptr) {
        printf("No modules found\n");
    }

    printf("Found Modules:\n");

    for (uint32_t i = 0; i < Modules->Count; i++) {
        ModuleInfo* Info = &Modules->data[i];
        printf("\t%s v%i\n", Info->Name.c_str, Info->StableVersion.Version);
    }
}

ModuleArray* GetModuleList() {
    if (ListAllModules != nullptr) {
        return ListAllModules();
    }
    return nullptr;
}