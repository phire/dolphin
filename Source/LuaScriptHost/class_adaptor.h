#pragma once

#include <stdint.h>

struct Class;
struct lua_State;

void AddClass(lua_State *L, Class& ClassDesc, const char* ModuleName, uint32_t ModuleVersion);
void RegisterClass(Class& ClassDesc);