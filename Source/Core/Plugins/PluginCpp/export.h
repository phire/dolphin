// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later#pragma once

#pragma once

#include <BasicTypes.h>
#include <vector>

struct Module;


namespace Zap
{

struct VersionInfo {
    uint32_t Version; // The actual version
    uint32_t MinVersion; // The minimum version that this version is backwards comparable with

    VersionInfo(uint32_t version, uint32_t minVersion) : Version(version), MinVersion(minVersion) {}
};

struct ModuleInfo;


extern std::vector<ModuleInfo*> AllModules;


#if defined(__clang__)
#define ZAP_ANNOTATE(string) [[clang::annotate("zap_" # string)]]
#else
#define ZAP_ANNOTATE(string)
#endif

struct ModuleInfo {
    const String Name;
    const String Description;
    const VersionInfo StableVersion;
    const Module* moduleObj;

    ModuleInfo(const char* name, const char* description, const VersionInfo version, const Module* obj) :
        Name(name),
        Description(description),
        StableVersion(version),
        moduleObj(obj)
    {
        AllModules.push_back(this);
    }
};

#define ZAP_REGISTER_MODULE(module, description, version) \
    namespace zap_module_ ## module ## _namespace = module; \
    namespace module { \
        extern const Module* zap_module_obj; \
        extern const Zap::ModuleInfo zap_module_info; \
        const Zap::ModuleInfo zap_module_info ( \
            # module, \
            description, \
            version, \
            zap_module_obj \
        ); \
    }

#if defined(__clang__)
    // #define EXPORTED __attribute__ ((annotate("foo_abi")))
    // #define FOO __attribute__ ((annotate("foo_abi")))
    // #define EXPORT_METHOD(class) __attribute__ ((annotate("foo_abi"), ))
    // #define CLASS(c) __attribute__((annotate("foo_class:" #c )))
    // #define FOO_CLASS inline namespace [[clang::annotate("foo_class")]]


#else
    #define EXPORTED
    #define FOO_CLASS inline namespace
#endif


}
