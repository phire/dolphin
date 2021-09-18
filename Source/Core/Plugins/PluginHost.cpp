// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Plugins/PluginHost.h"
#include "Plugins/ModuleManager.h"
#include "Common/FileUtil.h"

#include <dlfcn.h>
#include <fmt/format.h>

void Plugins::LoadAllPlugins()
{
    InitDiscoveryModule();

    auto PluginDir = File::GetExeDirectory() + "/Plugins";

    const auto entries = File::ScanDirectoryTree(PluginDir, false);

    for (const auto& entry : entries.children)
    {
        fmt::print("Loading plugin {}\n", entry.physicalName);
        // Attempt to load the .so file
        void* handle = dlopen(entry.physicalName.c_str(), RTLD_NOW);

        if (!handle)
        {
            fmt::print("dlopen of {} failed: {}\n", entry.physicalName, dlerror());
            continue;
        }

        void (*plugin_init)(void);

        // Locate the plugin_init function
        plugin_init = reinterpret_cast<void (*)(void)>(dlsym(handle, "plugin_init"));

        if (!plugin_init)
        {
            fmt::print("{} did not contain plugin_init function: {}\n", entry.physicalName, dlerror());
            continue;
        }

        // Actually call the plugin_init method
        plugin_init();
    }
}
