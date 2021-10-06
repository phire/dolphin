#pragma once

struct Class;
struct lua_State;

void AddClass(lua_State *L, Class& ClassDesc);
void RegisterClass(Class& ClassDesc);