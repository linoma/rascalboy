#include "gbaemu.h"
#include "topcodes.h"
#include "debug.h"
#include "topedec.h"

//---------------------------------------------------------------------------
void setup_handle_tables_t (void)
{
	int i;

	for (i=0; i<0x400; i++)
		opcode_handles_t [i] = tins_unknown;

	for (i=0; i<0x8; i++) {
		opcode_handles_t [0x60|i] =   tins_add;//ok
       opcode_handles_t [0x68|i] =   tins_sub;//ok
  		opcode_handles_t [0x70|i] =   tins_add_short_imm;//ok
       opcode_handles_t [0x78|i] =   tins_sub_reg_imm;//ok
       opcode_handles_t [0x140|i] =  tins_str_reg;//ok
       opcode_handles_t [0x150|i] =  tins_strb_reg;//ok
       opcode_handles_t [0x160|i] =  tins_ldr_reg;//ok
       opcode_handles_t [0x170|i] =  tins_ldrb_reg;//ok
       opcode_handles_t [0x148|i] =  tins_strh_reg;//ok
       opcode_handles_t [0x158|i] =  tins_ldrsb_reg;//ok
       opcode_handles_t [0x168|i] =  tins_ldrh_reg;//ok
       opcode_handles_t [0x178|i] =  tins_ldrsh;//ok
		opcode_handles_t [0x2D0|i] =  tins_push;
		opcode_handles_t [0x2F0|i] =  tins_pop;
	}

	for(i=0;i<0x20; i++) {
		opcode_handles_t[i] =         tins_lsl_imm;//ok
		opcode_handles_t[0x20|i] =    tins_lsr_imm;//ok
		opcode_handles_t[0x40|i] =    tins_asr_imm;//ok
		opcode_handles_t[0x80|i] =    tins_mov_imm;//ok
		opcode_handles_t[0xA0|i] =    tins_cmp_imm;//ok
  		opcode_handles_t[0xC0|i] =    tins_add_imm;//ok
		opcode_handles_t[0xE0|i] =    tins_sub_imm;//ok
		opcode_handles_t[0x120|i] =   tins_pc_rel_ldr;//ok//OK
		opcode_handles_t[0x180|i] =   tins_str_imm;//ok32
		opcode_handles_t[0x1A0|i] =   tins_ldr_imm;//ok32
		opcode_handles_t[0x1C0|i] =   tins_strb_imm;//ok
		opcode_handles_t[0x1E0|i] =   tins_ldrb_imm;//OK
		opcode_handles_t[0x200|i] =   tins_strh_imm;//OK
		opcode_handles_t[0x220|i] =   tins_ldrh_imm;//OK
		opcode_handles_t[0x240|i] =   tins_sp_rel_str;//ok
       opcode_handles_t[0x260|i] =   tins_sp_rel_ldr;//ok
       opcode_handles_t[0x280|i] =   tins_add_pc;//ok
     	opcode_handles_t[0x2A0|i] =   tins_add_sp;//ok
       opcode_handles_t[0x300|i] =   tins_stmia;//ok
       opcode_handles_t[0x320|i] =   tins_ldmia;//ok
	}

	for (i=0; i<0x20; i++){
		opcode_handles_t[0x380|i] =   tins_bu;
       opcode_handles_t[0x3A0|i] =   tins_bl;
       opcode_handles_t[0x3C0|i] =   tins_bl;
       opcode_handles_t[0x3E0|i] =   tins_bl;
   }
   for(i=0;i<0x3c;i++)
       opcode_handles_t[0x340|i] =   tins_b;
   for(i=0;i<4;i++)
       opcode_handles_t[0x37c|i] =   tins_swi;

   opcode_handles_t[0x105] =         tins_adc_reg;
   opcode_handles_t[0x106] =         tins_sbc;
   opcode_handles_t[0x108] =         tins_tst;
   opcode_handles_t[0x107] =         tins_ror_reg;
   opcode_handles_t[0x109] =         tins_neg;
	opcode_handles_t[0x100] =         tins_and;
   opcode_handles_t[0x101] =         tins_eor;
   opcode_handles_t[0x102] =         tins_lsl_reg;
	opcode_handles_t[0x10d] =         tins_mul;
	opcode_handles_t[0x10c] =         tins_orr;
	opcode_handles_t[0x10f] =         tins_mvn;
	opcode_handles_t[0x10e] =         tins_bic;
   opcode_handles_t[0x103] =         tins_lsr_reg;
   opcode_handles_t[0x104] =         tins_asr_reg;
   opcode_handles_t[0x10A] =         tins_cmp;//ok
   opcode_handles_t[0x10b] =         tins_cmn_reg;
	opcode_handles_t[0x110] =         tins_add_lo_hi;//ok
	opcode_handles_t[0x111] =         tins_add_lo_hi;//ok
	opcode_handles_t[0x112] =         tins_add_lo_hi;//ok
   opcode_handles_t[0x113] =         tins_add_lo_hi;//ok
   opcode_handles_t[0x114] =         tins_cmp_reg;//ok
   opcode_handles_t[0x115] =         tins_cmp_reg;//ok
   opcode_handles_t[0x116] =         tins_cmp_reg;//ok
   opcode_handles_t[0x117] =         tins_cmp_reg;//ok
   opcode_handles_t[0x118] =         tins_mov_hi_lo;//ok
	opcode_handles_t[0x119] =         tins_mov_hi_lo;//ok
	opcode_handles_t[0x11A] =         tins_mov_hi_lo;//ok
	opcode_handles_t[0x11B] =         tins_mov_hi_lo;//ok
   opcode_handles_t[0x11c] =         tins_bx;//ok
   opcode_handles_t[0x11D] =         tins_bx;//ok
   opcode_handles_t[0x11e] =         tins_bx;//ok
   opcode_handles_t[0x11f] =         tins_bx;//ok
   opcode_handles_t[0x2C0] =         tins_add_rel_sp;//ok
   opcode_handles_t[0x2C1] =         tins_add_rel_sp;//ok
   opcode_handles_t[0x2c2] =         tins_sub_sp;//ok
   opcode_handles_t[0x2c3] =         tins_sub_sp;//ok
}
#ifdef _DEBUG
//---------------------------------------------------------------------------
void setup_debug_handles_t()
{
	int i;

	for (i=0; i<0x400; i++)
		debug_handles_t [i] = unknown_debug_handle_t;

	for (i=0; i<0x8; i++) {
		debug_handles_t [0x60|i] = reg_debug_handle_t;
       debug_handles_t [0x68|i] = reg_debug_handle_t;
  		debug_handles_t [0x70|i]  = immshort_debug_handle_t;//ok
       debug_handles_t [0x78|i]  = immshort_debug_handle_t;//ok
       debug_handles_t [0x140|i] = reg_debug_handle_t;//ok
       debug_handles_t [0x150|i] = reg_debug_handle_t;//ok
       debug_handles_t [0x160|i] = reg_debug_handle_t;//ok
       debug_handles_t [0x170|i] = reg_debug_handle_t;//ok
       debug_handles_t [0x148|i] = reg_debug_handle_t;//ok
       debug_handles_t [0x158|i] = reg_debug_handle_t;//ok
       debug_handles_t [0x168|i] = reg_debug_handle_t;//ok
       debug_handles_t [0x178|i] = reg_debug_handle_t;//ok
		debug_handles_t [0x2D0|i] = stack_debug_handle_t;
		debug_handles_t [0x2F0|i] = stack_debug_handle_t;
	}

	for(i=0;i<0x20; i++) {
		debug_handles_t[i]       = immediate_debug_handle_t;//ok
		debug_handles_t[0x20|i]  = immediate_debug_handle_t;//ok
		debug_handles_t[0x40|i]  = immediate_debug_handle_t;//ok
		debug_handles_t[0x80|i]  = immlong_debug_handle_t;//ok
		debug_handles_t[0xA0|i]  = immlong_debug_handle_t;//ok
  		debug_handles_t[0xC0|i]  = immlong_debug_handle_t;//ok
		debug_handles_t[0xE0|i]  = immlong_debug_handle_t;//ok
		debug_handles_t[0x120|i] = rs_debug_handle_t;
		debug_handles_t[0x180|i] = immediate_debug_handle_t;//ok32
		debug_handles_t[0x1A0|i] = immediate_debug_handle_t;//ok32
		debug_handles_t[0x1C0|i] = immediate_debug_handle_t;//ok
		debug_handles_t[0x1E0|i] = immediate_debug_handle_t;//OK
		debug_handles_t[0x200|i] = immediate_debug_handle_t;//OK
		debug_handles_t[0x220|i] = immediate_debug_handle_t;//OK
		debug_handles_t[0x240|i] = rs_debug_handle_t;//ok
       debug_handles_t[0x260|i] = rs_debug_handle_t;//ok
       debug_handles_t[0x280|i] = rs_debug_handle_t;//ok
     	debug_handles_t[0x2A0|i] = rs_debug_handle_t;//ok
       debug_handles_t[0x300|i] = mdt_debug_handle_t;//ok
       debug_handles_t[0x320|i] = mdt_debug_handle_t;//ok
	}
	for (i=0; i<0x20; i++){
		debug_handles_t[0x380|i] = b_debug_handle_t;
       debug_handles_t[0x3C0|i] = bl_debug_handle_t;
       debug_handles_t[0x3E0|i] = bl_debug_handle_t;
   }
   for(i=0;i<0x3c;i++)
       debug_handles_t[0x340|i] = bcond_debug_handle_t;
   for(i=0;i<4;i++)
       debug_handles_t[0x37c|i] = swi_debug_handle_t;

   debug_handles_t[0x105] = standard_debug_handle_t;
   debug_handles_t[0x106] = standard_debug_handle_t;
   debug_handles_t[0x108] = standard_debug_handle_t;
   debug_handles_t[0x107] = standard_debug_handle_t;
   debug_handles_t[0x109] = standard_debug_handle_t;
	debug_handles_t[0x100] = standard_debug_handle_t;
   debug_handles_t[0x101] = standard_debug_handle_t;
   debug_handles_t[0x102] = standard_debug_handle_t;
	debug_handles_t[0x10d] = standard_debug_handle_t;
	debug_handles_t[0x10c] = standard_debug_handle_t;
	debug_handles_t[0x10f] = standard_debug_handle_t;
	debug_handles_t[0x10e] = standard_debug_handle_t;
   debug_handles_t[0x103] = standard_debug_handle_t;
   debug_handles_t[0x104] = standard_debug_handle_t;
   debug_handles_t[0x10A] = standard_debug_handle_t;//ok
   debug_handles_t[0x10b] = standard_debug_handle_t;
	debug_handles_t[0x110] = hireg_debug_handle_t;//ok
	debug_handles_t[0x111] = hireg_debug_handle_t;//ok
	debug_handles_t[0x112] = hireg_debug_handle_t;//ok
   debug_handles_t[0x113] = hireg_debug_handle_t;//ok
   debug_handles_t[0x114] = hireg_debug_handle_t;//ok
   debug_handles_t[0x115] = hireg_debug_handle_t;//ok
   debug_handles_t[0x116] = hireg_debug_handle_t;//ok
   debug_handles_t[0x117] = hireg_debug_handle_t;//ok
   debug_handles_t[0x118] = hireg_debug_handle_t;//ok
	debug_handles_t[0x119] = hireg_debug_handle_t;//ok
	debug_handles_t[0x11A] = hireg_debug_handle_t;//ok
	debug_handles_t[0x11B] = hireg_debug_handle_t;//ok
   debug_handles_t[0x11c] = bx_debug_handle_t;//ok
   debug_handles_t[0x11D] = bx_debug_handle_t;//ok
   debug_handles_t[0x11e] = bx_debug_handle_t;//ok
   debug_handles_t[0x11f] = bx_debug_handle_t;//ok
   debug_handles_t[0x2C0] = rs_debug_handle_t;//ok
   debug_handles_t[0x2C1] = rs_debug_handle_t;//ok
   debug_handles_t[0x2c2] = rs_debug_handle_t;//ok
   debug_handles_t[0x2c3] = rs_debug_handle_t;//ok
}
//---------------------------------------------------------------------------
void setup_string_tables_t()
{
   int i;

	for (i=0; i<0x400; i++)
		opcode_strings_t[i] = "Unknown";

	for (i=0; i<0x8; i++) {
		opcode_strings_t [0x60|i]  = "add";
       opcode_strings_t [0x68|i]  = "sub";
  		opcode_strings_t [0x70|i]  = "add";
       opcode_strings_t [0x78|i]  = "sub";
       opcode_strings_t [0x140|i] = "str";
       opcode_strings_t [0x150|i] = "strb";
       opcode_strings_t [0x160|i] = "ldr";
       opcode_strings_t [0x170|i] = "ldrb";
       opcode_strings_t [0x148|i] = "strh";
       opcode_strings_t [0x158|i] = "ldrsb";
       opcode_strings_t [0x168|i] = "ldrh";
       opcode_strings_t [0x178|i] = "ldrsh";
		opcode_strings_t [0x2D0|i] = "push";
		opcode_strings_t [0x2F0|i] = "pop";
	}

	for(i=0;i<0x20; i++) {
		opcode_strings_t[i] =        "lsl";
		opcode_strings_t[0x20|i]  =  "lsr";
		opcode_strings_t[0x40|i]  =  "asr";
		opcode_strings_t[0x80|i]  =  "mov";
		opcode_strings_t[0xA0|i]  =  "cmp";
  		opcode_strings_t[0xC0|i]  =  "add";
		opcode_strings_t[0xE0|i]  =  "sub";
		opcode_strings_t[0x120|i] =  "ldr";
		opcode_strings_t[0x180|i] =  "str";
		opcode_strings_t[0x1A0|i] =  "ldr";
		opcode_strings_t[0x1C0|i] =  "strb";
		opcode_strings_t[0x1E0|i] =  "ldrb";
		opcode_strings_t[0x200|i] =  "strh";
		opcode_strings_t[0x220|i] =  "ldrh";
		opcode_strings_t[0x240|i] =  "str";
       opcode_strings_t[0x260|i] =  "ldr";
       opcode_strings_t[0x280|i] =  "add";
     	opcode_strings_t[0x2A0|i] =  "add";
       opcode_strings_t[0x300|i] =  "stmia";
       opcode_strings_t[0x320|i] =  "ldmia";
	}

	for (i=0; i<0x20; i++){
		opcode_strings_t[0x380|i] =  "b";
       opcode_strings_t[0x3C0|i] =  "bl";
       opcode_strings_t[0x3E0|i] =  "bl";
   }
   for(i=0;i<0x3c;i++)
       opcode_strings_t[0x340|i] =  "b";
   for(i=0;i<4;i++)
       opcode_strings_t[0x37c|i] =  "swi";

   opcode_strings_t[0x105] =        "adc";
   opcode_strings_t[0x106] =        "sbc";
   opcode_strings_t[0x108] =        "tst";
   opcode_strings_t[0x107] =        "ror";
   opcode_strings_t[0x109] =        "neg";
	opcode_strings_t[0x100] =        "and";
   opcode_strings_t[0x101] =        "eor";
   opcode_strings_t[0x102] =        "lsl";
	opcode_strings_t[0x10d] =        "mul";
	opcode_strings_t[0x10c] =        "orr";
	opcode_strings_t[0x10f] =        "mvn";
	opcode_strings_t[0x10e] =        "bic";
   opcode_strings_t[0x103] =        "lsr";
   opcode_strings_t[0x104] =        "asr";
   opcode_strings_t[0x10A] =        "cmp";
   opcode_strings_t[0x10b] =        "cmn";
	opcode_strings_t[0x110] =        "add";
	opcode_strings_t[0x111] =        "add";
	opcode_strings_t[0x112] =        "add";
   opcode_strings_t[0x113] =        "add";
   opcode_strings_t[0x114] =        "cmp";
   opcode_strings_t[0x115] =        "cmp";
   opcode_strings_t[0x116] =        "cmp";
   opcode_strings_t[0x117] =        "cmp";
   opcode_strings_t[0x118] =        "mov";
	opcode_strings_t[0x119] =        "mov";
	opcode_strings_t[0x11A] =        "mov";
	opcode_strings_t[0x11B] =        "mov";
   opcode_strings_t[0x11c] =        "bx";
   opcode_strings_t[0x11D] =        "bx";
   opcode_strings_t[0x11e] =        "bx";
   opcode_strings_t[0x11f] =        "bx";
   opcode_strings_t[0x2C0] =        "add";
   opcode_strings_t[0x2C1] =        "add";
   opcode_strings_t[0x2c2] =        "sub";
   opcode_strings_t[0x2c3] =        "sub";
}
#endif


