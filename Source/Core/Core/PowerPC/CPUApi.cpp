// Copyright 2010 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Common/BitUtils.h"
#include "Common/CommonTypes.h"
#include "Core/CoreTiming.h"
#include "Core/PowerPC/CPUApi.h"
#include "Core/System.h"
#include "Core/Core.h"

#include "FunctionTraits.h"

#include <climits>
#include <compare>
#include <fmt/format.h>

#include <BasicTypes.h>

#include "Core/PowerPC/MMU.h"

#include <unordered_map>

#include "Binding.h"


namespace CpuApi {

static void BreakOnCycleEvent(Core::System& system, u64 userdata, s64 cyclesLate);

struct CpuMemory;

static u64 CurrentHandle = 0;
static bool s_initialized = false;
static std::vector<Functor<void (Core::System&, CpuMemory*, u64)>> BreakOnRun;

void Cpu_BreakOnRun(Functor<void (Core::System&, CpuMemory*, u64)> callback) {
  // TODO: Should this be callable from threads other than CPU?
  //       Currenty it's not
  //       How are we going to make sure this is only callable from the proper thread?
  if (s_initialized) {
    CurrentHandle += 1;
    u64 ticks = Core::System::GetInstance().GetCoreTiming().GetTicks();
    auto& system = Core::System::GetInstance();
    auto guard = Core::CPUThreadGuard(system);
    // CpuMemory cpu_memory_handle(guard);
    // callback(system, &cpu_memory_handle, ticks);
  } else {
    BreakOnRun.push_back(callback);
  }
}

static void InitCallback(Core::System& system, u64 userdata, s64 cycles_late) {
  CurrentHandle += 1;
  auto guard = Core::CPUThreadGuard(system);
  // auto cpu_memory_handle = CpuApi::CpuMemory (guard);
  // for(auto callback : BreakOnRun) {
  //   callback(system, &cpu_memory_handle, static_cast<u64>(cycles_late));
  // }
  BreakOnRun.clear();
  s_initialized = true;
}

static void BreakOnCycleEvent(Core::System& system, u64 userdata, s64 cyclesLate) {
  auto callback = std::bit_cast<Functor<void (Core::System&, CpuApi::CpuMemory*, u64)>>(userdata);
  auto guard = Core::CPUThreadGuard(system);
  // auto cpu_memory_handle = CpuApi::CpuMemory (guard);

  //callback(system, &cpu_memory_handle, static_cast<u64>(cyclesLate));
}

} // namespace CpuApi




