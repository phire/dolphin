// Copyright 2010 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Common/BitUtils.h"
#include "Common/CommonTypes.h"
#include "Core/CoreTiming.h"
#include "Core/PowerPC/CPUApi.h"

#include <climits>
#include <compare>
#include <fmt/format.h>

#include <BasicTypes.h>

#include "Core/PowerPC/MMU.h"

#include <unordered_map>



static void BreakOnCycleEvent(u64 userdata, s64 cyclesLate);

namespace CpuApi {

static u64 CurrentHandle = 0;
static bool s_initialized = false;
static std::vector<Functor<void (CpuMemory, u64)>> BreakOnRun;

ZAP_NAMESPACE_MODULE("Cpu", Zap::VersionInfo(1, 1), "Allows accessing CPU state and memory");

struct CpuMemory {
  ZAP_CHECKED_HANDLE(CpuMemory);

  u32 ReadU8(u32 address)
  {
    return PowerPC::HostRead_U8(address);
  }

  u32 ReadU16(u32 address)
  {
    return PowerPC::HostRead_U16(address);
  }

  u32 ReadU32(u32 address)
  {
    return PowerPC::HostRead_U32(address);
  }

  u64 ReadU64(u32 address)
  {
    return PowerPC::HostRead_U64(address);
  }

  float ReadFloat(u32 address) {
    return PowerPC::HostRead_F32(address);
  }

  double CpuMemory_ReadDouble(u32 address)
  {
    return PowerPC::HostRead_F64(address);
  }

  void CpuMemory_WriteU8(u32 address, u8 value) {

    PowerPC::HostWrite_U8(value, address);
  }

  void CpuMemory_WriteU16(u32 address, u16 value)
  {
    PowerPC::HostWrite_U16(value, address);
  }

  void CpuMemory_WriteU32(u32 address, u32 value)
  {
    PowerPC::HostWrite_U32(value, address);
  }

  void CpuMemory_WriteU64(u32 address, u64 value)
  {
    PowerPC::HostWrite_U64(value, address);
  }

  void CpuMemory_WriteFloat(u32 address, float value)
  {
    PowerPC::HostWrite_F32(value, address);
  }

  void CpuMemory_WriteDouble(u32 address, double value)
  {
    PowerPC::HostWrite_F64(value, address);
  }

  void CpuMemory_BreakOnCycle(s64 CyclesIntoFuture, Functor<void (CpuMemory, u64)> callback) {
    u64 Userdata = Common::BitCast<u64>(callback);
    CoreTiming::ScheduleAnonymousEvent(CyclesIntoFuture, BreakOnCycleEvent, Userdata);
  }
};



void Cpu_BreakOnRun(Functor<void (CpuMemoryHandle, u64)> callback) {
  // TODO: Should this be callable from threads other than CPU?
  //       Currenty it's not
  //       How are we going to make sure this is only callable from the proper thread?
  if (s_initialized) {
    CurrentHandle += 1;
    callback(CurrentHandle, CoreTiming::GetTicks());
  } else {
    BreakOnRun.push_back(callback);
  }
}

static void InitCallback(u64 userdata, s64 cycles_late) {
  CurrentHandle += 1;
  for(auto callback : BreakOnRun) {
    callback(CurrentHandle, static_cast<u64>(cycles_late));
  }
  BreakOnRun.clear();
  s_initialized = true;
}

ZAP_IGNORE void Init() {
  CoreTiming::ScheduleAnonymousEvent(0, InitCallback, 0);
}

ZAP_IGNORE void Shutdown() {
  s_initialized = false;
}

} // namespace CpuApi

static void BreakOnCycleEvent(u64 userdata, s64 cyclesLate) {
  auto callback = Common::BitCast<Functor<void (CpuApi::CpuMemory*, u64)>>(userdata);
  CpuApi::CpuMemory cpu_memory_handle;

  callback(&cpu_memory_handle, static_cast<u64>(cyclesLate));
}
