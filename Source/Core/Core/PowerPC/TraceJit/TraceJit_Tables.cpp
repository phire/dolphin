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

	{7,  &TraceJit::mulli}, //"mulli",    OPTYPE_INTEGER, FL_OUT_D | FL_IN_A | FL_RC_BIT, 2}},
	{8,  &TraceJit::subfic}, //"subfic",   OPTYPE_INTEGER, FL_OUT_D | FL_IN_A |	FL_SET_CA}},
	{10, &TraceJit::cmpXX}, //"cmpli",    OPTYPE_INTEGER, FL_IN_A | FL_SET_CRn}},
	{11, &TraceJit::cmpXX}, //"cmpi",     OPTYPE_INTEGER, FL_IN_A | FL_SET_CRn}},
	{12, &TraceJit::reg_imm}, //"addic",    OPTYPE_INTEGER, FL_OUT_D | FL_IN_A | FL_SET_CA}},
	{13, &TraceJit::reg_imm}, //"addic_rc", OPTYPE_INTEGER, FL_OUT_D | FL_IN_A | FL_SET_CR0}},
	{14, &TraceJit::reg_imm}, //"addi",     OPTYPE_INTEGER, FL_OUT_D | FL_IN_A0}},
	{15, &TraceJit::reg_imm}, //"addis",    OPTYPE_INTEGER, FL_OUT_D | FL_IN_A0}},

	{20, &TraceJit::rlwimix}, //"rlwimix",  OPTYPE_INTEGER, FL_OUT_A | FL_IN_A | FL_IN_S | FL_RC_BIT}},
	{21, &TraceJit::rlwinmx}, //"rlwinmx",  OPTYPE_INTEGER, FL_OUT_A | FL_IN_S | FL_RC_BIT}},
	{23, &TraceJit::rlwnmx}, //"rlwnmx",   OPTYPE_INTEGER, FL_OUT_A | FL_IN_S | FL_IN_B | FL_RC_BIT}},

	{24, &TraceJit::reg_imm}, //"ori",      OPTYPE_INTEGER, FL_OUT_A | FL_IN_S}},
	{25, &TraceJit::reg_imm}, //"oris",     OPTYPE_INTEGER, FL_OUT_A | FL_IN_S}},
	{26, &TraceJit::reg_imm}, //"xori",     OPTYPE_INTEGER, FL_OUT_A | FL_IN_S}},
	{27, &TraceJit::reg_imm}, //"xoris",    OPTYPE_INTEGER, FL_OUT_A | FL_IN_S}},
	{28, &TraceJit::reg_imm}, //"andi_rc",  OPTYPE_INTEGER, FL_OUT_A | FL_IN_S | FL_SET_CR0}},
	{29, &TraceJit::reg_imm}, //"andis_rc", OPTYPE_INTEGER, FL_OUT_A | FL_IN_S | FL_SET_CR0}},

	{32, &TraceJit::lXXx}, //"lwz",  OPTYPE_LOAD, FL_OUT_D | FL_IN_A}},
	{33, &TraceJit::lXXx}, //"lwzu", OPTYPE_LOAD, FL_OUT_D | FL_OUT_A | FL_IN_A}},
	{34, &TraceJit::lXXx}, //"lbz",  OPTYPE_LOAD, FL_OUT_D | FL_IN_A}},
	{35, &TraceJit::lXXx}, //"lbzu", OPTYPE_LOAD, FL_OUT_D | FL_OUT_A | FL_IN_A}},
	{40, &TraceJit::lXXx}, //"lhz",  OPTYPE_LOAD, FL_OUT_D | FL_IN_A}},
	{41, &TraceJit::lXXx}, //"lhzu", OPTYPE_LOAD, FL_OUT_D | FL_OUT_A | FL_IN_A}},
	{42, &TraceJit::lXXx}, //"lha",  OPTYPE_LOAD, FL_OUT_D | FL_IN_A}},
	{43, &TraceJit::lXXx}, //"lhau", OPTYPE_LOAD, FL_OUT_D | FL_OUT_A | FL_IN_A}},

	{44, &TraceJit::stX}, //"sth",  OPTYPE_STORE, FL_IN_A | FL_IN_S}},
	{45, &TraceJit::stX}, //"sthu", OPTYPE_STORE, FL_OUT_A | FL_IN_A | FL_IN_S}},
	{36, &TraceJit::stX}, //"stw",  OPTYPE_STORE, FL_IN_A | FL_IN_S}},
	{37, &TraceJit::stX}, //"stwu", OPTYPE_STORE, FL_OUT_A | FL_IN_A | FL_IN_S}},
	{38, &TraceJit::stX}, //"stb",  OPTYPE_STORE, FL_IN_A | FL_IN_S}},
	{39, &TraceJit::stX}, //"stbu", OPTYPE_STORE, FL_OUT_A | FL_IN_A | FL_IN_S}},

	{46, &TraceJit::lmw}, //"lmw",   OPTYPE_SYSTEM, FL_EVIL, 10}},
	{47, &TraceJit::stmw}, //"stmw",  OPTYPE_SYSTEM, FL_EVIL, 10}},

	{48, &TraceJit::lfs}, //"lfs",  OPTYPE_LOADFP, FL_IN_A}},
	{49, &TraceJit::Default}, //"lfsu", OPTYPE_LOADFP, FL_OUT_A | FL_IN_A}},
	{50, &TraceJit::lfd}, //"lfd",  OPTYPE_LOADFP, FL_IN_A}},
	{51, &TraceJit::Default}, //"lfdu", OPTYPE_LOADFP, FL_OUT_A | FL_IN_A}},

	{52, &TraceJit::stfs}, //"stfs",  OPTYPE_STOREFP, FL_IN_A}},
	{53, &TraceJit::stfs}, //"stfsu", OPTYPE_STOREFP, FL_OUT_A | FL_IN_A}},
	{54, &TraceJit::stfd}, //"stfd",  OPTYPE_STOREFP, FL_IN_A}},
	{55, &TraceJit::Default}, //"stfdu", OPTYPE_STOREFP, FL_OUT_A | FL_IN_A}},

	{56, &TraceJit::psq_l}, //"psq_l",   OPTYPE_PS, FL_IN_A}},
	{57, &TraceJit::psq_l}, //"psq_lu",  OPTYPE_PS, FL_OUT_A | FL_IN_A}},
	{60, &TraceJit::psq_st}, //"psq_st",  OPTYPE_PS, FL_IN_A}},
	{61, &TraceJit::psq_st}, //"psq_stu", OPTYPE_PS, FL_OUT_A | FL_IN_A}},

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
	{40,   &TraceJit::ps_sign}, //"ps_neg",     OPTYPE_PS, FL_RC_BIT}},
	{136,  &TraceJit::ps_sign}, //"ps_nabs",    OPTYPE_PS, FL_RC_BIT}},
	{264,  &TraceJit::ps_sign}, //"ps_abs",     OPTYPE_PS, FL_RC_BIT}},
	{64,   &TraceJit::Default}, //"ps_cmpu1",   OPTYPE_PS, FL_RC_BIT}},
	{72,   &TraceJit::ps_mr}, //"ps_mr",      OPTYPE_PS, FL_RC_BIT}},
	{96,   &TraceJit::Default}, //"ps_cmpo1",   OPTYPE_PS, FL_RC_BIT}},
	{528,  &TraceJit::ps_mergeXX}, //"ps_merge00", OPTYPE_PS, FL_RC_BIT}},
	{560,  &TraceJit::ps_mergeXX}, //"ps_merge01", OPTYPE_PS, FL_RC_BIT}},
	{592,  &TraceJit::ps_mergeXX}, //"ps_merge10", OPTYPE_PS, FL_RC_BIT}},
	{624,  &TraceJit::ps_mergeXX}, //"ps_merge11", OPTYPE_PS, FL_RC_BIT}},

	{1014, &TraceJit::Default}, //"dcbz_l",     OPTYPE_SYSTEM, 0}},
};

static GekkoOPTemplate table4_2[] =
{
	{10, &TraceJit::ps_sum}, //"ps_sum0",   OPTYPE_PS, 0}},
	{11, &TraceJit::ps_sum}, //"ps_sum1",   OPTYPE_PS, 0}},
	{12, &TraceJit::ps_muls}, //"ps_muls0",  OPTYPE_PS, 0}},
	{13, &TraceJit::ps_muls}, //"ps_muls1",  OPTYPE_PS, 0}},
	{14, &TraceJit::ps_maddXX}, //"ps_madds0", OPTYPE_PS, 0}},
	{15, &TraceJit::ps_maddXX}, //"ps_madds1", OPTYPE_PS, 0}},
	{18, &TraceJit::ps_arith}, //"ps_div",    OPTYPE_PS, 0, 16}},
	{20, &TraceJit::ps_arith}, //"ps_sub",    OPTYPE_PS, 0}},
	{21, &TraceJit::ps_arith}, //"ps_add",    OPTYPE_PS, 0}},
	{23, &TraceJit::ps_sel}, //"ps_sel",    OPTYPE_PS, 0}},
	{24, &TraceJit::ps_recip}, //"ps_res",    OPTYPE_PS, 0}},
	{25, &TraceJit::ps_arith}, //"ps_mul",    OPTYPE_PS, 0}},
	{26, &TraceJit::ps_recip}, //"ps_rsqrte", OPTYPE_PS, 0, 1}},
	{28, &TraceJit::ps_maddXX}, //"ps_msub",   OPTYPE_PS, 0}},
	{29, &TraceJit::ps_maddXX}, //"ps_madd",   OPTYPE_PS, 0}},
	{30, &TraceJit::ps_maddXX}, //"ps_nmsub",  OPTYPE_PS, 0}},
	{31, &TraceJit::ps_maddXX}, //"ps_nmadd",  OPTYPE_PS, 0}},
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
	{257, &TraceJit::crXXX}, //"crand",  OPTYPE_CR, FL_EVIL}},
	{129, &TraceJit::crXXX}, //"crandc", OPTYPE_CR, FL_EVIL}},
	{289, &TraceJit::crXXX}, //"creqv",  OPTYPE_CR, FL_EVIL}},
	{225, &TraceJit::crXXX}, //"crnand", OPTYPE_CR, FL_EVIL}},
	{33,  &TraceJit::crXXX}, //"crnor",  OPTYPE_CR, FL_EVIL}},
	{449, &TraceJit::crXXX}, //"cror",   OPTYPE_CR, FL_EVIL}},
	{417, &TraceJit::crXXX}, //"crorc",  OPTYPE_CR, FL_EVIL}},
	{193, &TraceJit::crXXX}, //"crxor",  OPTYPE_CR, FL_EVIL}},

	{150, &TraceJit::DoNothing}, //"isync",  OPTYPE_ICACHE, FL_EVIL}},
	{0,   &TraceJit::mcrf}, //"mcrf",   OPTYPE_SYSTEM, FL_EVIL}},

	{50,  &TraceJit::rfi}, //"rfi",    OPTYPE_SYSTEM, FL_ENDBLOCK | FL_CHECKEXCEPTIONS, 1}},
	{18,  &TraceJit::Default}, //"rfid",   OPTYPE_SYSTEM, FL_ENDBLOCK | FL_CHECKEXCEPTIONS}}
};


static GekkoOPTemplate table31[] =
{
	{28,  &TraceJit::boolX}, //"andx",   OPTYPE_INTEGER, FL_OUT_A | FL_IN_SB | FL_RC_BIT}},
	{60,  &TraceJit::boolX}, //"andcx",  OPTYPE_INTEGER, FL_OUT_A | FL_IN_SB | FL_RC_BIT}},
	{444, &TraceJit::boolX}, //"orx",    OPTYPE_INTEGER, FL_OUT_A | FL_IN_SB | FL_RC_BIT}},
	{124, &TraceJit::boolX}, //"norx",   OPTYPE_INTEGER, FL_OUT_A | FL_IN_SB | FL_RC_BIT}},
	{316, &TraceJit::boolX}, //"xorx",   OPTYPE_INTEGER, FL_OUT_A | FL_IN_SB | FL_RC_BIT}},
	{412, &TraceJit::boolX}, //"orcx",   OPTYPE_INTEGER, FL_OUT_A | FL_IN_SB | FL_RC_BIT}},
	{476, &TraceJit::boolX}, //"nandx",  OPTYPE_INTEGER, FL_OUT_A | FL_IN_SB | FL_RC_BIT}},
	{284, &TraceJit::boolX}, //"eqvx",   OPTYPE_INTEGER, FL_OUT_A | FL_IN_SB | FL_RC_BIT}},
	{0,   &TraceJit::cmpXX}, //"cmp",    OPTYPE_INTEGER, FL_IN_AB | FL_SET_CRn}},
	{32,  &TraceJit::cmpXX}, //"cmpl",   OPTYPE_INTEGER, FL_IN_AB | FL_SET_CRn}},
	{26,  &TraceJit::cntlzwx}, //"cntlzwx",OPTYPE_INTEGER, FL_OUT_A | FL_IN_S | FL_RC_BIT}},
	{922, &TraceJit::extshx}, //"extshx", OPTYPE_INTEGER, FL_OUT_A | FL_IN_S | FL_RC_BIT}},
	{954, &TraceJit::extsbx}, //"extsbx", OPTYPE_INTEGER, FL_OUT_A | FL_IN_S | FL_RC_BIT}},
	{536, &TraceJit::srwx}, //"srwx",   OPTYPE_INTEGER, FL_OUT_A | FL_IN_B | FL_IN_S | FL_RC_BIT}},
	{792, &TraceJit::srawx}, //"srawx",  OPTYPE_INTEGER, FL_OUT_A | FL_IN_B | FL_IN_S | FL_RC_BIT}},
	{824, &TraceJit::srawix}, //"srawix", OPTYPE_INTEGER, FL_OUT_A | FL_IN_B | FL_IN_S | FL_RC_BIT}},
	{24,  &TraceJit::slwx}, //"slwx",   OPTYPE_INTEGER, FL_OUT_A | FL_IN_B | FL_IN_S | FL_RC_BIT}},

	{54,   &TraceJit::dcbst}, //"dcbst",  OPTYPE_DCACHE, 0, 4}},
	{86,   &TraceJit::Default}, //"dcbf",   OPTYPE_DCACHE, 0, 4}},
	{246,  &TraceJit::DoNothing}, //"dcbtst", OPTYPE_DCACHE, 0, 1}},
	{278,  &TraceJit::DoNothing}, //"dcbt",   OPTYPE_DCACHE, 0, 1}},
	{470,  &TraceJit::Default}, //"dcbi",   OPTYPE_DCACHE, 0, 4}},
	{758,  &TraceJit::DoNothing}, //"dcba",   OPTYPE_DCACHE, 0, 4}},
	{1014, &TraceJit::dcbz}, //"dcbz",   OPTYPE_DCACHE, 0, 4}},

	//load word
	{23,  &TraceJit::lXXx}, //"lwzx",  OPTYPE_LOAD, FL_OUT_D | FL_IN_A0 | FL_IN_B}},
	{55,  &TraceJit::lXXx}, //"lwzux", OPTYPE_LOAD, FL_OUT_D | FL_OUT_A | FL_IN_A | FL_IN_B}},

	//load halfword
	{279, &TraceJit::lXXx}, //"lhzx",  OPTYPE_LOAD, FL_OUT_D | FL_IN_A0 | FL_IN_B}},
	{311, &TraceJit::lXXx}, //"lhzux", OPTYPE_LOAD, FL_OUT_D | FL_OUT_A | FL_IN_A | FL_IN_B}},

	//load halfword signextend
	{343, &TraceJit::lXXx}, //"lhax",  OPTYPE_LOAD, FL_OUT_D | FL_IN_A0 | FL_IN_B}},
	{375, &TraceJit::lXXx}, //"lhaux", OPTYPE_LOAD, FL_OUT_D | FL_OUT_A | FL_IN_A | FL_IN_B}},

	//load byte
	{87,  &TraceJit::lXXx}, //"lbzx",  OPTYPE_LOAD, FL_OUT_D | FL_IN_A0 | FL_IN_B}},
	{119, &TraceJit::lXXx}, //"lbzux", OPTYPE_LOAD, FL_OUT_D | FL_OUT_A | FL_IN_A | FL_IN_B}},

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
	{151, &TraceJit::stXx}, //"stwx",   OPTYPE_STORE, FL_IN_A0 | FL_IN_B}},
	{183, &TraceJit::stXx}, //"stwux",  OPTYPE_STORE, FL_OUT_A | FL_IN_A | FL_IN_B}},

	//store halfword
	{407, &TraceJit::stXx}, //"sthx",   OPTYPE_STORE, FL_IN_A0 | FL_IN_B}},
	{439, &TraceJit::stXx}, //"sthux",  OPTYPE_STORE, FL_OUT_A | FL_IN_A | FL_IN_B}},

	//store byte
	{215, &TraceJit::stXx}, //"stbx",   OPTYPE_STORE, FL_IN_A0 | FL_IN_B}},
	{247, &TraceJit::stXx}, //"stbux",  OPTYPE_STORE, FL_OUT_A | FL_IN_A | FL_IN_B}},

	//store bytereverse
	{662, &TraceJit::Default}, //"stwbrx", OPTYPE_STORE, FL_IN_A0 | FL_IN_B}},
	{918, &TraceJit::Default}, //"sthbrx", OPTYPE_STORE, FL_IN_A | FL_IN_B}},

	{661, &TraceJit::Default}, //"stswx",  OPTYPE_STORE, FL_EVIL}},
	{725, &TraceJit::Default}, //"stswi",  OPTYPE_STORE, FL_EVIL}},

	// fp load/store
	{535, &TraceJit::lfsx}, //"lfsx",  OPTYPE_LOADFP, FL_IN_A0 | FL_IN_B}},
	{567, &TraceJit::Default}, //"lfsux", OPTYPE_LOADFP, FL_IN_A | FL_IN_B}},
	{599, &TraceJit::Default}, //"lfdx",  OPTYPE_LOADFP, FL_IN_A0 | FL_IN_B}},
	{631, &TraceJit::Default}, //"lfdux", OPTYPE_LOADFP, FL_IN_A | FL_IN_B}},

	{663, &TraceJit::stfsx}, //"stfsx",  OPTYPE_STOREFP, FL_IN_A0 | FL_IN_B}},
	{695, &TraceJit::Default}, //"stfsux", OPTYPE_STOREFP, FL_IN_A | FL_IN_B}},
	{727, &TraceJit::Default}, //"stfdx",  OPTYPE_STOREFP, FL_IN_A0 | FL_IN_B}},
	{759, &TraceJit::Default}, //"stfdux", OPTYPE_STOREFP, FL_IN_A | FL_IN_B}},
	{983, &TraceJit::Default}, //"stfiwx", OPTYPE_STOREFP, FL_IN_A0 | FL_IN_B}},

	{19,  &TraceJit::mfcr}, //"mfcr",   OPTYPE_SYSTEM, FL_OUT_D}},
	{83,  &TraceJit::mfmsr}, //"mfmsr",  OPTYPE_SYSTEM, FL_OUT_D}},
	{144, &TraceJit::mtcrf}, //"mtcrf",  OPTYPE_SYSTEM, 0}},
	{146, &TraceJit::mtmsr}, //"mtmsr",  OPTYPE_SYSTEM, FL_ENDBLOCK}},
	{210, &TraceJit::Default}, //"mtsr",   OPTYPE_SYSTEM, 0}},
	{242, &TraceJit::Default}, //"mtsrin", OPTYPE_SYSTEM, 0}},
	{339, &TraceJit::mfspr}, //"mfspr",  OPTYPE_SPR, FL_OUT_D}},
	{467, &TraceJit::mtspr}, //"mtspr",  OPTYPE_SPR, 0, 2}},
	{371, &TraceJit::mftb}, //"mftb",   OPTYPE_SYSTEM, FL_OUT_D | FL_TIMER}},
	{512, &TraceJit::mcrxr}, //"mcrxr",  OPTYPE_SYSTEM, 0}},
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
	{266,  &TraceJit::addx}, //"addx",    OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_RC_BIT}},
	{778,  &TraceJit::addx}, //"addx",    OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_RC_BIT}},
	{10,   &TraceJit::addcx}, //"addcx",   OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_SET_CA | FL_RC_BIT}},
	{522,  &TraceJit::addcx}, //"addcox",   OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_SET_CA | FL_RC_BIT}},
	{138,  &TraceJit::addex}, //"addex",   OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_READ_CA | FL_SET_CA | FL_RC_BIT}},
	{650,  &TraceJit::addex}, //"addeox",   OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_READ_CA | FL_SET_CA | FL_RC_BIT}},
	{234,  &TraceJit::addmex}, //"addmex",  OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_READ_CA | FL_SET_CA | FL_RC_BIT}},
	{202,  &TraceJit::addzex}, //"addzex",  OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_READ_CA | FL_SET_CA | FL_RC_BIT}},
	{491,  &TraceJit::divwx}, //"divwx",   OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_RC_BIT, 39}},
	{1003, &TraceJit::divwx}, //"divwox",   OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_RC_BIT, 39}},
	{459,  &TraceJit::divwux}, //"divwux",  OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_RC_BIT, 39}},
	{971,  &TraceJit::divwux}, //"divwuox",  OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_RC_BIT, 39}},
	{75,   &TraceJit::Default}, //"mulhwx",  OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_RC_BIT, 4}},
	{11,   &TraceJit::mulhwux}, //"mulhwux", OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_RC_BIT, 4}},
	{235,  &TraceJit::mullwx}, //"mullwx",  OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_RC_BIT, 4}},
	{747,  &TraceJit::mullwx}, //"mullwox",  OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_RC_BIT, 4}},
	{104,  &TraceJit::negx}, //"negx",    OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_RC_BIT}},
	{40,   &TraceJit::subfx}, //"subfx",   OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_RC_BIT}},
	{552,  &TraceJit::subfx}, //"subox",   OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_RC_BIT}},
	{8,    &TraceJit::subfcx}, //"subfcx",  OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_SET_CA | FL_RC_BIT}},
	{520,  &TraceJit::subfcx}, //"subfcox",  OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_SET_CA | FL_RC_BIT}},
	{136,  &TraceJit::subfex}, //"subfex",  OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_READ_CA | FL_SET_CA | FL_RC_BIT}},
	{232,  &TraceJit::subfmex}, //"subfmex", OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_READ_CA | FL_SET_CA | FL_RC_BIT}},
	{200,  &TraceJit::subfzex}, //"subfzex", OPTYPE_INTEGER, FL_OUT_D | FL_IN_AB | FL_READ_CA | FL_SET_CA | FL_RC_BIT}},
};

static GekkoOPTemplate table59[] =
{
	{18, &TraceJit::fp_arith}, //{"fdivsx",   OPTYPE_FPU, FL_RC_BIT_F, 16}},
	{20, &TraceJit::fp_arith}, //"fsubsx",   OPTYPE_FPU, FL_RC_BIT_F}},
	{21, &TraceJit::fp_arith}, //"faddsx",   OPTYPE_FPU, FL_RC_BIT_F}},
//	{22, &TraceJit::Default}, //"fsqrtsx",  OPTYPE_FPU, FL_RC_BIT_F}}, // Not implemented on gekko
	{24, &TraceJit::Default}, //"fresx",    OPTYPE_FPU, FL_RC_BIT_F}},
	{25, &TraceJit::fp_arith}, //"fmulsx",   OPTYPE_FPU, FL_RC_BIT_F}},
	{28, &TraceJit::fmaddXX}, //"fmsubsx",  OPTYPE_FPU, FL_RC_BIT_F}},
	{29, &TraceJit::fmaddXX}, //"fmaddsx",  OPTYPE_FPU, FL_RC_BIT_F}},
	{30, &TraceJit::fmaddXX}, //"fnmsubsx", OPTYPE_FPU, FL_RC_BIT_F}},
	{31, &TraceJit::fmaddXX}, //"fnmaddsx", OPTYPE_FPU, FL_RC_BIT_F}},
};

static GekkoOPTemplate table63[] =
{
	{264, &TraceJit::fsign},   //"fabsx",   OPTYPE_FPU, FL_RC_BIT_F}},
	{32,  &TraceJit::fcmpx},   //"fcmpo",   OPTYPE_FPU, FL_RC_BIT_F}},
	{0,   &TraceJit::fcmpx},   //"fcmpu",   OPTYPE_FPU, FL_RC_BIT_F}},
	{14,  &TraceJit::Default}, //"fctiwx",  OPTYPE_FPU, FL_RC_BIT_F}},
	{15,  &TraceJit::Default}, //"fctiwzx", OPTYPE_FPU, FL_RC_BIT_F}},
	{72,  &TraceJit::fmrx},    //"fmrx",    OPTYPE_FPU, FL_RC_BIT_F}},
	{136, &TraceJit::fsign},   //"fnabsx",  OPTYPE_FPU, FL_RC_BIT_F}},
	{40,  &TraceJit::fsign},   //"fnegx",   OPTYPE_FPU, FL_RC_BIT_F}},
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
	{18, &TraceJit::fp_arith}, //"fdivx",    OPTYPE_FPU, FL_RC_BIT_F, 30}},
	{20, &TraceJit::fp_arith}, //"fsubx",    OPTYPE_FPU, FL_RC_BIT_F}},
	{21, &TraceJit::fp_arith}, //"faddx",    OPTYPE_FPU, FL_RC_BIT_F}},
	{22, &TraceJit::Default}, //"fsqrtx",   OPTYPE_FPU, FL_RC_BIT_F}},
	{23, &TraceJit::Default}, //"fselx",    OPTYPE_FPU, FL_RC_BIT_F}},
	{25, &TraceJit::fp_arith}, //"fmulx",    OPTYPE_FPU, FL_RC_BIT_F}},
	{26, &TraceJit::frsqrtex}, //"frsqrtex", OPTYPE_FPU, FL_RC_BIT_F}},
	{28, &TraceJit::fmaddXX}, //"fmsubx",   OPTYPE_FPU, FL_RC_BIT_F}},
	{29, &TraceJit::fmaddXX}, //"fmaddx",   OPTYPE_FPU, FL_RC_BIT_F}},
	{30, &TraceJit::fmaddXX}, //"fnmsubx",  OPTYPE_FPU, FL_RC_BIT_F}},
	{31, &TraceJit::fmaddXX}, //"fnmaddx",  OPTYPE_FPU, FL_RC_BIT_F}},
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
