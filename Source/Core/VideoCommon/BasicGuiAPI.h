// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Common/CommonTypes.h"

#include <functional>
#include <vector>

namespace BasicGuiAPI {

typedef u64 BasicGuiHandle;

std::vector<std::function<void (BasicGuiHandle)>>& getCallbacks();
BasicGuiHandle StartDraw();
void EndDraw();

} // BasicGuiAPI