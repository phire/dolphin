// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include "TraceJit.h"
#include "TraceJit_Tables.h"

// Should be moved in to the Jit class
typedef void (TraceJit::*_Instruction) (UGeckoInstruction instCode);

static _Instruction dynaOpTable[64];
static _Instruction dynaOpTable4[1024];
static _Instruction dynaOpTable19[1024];
static _Instruction dynaOpTable31[1024];
static _Instruction dynaOpTable59[32];
static _Instruction dynaOpTable63[1024];
void TraceJit::DynaRunTable4(UGeckoInstruction _inst)  {(this->*dynaOpTable4 [_inst.SUBOP10])(_inst);}
void TraceJit::DynaRunTable19(UGeckoInstruction _inst) {(this->*dynaOpTable19[_inst.SUBOP10])(_inst);}
void TraceJit::DynaRunTable31(UGeckoInstruction _inst) {(this->*dynaOpTable31[_inst.SUBOP10])(_inst);}
void TraceJit::DynaRunTable59(UGeckoInstruction _inst) {(this->*dynaOpTable59[_inst.SUBOP5 ])(_inst);}
void TraceJit::DynaRunTable63(UGeckoInstruction _inst) {(this->*dynaOpTable63[_inst.SUBOP10])(_inst);}

struct GekkoOPTemplate
{
	int opcode;
	_Instruction Inst;
	//GekkoOPInfo opinfo; // Doesn't need opinfo, Interpreter fills it out
};

static GekkoOPTemplate primarytable[] =
{
	{4,  &TraceJit::DynaRunTable4}, //"RunTable4",  OPTYPE_SUBTABLE | (4<<24), 0}},
	{19, &TraceJit::DynaRunTable19}, //"RunTable19", OPTYPE_SUBTABLE | (19<<24), 0}},
	{31, &TraceJit::DynaRunTable31}, //"RunTable31", OPTYPE_SUBTABLE | (31<<24), 0}},
	{59, &TraceJit::DynaRunTable59}, //"RunTable59", OPTYPE_SUBTABLE | (59<<24), 0}},
	{63, &TraceJit::DynaRunTable63}, //"RunTable63", OPTYPE_SUBTABLE | (63<<24), 0}},

	{16, &TraceJit::bcx}, //"bcx", OPTYPE_SYSTEM, FL_ENDBLOCK}},
	{18, &TraceJit::bx}, //"bx",  OPTYPE_SYSTEM, FL_ENDBLOCK}},

	{1,  &TraceJit::HLEFunction}, //"HLEFunction", OPTYPE_SYSTEM, FL_ENDBLOCK}},
	{2,  &TraceJit::Default}, //"DynaBlock",   OPTYPE_SYSTEM, 0}},
	{3,  &TraceJit::twx}, //"twi",         OPTYPE_SYSTEM, FL_ENDBLOCK}},
	{17, &TraceJit::sc}, //"sc",          OPTYPE_SYSTEM, FL_ENDBLOCK, 1}},

	{7,  &TraceJit::Default}, //"mulli",    OPTYPE_INTEGER, FL_OUT_D | FL_IN_A | FL_RC_BIT, 2}},
	{8,  &TraceJit::Default}, //"subfic",   OPTYPE_INTEGER, FL_OUT_D | FL_IN_A |	FL_SET_CA}},
	{10, &TraceJit::Default}, //"cmpli",    OPTYPE_INTEGER, FL_IN_A | FL_SET_CRn}},
	{11, &TraceJit::Default}, //"cmpi",     OPTYPE_INTEGER, FL_IN_A | FL_SET_CRn}},
	{12, &TraceJit::Default}, //"addic",    OPTYPE_INTEGER, FL_OUT_D | FL_IN_A | FL_SET_CA}},
	{13, &TraceJit::Default}, //"addic_rc", OPTYPE_INTEGER, FL_OUT_D | FL_IN_A | FL_SET_CR0}},
	{14, &TraceJit::Default}, //"addi",     OPTYPE_INTEGER, FL_OUT_D | FL_IN_A0}},
	{15, &TraceJit::Default}, //"addis",    OPTYPE_INTEGER, FL_OUT_D | FL_IN_A0}},

	{20, &TraceJit::Default}, //"rlwimix",  OPTYPE_INTEGER, FL_OUT_A | FL_IN_A | FL_IN_S | FL_RC_BIT}},
	{21, &TraceJit::Default}, //"rlwinmx",  OPTYPE_INTEGER, FL_OUT_A | FL_IN_S | FL_RC_BIT}},
	{23, &TraceJit::Default}, //"rlwnmx",   OPTYPE_INTEGER, FL_OUT_A | FL_IN_S | FL_IN_B | FL_RC_BIT}},

	{24, &TraceJit::Default}, //"ori",      OPTYPE_INTEGER, FL_OUT_A | FL_IN_S}},
	{25, &TraceJit::Default}, //"oris",     OPTYPE_INTEGER, FL_OUT_A | FL_IN_S}},
	{26, &TraceJit::Default}, //"xori",     OPTYPE_INTEGER, FL_OUT_A | FL_IN_S}},
	{27, &TraceJit::Default}, //"xoris",    OPTYPE_INTEGER, FL_OUT_A | FL_IN_S}},
	{28, &TraceJit::Default}, //"andi_rc",  OPTYPE_INTEGER, FL_OUT_A | FL_IN_S | FL_SET_CR0}},
	{29, &TraceJit::Default}, //"andis_rc", OPTYPE_INTEGER, FL_OUT_A | FL_IN_S | FL_SET_CR0}},

	{32, &TraceJit::Default}, //"lwz",  OPTYPE_LOAD, FL_OUT_D | FL_IN_A}},
	{33, &TraceJit::Default}, //"lwzu", OPTYPE_LOAD, FL_OUT_D | FL_OUT_A | FL_IN_A}},
	{34, &TraceJit::Default}, //"lbz",  OPTYPE_LOAD, FL_OUT_D | FL_IN_A}},
	{35, &TraceJit::Default}, //"lbzu", OPTYPE_LOAD, FL_OUT_D | FL_OUT_A | FL_IN_A}},
	{40, &TraceJit::Default}, //"lhz",  OPTYPE_LOAD, FL_OUT_D | FL_IN_A}},
	{41, &TraceJit::Default}, //"lhzu", OPTYPE_LOAD, FL_OUT_D | FL_OUT_A | FL_IN_A}},
	{42, &TraceJit::Default}, //"lha",  OPTYPE_LOAD, FL_OUT_D | FL_IN_A}},
	{43, &TraceJit::Default}, //"lhau", OPTYPE_LOAD, FL_OUT_D | FL_OUT_A | FL_IN_A}},

	{44, &TraceJit::Default}, //"sth",  OPTYPE_STORE, FL_IN_A | FL_IN_S}},
	{45, &TraceJit::Default}, //"sthu", OPTYPE_STORE, FL_OUT_A | FL_IN_A | FL_IN_S}},
	{36, &TraceJit::Default}, //"stw",  OPTYPE_STORE, FL_IN_A | FL_IN_S}},
	{37, &TraceJit::Default}, //"stwu", OPTYPE_STORE, FL_OUT_A | FL_IN_A | FL_IN_S}},
	{38, &TraceJit::Default}, //"stb",  OPTYPE_STORE, FL_IN_A | FL_IN_S}},
	{39, &TraceJit::Default}, //"stbu", OPTYPE_STORE, FL_OUT_A | FL_IN_A | FL_IN_S}},

	{46, &TraceJit::Default}, //"lmw",   OPTYPE_SYSTEM, FL_EVIL, 10}},
	{47, &TraceJit::Default}, //"stmw",  OPTYPE_SYSTEM, FL_EVIL, 10}},

	{48, &TraceJit::Default}, //"lfs",  OPTYPE_LOADFP, FL_IN_A}},
	{49, &TraceJit::Default}, //"lfsu", OPTYPE_LOADFP, FL_OUT_A | FL_IN_A}},
	{50, &TraceJit::Default}, //"lfd",  OPTYPE_LOADFP, FL_IN_A}},
	{51, &TraceJit::Default}, //"lfdu", OPTYPE_LOADFP, FL_OUT_A | FL_IN_A}},

	{52, &TraceJit::Default}, //"stfs",  OPTYPE_STOREFP, FL_IN_A}},
	{53, &TraceJit::Default}, //"stfsu", OPTYPE_STOREFP, FL_OUT_A | FL_IN_A}},
	{54, &TraceJit::Default}, //"stfd",  OPTYPE_STOREFP, FL_IN_A}},
	{55, &TraceJit::Default}, //"stfdu", OPTYPE_STOREFP, FL_OUT_A | FL_IN_A}},

	{56, &TraceJit::Default}, //"psq_l",   OPTYPE_PS, FL_IN_A}},
	{57, &TraceJit::Default}, //"psq_lu",  OPTYPE_PS, FL_OUT_A | FL_IN_A}},
	{60, &TraceJit::Default}, //"psq_st",  OPTYPE_PS, FL_IN_A}},
	{61, &TraceJit::Default}, //"psq_stu", OPTYPE_PS, FL_OUT_A | FL_IN_A}},

	//missing: 0, 5, 6, 9, 22, 30, 62, 58
	{0,  &TraceJit::Default}, //"unknown_instruction", OPTYPE_UNKNOWN, 0}},
	{5,  &TraceJit::Default}, //"unknown_instruction", OPTYPE_UNKNOWN, 0}},
	{6,  &TraceJit::Default}, //"unknown_instruction", OPTYPE_UNKNOWN, 0}},
	{9,  &TraceJit::Default}, //"unknown_instruction", OPTYPE_UNKNOWN, 0}},
	{22, &TraceJit::Default}, //"unknown_instruction", OPTYPE_UNKNOWN, 0}},
	{30, &TraceJit::Default}, //"unknown_instruction", OPTYPE_UNKNOWN, 0}},
	{62, &TraceJit::Default}, //"unknown_instruction", OPTYPE_UNKNOWN, 0}},
	{58, &TraceJit::Default}, //"unknown_instruction", OPTYPE_UNKNOWN, 0}},
};

static GekkoOPTemplate table4[] =
{    //SUBOP10
	{0,    &TraceJit::Default}, //"ps_cmpu0",   OPTYPE_PS, FL_SET_CRn}},
	{32,   &TraceJit::Default}, //"ps_cmpo0",   OPTYPE_PS, FL_SET_CRn}},
	{40,   &TraceJit::Default}, //"ps_neg",     OPTYPE_PS, FL_RC_BIT}},
	{136,  &TraceJit::Default}, //"ps_nabs",    OPTYPE_PS, FL_RC_BIT}},
	{264,  &TraceJit::Default}, //"ps_abs",     OPTYPE_PS, FL_RC_BIT}},
	{64,   &TraceJit::Default}, //"ps_cmpu1",   OPTYPE_PS, FL_RC_BIT}},
	{72,   &TraceJit::Default}, //"ps_mr",      OPTYPE_PS, FL_RC_BIT}},
	{96,   &TraceJit::Default}, //"ps_cmpo1",   OPTYPE_PS, FL_RC_BIT}},
	{528,  &TraceJit::Default}, //"ps_merge00", OPTYPE_PS, FL_RC_BIT}},
	{560,  &TraceJit::Default}, //"ps_merge01", OPTYPE_PS, FL_RC_BIT}},
	{592,  &TraceJit::Default}, //"ps_merge10", OPTYPE_PS, FL_RC_BIT}},
	{624,  &TraceJit::Default}, //"ps_merge11", OPTYPE_PS, FL_RC_BIT}},

	{1014, &TraceJit::Default}, //"dcbz_l",     OPTYPE_SYSTEM, 0}},
};

static GekkoOPTemplate table4_2[] =
{
	{10, &TraceJit::Default}, //"ps_sum0",   OPTYPE_PS, 0}},
	{11, &TraceJit::Default}, //"ps_sum1",   OPTYPE_PS, 0}},
	{12, &TraceJit::Default}, //"ps_muls0",  OPTYPE_PS, 0}},
	{13, &TraceJit::Default}, //"ps_muls1",  OPTYPE_PS, 0}},
	{14, &TraceJit::Default}, //"ps_madds0", OPTYPE_PS, 0}},
	{15, &TraceJit::Default}, //"ps_madds1", OPTYPE_PS, 0}},
	{18, &TraceJit::Default}, //"ps_div",    OPTYPE_PS, 0, 16}},
	{20, &TraceJit::Default}, //"ps_sub",    OPTYPE_PS, 0}},
	{21, &TraceJit::Default}, //"ps_add",    OPTYPE_PS, 0}},
	{23, &TraceJit::Default}, //"ps_sel",    OPTYPE_PS, 0}},
	{24, &TraceJit::Default}, //"ps_res",    OPTYPE_PS, 0}},
	{25, &TraceJit::Default}, //"ps_mul",    OPTYPE_PS, 0}},
	{26, &TraceJit::Default}, //"ps_rsqrte", OPTYPE_PS, 0, 1}},
	{28, &TraceJit::Default}, //"ps_msub",   OPTYPE_PS, 0}},
	{29, &TraceJit::Default}, //"ps_madd",   OPTYPE_PS, 0}},
	{30, &TraceJit::Default}, //"ps_nmsub",  OPTYPE_PS, 0}},
	{31, &TraceJit::Default}, //"ps_nmadd",  OPTYPE_PS, 0}},
};


static GekkoOPTemplate table4_3[] =
{
	{6,  &TraceJit::Default}, //"psq_lx",   OPTYPE_PS, 0}},
	{7,  &TraceJit::Default}, //"psq_stx",  OPTYPE_PS, 0}},
	{38, &TraceJit::Default}, //"psq_lux",  OPTYPE_PS, 0}},
	{39, &TraceJit::Default}, //"psq_stux", OPTYPE_PS, 0}},
};

static GekkoOPTemplate table19[] =
{
	{528, &TraceJit::bcctrx}, //"bcctrx", OPTYPE_BRANCH, FL_ENDBLOCK}},
	{16,  &TraceJit::bclrx}, //"bclrx",  OPTYPE_BRANCH, FL_ENDBLOCK}},
	{257, &TraceJit::Default}, //"crand",  OPTYPE_CR, FL_EVIL}},
	{129, &TraceJit::Default}, //"crandc", OPTYPE_CR, FL_EVIL}},
	{289, &TraceJit::Default}, //"creqv",  OPTYPE_CR, FL_EVIL}},
	{225, &TraceJit::Default}, //"crnand", OPTYPE_CR, FL_EVIL}},
	{33,  &TraceJit::Default}, //"crnor",  OPTYPE_CR, FL_EVIL}},
	{449, &TraceJit::Default}, //"cror",   OPTYPE_CR, FL_EVIL}},
	{417, &TraceJit::Default}, //"crorc",  OPTYPE_CR, FL_EVIL}},
	{193, &TraceJit::Default}, //"crxor",  OPTYPE_CR, FL_EVIL}},

	{150, &TraceJit::DoNothing}, //"isync",  OPTYPE_ICACHE, FL_EVIL}},
	{0,   &TraceJit::Default}, //"mcrf",   OPTYPE_SYSTEM, FL_EVIL}},

	{50,  &TraceJit::rfi}, //"rfi",    OPTYPE_SYSTEM, FL_ENDBLOCK | FL_CHECKEXCEPTIONS, 1}},
	{18,  &TraceJit::Default}, //"rfid",   OPTYPE_SYSTEM, FL_ENDBLOCK | FL_CHECKEXCEPTIONS}}
};


static GekkoOPTemplate table31[] =
{
	{28,  &TraceJit::Default}, //"andx",   OPTYPE_INTEGER, FL_OUT_A | FL_IN_SB | FL_RC_BIT}},
	{60,  &TraceJit::Default}, //"andcx",  OPTYPE_INTEGER, FL_OUT_A | FL_IN_SB | FL_RC_BIT}},
	{444, &TraceJit::Default}, //"orx",    OPTYPE_INTEGER, FL_OUT_A | FL_IN_SB | FL_RC_BIT}},
	{124, &TraceJit::Default}, //"norx",   OPTYPE_INTEGER, FL_OUT_A | FL_IN_SB | FL_RC_BIT}},
	{316, &TraceJit::Default}, //"xorx",   OPTYPE_INTEGER, FL_OUT_A | FL_IN_SB | FL_RC_BIT}},
	{412, &TraceJit::Default}, //"orcx",   OPTYPE_INTEGER, FL_OUT_A | FL_IN_SB | FL_RC_BIT}},
	{476, &TraceJit::Default}, //"nandx",  OPTYPE_INTEGER, FL_OUT_A | FL_IN_SB | FL_RC_BIT}},
	{284, &TraceJit::Default}, //"eqvx",   OPTYPE_INTEGER, FL_OUT_A | FL_IN_SB | FL_RC_BIT}},
	{0,   &TraceJit::Default}, //"cmp",    OPTYPE_INTEGER, FL_IN_AB | FL_SET_CRn}},
	{32,  &TraceJit::Default}, //"cmpl",   OPTYPE_INTEGER, FL_IN_AB | FL_SET_CRn}},
	{26,  &TraceJit::Default}, //"cntlzwx",OPTYPE_INTEGER, FL_OUT_A | FL_IN_S | FL_RC_BIT}},
	{922, &TraceJit::Default}, //"extshx", OPTYPE_INTEGER, FL_OUT_A | FL_IN_S | FL_RC_BIT}},
	{954, &TraceJit::Default}, //"extsbx", OPTYPE_INTEGER, FL_OUT_A | FL_IN_S | FL_RC_BIT}},
	{536, &TraceJit::Default}, //"srwx",   OPTYPE_INTEGER, FL_OUT_A | FL_IN_B | FL_IN_S | FL_RC_BIT}},
	{792, &TraceJit::Default}, //"srawx",  OPTYPE_INTEGER, FL_OUT_A | FL_IN_B | FL_IN_S | FL_RC_BIT}},
	{824, &TraceJit::Default}, //"srawix", OPTYPE_INTEGER, FL_OUT_A | FL_IN_B | FL_IN_S | FL_RC_BIT}},
	{24,  &TraceJit::Default}, //"slwx",   OPTYPE_INTEGER, FL_OUT_A | FL_IN_B | FL_IN_S | FL_RC_BIT}},

	{54,   &TraceJit::Default}, //"dcbst",  OPTYPE_DCACHE, 0, 4}},
	{86,   &TraceJit::Default}, //"dcbf",   OPTYPE_DCACHE, 0, 4}},
	{246,  &TraceJit::DoNothing}, //"dcbtst", OPTYPE_DCACHE, 0, 1}},
	{278,  &TraceJit::DoNothing}, //"dcbt",   OPTYPE_DCACHE, 0, 1}},
	{470,  &TraceJit::Default}, //"dcbi",   OPTYPE_DCACHE, 0, 4}},
	{758,  &TraceJit::DoNothing}, //"dcba",   OPTYPE_DCACHE, 0, 4}},
	{1014, &TraceJit::Default}, //"dcbz",   OPTYPE_DCACHE, 0, 4}},

	//load word
	{23,  &TraceJit::Default}, //"lwzx",  OPTYPE_LOAD, FL_OUT_D | FL_IN_A0 | FL_IN_B}},
	{55,  &TraceJit::Default}, //"lwzux", OPTYPE_LOAD, FL_OUT_D | FL_OUT_A | FL_IN_A | FL_IN_B}},

	//load halfword
	{279, &TraceJit::Default}, //"lhzx",  OPTYPE_LOAD, FL_OUT_D | FL_IN_A0 | FL_IN_B}},
	{311, &TraceJit::Default}, //"lhzux", OPTYPE_LOAD, FL_OUT_D | FL_OUT_A | FL_IN_A | FL_IN_B}},

	//load halfword signextend
	{343, &TraceJit::Default}, //"lhax",  OPTYPE_LOAD, FL_OUT_D | FL_IN_A0 | FL_IN_B}},
	{375, &TraceJit::Default}, //"lhaux", OPTYPE_LOAD, FL_OUT_D | FL_OUT_A | FL_IN_A | FL_IN_B}},

	//load byte
	{87,  &TraceJit::Default}, //"lbzx",  OPTYPE_LOAD, FL_OUT_D | FL_IN_A0 | FL_IN_B}},
	{119, &TraceJit::Default}, //"lbzux", OPTYPE_LOAD, FL_OUT_D | FL_OUT_A | FL_IN_A | FL_IN_B}},

	//load byte reverse
	{534, &TraceJit::Default}, //"lwbrx", OPTYPE_LOAD, FL_OUT_D | FL_IN_A0 | FL_IN_B}},
	{790, &TraceJit::Default}, //"lhbrx", OPTYPE_LOAD, FL_OUT_D | FL_IN_A0 | FL_IN_B}},

	// Conditional load/store (Wii SMP)
	{150, &TraceJit::Default}, //"stwcxd", OPTYPE_STORE, FL_EVIL | FL_SET_CR0}},
	{20,  &TraceJit::Default}, //"lwarx",  OPTYPE_LOAD, FL_EVIL | FL_OUT_D | FL_IN_A0B | FL_SET_CR0}},

	//load string (interpret these)
	{533, &TraceJit::Default}, //"lswx",  OPTYPE_LOAD, FL_EVIL | FL_IN_A | FL_OUT_D}},
	{597, &TraceJit::Default}, //"lswi",  OPTYPE_LOAD, FL_EVIL | FL_IN_AB | FL_OUT_D}},

	//store word
	{151, &TraceJit::Default}, //"stwx",   OPTYPE_STORE, FL_IN_A0 | FL_IN_B}},
	{183, &TraceJit::Default}, //"stwux",  OPTYPE_STORE, FL_OUT_A | FL_IN_A | FL_IN_B}},

	//store halfword
	{407, &TraceJit::Default}, //"sthx",   OPTYPE_STORE, FL_IN_A0 | FL_IN_B}},
	{439, &TraceJit::Default}, //"sthux",  OPTYPE_STORE, FL_OUT_A | FL_IN_A | FL_IN_B}},

	//store byte
	{215, &TraceJit::Default}, //"stbx",   OPTYPE_STORE, FL_IN_A0 | FL_IN_B}},
	{247, &TraceJit::Default}, //"stbux",  OPTYPE_STORE, FL_OUT_A | FL_IN_A | FL_IN_B}},

	//store bytereverse
	{662, &TraceJit::Default}, //"stwbrx", OPTYPE_STORE, FL_IN_A0 | FL_IN_B}},
	{918, &TraceJit::Default}, //"sthbrx", OPTYPE_STORE, FL_IN_A | FL_IN_B}},

	{661, &TraceJit::Default}, //"stswx",  OPTYPE_STORE, FL_EVIL}},
	{725, &TraceJit::Default}, //"stswi",  OPTYPE_STORE, FL_EVIL}},

	// fp load/store
	{535, &TraceJit::Default}, //"lfsx",  OPTYPE_LOADFP, FL_IN_A0 | FL_IN_B}},
	{567, &TraceJit::Default}, //"lfsux", OPTYPE_LOADFP, FL_IN_A | FL_IN_B}},
	{599, &TraceJit::Default}, //"lfdx",  OPTYPE_LOADFP, FL_IN_A0 | FL_IN_B}},
	{631, &TraceJit::Default}, //"lfdux", OPTYPE_LOADFP, FL_IN_A | FL_IN_B}},

	{663, &TraceJit::Default}, //"stfsx",  OPTYPE_STOREFP, FL_IN_A0 | FL_IN_B}},
	{695, &TraceJit::Default}, //"stfsux", OPTYPE_STOREFP, FL_IN_A | FL_IN_B}},
	{727, &TraceJit::Default}, //"stfdx",  OPTYPE_STOREFP, FL_IN_A0 | FL_IN_B}},
	{759, &TraceJit::Default}, //"stfdux", OPTYPE_STOREFP, FL_IN_A | FL_IN_B}},
	{983, &TraceJit::Default}, //"stfiwx", OPTYPE_STOREFP, FL_IN_A0 | FL_IN_B}},

	{19,  &TraceJit::Default}, //"mfcr",   OPTYPE_SYSTEM, FL_OUT_D}},
	{83,  &TraceJit::Default}, //"mfmsr",  OPTYPE_SYSTEM, FL_OUT_D}},
	{144, &TraceJit::Default}, //"mtcrf",  OPTYPE_SYSTEM, 0}},
	{146, &TraceJit::mtmsr}, //"mtmsr",  OPTYPE_SYSTEM, FL_ENDBLOCK}},
	{210, &TraceJit::Default}, //"mtsr",   OPTYPE_SYSTEM, 0}},
	{242, &TraceJit::Default}, //"mtsrin", OPTYPE_SYSTEM, 0}},
	{339, &TraceJit::Default}, //"mfspr",  OPTYPE_SPR, FL_OUT_D}},
	{467, &TraceJit::Default}, //"mtspr",  OPTYPE_SPR, 0, 2}},
	{371, &TraceJit::Default}, //"mftb",   OPTYPE_SYSTEM, FL_OUT_D | FL_TIMER}},
	{512, &TraceJit::Default}, //"mcrxr",  OPTYPE_SYSTEM, 0}},
	{595, &TraceJit::Default}, //"mfsr",   OPTYPE_SYSTEM, FL_OUT_D, 2}},
	{659, &TraceJit::Default}, //"mfsrin", OPTYPE_SYSTEM, FL_OUT_D, 2}},

	{4,   &TraceJit::twx}, //"tw",     OPTYPE_SYSTEM, FL_ENDBLOCK, 1}},
	{598, &TraceJit::DoNothing}, //"sync",   OPTYPE_SYSTEM, 0, 2}},
	{982, &TraceJit::icbi}, //"icbi",   OPTYPE_SYSTEM, FL_ENDBLOCK, 3}},

	// Unused instructions on GC
	{310, &TraceJit::Default}, //"eciwx",   OPTYPE_INTEGER, FL_RC_BIT}},
	{438, &TraceJit::Default}, //"ecowx",   OPTYPE_INTEGER, FL_RC_BIT}},
	{854, &TraceJit::Default}, //"eieio",   OPTYPE_INTEGER, FL_RC_BIT}},
	{306, &TraceJit::Default}, //"tlbie",   OPTYPE_SYSTEM, 0}},
	{370, &TraceJit::Default}, //"tlbia",   OPTYPE_SYSTEM, 0}},
	{566, &TraceJit::Default}, //"tlbsync", OPTYPE_SYSTEM, 0}},
};

static GekkoOPTemplate table31_2[] =
{
	{266,  &TraceJit::Default}, //"addx",    OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_RC_BIT}},
	{778,  &TraceJit::Default}, //"addx",    OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_RC_BIT}},
	{10,   &TraceJit::Default}, //"addcx",   OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_SET_CA | FL_RC_BIT}},
	{522,  &TraceJit::Default}, //"addcox",   OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_SET_CA | FL_RC_BIT}},
	{138,  &TraceJit::Default}, //"addex",   OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_READ_CA | FL_SET_CA | FL_RC_BIT}},
	{650,  &TraceJit::Default}, //"addeox",   OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_READ_CA | FL_SET_CA | FL_RC_BIT}},
	{234,  &TraceJit::Default}, //"addmex",  OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_READ_CA | FL_SET_CA | FL_RC_BIT}},
	{202,  &TraceJit::Default}, //"addzex",  OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_READ_CA | FL_SET_CA | FL_RC_BIT}},
	{491,  &TraceJit::Default}, //"divwx",   OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_RC_BIT, 39}},
	{1003, &TraceJit::Default}, //"divwox",   OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_RC_BIT, 39}},
	{459,  &TraceJit::Default}, //"divwux",  OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_RC_BIT, 39}},
	{971,  &TraceJit::Default}, //"divwuox",  OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_RC_BIT, 39}},
	{75,   &TraceJit::Default}, //"mulhwx",  OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_RC_BIT, 4}},
	{11,   &TraceJit::Default}, //"mulhwux", OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_RC_BIT, 4}},
	{235,  &TraceJit::Default}, //"mullwx",  OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_RC_BIT, 4}},
	{747,  &TraceJit::Default}, //"mullwox",  OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_RC_BIT, 4}},
	{104,  &TraceJit::Default}, //"negx",    OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_RC_BIT}},
	{40,   &TraceJit::Default}, //"subfx",   OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_RC_BIT}},
	{552,  &TraceJit::Default}, //"subox",   OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_RC_BIT}},
	{8,    &TraceJit::Default}, //"subfcx",  OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_SET_CA | FL_RC_BIT}},
	{520,  &TraceJit::Default}, //"subfcox",  OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_SET_CA | FL_RC_BIT}},
	{136,  &TraceJit::Default}, //"subfex",  OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_READ_CA | FL_SET_CA | FL_RC_BIT}},
	{232,  &TraceJit::Default}, //"subfmex", OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_READ_CA | FL_SET_CA | FL_RC_BIT}},
	{200,  &TraceJit::Default}, //"subfzex", OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_READ_CA | FL_SET_CA | FL_RC_BIT}},
};

static GekkoOPTemplate table59[] =
{
	{18, &TraceJit::Default}, //{"fdivsx",   OPTYPE_FPU, FL_RC_BIT_F, 16}},
	{20, &TraceJit::Default}, //"fsubsx",   OPTYPE_FPU, FL_RC_BIT_F}},
	{21, &TraceJit::Default}, //"faddsx",   OPTYPE_FPU, FL_RC_BIT_F}},
//	{22, &TraceJit::Default}, //"fsqrtsx",  OPTYPE_FPU, FL_RC_BIT_F}}, // Not implemented on gekko
	{24, &TraceJit::Default}, //"fresx",    OPTYPE_FPU, FL_RC_BIT_F}},
	{25, &TraceJit::Default}, //"fmulsx",   OPTYPE_FPU, FL_RC_BIT_F}},
	{28, &TraceJit::Default}, //"fmsubsx",  OPTYPE_FPU, FL_RC_BIT_F}},
	{29, &TraceJit::Default}, //"fmaddsx",  OPTYPE_FPU, FL_RC_BIT_F}},
	{30, &TraceJit::Default}, //"fnmsubsx", OPTYPE_FPU, FL_RC_BIT_F}},
	{31, &TraceJit::Default}, //"fnmaddsx", OPTYPE_FPU, FL_RC_BIT_F}},
};

static GekkoOPTemplate table63[] =
{
	{264, &TraceJit::Default},   //"fabsx",   OPTYPE_FPU, FL_RC_BIT_F}},
	{32,  &TraceJit::Default},   //"fcmpo",   OPTYPE_FPU, FL_RC_BIT_F}},
	{0,   &TraceJit::Default},   //"fcmpu",   OPTYPE_FPU, FL_RC_BIT_F}},
	{14,  &TraceJit::Default}, //"fctiwx",  OPTYPE_FPU, FL_RC_BIT_F}},
	{15,  &TraceJit::Default}, //"fctiwzx", OPTYPE_FPU, FL_RC_BIT_F}},
	{72,  &TraceJit::Default},    //"fmrx",    OPTYPE_FPU, FL_RC_BIT_F}},
	{136, &TraceJit::Default},   //"fnabsx",  OPTYPE_FPU, FL_RC_BIT_F}},
	{40,  &TraceJit::Default},   //"fnegx",   OPTYPE_FPU, FL_RC_BIT_F}},
	{12,  &TraceJit::Default}, //"frspx",   OPTYPE_FPU, FL_RC_BIT_F}},

	{64,  &TraceJit::Default}, //"mcrfs",   OPTYPE_SYSTEMFP, 0}},
	{583, &TraceJit::Default}, //"mffsx",   OPTYPE_SYSTEMFP, 0}},
	{70,  &TraceJit::Default}, //"mtfsb0x", OPTYPE_SYSTEMFP, 0, 2}},
	{38,  &TraceJit::Default}, //"mtfsb1x", OPTYPE_SYSTEMFP, 0, 2}},
	{134, &TraceJit::Default}, //"mtfsfix", OPTYPE_SYSTEMFP, 0, 2}},
	{711, &TraceJit::Default}, //"mtfsfx",  OPTYPE_SYSTEMFP, 0, 2}},
};

static GekkoOPTemplate table63_2[] =
{
	{18, &TraceJit::Default}, //"fdivx",    OPTYPE_FPU, FL_RC_BIT_F, 30}},
	{20, &TraceJit::Default}, //"fsubx",    OPTYPE_FPU, FL_RC_BIT_F}},
	{21, &TraceJit::Default}, //"faddx",    OPTYPE_FPU, FL_RC_BIT_F}},
	{22, &TraceJit::Default}, //"fsqrtx",   OPTYPE_FPU, FL_RC_BIT_F}},
	{23, &TraceJit::Default}, //"fselx",    OPTYPE_FPU, FL_RC_BIT_F}},
	{25, &TraceJit::Default}, //"fmulx",    OPTYPE_FPU, FL_RC_BIT_F}},
	{26, &TraceJit::Default}, //"frsqrtex", OPTYPE_FPU, FL_RC_BIT_F}},
	{28, &TraceJit::Default}, //"fmsubx",   OPTYPE_FPU, FL_RC_BIT_F}},
	{29, &TraceJit::Default}, //"fmaddx",   OPTYPE_FPU, FL_RC_BIT_F}},
	{30, &TraceJit::Default}, //"fnmsubx",  OPTYPE_FPU, FL_RC_BIT_F}},
	{31, &TraceJit::Default}, //"fnmaddx",  OPTYPE_FPU, FL_RC_BIT_F}},
};

namespace TraceJitTables
{

void CompileInstruction(PPCAnalyst::CodeOp & op)
{
	TraceJit *traceJit = (TraceJit *)jit;
	(traceJit->*dynaOpTable[op.inst.OPCD])(op.inst);
	GekkoOPInfo *info = op.opinfo;
	if (info) {
#ifdef OPLOG
		if (!strcmp(info->opname, OP_TO_LOG)){  ///"mcrfs"
			rsplocations.push_back(jit.js.compilerPC);
		}
#endif
		info->compileCount++;
		info->lastUse = jit->js.compilerPC;
	}
}

void InitTables()
{
	// once initialized, tables are read-only
	static bool initialized = false;
	if (initialized)
		return;

	//clear
	for (auto& tpl : dynaOpTable59)
	{
		tpl = &TraceJit::unknown_instruction;
	}

	for (int i = 0; i < 1024; i++)
	{
		dynaOpTable4 [i] = &TraceJit::unknown_instruction;
		dynaOpTable19[i] = &TraceJit::unknown_instruction;
		dynaOpTable31[i] = &TraceJit::unknown_instruction;
		dynaOpTable63[i] = &TraceJit::unknown_instruction;
	}

	for (auto& tpl : primarytable)
	{
		dynaOpTable[tpl.opcode] = tpl.Inst;
	}

	for (int i = 0; i < 32; i++)
	{
		int fill = i << 5;
		for (auto& tpl : table4_2)
		{
			int op = fill+tpl.opcode;
			dynaOpTable4[op] = tpl.Inst;
		}
	}

	for (int i = 0; i < 16; i++)
	{
		int fill = i << 6;
		for (auto& tpl : table4_3)
		{
			int op = fill+tpl.opcode;
			dynaOpTable4[op] = tpl.Inst;
		}
	}

	for (auto& tpl : table4)
	{
		int op = tpl.opcode;
		dynaOpTable4[op] = tpl.Inst;
	}

	for (auto& tpl : table31)
	{
		int op = tpl.opcode;
		dynaOpTable31[op] = tpl.Inst;
	}

	for (int i = 0; i < 1; i++)
	{
		int fill = i << 9;
		for (auto& tpl : table31_2)
		{
			int op = fill + tpl.opcode;
			dynaOpTable31[op] = tpl.Inst;
		}
	}

	for (auto& tpl : table19)
	{
		int op = tpl.opcode;
		dynaOpTable19[op] = tpl.Inst;
	}

	for (auto& tpl : table59)
	{
		int op = tpl.opcode;
		dynaOpTable59[op] = tpl.Inst;
	}

	for (auto& tpl : table63)
	{
		int op = tpl.opcode;
		dynaOpTable63[op] = tpl.Inst;
	}

	for (int i = 0; i < 32; i++)
	{
		int fill = i << 5;
		for (auto& tpl : table63_2)
		{
			int op = fill + tpl.opcode;
			dynaOpTable63[op] = tpl.Inst;
		}
	}

	initialized = true;
}

}  // namespace
