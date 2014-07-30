// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <cmath>
#include <limits>

#ifdef _WIN32
#include <intrin.h>
#endif

#include "Common/MathUtil.h"
#include "Core/PowerPC/Interpreter/Interpreter.h"
#include "Core/PowerPC/Interpreter/Interpreter_FPUtils.h"

using namespace MathUtil;

// Extremely rare - actually, never seen.
// Star Wars : Rogue Leader spams that at some point :|
void Interpreter::Helper_UpdateCR1()
{
	SetCRField(1, (FPSCR.FX << 4) | (FPSCR.FEX << 3) | (FPSCR.VX << 2) | FPSCR.OX);
}

void Interpreter::Helper_FloatCompareOrdered(UGeckoInstruction _inst, double fa, double fb)
{
	int compareResult;

	if (fa < fb)
	{
		compareResult = FPCC::FL;
	}
	else if (fa > fb)
	{
		compareResult = FPCC::FG;
	}
	else if (fa == fb)
	{
		compareResult = FPCC::FE;
	}
	else // NaN
	{
		FPSCR.FX = 1;
		compareResult = FPCC::FU;
		if (IsSNAN(fa) || IsSNAN(fb))
		{
			SetFPException(FPSCR_VXSNAN);
			if (FPSCR.VE == 0)
			{
				SetFPException(FPSCR_VXVC);
			}
		}
		else // QNaN
		{
			SetFPException(FPSCR_VXVC);
		}
	}

	FPSCR.FPRF = compareResult;
	SetCRField(_inst.CRFD, compareResult);
}

void Interpreter::Helper_FloatCompareUnordered(UGeckoInstruction _inst, double fa, double fb)
{
	int compareResult;

	if (fa < fb)
	{
		compareResult = FPCC::FL;
	}
	else if (fa > fb)
	{
		compareResult = FPCC::FG;
	}
	else if (fa == fb)
	{
		compareResult = FPCC::FE;
	}
	else
	{
		compareResult = FPCC::FU;
		if (IsSNAN(fa) || IsSNAN(fb))
		{
			FPSCR.FX = 1;
			SetFPException(FPSCR_VXSNAN);
		}
	}
	FPSCR.FPRF = compareResult;
	SetCRField(_inst.CRFD, compareResult);
}

void Interpreter::fcmpo(UGeckoInstruction _inst)
{
	Helper_FloatCompareOrdered(_inst, rPS0(_inst.FA), rPS0(_inst.FB));
}

void Interpreter::fcmpu(UGeckoInstruction _inst)
{
	Helper_FloatCompareUnordered(_inst, rPS0(_inst.FA), rPS0(_inst.FB));
}

// Apply current rounding mode
void Interpreter::fctiwx(UGeckoInstruction _inst)
{
	const double b = rPS0(_inst.FB);
	u32 value;
	if (b > (double)0x7fffffff)
	{
		value = 0x7fffffff;
		SetFPException(FPSCR_VXCVI);
		FPSCR.FI = 0;
		FPSCR.FR = 0;
	}
	else if (b < -(double)0x80000000)
	{
		value = 0x80000000;
		SetFPException(FPSCR_VXCVI);
		FPSCR.FI = 0;
		FPSCR.FR = 0;
	}
	else
	{
		s32 i = 0;
		switch (FPSCR.RN)
		{
		case 0: // nearest
			{
				double t = b + 0.5;
				i = (s32)t;
				if (t - i < 0 || (t - i == 0 && b > 0)) i--;
				break;
			}
		case 1: // zero
			i = (s32)b;
			break;
		case 2: // +inf
			i = (s32)b;
			if (b - i > 0) i++;
			break;
		case 3: // -inf
			i = (s32)b;
			if (b - i < 0) i--;
			break;
		}
		value = (u32)i;
		double di = i;
		if (di == b)
		{
			FPSCR.FI = 0;
			FPSCR.FR = 0;
		}
		else
		{
			SetFI(1);
			FPSCR.FR = fabs(di) > fabs(b);
		}
	}
	// based on HW tests
	// FPRF is not affected
	riPS0(_inst.FD) = 0xfff8000000000000ull | value;
	if (value == 0 && ( (*(u64*)&b) & DOUBLE_SIGN ))
		riPS0(_inst.FD) |= 0x100000000ull;
	if (_inst.Rc)
		Helper_UpdateCR1();
}

// Always round toward zero
void Interpreter::fctiwzx(UGeckoInstruction _inst)
{
	const double b = rPS0(_inst.FB);
	u32 value;
	if (b > (double)0x7fffffff)
	{
		value = 0x7fffffff;
		SetFPException(FPSCR_VXCVI);
		FPSCR.FI = 0;
		FPSCR.FR = 0;
	}
	else if (b < -(double)0x80000000)
	{
		value = 0x80000000;
		SetFPException(FPSCR_VXCVI);
		FPSCR.FI = 0;
		FPSCR.FR = 0;
	}
	else
	{
		s32 i = (s32)b;
		double di = i;
		if (di == b)
		{
			FPSCR.FI = 0;
			FPSCR.FR = 0;
		}
		else
		{
			SetFI(1);
			FPSCR.FR = fabs(di) > fabs(b);
		}
		value = (u32)i;
	}
	// based on HW tests
	// FPRF is not affected
	riPS0(_inst.FD) = 0xfff8000000000000ull | value;
	if (value == 0 && ( (*(u64*)&b) & DOUBLE_SIGN ))
		riPS0(_inst.FD) |= 0x100000000ull;
	if (_inst.Rc)
		Helper_UpdateCR1();
}

void Interpreter::fmrx(UGeckoInstruction _inst)
{
	riPS0(_inst.FD) = riPS0(_inst.FB);
	// This is a binary instruction. Does not alter FPSCR
	if (_inst.Rc) Helper_UpdateCR1();
}

void Interpreter::fabsx(UGeckoInstruction _inst)
{
	rPS0(_inst.FD) = fabs(rPS0(_inst.FB));
	// This is a binary instruction. Does not alter FPSCR
	if (_inst.Rc) Helper_UpdateCR1();
}

void Interpreter::fnabsx(UGeckoInstruction _inst)
{
	riPS0(_inst.FD) = riPS0(_inst.FB) | (1ULL << 63);
	// This is a binary instruction. Does not alter FPSCR
	if (_inst.Rc) Helper_UpdateCR1();
}

void Interpreter::fnegx(UGeckoInstruction _inst)
{
	riPS0(_inst.FD) = riPS0(_inst.FB) ^ (1ULL << 63);
	// This is a binary instruction. Does not alter FPSCR
	if (_inst.Rc) Helper_UpdateCR1();
}

void Interpreter::fselx(UGeckoInstruction _inst)
{
	rPS0(_inst.FD) = (rPS0(_inst.FA) >= -0.0) ? rPS0(_inst.FC) : rPS0(_inst.FB);
	// This is a binary instruction. Does not alter FPSCR
	if (_inst.Rc) Helper_UpdateCR1();
}

void Interpreter::frspx(UGeckoInstruction _inst)
{
	riPS0(_inst.FD) = riPS1(_inst.FD) = RoundToSingle(riPS0(_inst.FB));
	UpdateFPRF(rPS0(_inst.FD));
	if (_inst.Rc) Helper_UpdateCR1();
}

void Interpreter::fmulx(UGeckoInstruction _inst)
{
	riPS0(_inst.FD) = MultiplyDoublePrecision(riPS0(_inst.FA), riPS0(_inst.FC));
	UpdateFPRF(rPS0(_inst.FD));
	if (_inst.Rc) Helper_UpdateCR1();
}
void Interpreter::fmulsx(UGeckoInstruction _inst)
{
	u64 value = MultiplySinglePrecision(riPS0(_inst.FA), riPS0(_inst.FC));
	riPS0(_inst.FD) = riPS1(_inst.FD) = value;
	UpdateFPRF(rPS0(_inst.FD));
	if (_inst.Rc) Helper_UpdateCR1();
}

void Interpreter::fmaddx(UGeckoInstruction _inst)
{
	riPS0(_inst.FD) = MaddDoublePrecision(riPS0(_inst.FA), riPS0(_inst.FC), riPS0(_inst.FB), false, false);
	UpdateFPRF(rPS0(_inst.FD));
	if (_inst.Rc) Helper_UpdateCR1();
}

void Interpreter::fmaddsx(UGeckoInstruction _inst)
{
	u64 value = MaddSinglePrecision(riPS0(_inst.FA), riPS0(_inst.FC), riPS0(_inst.FB), false, false);
	riPS0(_inst.FD) = riPS1(_inst.FD) = value;
	UpdateFPRF(rPS0(_inst.FD));
	if (_inst.Rc) Helper_UpdateCR1();
}

void Interpreter::faddx(UGeckoInstruction _inst)
{
	riPS0(_inst.FD) = AddDoublePrecision(riPS0(_inst.FA), riPS0(_inst.FB));
	UpdateFPRF(rPS0(_inst.FD));
	if (_inst.Rc) Helper_UpdateCR1();
}

void Interpreter::faddsx(UGeckoInstruction _inst)
{
	riPS0(_inst.FD) = riPS1(_inst.FD) = AddSinglePrecision(riPS0(_inst.FA), riPS0(_inst.FB));
	UpdateFPRFSingle(riPS0(_inst.FD));
	if (_inst.Rc) Helper_UpdateCR1();
}

void Interpreter::fdivx(UGeckoInstruction _inst)
{
	riPS0(_inst.FD) = DivDoublePrecision(riPS0(_inst.FA), riPS0(_inst.FB));
	UpdateFPRF(rPS0(_inst.FD));
	if (_inst.Rc) Helper_UpdateCR1();
}

void Interpreter::fdivsx(UGeckoInstruction _inst)
{
	riPS0(_inst.FD) = riPS1(_inst.FD) = DivSinglePrecision(riPS0(_inst.FA), riPS0(_inst.FB));
	UpdateFPRF(rPS0(_inst.FD));
	if (_inst.Rc) Helper_UpdateCR1();
}

// Single precision only.
void Interpreter::fresx(UGeckoInstruction _inst)
{
	double b = rPS0(_inst.FB);
	rPS0(_inst.FD) = rPS1(_inst.FD) = ApproximateReciprocal(b);
	if (b == 0.0)
	{
		SetFPException(FPSCR_ZX);
	}
	UpdateFPRF(rPS0(_inst.FD));
	if (_inst.Rc) Helper_UpdateCR1();
}

void Interpreter::frsqrtex(UGeckoInstruction _inst)
{
	double b = rPS0(_inst.FB);
	if (b < 0.0)
	{
		SetFPException(FPSCR_VXSQRT);
	}
	else if (b == 0.0)
	{
		SetFPException(FPSCR_ZX);
	}
	rPS0(_inst.FD) = ApproximateReciprocalSquareRoot(b);
	UpdateFPRF(rPS0(_inst.FD));
	if (_inst.Rc) Helper_UpdateCR1();
}

void Interpreter::fmsubx(UGeckoInstruction _inst)
{
	riPS0(_inst.FD) = MaddDoublePrecision(riPS0(_inst.FA), riPS0(_inst.FC), riPS0(_inst.FB), true, false);
	UpdateFPRF(rPS0(_inst.FD));
	if (_inst.Rc) Helper_UpdateCR1();
}

void Interpreter::fmsubsx(UGeckoInstruction _inst)
{
	riPS0(_inst.FD) = riPS1(_inst.FD) =
		MaddSinglePrecision(riPS0(_inst.FA), riPS0(_inst.FC), riPS0(_inst.FB), true, false);
	UpdateFPRF(rPS0(_inst.FD));
	if (_inst.Rc) Helper_UpdateCR1();
}

void Interpreter::fnmaddx(UGeckoInstruction _inst)
{
	riPS0(_inst.FD) = MaddDoublePrecision(riPS0(_inst.FA), riPS0(_inst.FC), riPS0(_inst.FB), false, true);
	UpdateFPRF(rPS0(_inst.FD));
	if (_inst.Rc) Helper_UpdateCR1();
}
void Interpreter::fnmaddsx(UGeckoInstruction _inst)
{
	riPS0(_inst.FD) = riPS1(_inst.FD) =
		MaddSinglePrecision(riPS0(_inst.FA), riPS0(_inst.FC), riPS0(_inst.FB), false, true);
	UpdateFPRF(rPS0(_inst.FD));
	if (_inst.Rc) Helper_UpdateCR1();
}

void Interpreter::fnmsubx(UGeckoInstruction _inst)
{
	riPS0(_inst.FD) = MaddDoublePrecision(riPS0(_inst.FA), riPS0(_inst.FC), riPS0(_inst.FB), true, true);
	UpdateFPRF(rPS0(_inst.FD));
	if (_inst.Rc) Helper_UpdateCR1();
}

// fnmsubsx does not handle QNAN properly - see NI_msub
void Interpreter::fnmsubsx(UGeckoInstruction _inst)
{
	riPS0(_inst.FD) = riPS1(_inst.FD) =
		MaddSinglePrecision(riPS0(_inst.FA), riPS0(_inst.FC), riPS0(_inst.FB), true, true);
	UpdateFPRF(rPS0(_inst.FD));
	if (_inst.Rc) Helper_UpdateCR1();
}

void Interpreter::fsubx(UGeckoInstruction _inst)
{
	riPS0(_inst.FD) = SubDoublePrecision(riPS0(_inst.FA), riPS0(_inst.FB));
	UpdateFPRF(rPS0(_inst.FD));
	if (_inst.Rc) Helper_UpdateCR1();
}

void Interpreter::fsubsx(UGeckoInstruction _inst)
{
	riPS0(_inst.FD) = riPS1(_inst.FD) = SubSinglePrecision(riPS0(_inst.FA), riPS0(_inst.FB));
	UpdateFPRF(rPS0(_inst.FD));
	if (_inst.Rc) Helper_UpdateCR1();
}

void Interpreter::fsqrtx(UGeckoInstruction _inst)
{
	// GEKKO is not supposed to support this instruction.
	// PanicAlert("fsqrtx");
	double b = rPS0(_inst.FB);
	if (b < 0.0) {
		FPSCR.VXSQRT = 1;
	}
	rPS0(_inst.FD) = sqrt(b);
	UpdateFPRF(rPS0(_inst.FD));
	if (_inst.Rc) Helper_UpdateCR1();
}
