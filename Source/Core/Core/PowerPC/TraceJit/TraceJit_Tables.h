// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#ifndef TRACEJIT_TABLES_H
#define TRACEJIT_TABLES_H

#include "../Gekko.h"
#include "../PPCTables.h"
#include "TraceJit.h"

namespace TraceJitTables
{
	void CompileInstruction(PPCAnalyst::CodeOp & op);
	void InitTables();
}
#endif
