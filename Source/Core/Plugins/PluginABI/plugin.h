// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later#pragma once

#pragma once


extern "C" {

#include <stdint.h>

#ifdef _WIN32
#define EXPORTED extern "C" __declspec(dllexport)
#else
#define EXPORTED extern "C" __attribute__ ((visibility ("default")))
#endif

// Some functions require a handle to identify a function
// call as coming from a given mod
// Mods should keep it around
typedef uint64_t mod_handle_t;

void plugin_init(mod_handle_t mod_id);
void plugin_requestShutdown(mod_handle_t mod_id);

void plugin_destory();
}