// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Plugins/PluginHost.h"
#include "Common/Logging/Log.h"
#include "Plugins/ModuleManager.h"

#include <dlfcn.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <vector>

static uint64_t Module_id = 0x1000;
static std::vector<Plugins::PluginFiles> plugin_entries;

void* Plugins::GetFnPtrFunctor::GetFunction(void* data_void, String* module_name, int64_t version, String* function) {
    auto data = reinterpret_cast<GetFnPtrFunctorData*>(data_void);

    auto plugin = &plugin_entries[data->plugin_idx];


    // Because we give one of these Functors to each plugin, we now know which plugin called us
    uint64_t plugin_id [[maybe_unused]] = plugin->ModuleId;

    // In future versions of this API, we can do fancy things based on which plugin called us
    // and create per-plugin wrappers for every API function

    // For now, we just call return the function pointer
    return GetFnPtr(module_name->Data, version, function->Data);
}

void Plugins::Init()
{
    InitDiscoveryModule();
    InitLoggingModule();
    InitBasicGuiModule();
    InitCPUModule();
}

std::vector<Plugins::PluginFiles> Plugins::GetAllPlugins()
{
    auto PluginDir = File::GetUserPath(D_LOAD_IDX) + "Plugins";

    fmt::print("Scanning for plugins in {}\n", PluginDir);

    const File::FSTEntry entries = File::ScanDirectoryTree(PluginDir, true);

    for(const auto& entry : entries.children)
    {
        PluginFiles file = {entry.physicalName, entry.virtualName, false};
        if(!AlreadyAdded(file)) {
            fmt::print("Found {}, {}\n", file.physicalName, file.virtualName);
            plugin_entries.push_back(file);
        }
    }

    return plugin_entries;
}

bool Plugins::AlreadyAdded(Plugins::PluginFiles plugin)
{
    for(PluginFiles& file : plugin_entries)
    {
        if(file.virtualName == plugin.virtualName)
            return true;
    }

    return false;
}

void Plugins::LoadPlugin(u32 id)
{
        fmt::print("Loading plugin {}\n", plugin_entries.at(id).physicalName);
        // Attempt to load the .so file
        void* handle = dlopen(plugin_entries.at(id).physicalName.c_str(), RTLD_NOW);

        if (!handle)
        {
            fmt::print("dlopen of {} failed: {}\n", plugin_entries.at(id).physicalName, dlerror());
            return;
        }



        // Locate the plugin_init function
        plugin_entries.at(id).plugin_init = reinterpret_cast<void (*)(void*)>(dlsym(handle, "plugin_init"));
        plugin_entries.at(id).plugin_requestShutdown = reinterpret_cast<void (*)(uint64_t)>(dlsym(handle, "plugin_requestShutdown"));

        if (!plugin_entries.at(id).plugin_init)
        {
            fmt::print("{} did not contain plugin_init function: {}\n", plugin_entries.at(id).physicalName, dlerror());
            return;
        }

        if (!plugin_entries.at(id).plugin_requestShutdown)
        {
            fmt::print("{} did not contain plugin_requestShutdown function: {}\n", plugin_entries.at(id).physicalName, dlerror());
            return;
        }

        plugin_entries.at(id).Active = true;
        plugin_entries.at(id).ModuleId = Module_id++;

        plugin_entries.at(id).FnPtrFunctor = GetFnPtrFunctor(id);

        // Actually call the plugin_init method
        plugin_entries.at(id).plugin_init(plugin_entries.at(id).FnPtrFunctor);
}

void Plugins::ShutdownPlugin(u32 id)
{
    fmt::print("Requesting shutdown of {}\n", plugin_entries.at(id).virtualName);
    plugin_entries.at(id).Active = false;
    plugin_entries.at(id).plugin_requestShutdown(Module_id);
}
