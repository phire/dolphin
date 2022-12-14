// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdint.h>
#include <string>

#include "Plugins/PluginCpp/BasicTypes.h"

struct VersionInfo {
    uint32_t Version; // The actual version
    uint32_t MinVersion; // The minimum version that this version is backwards comparable with
};

struct ModuleInfo
{
    String Name;
    String Description;
    VersionInfo StableVersion;
    Array<VersionInfo> OtherVersions = {};
};

struct Module;

void RegisterModuleDefintion(Module* ModuleType, ModuleInfo Info);
Array<ModuleInfo>& GetAllModules();
struct Module* GetModuleDefintion(const char* ModuleName, uint32_t Version);

void* GetFnPtr(const char* ModuleName, uint32_t Version, const char* FunctionName);


void InitDiscoveryModule();
void InitLoggingModule();
void InitBasicGuiModule();
void InitCPUModule();


