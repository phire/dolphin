// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdint.h>
#include <string>

#include "Plugins/PluginCpp/BasicTypes.h"

struct Module;
namespace Discovery
{
    struct ModuleInfo;
}

//void RegisterModuleDefintion(Module* ModuleType, Discovery::ModuleInfo Info);
Array<Discovery::ModuleInfo>* GetAllModules();
struct Module* GetModuleDefintion(const char* ModuleName, uint32_t Version);

void* GetFnPtr(const char* ModuleName, uint32_t Version, const char* FunctionName);


void InitDiscoveryModule();
void InitLoggingModule();
void InitBasicGuiModule();
void InitCPUModule();


