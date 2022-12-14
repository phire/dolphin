// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <functional>
#include <vector>

#include "Common/FileUtil.h"

#include "PluginCpp/BasicTypes.h"
#include "ModuleManager.h"

namespace Plugins {

struct PluginFiles;

extern FunctorOwner PluginHostOwner;


// TODO: Sub-classing a functor is currently way too complex. We need a c++ wrapper to make this simple
class GetFnPtrFunctor : public Functor<void*(String*, int64_t, String*)>
{
    struct GetFnPtrFunctorData : public Functor<void*(String*, int64_t, String*)>::FunctorData
    {
        u32 plugin_idx;
        std::function<void*(String*, int64_t, String*)> GetFunction;
    };

    GetFnPtrFunctorData data_instance;

    static void* GetFunction(void* data_void, String* module_name, int64_t version, String* function);

public:
    GetFnPtrFunctor() = default;

    GetFnPtrFunctor(u32 plugin_idx) {
        data_instance.FnPtr = &GetFunction;
        data_instance.Owner = nullptr;
        data_instance.plugin_idx = plugin_idx;

        Data = &data_instance;
    }

    GetFnPtrFunctor(const GetFnPtrFunctor& other) {
        data_instance = other.data_instance;
        Data = &data_instance;
    }
};

struct PluginFiles
{
    std::string physicalName;
    std::string virtualName;
    bool Active = false;
    uint64_t ModuleId;
    void (*plugin_init)(void*);
    void (*plugin_requestShutdown)(uint64_t);
    GetFnPtrFunctor FnPtrFunctor;
};


void Init();

std::vector<Plugins::PluginFiles> GetAllPlugins();
void LoadPlugin(u32 id);
void ShutdownPlugin(u32 id);
bool AlreadyAdded(Plugins::PluginFiles plugin);

} // Plugins