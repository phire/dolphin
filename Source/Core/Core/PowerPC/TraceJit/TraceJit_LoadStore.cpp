// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

// TODO(ector): Tons of pshufb optimization of the loads/stores, for SSSE3+, possibly SSE4, only.
// Should give a very noticable speed boost to paired single heavy code.

#include "Common.h"

#include "TraceJit.h"
#include "TraceJitAsm.h"
#include "TraceJitRegCache.h"

void TraceJit::icbi(UGeckoInstruction inst)
{
	Default(inst);
	WriteExit(js.compilerPC + 4, 0);
}
