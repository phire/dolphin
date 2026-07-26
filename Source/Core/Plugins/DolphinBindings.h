#pragma once

#include "Core/PowerPC/CPUApi.h"

void constexpr dolphin_bindings(auto bind) {
    CpuApi::CpuMemory::bindings(bind);
}
