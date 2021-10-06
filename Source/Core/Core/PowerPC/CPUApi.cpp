// Copyright 2010 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Common/BitUtils.h"
#include "Common/CommonTypes.h"
#include "Core/PowerPC/CPUApi.h"
#include "Core/CoreTiming.h"

#include <fmt/format.h>

#include <BasicTypes.h>
#include <export.h>

#include "Core/PowerPC/MMU.h"

#include <unordered_map>

namespace CpuApi {

static CpuMemoryHandle CurrentHandle = 0;

EXPORTED u32 CpuMemory_ReadU8(CpuMemoryHandle handle, u32 address)
{
  if (handle != CurrentHandle) {
    // TODO: raise error
    return 0;
  }

  return PowerPC::HostRead_U8(address);
}

EXPORTED u32 CpuMemory_ReadU16(CpuMemoryHandle handle, u32 address)
{
  if (handle != CurrentHandle) {
    return 0;
  }

  return PowerPC::HostRead_U16(address);
}

EXPORTED u32 CpuMemory_ReadU32(CpuMemoryHandle handle, u32 address)
{
  if (handle != CurrentHandle) {
    return 0;
  }

  return PowerPC::HostRead_U32(address);
}

EXPORTED u64 CpuMemory_ReadU64(CpuMemoryHandle handle, u32 address)
{
  if (handle != CurrentHandle) {
    return 0;
  }

  return PowerPC::HostRead_U64(address);
}

inline WrappedFloat ReadFloat(CpuMemoryHandle handle, u32 address)
{
  if (handle != CurrentHandle) {
    return 0;
  }

  return PowerPC::HostRead_F32(address);
}

EXPORTED u32 CpuMemory_ReadFloat(CpuMemoryHandle handle, u32 address) {
  return ReadFloat(handle, address);
}

inline WrappedDouble ReadDouble(CpuMemoryHandle handle, u32 address)
{
  if (handle != CurrentHandle) {
    return 0;
  }

  return PowerPC::HostRead_F64(address);
}

EXPORTED u64 CpuMemory_ReadDouble(CpuMemoryHandle handle, u32 address) {
  return ReadDouble(handle, address);
}

static void BreakOnCycleEvent(u64 userdata, s64 cyclesLate) {
  CurrentHandle += 1;
  auto callback = Common::BitCast<Functor<void (CpuMemoryHandle, u64)>>(userdata);
  callback(CurrentHandle, static_cast<u64>(cyclesLate));
}

EXPORTED void CpuMemory_BreakOnCycle(CpuMemoryHandle handle, s64 CyclesIntoFuture, Functor<void (CpuMemoryHandle, u64)> callback) {
  if (handle != CurrentHandle) {
    return;
  }

  u64 Userdata = Common::BitCast<u64>(callback);
  CoreTiming::ScheduleAnonymousEvent(CyclesIntoFuture, BreakOnCycleEvent, Userdata);
}

static bool s_initialized = false;
static std::vector<Functor<void (CpuMemoryHandle, u64)>> BreakOnRun;

EXPORTED void Cpu_BreakOnRun(Functor<void (CpuMemoryHandle, u64)> callback) {
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
  
void Init() {
  CoreTiming::ScheduleAnonymousEvent(0, InitCallback, 0);
}

void Shutdown() {
  s_initialized = false;
}

} // namespace CpuApi
