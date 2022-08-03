#pragma once

#include <APIDiscovery.h>
#include <BasicTypes.h>
#include <export.h>

#include "Plugins/ModuleManager.h"

namespace Discovery {
    struct ModuleInfo
    {
        String Name;
        String Description;
        Zap::VersionInfo StableVersion;
        Array<Zap::VersionInfo> OtherVersions = {};
    };

    Array<ModuleInfo>* GetAllModules();
    constexpr auto GetModuleDefintion = ::GetModuleDefintion;
    void* GetFunctionPtr(String ModuleName, uint32_t version, String FunctionName);
}
