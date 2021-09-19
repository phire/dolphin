#pragma once
#include <stdint.h>

// Just a manual implementation of the required structs.
// In the future, we should be able to autogenerate these

struct VersionInfo {
    uint32_t Version; // The actual version
    uint32_t MinVersion; // The minimum version that this version is backwards compatable to
};

struct VersionInfoArray {
    uint64_t Count;
    VersionInfo* elements;
};

struct String
{
    uint32_t Len;
    const char* c_str;
};

struct ModuleInfo
{
    String Name;
    String Description;
    VersionInfo StableVersion;
    VersionInfoArray OtherVersions = {};
};

struct ModuleArray {
    uint64_t Count;
    ModuleInfo* data;
};

ModuleArray* GetModuleList();
