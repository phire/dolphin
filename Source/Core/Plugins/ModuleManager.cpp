// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Plugins/ModuleManager.h"
#include <plugin.h>
#include <APIDiscovery.h>

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

static Array<ModuleInfo> AllModules;
static std::vector<ModuleInfo> AllModulesVector;
typedef std::pair<std::string, uint32_t> ModuleKey;
static std::map<ModuleKey, Module*> ModuleMap;

typedef std::pair<ModuleKey, std::string> FunctionKey;
static std::map<FunctionKey, void*> FunctionMap;

static void RegisterFunctions(ModuleKey moduleKey, Module* module) {
    // Global functions
    for (int i = 0; i < module->GlobalFunctionsCount; i++) {
        auto &f = module->GlobalFunctions[i];
        FunctionMap[std::make_pair(moduleKey, f.FunctionName)] = reinterpret_cast<void*>(f.FnPtr);
    }

    // Class method
    for (int i = 0; i < module->ClassCount; i++) {
        std::string baseName = module->Classes[i].ClassName;
        baseName += "::";

        for (int j = 0; j < module->Classes[i].MethodCount; j++) {
            auto &f = module->Classes[i].Methods[j];

            FunctionMap[std::make_pair(moduleKey, baseName + f.FunctionName)] = reinterpret_cast<void*>(f.FnPtr);
        }
    }
}

void RegisterModuleDefintion(Module* ModuleType, ModuleInfo Info) {
    auto key = std::make_pair(Info.Name.to_string(), Info.StableVersion.Version);
    ModuleMap[key] = ModuleType;
    AllModulesVector.push_back(Info);
    AllModules.Count = AllModulesVector.size();
    AllModules.elements = AllModulesVector.data();

    RegisterFunctions(key, ModuleType);
}

Array<ModuleInfo>& GetAllModules() {
    return AllModules;
}

struct Module* GetModuleDefintion(const char* ModuleName, uint32_t Version) {
    const auto it = ModuleMap.find(std::make_pair(ModuleName, Version));

    if (it != ModuleMap.end()) {
        return it->second;
    }
    return nullptr;
}

void* GetFnPtr(const char* ModuleName, uint32_t Version, const char* FunctionName) {
    const auto it = FunctionMap.find(std::make_pair(std::make_pair(ModuleName, Version), FunctionName));

    if (it != FunctionMap.end()) {
        return it->second;
    }
    return nullptr;
}
