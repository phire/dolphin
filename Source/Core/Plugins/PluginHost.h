// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <vector>

#include "Common/FileUtil.h"

namespace Plugins {

struct PluginFiles
{
    std::string physicalName;
    std::string virtualName;
    bool Active = false;
    uint64_t ModuleId;
    void (*plugin_init)(uint64_t);
    void (*plugin_requestShutdown)(uint64_t);
};


void Init();

std::vector<Plugins::PluginFiles> GetAllPlugins();
void LoadPlugin(u32 id);
void ShutdownPlugin(u32 id);
bool AlreadyAdded(Plugins::PluginFiles plugin);
static std::vector<Plugins::PluginFiles> plugin_entries;
} // Plugins