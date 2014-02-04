// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <map>

// for the PROFILER stuff
#ifdef _WIN32
#include <windows.h>
#endif

#include "Common.h"
#include "../../HLE/HLE.h"
#include "../../PatchEngine.h"
#include "../Profiler.h"
#include "TraceJit.h"
#include "TraceJitAsm.h"
#include "TraceJitRegCache.h"
#include "TraceJit_Tables.h"
#include "HW/ProcessorInterface.h"
#if defined(_DEBUG) || defined(DEBUGFAST)
#include "PowerPCDisasm.h"
#endif

using namespace Gen;
using namespace PowerPC;

// Expermental Tracing jit.

static int CODE_SIZE = 1024*1024*32;

namespace CPUCompare
{
	extern u32 m_BlockStart;
}

void TraceJit::Init()
{
	jo.optimizeStack = true;
	/* This will enable block linking in JitBlockCache::FinalizeBlock(), it gives faster execution but may not
	   be as stable as the alternative (to not link the blocks). However, I have not heard about any good examples
	   where this cause problems, so I'm enabling this by default, since I seem to get perhaps as much as 20% more
	   fps with this option enabled. If you suspect that this option cause problems you can also disable it from the
	   debugging window. */
	if (Core::g_CoreStartupParameter.bEnableDebugging)
	{
		jo.enableBlocklink = false;
		Core::g_CoreStartupParameter.bSkipIdle = false;
	}
	else
	{
		if (!Core::g_CoreStartupParameter.bJITBlockLinking)
		{
			jo.enableBlocklink = false;
		}
		else
			jo.enableBlocklink = !Core::g_CoreStartupParameter.bMMU;
	}
	jo.fpAccurateFcmp = Core::g_CoreStartupParameter.bEnableFPRF;
	jo.optimizeGatherPipe = true;
	jo.fastInterrupts = false;
	jo.accurateSinglePrecision = true;
	js.memcheck = Core::g_CoreStartupParameter.bMMU;

	gpr.SetEmitter(this);
	fpr.SetEmitter(this);

	trampolines.Init();
	AllocCodeSpace(CODE_SIZE);

	blocks.Init();
	asm_routines.Init();
}

void TraceJit::ClearCache()
{
	blocks.Clear();
	trampolines.ClearCodeSpace();
	ClearCodeSpace();
}

void TraceJit::Shutdown()
{
	FreeCodeSpace();

	blocks.Shutdown();
	trampolines.Shutdown();
	asm_routines.Shutdown();
}

// This is only called by Default() in this file. It will execute an instruction with the interpreter functions.
void TraceJit::WriteCallInterpreter(UGeckoInstruction inst)
{
	gpr.Flush(FLUSH_ALL);
	fpr.Flush(FLUSH_ALL);
	if (js.isLastInstruction)
	{
		MOV(32, M(&PC), Imm32(js.compilerPC));
		MOV(32, M(&NPC), Imm32(js.compilerPC + 4));
	}
	Interpreter::_interpreterInstruction instr = GetInterpreterOp(inst);
	ABI_CallFunctionC((void*)instr, inst.hex);
}

void TraceJit::unknown_instruction(UGeckoInstruction inst)
{
	PanicAlert("unknown_instruction %08x - Fix me ;)", inst.hex);
}

void TraceJit::Default(UGeckoInstruction _inst)
{
	WriteCallInterpreter(_inst.hex);
}

void TraceJit::HLEFunction(UGeckoInstruction _inst)
{
	gpr.Flush(FLUSH_ALL);
	fpr.Flush(FLUSH_ALL);
	ABI_CallFunctionCC((void*)&HLE::Execute, js.compilerPC, _inst.hex);
}

void TraceJit::DoNothing(UGeckoInstruction _inst)
{
	// Yup, just don't do anything.
}

static const bool ImHereDebug = false;
static const bool ImHereLog = false;
static std::map<u32, int> been_here;

static void ImHere()
{
	static File::IOFile f;
	if (ImHereLog)
	{
		if (!f)
		{
#ifdef _M_X64
			f.Open("log64.txt", "w");
#else
			f.Open("log32.txt", "w");
#endif
		}
		fprintf(f.GetHandle(), "%08x\n", PC);
	}
	if (been_here.find(PC) != been_here.end())
	{
		been_here.find(PC)->second++;
		if ((been_here.find(PC)->second) & 1023)
			return;
	}
	DEBUG_LOG(DYNA_REC, "I'm here - PC = %08x , LR = %08x", PC, LR);
	been_here[PC] = 1;
}

void TraceJit::Cleanup()
{
	if (jo.optimizeGatherPipe && js.fifoBytesThisBlock > 0)
	{
		ABI_CallFunction((void *)&GPFifo::CheckGatherPipe);
	}

	// SPEED HACK: MMCR0/MMCR1 should be checked at run-time, not at compile time.
	if (MMCR0.Hex || MMCR1.Hex)
		ABI_CallFunctionCCC((void *)&PowerPC::UpdatePerformanceMonitor, js.downcountAmount, jit->js.numLoadStoreInst, jit->js.numFloatingPointInst);
}

void TraceJit::WriteExit(u32 destination, int exit_num)
{
	Cleanup();

	SUB(32, M(&CoreTiming::downcount), js.downcountAmount > 127 ? Imm32(js.downcountAmount) : Imm8(js.downcountAmount));

	//If nobody has taken care of this yet (this can be removed when all branches are done)
	JitBlock *b = js.curBlock;
	b->exitAddress[exit_num] = destination;
	b->exitPtrs[exit_num] = GetWritableCodePtr();

	// Link opportunity!
	if (jo.enableBlocklink)
	{
		int block = blocks.GetBlockNumberFromStartAddress(destination);
		if (block >= 0)
		{
			// It exists! Joy of joy!
			JMP(blocks.GetBlock(block)->checkedEntry, true);
			b->linkStatus[exit_num] = true;
			return;
		}
	}
	MOV(32, M(&PC), Imm32(destination));
	JMP(asm_routines.dispatcher, true);
}

void TraceJit::WriteExitDestInEAX()
{
	MOV(32, M(&PC), R(EAX));
	Cleanup();
	SUB(32, M(&CoreTiming::downcount), js.downcountAmount > 127 ? Imm32(js.downcountAmount) : Imm8(js.downcountAmount));
	JMP(asm_routines.dispatcher, true);
}

void TraceJit::WriteRfiExitDestInEAX()
{
	MOV(32, M(&PC), R(EAX));
	MOV(32, M(&NPC), R(EAX));
	Cleanup();
	ABI_CallFunction(reinterpret_cast<void *>(&PowerPC::CheckExceptions));
	SUB(32, M(&CoreTiming::downcount), js.downcountAmount > 127 ? Imm32(js.downcountAmount) : Imm8(js.downcountAmount));
	JMP(asm_routines.dispatcher, true);
}

void TraceJit::WriteExceptionExit()
{
	Cleanup();
	MOV(32, R(EAX), M(&PC));
	MOV(32, M(&NPC), R(EAX));
	ABI_CallFunction(reinterpret_cast<void *>(&PowerPC::CheckExceptions));
	SUB(32, M(&CoreTiming::downcount), js.downcountAmount > 127 ? Imm32(js.downcountAmount) : Imm8(js.downcountAmount));
	JMP(asm_routines.dispatcher, true);
}

void TraceJit::WriteExternalExceptionExit()
{
	Cleanup();
	MOV(32, R(EAX), M(&PC));
	MOV(32, M(&NPC), R(EAX));
	ABI_CallFunction(reinterpret_cast<void *>(&PowerPC::CheckExternalExceptions));
	SUB(32, M(&CoreTiming::downcount), js.downcountAmount > 127 ? Imm32(js.downcountAmount) : Imm8(js.downcountAmount));
	JMP(asm_routines.dispatcher, true);
}

void STACKALIGN TraceJit::Run()
{
	CompiledCode pExecAddr = (CompiledCode)asm_routines.enterCode;
	pExecAddr();
}

void TraceJit::SingleStep()
{
	CompiledCode pExecAddr = (CompiledCode)asm_routines.enterCode;
	pExecAddr();
}

void TraceJit::Trace()
{
	char regs[500] = "";
	char fregs[750] = "";

#ifdef JIT_LOG_GPR
	for (int i = 0; i < 32; i++)
	{
		char reg[50];
		sprintf(reg, "r%02d: %08x ", i, PowerPC::ppcState.gpr[i]);
		strncat(regs, reg, 500);
	}
#endif

#ifdef JIT_LOG_FPR
	for (int i = 0; i < 32; i++)
	{
		char reg[50];
		sprintf(reg, "f%02d: %016x ", i, riPS0(i));
		strncat(fregs, reg, 750);
	}
#endif

	DEBUG_LOG(DYNA_REC, "JIT64 PC: %08x SRR0: %08x SRR1: %08x CRfast: %02x%02x%02x%02x%02x%02x%02x%02x FPSCR: %08x MSR: %08x LR: %08x %s %s",
		PC, SRR0, SRR1, PowerPC::ppcState.cr_fast[0], PowerPC::ppcState.cr_fast[1], PowerPC::ppcState.cr_fast[2], PowerPC::ppcState.cr_fast[3],
		PowerPC::ppcState.cr_fast[4], PowerPC::ppcState.cr_fast[5], PowerPC::ppcState.cr_fast[6], PowerPC::ppcState.cr_fast[7], PowerPC::ppcState.fpscr,
		PowerPC::ppcState.msr, PowerPC::ppcState.spr[8], regs, fregs);
}

void STACKALIGN TraceJit::Jit(u32 em_address)
{
	if (GetSpaceLeft() < 0x10000 || blocks.IsFull() || Core::g_CoreStartupParameter.bJITNoBlockCache)
	{
		ClearCache();
	}

	int block_num = blocks.AllocateBlock(em_address);
	JitBlock *b = blocks.GetBlock(block_num);
	blocks.FinalizeBlock(block_num, jo.enableBlocklink, DoJit(em_address, &code_buffer, b));
}

const u8* TraceJit::DoJit(u32 em_address, PPCAnalyst::CodeBuffer *code_buf, JitBlock *b)
{
	int blockSize = code_buf->GetSize();

	// Memory exception on instruction fetch
	bool memory_exception = false;

	// A broken block is a block that does not end in a branch
	bool broken_block = false;

	if (Core::g_CoreStartupParameter.bEnableDebugging)
	{
		// Comment out the following to disable breakpoints (speed-up)
		if (!Profiler::g_ProfileBlocks)
		{
			if (GetState() == CPU_STEPPING)
				blockSize = 1;
			Trace();
		}
	}

	if (em_address == 0)
	{
		// Memory exception occurred during instruction fetch
		memory_exception = true;
	}

	if (Core::g_CoreStartupParameter.bMMU && (em_address & JIT_ICACHE_VMEM_BIT))
	{
		if (!Memory::TranslateAddress(em_address, Memory::FLAG_OPCODE))
		{
			// Memory exception occurred during instruction fetch
			memory_exception = true;
		}
	}

	int size = 0;
	js.firstFPInstructionFound = false;
	js.isLastInstruction = false;
	js.blockStart = em_address;
	js.fifoBytesThisBlock = 0;
	js.curBlock = b;
	js.block_flags = 0;
	js.cancel = false;
	jit->js.numLoadStoreInst = 0;
	jit->js.numFloatingPointInst = 0;

	// Analyze the block, collect all instructions it is made of (including inlining,
	// if that is enabled), reorder instructions for optimal performance, and join joinable instructions.
	u32 nextPC = em_address;
	u32 merged_addresses[32];
	const int capacity_of_merged_addresses = sizeof(merged_addresses) / sizeof(merged_addresses[0]);
	int size_of_merged_addresses = 0;
	if (!memory_exception)
	{
		// If there is a memory exception inside a block (broken_block==true), compile up to that instruction.
		nextPC = PPCAnalyst::Flatten(em_address, &size, &js.st, &js.gpa, &js.fpa, broken_block, code_buf, blockSize, merged_addresses, capacity_of_merged_addresses, size_of_merged_addresses);
	}

	PPCAnalyst::CodeOp *ops = code_buf->codebuffer;

	const u8 *start = AlignCode4(); // TODO: Test if this or AlignCode16 make a difference from GetCodePtr
	b->checkedEntry = start;
	b->runCount = 0;

	// Downcount flag check. The last block decremented downcounter, and the flag should still be available.
	FixupBranch skip = J_CC(CC_NBE);
	MOV(32, M(&PC), Imm32(js.blockStart));
	JMP(asm_routines.doTiming, true);  // downcount hit zero - go doTiming.
	SetJumpTarget(skip);

	const u8 *normalEntry = GetCodePtr();
	b->normalEntry = normalEntry;

	if (ImHereDebug)
		ABI_CallFunction((void *)&ImHere); //Used to get a trace of the last few blocks before a crash, sometimes VERY useful

	// Conditionally add profiling code.
	if (Profiler::g_ProfileBlocks) {
		ADD(32, M(&b->runCount), Imm8(1));
#ifdef _WIN32
		b->ticCounter = 0;
		b->ticStart = 0;
		b->ticStop = 0;
#else
//TODO
#endif
		// get start tic
		PROFILER_QUERY_PERFORMANCE_COUNTER(&b->ticStart);
	}
#if defined(_DEBUG) || defined(DEBUGFAST) || defined(NAN_CHECK)
	// should help logged stack-traces become more accurate
	MOV(32, M(&PC), Imm32(js.blockStart));
#endif

	// Start up the register allocators
	// They use the information in gpa/fpa to preload commonly used registers.
	gpr.Start(js.gpa);
	fpr.Start(js.fpa);

	js.downcountAmount = 0;
	if (!Core::g_CoreStartupParameter.bEnableDebugging)
	{
		for (int i = 0; i < size_of_merged_addresses; ++i)
		{
			const u32 address = merged_addresses[i];
			js.downcountAmount += PatchEngine::GetSpeedhackCycles(address);
		}
	}

	js.skipnext = false;
	js.blockSize = size;
	js.compilerPC = nextPC;
	// Translate instructions
	for (int i = 0; i < (int)size; i++)
	{
		js.compilerPC = ops[i].address;
		js.op = &ops[i];
		js.instructionNumber = i;
		const GekkoOPInfo *opinfo = ops[i].opinfo;
		js.downcountAmount += (opinfo->numCyclesMinusOne + 1);

		if (i == (int)size - 1)
		{
			// WARNING - cmp->branch merging will screw this up.
			js.isLastInstruction = true;
			js.next_inst = 0;
			if (Profiler::g_ProfileBlocks) {
				// CAUTION!!! push on stack regs you use, do your stuff, then pop
				PROFILER_VPUSH;
				// get end tic
				PROFILER_QUERY_PERFORMANCE_COUNTER(&b->ticStop);
				// tic counter += (end tic - start tic)
				PROFILER_ADD_DIFF_LARGE_INTEGER(&b->ticCounter, &b->ticStop, &b->ticStart);
				PROFILER_VPOP;
			}
		}
		else
		{
			// help peephole optimizations
			js.next_inst = ops[i + 1].inst;
			js.next_compilerPC = ops[i + 1].address;
		}

		if (jo.optimizeGatherPipe && js.fifoBytesThisBlock >= 32)
		{
			js.fifoBytesThisBlock -= 32;
			MOV(32, M(&PC), Imm32(jit->js.compilerPC)); // Helps external systems know which instruction triggered the write
			u32 registersInUse = RegistersInUse();
			ABI_PushRegistersAndAdjustStack(registersInUse, false);
			ABI_CallFunction((void *)&GPFifo::CheckGatherPipe);
			ABI_PopRegistersAndAdjustStack(registersInUse, false);
		}

		u32 function = HLE::GetFunctionIndex(ops[i].address);
		if (function != 0)
		{
			int type = HLE::GetFunctionTypeByIndex(function);
			if (type == HLE::HLE_HOOK_START || type == HLE::HLE_HOOK_REPLACE)
			{
				int flags = HLE::GetFunctionFlagsByIndex(function);
				if (HLE::IsEnabled(flags))
				{
					HLEFunction(function);
					if (type == HLE::HLE_HOOK_REPLACE)
					{
						MOV(32, R(EAX), M(&NPC));
						js.downcountAmount += js.st.numCycles;
						WriteExitDestInEAX();
						break;
					}
				}
			}
		}

		if (!ops[i].skip)
		{
			if ((opinfo->flags & FL_USE_FPU) && !js.firstFPInstructionFound)
			{
				gpr.Flush(FLUSH_ALL);
				fpr.Flush(FLUSH_ALL);

				//This instruction uses FPU - needs to add FP exception bailout
				TEST(32, M(&PowerPC::ppcState.msr), Imm32(1 << 13)); // Test FP enabled bit
				FixupBranch b1 = J_CC(CC_NZ, true);

				// If a FPU exception occurs, the exception handler will read
				// from PC.  Update PC with the latest value in case that happens.
				MOV(32, M(&PC), Imm32(ops[i].address));
				OR(32, M((void *)&PowerPC::ppcState.Exceptions), Imm32(EXCEPTION_FPU_UNAVAILABLE));
				WriteExceptionExit();

				SetJumpTarget(b1);

				js.firstFPInstructionFound = true;
			}

			// Add an external exception check if the instruction writes to the FIFO.
			if (jit->js.fifoWriteAddresses.find(ops[i].address) != jit->js.fifoWriteAddresses.end())
			{
				gpr.Flush(FLUSH_ALL);
				fpr.Flush(FLUSH_ALL);

				TEST(32, M((void *)&PowerPC::ppcState.Exceptions), Imm32(EXCEPTION_ISI | EXCEPTION_PROGRAM | EXCEPTION_SYSCALL | EXCEPTION_FPU_UNAVAILABLE | EXCEPTION_DSI | EXCEPTION_ALIGNMENT));
				FixupBranch clearInt = J_CC(CC_NZ, true);
				TEST(32, M((void *)&PowerPC::ppcState.Exceptions), Imm32(EXCEPTION_EXTERNAL_INT));
				FixupBranch noExtException = J_CC(CC_Z, true);
				TEST(32, M((void *)&PowerPC::ppcState.msr), Imm32(0x0008000));
				FixupBranch noExtIntEnable = J_CC(CC_Z, true);
				TEST(32, M((void *)&ProcessorInterface::m_InterruptCause), Imm32(ProcessorInterface::INT_CAUSE_CP | ProcessorInterface::INT_CAUSE_PE_TOKEN | ProcessorInterface::INT_CAUSE_PE_FINISH));
				FixupBranch noCPInt = J_CC(CC_Z, true);

				MOV(32, M(&PC), Imm32(ops[i].address));
				WriteExternalExceptionExit();

				SetJumpTarget(noCPInt);
				SetJumpTarget(noExtIntEnable);
				SetJumpTarget(noExtException);
				SetJumpTarget(clearInt);
			}

			if (Core::g_CoreStartupParameter.bEnableDebugging && breakpoints.IsAddressBreakPoint(ops[i].address) && GetState() != CPU_STEPPING)
			{
				gpr.Flush(FLUSH_ALL);
				fpr.Flush(FLUSH_ALL);

				MOV(32, M(&PC), Imm32(ops[i].address));
				ABI_CallFunction(reinterpret_cast<void *>(&PowerPC::CheckBreakPoints));
				TEST(32, M((void*)PowerPC::GetStatePtr()), Imm32(0xFFFFFFFF));
				FixupBranch noBreakpoint = J_CC(CC_Z);

				WriteExit(ops[i].address, 0);
				SetJumpTarget(noBreakpoint);
			}

			TraceJitTables::CompileInstruction(ops[i]);

			if (js.memcheck && (opinfo->flags & FL_LOADSTORE))
			{
				// In case we are about to jump to the dispatcher, flush regs
				gpr.Flush(FLUSH_ALL);
				fpr.Flush(FLUSH_ALL);

				TEST(32, M((void *)&PowerPC::ppcState.Exceptions), Imm32(EXCEPTION_DSI));
				FixupBranch noMemException = J_CC(CC_Z, true);

				// If a memory exception occurs, the exception handler will read
				// from PC.  Update PC with the latest value in case that happens.
				MOV(32, M(&PC), Imm32(ops[i].address));
				WriteExceptionExit();
				SetJumpTarget(noMemException);
			}

			if (opinfo->flags & FL_LOADSTORE)
				++jit->js.numLoadStoreInst;

			if (opinfo->flags & FL_USE_FPU)
				++jit->js.numFloatingPointInst;
		}

#if defined(_DEBUG) || defined(DEBUGFAST)
		if (gpr.SanityCheck() || fpr.SanityCheck())
		{
			char ppcInst[256];
			DisassembleGekko(ops[i].inst.hex, em_address, ppcInst, 256);
			//NOTICE_LOG(DYNA_REC, "Unflushed register: %s", ppcInst);
		}
#endif
		if (js.skipnext) {
			js.skipnext = false;
			i++; // Skip next instruction
		}

		if (js.cancel)
			break;
	}

	u32 function = HLE::GetFunctionIndex(js.blockStart);
	if (function != 0)
	{
		int type = HLE::GetFunctionTypeByIndex(function);
		if (type == HLE::HLE_HOOK_END)
		{
			int flags = HLE::GetFunctionFlagsByIndex(function);
			if (HLE::IsEnabled(flags))
			{
				HLEFunction(function);
			}
		}
	}

	if (memory_exception)
	{
		// Address of instruction could not be translated
		MOV(32, M(&NPC), Imm32(js.compilerPC));

		OR(32, M((void *)&PowerPC::ppcState.Exceptions), Imm32(EXCEPTION_ISI));

		// Remove the invalid instruction from the icache, forcing a recompile
#ifdef _M_IX86
		MOV(32, M(jit->GetBlockCache()->GetICachePtr(js.compilerPC)), Imm32(JIT_ICACHE_INVALID_WORD));
#else
		MOV(64, R(RAX), ImmPtr(jit->GetBlockCache()->GetICachePtr(js.compilerPC)));
		MOV(32,MatR(RAX),Imm32(JIT_ICACHE_INVALID_WORD));
#endif

		WriteExceptionExit();
	}

	if (broken_block)
	{
		gpr.Flush(FLUSH_ALL);
		fpr.Flush(FLUSH_ALL);
		WriteExit(nextPC, 0);
	}

	b->flags = js.block_flags;
	b->codeSize = (u32)(GetCodePtr() - normalEntry);
	b->originalSize = size;

#ifdef JIT_LOG_X86
	LogGeneratedX86(size, code_buf, normalEntry, b);
#endif

	return normalEntry;
}

u32 TraceJit::RegistersInUse()
{
#ifdef _M_X64
	u32 result = 0;
	for (int i = 0; i < NUMXREGS; i++)
	{
		if (!gpr.IsFreeX(i))
			result |= (1 << i);
		if (!fpr.IsFreeX(i))
			result |= (1 << (16 + i));
	}
	return result;
#else
	// not needed
	return 0;
#endif
}
