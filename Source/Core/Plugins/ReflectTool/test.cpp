//#include <BasicTypes.h>
#include <export.h>

typedef u64 CpuMemoryHandle;

u32 CpuMemory_ReadU8(CpuMemoryHandle handle, u32 address);

FOO_CLASS CpuMemory {
    FOO u32 ReadU32(CpuMemoryHandle handle, u32 address);
}