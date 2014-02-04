// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include "Common.h"

#include "../../HW/SystemTimers.h"
#include "HW/ProcessorInterface.h"

#include "TraceJit.h"
#include "TraceJitRegCache.h"

void TraceJit::mtmsr(UGeckoInstruction inst)
{
	INSTRUCTION_START
 	// Don't interpret this, if we do we get thrown out
	//JITDISABLE(bJITSystemRegistersOff)
	if (!gpr.R(inst.RS).IsImm())
	{
		gpr.Lock(inst.RS);
		gpr.BindToRegister(inst.RS, true, false);
	}
	MOV(32, M(&MSR), gpr.R(inst.RS));
	gpr.UnlockAll();
	gpr.Flush(FLUSH_ALL);
	fpr.Flush(FLUSH_ALL);

	// If some exceptions are pending and EE are now enabled, force checking
	// external exceptions when going out of mtmsr in order to execute delayed
	// interrupts as soon as possible.
	TEST(32, M(&MSR), Imm32(0x8000));
	FixupBranch eeDisabled = J_CC(CC_Z);

	TEST(32, M((void*)&PowerPC::ppcState.Exceptions), Imm32(EXCEPTION_EXTERNAL_INT | EXCEPTION_PERFORMANCE_MONITOR | EXCEPTION_DECREMENTER));
	FixupBranch noExceptionsPending = J_CC(CC_Z);

	// Check if a CP interrupt is waiting and keep the GPU emulation in sync (issue 4336)
	TEST(32, M((void *)&ProcessorInterface::m_InterruptCause), Imm32(ProcessorInterface::INT_CAUSE_CP));
	FixupBranch cpInt = J_CC(CC_NZ);

	MOV(32, M(&PC), Imm32(js.compilerPC + 4));
	WriteExternalExceptionExit();

	SetJumpTarget(cpInt);
	SetJumpTarget(noExceptionsPending);
	SetJumpTarget(eeDisabled);

	WriteExit(js.compilerPC + 4, 0);

	js.firstFPInstructionFound = false;
}
