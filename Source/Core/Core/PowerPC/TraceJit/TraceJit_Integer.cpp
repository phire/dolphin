// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include "TraceJit.h"
#include "TraceJitRegCache.h"
#include "TraceJitAsm.h"

void TraceJit::twx(UGeckoInstruction inst)
{
	INSTRUCTION_START
	JITDISABLE(bJITIntegerOff)

	s32 a = inst.RA;

	gpr.Flush(FLUSH_ALL);
	fpr.Flush(FLUSH_ALL);

	MOV(8, R(AL), Imm8(inst.TO));

	if (inst.OPCD == 3) // twi
		CMP(32, gpr.R(a), gpr.R(inst.RB));
	else // tw
		CMP(32, gpr.R(a), Imm32((s32)(s16)inst.SIMM_16));

	FixupBranch al = J_CC(CC_L);
	FixupBranch ag = J_CC(CC_G);
	FixupBranch ae = J_CC(CC_Z);
	// FIXME: will never be reached. But also no known code uses it...
	FixupBranch ll = J_CC(CC_NO);
	FixupBranch lg = J_CC(CC_O);

	SetJumpTarget(al);
	TEST(8, R(AL), Imm8(16));
	FixupBranch exit1 = J_CC(CC_NZ);
	FixupBranch take1 = J();
	SetJumpTarget(ag);
	TEST(8, R(AL), Imm8(8));
	FixupBranch exit2 = J_CC(CC_NZ);
	FixupBranch take2 = J();
	SetJumpTarget(ae);
	TEST(8, R(AL), Imm8(4));
	FixupBranch exit3 = J_CC(CC_NZ);
	FixupBranch take3 = J();
	SetJumpTarget(ll);
	TEST(8, R(AL), Imm8(2));
	FixupBranch exit4 = J_CC(CC_NZ);
	FixupBranch take4 = J();
	SetJumpTarget(lg);
	TEST(8, R(AL), Imm8(1));
	FixupBranch exit5 = J_CC(CC_NZ);
	FixupBranch take5 = J();

	SetJumpTarget(take1);
	SetJumpTarget(take2);
	SetJumpTarget(take3);
	SetJumpTarget(take4);
	SetJumpTarget(take5);
	LOCK();
	OR(32, M((void *)&PowerPC::ppcState.Exceptions), Imm32(EXCEPTION_PROGRAM));
	WriteExceptionExit();

	SetJumpTarget(exit1);
	SetJumpTarget(exit2);
	SetJumpTarget(exit3);
	SetJumpTarget(exit4);
	SetJumpTarget(exit5);
	WriteExit(js.compilerPC + 4, 1);
}
