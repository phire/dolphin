// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Common/CommonTypes.h"
#include "PluginCpp/BasicTypes.h"
#include <APIDiscovery.h>

#include "Plugins/ModuleManager.h"

using CpuMemoryHandle = u64;

namespace CpuApi {

extern u32 CpuMemory_ReadU8(CpuMemoryHandle handle, u32 address);
extern u32 CpuMemory_ReadU16(CpuMemoryHandle handle, u32 address);
extern u32 CpuMemory_ReadU32(CpuMemoryHandle handle, u32 address);
extern u64 CpuMemory_ReadU64(CpuMemoryHandle handle, u32 address);
extern u32 CpuMemory_ReadFloat(CpuMemoryHandle handle, u32 address);
extern u64 CpuMemory_ReadDouble(CpuMemoryHandle handle, u32 address);
extern void CpuMemory_WriteU8(CpuMemoryHandle handle, u32 address, u8 value);
extern void CpuMemory_WriteU16(CpuMemoryHandle handle, u32 address, u16 value);
extern void CpuMemory_WriteU32(CpuMemoryHandle handle, u32 address, u32 value);
extern void CpuMemory_WriteU64(CpuMemoryHandle handle, u32 address, u64 value);
extern void CpuMemory_WriteFloat(CpuMemoryHandle handle, u32 address, float value);
extern void CpuMemory_WriteDouble(CpuMemoryHandle handle, u32 address, double value);
extern void CpuMemory_BreakOnCycle(CpuMemoryHandle handle, s64 CyclesIntoFuture, Functor<void (CpuMemoryHandle, u64)> callback);
extern void Cpu_BreakOnRun(Functor<void (CpuMemoryHandle, u64)> callback);

}


static Argument Read_Args[] = {
    {
        .Name = "address",
        .Type = "uint32_t"
    },
};

static Argument WriteU8_Args[] = {
    {
        .Name = "address",
        .Type = "uint32_t"
    },
    {
        .Name = "value",
        .Type = "uint8_t"
    }

};

static Argument WriteU16_Args[] = {
    {
        .Name = "address",
        .Type = "uint32_t"
    },
    {
        .Name = "value",
        .Type = "uint16_t"
    }

};

static Argument WriteU32_Args[] = {
    {
        .Name = "address",
        .Type = "uint32_t"
    },
    {
        .Name = "value",
        .Type = "uint32_t"
    }

};

static Argument WriteU64_Args[] = {
    {
        .Name = "address",
        .Type = "uint32_t"
    },
    {
        .Name = "value",
        .Type = "uint64_t"
    }

};

static Argument WriteFloat_Args[] = {
    {
        .Name = "address",
        .Type = "uint32_t"
    },
    {
        .Name = "value",
        .Type = "WrappedFloat"
    }

};

static Argument WriteDouble_Args[] = {
    {
        .Name = "address",
        .Type = "uint32_t"
    },
    {
        .Name = "value",
        .Type = "WrappedDouble"
    }

};

static Argument BreakOnCycle_Args[] = {
    {
        .Name = "CyclesIntoFuture",
        .Type = "uint64_t"
    },
    {
        .Name = "CallbackFn",
        .Type = "void (CpuMemoryHandle, uint64_t)"
    },
};

static Function CpuMemory_Methods[] = {
    {
        .FunctionName = "ReadU8",
        .ReturnType = "uint32_t",
        .ReturnOwnership = true,
        .ArgumentCount = ARRAY_LEN(Read_Args),
        .Arguments = Read_Args,
        .Symbol = SYMBOL(CpuMemory_ReadU8),
        .FnPtr = reinterpret_cast<void* (*)(void)>(&CpuApi::CpuMemory_ReadU8)
    },
    {
        .FunctionName = "ReadU16",
        .ReturnType = "uint32_t",
        .ReturnOwnership = true,
        .ArgumentCount = ARRAY_LEN(Read_Args),
        .Arguments = Read_Args,
        .Symbol = SYMBOL(CpuMemory_ReadU16),
        .FnPtr = reinterpret_cast<void* (*)(void)>(&CpuApi::CpuMemory_ReadU16)
    },
    {
        .FunctionName = "ReadU32",
        .ReturnType = "uint32_t",
        .ReturnOwnership = true,
        .ArgumentCount = ARRAY_LEN(Read_Args),
        .Arguments = Read_Args,
        .Symbol = SYMBOL(CpuMemory_ReadU32),
        .FnPtr = reinterpret_cast<void* (*)(void)>(&CpuApi::CpuMemory_ReadU32)
    },
    {
        .FunctionName = "ReadU64",
        .ReturnType = "uint64_t",
        .ReturnOwnership = true,
        .ArgumentCount = ARRAY_LEN(Read_Args),
        .Arguments = Read_Args,
        .Symbol = SYMBOL(CpuMemory_ReadU64),
        .FnPtr = reinterpret_cast<void* (*)(void)>(&CpuApi::CpuMemory_ReadU64)
    },
    {
        .FunctionName = "ReadFloat",
        .ReturnType = "WrappedFloat",
        .ReturnOwnership = true,
        .ArgumentCount = ARRAY_LEN(Read_Args),
        .Arguments = Read_Args,
        .Symbol = SYMBOL(CpuMemory_ReadFloat),
        .FnPtr = reinterpret_cast<void* (*)(void)>(&CpuApi::CpuMemory_ReadFloat)
    },
    {
        .FunctionName = "ReadDouble",
        .ReturnType = "WrappedDouble",
        .ReturnOwnership = true,
        .ArgumentCount = ARRAY_LEN(Read_Args),
        .Arguments = Read_Args,
        .Symbol = SYMBOL(CpuMemory_ReadDouble),
        .FnPtr = reinterpret_cast<void* (*)(void)>(&CpuApi::CpuMemory_ReadDouble)
    },
    {
        .FunctionName = "WriteU8",
        .ReturnType = "void",
        .ReturnOwnership = true,
        .ArgumentCount = ARRAY_LEN(WriteU8_Args),
        .Arguments = WriteU8_Args,
        .Symbol = SYMBOL(CpuMemory_WriteU8),
        .FnPtr = reinterpret_cast<void* (*)(void)>(&CpuApi::CpuMemory_WriteU8)
    },
    {
        .FunctionName = "WriteU16",
        .ReturnType = "void",
        .ReturnOwnership = true,
        .ArgumentCount = ARRAY_LEN(WriteU16_Args),
        .Arguments = WriteU16_Args,
        .Symbol = SYMBOL(CpuMemory_WriteU16),
        .FnPtr = reinterpret_cast<void* (*)(void)>(&CpuApi::CpuMemory_WriteU16)
    },
    {
        .FunctionName = "WriteU32",
        .ReturnType = "void",
        .ReturnOwnership = true,
        .ArgumentCount = ARRAY_LEN(WriteU32_Args),
        .Arguments = WriteU32_Args,
        .Symbol = SYMBOL(CpuMemory_WriteU32),
        .FnPtr = reinterpret_cast<void* (*)(void)>(&CpuApi::CpuMemory_WriteU32)
    },
    {
        .FunctionName = "WriteU64",
        .ReturnType = "void",
        .ReturnOwnership = true,
        .ArgumentCount = ARRAY_LEN(WriteU64_Args),
        .Arguments = WriteU64_Args,
        .Symbol = SYMBOL(CpuMemory_WriteU64),
        .FnPtr = reinterpret_cast<void* (*)(void)>(&CpuApi::CpuMemory_WriteU64)
    },
    {
        .FunctionName = "WriteFloat",
        .ReturnType = "void",
        .ReturnOwnership = true,
        .ArgumentCount = ARRAY_LEN(WriteFloat_Args),
        .Arguments = WriteFloat_Args,
        .Symbol = SYMBOL(CpuMemory_WriteFloat),
        .FnPtr = reinterpret_cast<void* (*)(void)>(&CpuApi::CpuMemory_WriteFloat)
    },
    {
        .FunctionName = "WriteDouble",
        .ReturnType = "void",
        .ReturnOwnership = true,
        .ArgumentCount = ARRAY_LEN(WriteDouble_Args),
        .Arguments = WriteDouble_Args,
        .Symbol = SYMBOL(CpuMemory_WriteDouble),
        .FnPtr = reinterpret_cast<void* (*)(void)>(&CpuApi::CpuMemory_WriteDouble)
    },
    {
        .FunctionName = "BreakOnCycle",
        .ReturnType = "void",
        .ReturnOwnership = false,
        .ArgumentCount = ARRAY_LEN(BreakOnCycle_Args),
        .Arguments = BreakOnCycle_Args,
        .Symbol = SYMBOL(CpuMemory_BreakOnCycle),
        .FnPtr = reinterpret_cast<void* (*)(void)>(&CpuApi::CpuMemory_BreakOnCycle)
    },
};

static Class Cpu_Classes[] = {
    {
        .ClassName = "CpuMemoryHandle",
        .MethodCount = ARRAY_LEN(CpuMemory_Methods),
        .MemberCount = 0,
        .Methods = CpuMemory_Methods,
        .Members = nullptr
    }
};


static Argument Cpu_BreakOnRun_Args[] = {
    {
        .Name = "CallbackFn",
        .Type = "void (CpuMemoryHandle, uint64_t)"
    },
};

static Function Cpu_Functions[] = {
    {
        .FunctionName = "BreakOnRun",
        .ReturnType = "void",
        .ReturnOwnership = false,
        .ArgumentCount = ARRAY_LEN(Cpu_BreakOnRun_Args),
        .Arguments = Cpu_BreakOnRun_Args,
        .Symbol = SYMBOL(Cpu_BreakOnRun),
        .FnPtr = reinterpret_cast<void* (*)(void)>(&CpuApi::Cpu_BreakOnRun)
    }
};


static Argument BreakOnCycleCallback_Args[] = {
    {
        .Name = "Context",
        .Type = "CpuMemoryHandle"
    },
    {
        .Name = "CyclesLate",
        .Type = "uint64_t"
    },
};


static Function Cpu_Callbacks[] = {
    {
        .FunctionName = "BreakOnCycleCallback",
        .ReturnType = "void",
        .ReturnOwnership = false,
        .ArgumentCount = ARRAY_LEN(BreakOnCycleCallback_Args),
        .Arguments = BreakOnCycleCallback_Args,
        .Symbol = nullptr,
        .FnPtr = nullptr
    }
};

static Module Cpu = {
    .Version = 1,
    .GlobalFunctionsCount = ARRAY_LEN(Cpu_Functions),
    .CallbackCount = ARRAY_LEN(Cpu_Callbacks),
    .ClassCount = ARRAY_LEN(Cpu_Classes),
    .GlobalFunctions = Cpu_Functions,
    .Callbacks = Cpu_Callbacks,
    .Classes = Cpu_Classes
};

void InitCPUModule() {
    RegisterModuleDefintion(&Cpu, ModuleInfo {
        String("Cpu"),
        String("Allows accessing CPU state and memory"),
        VersionInfo{1, 1}
    });
}