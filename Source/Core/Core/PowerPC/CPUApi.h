// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Common/CommonTypes.h"
#include "Core/Core.h"

#include <functional>
#include <vector>


#include "Core/PowerPC/MMU.h"
#include "Binding.h"



namespace CpuApi {

    struct CpuMemory {

  u32 ReadU8(u32 address)
  {
    return PowerPC::MMU::HostRead<u8>(m_cpu_guard, address);
  }

  u32 ReadU16(u32 address)
  {
    return PowerPC::MMU::HostRead<u16>(m_cpu_guard, address);
  }

  u32 ReadU32(u32 address)
  {
    return PowerPC::MMU::HostRead<u32>(m_cpu_guard, address);
  }

  u64 ReadU64(u32 address)
  {
    return PowerPC::MMU::HostRead<u64>(m_cpu_guard, address);
  }

  float ReadFloat(u32 address) {
    return PowerPC::MMU::HostRead<float>(m_cpu_guard, address);
  }

  double ReadDouble(u32 address)
  {
    return PowerPC::MMU::HostRead<double>(m_cpu_guard, address);
  }

  void WriteU8(u32 address, u8 value) {

    PowerPC::MMU::HostWrite<u8>(m_cpu_guard, value, address);
  }

  void WriteU16(u32 address, u16 value)
  {
    PowerPC::MMU::HostWrite<u16>(m_cpu_guard, value, address);
  }

  void WriteU32(u32 address, u32 value)
  {
    PowerPC::MMU::HostWrite<u32>(m_cpu_guard, value, address);
  }

  void WriteU64(u32 address, u64 value)
  {
    PowerPC::MMU::HostWrite<u64>(m_cpu_guard, value, address);
  }

  void WriteFloat(u32 address, float value)
  {
    PowerPC::MMU::HostWrite<float>(m_cpu_guard, value, address);
  }

  void WriteDouble(u32 address, double value)
  {
    PowerPC::MMU::HostWrite<double>(m_cpu_guard, value, address);
  }

//   void BreakOnCycle(s64 CyclesIntoFuture, Functor<void (Core::System&, CpuMemory*, u64)> callback) {
//     u64 Userdata = std::bit_cast<u64>(callback);
//     Core::System::GetInstance().GetCoreTiming().ScheduleAnonymousEvent(CyclesIntoFuture, BreakOnCycleEvent, Userdata);
//   }

  CpuMemory(Core::System& system) : m_cpu_guard(system) {}
  Core::CPUThreadGuard m_cpu_guard;

  static void constexpr bindings(auto bind) {
    bind(Method<&CpuMemory::ReadU8, "read-u8"_t>{});
    bind(Method<&CpuMemory::ReadU16, "read-u16"_t>{});
    bind(Method<&CpuMemory::ReadU32, "read-u32"_t>{});
    bind(Method<&CpuMemory::ReadU64, "read-u64"_t>{});
    bind(Method<&CpuMemory::ReadFloat, "read-float"_t>{});
    bind(Method<&CpuMemory::ReadDouble, "read-double"_t>{});
    bind(Method<&CpuMemory::WriteU8, "write-u8"_t>{});
    bind(Method<&CpuMemory::WriteU16, "write-u16"_t>{});
    bind(Method<&CpuMemory::WriteU32, "write-u32"_t>{});
    bind(Method<&CpuMemory::WriteU64, "write-u64"_t>{});
    bind(Method<&CpuMemory::WriteFloat, "write-float"_t>{});
    bind(Method<&CpuMemory::WriteDouble, "write-double"_t>{});
  }

};

}