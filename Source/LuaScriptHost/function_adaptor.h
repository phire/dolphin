// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <cstdint>

struct ArgHolder;

extern std::array<void*(*)(void*(*)(void), ArgHolder *), 21> CallWithArgs;