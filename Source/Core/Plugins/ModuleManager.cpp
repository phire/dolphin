// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Plugins/ModuleManager.h"
#include <plugin.h>
#include <APIDiscovery.h>

#include <algorithm>
#include <map>
#include <string>
#include <utility>

static std::map<std::pair<std::string, uint32_t>, Module*> ModuleMap;


void RegisterModuleDefintion(std::string ModuleName, uint32_t Version, Module* ModuleType) {
    ModuleMap[std::make_pair(ModuleName, Version)] = ModuleType;
}


extern "C" {

__attribute__ ((visibility("default"))) struct Module* GetModuleDefintion(const char* ModuleName, uint32_t Version) {
    const auto it = ModuleMap.find(std::make_pair(ModuleName, Version));

    if (it != ModuleMap.end()) {
        return it->second;
    }
    return nullptr;
}

}