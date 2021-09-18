// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdint.h>
#include <string>

struct Module;

void RegisterModuleDefintion(std::string ModuleName, uint32_t Version, Module* ModuleType);

void InitDiscoveryModule();
