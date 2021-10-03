// Copyright 2010 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Common/CommonTypes.h"
#include "Core/PowerPC/CPUApi.h"

#include <fmt/format.h>

#include <BasicTypes.h>
#include <export.h>

#include "Core/PowerPC/MMU.h"

namespace CpuApi {

static CpuMemoryHandle CurrentHandle = 0;

extern "C" {

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

EXPORTED WrappedFloat CpuMemory_ReadFloat(CpuMemoryHandle handle, u32 address)
{
  if (handle != CurrentHandle) {
    return 0;
  }

  return PowerPC::HostRead_F32(address);
}

EXPORTED WrappedDouble CpuMemory_ReadDouble(CpuMemoryHandle handle, u32 address)
{
  if (handle != CurrentHandle) {
    return 0;
  }

  return PowerPC::HostRead_F64(address);
}

}

} // namespace CpuApi
