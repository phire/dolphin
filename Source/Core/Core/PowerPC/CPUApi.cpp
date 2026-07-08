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

struct CpuMemory {

// static Core::System& m_system = Core::System::GetInstance();
// static  Core::CPUThreadGuard m_cpu_guard = Core::CPUThreadGuard(m_system);

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

  void BreakOnCycle(s64 CyclesIntoFuture, Functor<void (Core::System&, CpuMemory*, u64)> callback) {
    u64 Userdata = std::bit_cast<u64>(callback);
    Core::System::GetInstance().GetCoreTiming().ScheduleAnonymousEvent(CyclesIntoFuture, BreakOnCycleEvent, Userdata);
  }



  CpuMemory(Core::CPUThreadGuard& guard) : m_cpu_guard(guard) {}
  Core::CPUThreadGuard& m_cpu_guard;

};



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

ZAP_IGNORE void Init() {
  Core::System::GetInstance().GetCoreTiming().ScheduleAnonymousEvent(0, &InitCallback, 0);
}

ZAP_IGNORE void Shutdown() {
  s_initialized = false;
}


static void BreakOnCycleEvent(Core::System& system, u64 userdata, s64 cyclesLate) {
  auto callback = std::bit_cast<Functor<void (Core::System&, CpuApi::CpuMemory*, u64)>>(userdata);
  auto guard = Core::CPUThreadGuard(system);
  // auto cpu_memory_handle = CpuApi::CpuMemory (guard);

  //callback(system, &cpu_memory_handle, static_cast<u64>(cyclesLate));
}



void foo(int b) {

  fmt::print("foo: {}\n", b);
}

Plugin::ResourceBinding BindCpuApi() {
  using namespace Plugin;

  Resource<CpuMemory> r("Cpu", "Allows accessing emulated CPU memory");

  auto read_sig = Sig({{"address", "address (in emulated memory) to read from"}});
  auto write_sig = Sig({{"address", "address (in emulated memory) to write to"}, {"value", "value to write"}});

  r.add(wrap<&CpuMemory::ReadU8>("ReadU8"), read_sig);
  r.add(wrap<&CpuMemory::ReadU16>("ReadU16"), read_sig);
  r.add(wrap<&CpuMemory::ReadU32>("ReadU32"), read_sig);
  r.add(wrap<&CpuMemory::ReadFloat>("ReadFloat"), read_sig);
  r.add(wrap<&CpuMemory::ReadDouble>("ReadDouble"), read_sig);
  r.add(wrap<&CpuMemory::ReadU64>("ReadU64"), read_sig);
  r.add(wrap<&CpuMemory::ReadU16>("ReadU16"), read_sig);
  r.add(wrap<&CpuMemory::WriteU8>("WriteU8"), write_sig);
  r.add(wrap<&CpuMemory::WriteU16>("WriteU16"), write_sig);
  r.add(wrap<&CpuMemory::WriteU32>("WriteU32"), write_sig);
  r.add(wrap<&CpuMemory::WriteU64>("WriteU64"), write_sig);
  r.add(wrap<&CpuMemory::WriteFloat>("WriteFloat"), write_sig);
  r.add(wrap<&CpuMemory::WriteDouble>("WriteDouble"), write_sig);

  return r;
}

} // namespace CpuApi




