// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Plugins/ModuleManager.h"
#include <plugin.h>
#include <APIDiscovery.h>

#include <algorithm>
#include <map>
#include <string>
#include <utility>

static Array<ModuleInfo> AllModules;
static std::vector<ModuleInfo> AllModulesVector;
static std::map<std::pair<std::string, uint32_t>, Module*> ModuleMap;

void RegisterModuleDefintion(Module* ModuleType, ModuleInfo Info) {

    ModuleMap[std::make_pair(Info.Name.to_string(), Info.StableVersion.Version)] = ModuleType;
    AllModulesVector.push_back(Info);
    AllModules.Count = AllModulesVector.size();
    AllModules.elements = AllModulesVector.data();
}

Array<ModuleInfo>& GetAllModules() {
    return AllModules;
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