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


//extern std::vector<ModuleInfo*> AllModules;


#if defined(__clang__)
#define ZAP_ANNOTATE(string) [[clang::annotate("zap_" # string)]]
#else
#define ZAP_ANNOTATE(string)
#endif

namespace Test {

}

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
        //AllModules.push_back(this);
    }
};

// #define ZAP_REGISTER_MODULE(module, description, version) \
//     namespace zap_module_ ## module ## _namespace = module; \
//     namespace module { \
//         extern const Module* zap_module_obj; \
//         extern const Zap::ModuleInfo zap_module_info; \
//         const Zap::ModuleInfo zap_module_info ( \
//             # module, \
//             description, \
//             version, \
//             zap_module_obj \
//         ); \
//     }

#define ZAP_NAMESPACE_MODULE(name, version, description) \
    namespace zap_module { \
        extern const Module* zap_module_obj; \
        static const Zap::ModuleInfo zap_module_info ( \
            name, \
            description, \
            version, \
            zap_module_obj \
        ); \
    }

constexpr uint64_t hash_ident(const char* s) {
uint64_t h = 0;
while (*s)
{
    h = h * 6364136223846793005ULL + *s + 0xda3e39cb94b95bdbULL;
    s++;
}
return h;
}

#define ZAP_CHECKED_HANDLE(_struct) \
private:  \
  uint64_t cookie; \
  bool zap_is_cookie_valid() { return cookie == Zap::hash_ident( #_struct ); } \
public: \
  _struct() : cookie(Zap::hash_ident( #_struct )) {} \
  ~_struct() { cookie = 0; } \


#if defined(__clang__)
    #define ZAP_IGNORE __attribute__((annotate("zap_ignore")))
#else
    #define ZAP_IGNORE
#endif


}
