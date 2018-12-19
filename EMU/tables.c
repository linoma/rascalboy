#include "gbaemu.h"
#include "opcodes.h"
#include "opedec.h"
#include "io.h"
#include "sound.h"
#include "debug.h"

extern void SetLcd(u16,u8);
//-----------------------------------------------------------------------
void setup_hwdt_handles2(u32 base,ARMOPCODE handle,ARMOPCODE handle2,ARMOPCODE handle3,ARMOPCODE handle4,ARMOPCODE handle2wb,ARMOPCODE handle4wb)
{
	opcode_handles[base]        = handle;
	opcode_handles[base|0x20]   = handle;
	opcode_handles[base|0x80]   = handle3;//up          
	opcode_handles[base|0xA0]   = handle3;//up
	opcode_handles[base|0x100]  = handle2;
	opcode_handles[base|0x120]  = handle2wb;          //writeback
	opcode_handles[base|0x180]  = handle4;//up
	opcode_handles[base|0x1A0]  = handle4wb;//up      //writeback
}
//-----------------------------------------------------------------------
void setup_sdt_handles2(u32 base,ARMOPCODE handle,ARMOPCODE handle2,ARMOPCODE handle3,ARMOPCODE handle4,ARMOPCODE handle3wb,ARMOPCODE handle4wb)
{
	int i;

	for (i=0; i<0x10; i++) {
		opcode_handles [base|i]		  = handle;
		opcode_handles [base|0x20|i]  = handle;
		opcode_handles [base|0x80|i]  = handle2;
		opcode_handles [base|0xA0|i]  = handle2;
		opcode_handles [base|0x100|i] = handle3;
		opcode_handles [base|0x180|i] = handle4;
		opcode_handles [base|0x120|i] = handle3wb;//writeback
		opcode_handles [base|0x1A0|i] = handle4wb;//writeback
	}
}
//-----------------------------------------------------------------------
void setup_dp_handle(u32 base,ARMOPCODE ins,ARMOPCODE ins_reg,ARMOPCODE ins_imm)
{
	u8 i;

	for (i=0; i<0x10;i += (u8)2){
		opcode_handles[base|i]        = ins;//ok
       opcode_handles[base|0x200|i]  = ins_imm;//ok
       opcode_handles[base|0x201|i]  = ins_imm;//ok
	}
   for(i=0;i<8;i++){
       if((i & 1) != 0)
           opcode_handles[base|i]    = ins_reg;
   }
}
//-----------------------------------------------------------------------
/*void setup_hwdt_handles (u32 base, void *handle)
{
	(void *)opcode_handles[base]        = handle;
	(void *)opcode_handles[base|0x20]   = handle;//32
	(void *)opcode_handles[base|0x80]   = handle;//128
	(void *)opcode_handles[base|0xA0]   = handle;//128|32
	(void *)opcode_handles[base|0x100]  = handle;//256
	(void *)opcode_handles[base|0x120]  = handle;//256|32
	(void *)opcode_handles[base|0x180]  = handle;//256|128
	(void *)opcode_handles[base|0x1A0]  = handle;//256|128|32
}*/
//-----------------------------------------------------------------------
void setup_handle_tables (void)
{
	int i, n;

	for (i=0; i<4096; i++)
		opcode_handles[i] = unknown_opcode;

   setup_dp_handle( 0x00,and_,and_reg,and_imm);//ok
	setup_dp_handle( 0x10,ands,ands_reg,ands_imm);//ok
	setup_dp_handle( 0x20,eor,eor_reg,eor_imm);//ok
	setup_dp_handle( 0x30,eors,eors_reg,eors_imm);//ok
	setup_dp_handle( 0x40,sub,sub_reg,sub_imm);//ok
	setup_dp_handle( 0x50,subs,subs_reg,subs_imm);//ok
  	setup_dp_handle( 0x60,rs,rs_reg,rs_imm);//ok
	setup_dp_handle( 0x70,rss,rss_reg,rss_imm);//ok
	setup_dp_handle( 0x80,add,add_reg,add_imm);//ok
	setup_dp_handle( 0x90,adds,adds_reg,adds_imm);//ok
	setup_dp_handle( 0xA0,adc,adc_reg,adc_imm);//ok
	setup_dp_handle( 0xB0,adcs,adcs_reg,adcs_imm);//ok
	setup_dp_handle( 0xC0,subc,subc_reg,subc_imm);//ok
	setup_dp_handle( 0xD0,subcs,subcs_reg,subcs_imm);//ok
	setup_dp_handle( 0xE0,rsc,rsc,rsc);//ok
	setup_dp_handle( 0xF0,rsc,rsc,rsc);//ok
	setup_dp_handle(0x110,tst,tst_reg,tst_imm);//ok
	setup_dp_handle(0x130,teq,teq_reg,teq_imm);//ok
	setup_dp_handle(0x150,cmp,cmp_reg,cmp_imm);//ok
	setup_dp_handle(0x170,cmn,cmn_reg,cmn_imm);//ok
	setup_dp_handle(0x180,orr,orr_reg,orr_imm);//ok
	setup_dp_handle(0x190,orrs,orrs_reg,orrs_imm);//ok
   setup_dp_handle(0x1A0,mov,mov_reg,mov_imm);//ok
   setup_dp_handle(0x1B0,movs,movs_reg,movs_imm);//ok
   setup_dp_handle(0x1C0,bic,bic_reg,bic_imm);//ok
   setup_dp_handle(0x1D0,bics,bics_reg,bics_imm);//ok
	setup_dp_handle(0x1E0,mvn,mvn_reg,mvn_imm);//ok
   setup_dp_handle(0x1F0,mvns,mvns_reg,mvns_imm);//ok

	for (i=0; i<0x10; i++) {
       if(!(i & 1)){
		    opcode_handles[0x100|i] = ins_mrs_cpsr;//ok
           opcode_handles[0x120|i] = ins_msr_cpsr;//ok
           opcode_handles[0x140|i] = ins_mrs_spsr;//ok
		    opcode_handles[0x160|i] = ins_msr_spsr;//ok
           opcode_handles[0x320|i] = ins_msr_cpsr;//ok
           opcode_handles[0x360|i] = ins_msr_spsr;
       }
		for (n=0; n<0xF; n++) {
			opcode_handles[0x800|(n<<5)|i] = stm;//ok
			opcode_handles[0x810|(n<<5)|i] = ldm;//ok
		}
	}
   setup_sdt_handles2(0x400,str_postdownimm,str_postupimm,str_predownimm,str_preupimm,str_predownimmwb,str_preupimmwb);
   setup_sdt_handles2(0x440,strb_postdownimm,strb_postupimm,strb_predownimm,strb_preupimm,strb_predownimmwb,strb_preupimmwb);
   setup_sdt_handles2(0x600,str_postdown,str_postup,str_predown,str_preup,str_predownwb,str_preupwb);
   setup_sdt_handles2(0x640,strb_postdown,strb_postup,strb_predown,strb_preup,strb_predownwb,strb_preupwb);
   setup_sdt_handles2(0x410,ldr_postdownimm,ldr_postupimm,ldr_predownimm,ldr_preupimm,ldr_predownimmwb,ldr_preupimmwb);
	setup_sdt_handles2(0x450,ldrb_postdownimm,ldrb_postupimm,ldrb_predownimm,ldrb_preupimm,ldrb_predownimmwb,ldrb_preupimmwb);
	setup_sdt_handles2(0x610,ldr_postdown,ldr_postup,ldr_predown,ldr_preup,ldr_predownwb,ldr_preupwb);
	setup_sdt_handles2(0x650,ldrb_postdown,ldrb_postup,ldrb_predown,ldrb_preup,ldrb_predownwb,ldrb_preupwb);

	setup_hwdt_handles2(0xB,strh_postdown,strh_predown,strh_postup,strh_preup,strh_predownwb,strh_preupwb);
	setup_hwdt_handles2(0x1B,ldrh_postdown,ldrh_predown,ldrh_postup,ldrh_preup,ldrh_predownwb,ldrh_preupwb);
	setup_hwdt_handles2(0x4B,strh_postdownimm,strh_predownimm,strh_postupimm,strh_preupimm,strh_predownimmwb,strh_preupimmwb);
	setup_hwdt_handles2(0x5B,ldrh_postdownimm,ldrh_predownimm,ldrh_postupimm,ldrh_preupimm,ldrh_predownimmwb,ldrh_preupimmwb);

	setup_hwdt_handles2(0x1D,ldrsb_postdown,ldrsb_predown,ldrsb_postup,ldrsb_preup,ldrsb_predownwb,ldrsb_preupwb);
	setup_hwdt_handles2(0x5D,ldrsb_postdownimm,ldrsb_predownimm,ldrsb_postupimm,ldrsb_preupimm,ldrsb_predownimmwb,ldrsb_preupimmwb);
	setup_hwdt_handles2(0x1F,ldrsh_postdown,ldrsh_predown,ldrsh_postup,ldrsh_preup,ldrsb_predownwb,ldrsh_preupwb);
	setup_hwdt_handles2(0x5F,ldrsh_postdownimm,ldrsh_predownimm,ldrsh_postupimm,ldrsh_preupimm,ldrsh_predownimmwb,ldrsh_preupimmwb);

/*   setup_hwdt_handles(0x1D,hsdt);//ok
	setup_hwdt_handles(0x1F,hsdt);//ok
   setup_hwdt_handles(0x5D,hsdt);//ok
	setup_hwdt_handles(0x5F,hsdt);//ok*/

	opcode_handles[0x9]  = ins_mul;//ok
	opcode_handles[0x19] = ins_muls;//ok
	opcode_handles[0x29] = ins_mla;//ok
	opcode_handles[0x39] = ins_mlas;//ok
	opcode_handles[0x89] = ins_mull_unsigned;
	opcode_handles[0x99] = ins_mulls_unsigned;
	opcode_handles[0xA9] = ins_mlal_unsigned;
	opcode_handles[0xB9] = ins_mlals_unsigned;
	opcode_handles[0xC9] = ins_mull;
	opcode_handles[0xD9] = ins_mulls;
	opcode_handles[0xE9] = ins_mlal;//long mla signed
	opcode_handles[0xF9] = ins_mlals;//long mlas signed
   opcode_handles[0x109] = ins_swp;//ok
   opcode_handles[0x121] = ins_bx;
	opcode_handles[0x149] = ins_swpb;//ok


	for (i=0; i<0x100; i++)
		opcode_handles[0xF00|i] = ins_swi;

	for (i=0; i<0x80; i++) {
		opcode_handles[0xA00|i] = ins_bmi;
		opcode_handles[0xA80|i] = ins_bmi;
       opcode_handles[0xB00|i] = ins_blmi;
		opcode_handles[0xB80|i] = ins_blmi;
	}
}
//-----------------------------------------------------------------------
void setup_io_handle_tables (void)
{
	int i;

	for (i=0; i<0x3FF; i++)
	    io_write_handles[i] = standard_io_write_handle;
   for(i=0;i<0x56;i++)
	    io_write_handles[i] = SetLcd;
   for(i=0x60;i<0xA7;i++)
       io_write_handles[i] = sound_write;
   for(i=0xB0;i<0xBC;i++)
       io_write_handles[i] = reg_dm0cnt_write;
   for(i=0xBC;i<0xC8;i++)
       io_write_handles[i] = reg_dm1cnt_write;
   for(i=0xC8;i<0xD4;i++)
       io_write_handles[i] = reg_dm2cnt_write;
   for(i=0xD4;i<0xDF;i++)
       io_write_handles[i] = reg_dm3cnt_write;
   for(i=0x100;i<0x110;i++)
       io_write_handles[i] = timer_io_handle;
   for(i=0x130;i<0x131;i++)
       io_write_handles[i] = reg_keyinput_write;

	io_write_handles[0x200] = ie_io_handle;
   io_write_handles[0x201] = ie_io_handle;
	io_write_handles[0x202] = if_io_handle;
   io_write_handles[0x203] = if_io_handle;
   io_write_handles[0x208] = ime_io_handle;
   io_write_handles[0x209] = ime_io_handle;

   for(i=0x120;i<0x12C;i++)
       io_write_handles[i] = com_io_handle;
   for(i=0x134;i<0x136;i++)
       io_write_handles[i] = com_io_handle;

   io_write_handles[0x140] = joy_io_handle;
   io_write_handles[0x141] = joy_io_handle;

   for(i=0x150;i<0x160;i++)
       io_write_handles[i] = joy_io_handle;

   io_write_handles[0x204] = wait_io_handle;
   io_write_handles[0x205] = wait_io_handle;
   io_write_handles[0x301] = power_mode;
}                                                          //600e088
//-----------------------------------------------------------------------
#ifdef _DEBUG
//-----------------------------------------------------------------------
void setup_dp_strings(u32 base, char *string)
{
	u8 i;

	for (i=0; i<0x10;i += (u8)2){
		opcode_strings[base|i]        = string;//ok
       opcode_strings[base|0x200|i]  = string;//ok
       opcode_strings[base|0x201|i]  = string;//ok
	}
   for(i=0;i<8;i++){
       if((i & 1) != 0)
           opcode_strings[base|i]    = string;
   }
}
//-----------------------------------------------------------------------
void setup_sdt_strings (u32 base, char *string)
{
	int i;

	for (i=0; i<0x10; i++) {
		opcode_strings [base|i]		  = string;
		opcode_strings [base|0x20|i]  = string;
		opcode_strings [base|0x80|i]  = string;
		opcode_strings [base|0xA0|i]  = string;
		opcode_strings [base|0x100|i] = string;
		opcode_strings [base|0x120|i] = string;
		opcode_strings [base|0x180|i] = string;
		opcode_strings [base|0x1A0|i] = string;
	}
}
//-----------------------------------------------------------------------
void setup_string_tables(void)
{
	int i, n;

   for(i=0;i<16;i++)
       irq_strings[i] = "Unknown";
   irq_strings[0] = "VBlank";
   irq_strings[1] = "HBlank";
   irq_strings[2] = "VCount";
   irq_strings[3] = "Timer 0";
   irq_strings[4] = "Timer 1";
   irq_strings[5] = "Timer 2";
   irq_strings[6] = "Timer 3";
   irq_strings[8] = "Dma 0";
   irq_strings[9] = "Dma 1";
   irq_strings[10] = "Dma 2";
   irq_strings[11] = "Dma 3";

	for (i=0; i<0x1000; i++)
		opcode_strings [i] = "Unknown";

   setup_dp_strings( 0x00,"and");//ok
	setup_dp_strings( 0x10,"ands");//ok
	setup_dp_strings( 0x20,"eor");//ok
	setup_dp_strings( 0x30,"eors");//ok
	setup_dp_strings( 0x40,"sub");//ok
	setup_dp_strings( 0x50,"sub");//ok
  	setup_dp_strings( 0x60,"rsb");//ok
	setup_dp_strings( 0x70,"rsbs");//ok
	setup_dp_strings( 0x80,"add");//ok
	setup_dp_strings( 0x90,"adds");//ok
	setup_dp_strings( 0xA0,"adc");//ok
	setup_dp_strings( 0xB0,"adcs");//ok
	setup_dp_strings( 0xC0,"subc");//ok
	setup_dp_strings( 0xD0,"subcs");//ok
	setup_dp_strings( 0xE0,"rsbc");//ok
	setup_dp_strings( 0xF0,"rsbcs");//ok
	setup_dp_strings(0x110,"tst");//ok
	setup_dp_strings(0x130,"teq");//ok
	setup_dp_strings(0x150,"cmp");//ok
	setup_dp_strings(0x170,"cmn");//ok
	setup_dp_strings(0x180,"orr");//ok
	setup_dp_strings(0x190,"orrs");//ok
   setup_dp_strings(0x1A0,"mov");//ok
   setup_dp_strings(0x1B0,"movs");//ok
   setup_dp_strings(0x1C0,"bic");//ok
   setup_dp_strings(0x1D0,"bics");//ok
	setup_dp_strings(0x1E0,"mvn");//ok
   setup_dp_strings(0x1F0,"mvns");//ok

	for (i=0; i<0x10; i++) {
       if(!(i & 1)){
		    opcode_strings[0x100|i] = "mrs";//ok
           opcode_strings[0x120|i] = "msr";
           opcode_strings[0x140|i] = "mrs";
		    opcode_strings[0x160|i] = "msr";
           opcode_strings[0x320|i] = "msr";
           opcode_strings[0x360|i] = "msr";
       }
		for (n=0; n<0xF; n++) {
			opcode_strings[0x800|(n<<5)|i] = "stm";//ok
			opcode_strings[0x810|(n<<5)|i] = "ldm";//ok
		}
	}

	setup_sdt_strings(0x600,"str");
	setup_sdt_strings(0x640,"strb");
	setup_sdt_strings(0x400,"str");
	setup_sdt_strings(0x440,"strb");
	setup_sdt_strings(0x610,"ldr");
	setup_sdt_strings(0x650,"ldrb");
	setup_sdt_strings(0x410,"ldr");
	setup_sdt_strings(0x450,"ldrb");
	setup_sdt_strings(0x5B,"ldrh");
	setup_sdt_strings(0x1B,"ldrh");
	setup_sdt_strings(0x5F,"ldrsh");
	setup_sdt_strings(0x1F,"ldrsh");
	setup_sdt_strings(0x5D,"ldrsb");
	setup_sdt_strings(0x1D,"ldrsb");
	setup_sdt_strings(0x4B,"strh");
	setup_sdt_strings(0xB, "strh");

   setup_sdt_strings(0x5B,"ldrh");
   setup_sdt_strings(0x1B,"ldrh");
   setup_sdt_strings(0x5F,"ldrsh");
   setup_sdt_strings(0x1F,"ldrsh");
   setup_sdt_strings(0x5D,"ldrsb");
   setup_sdt_strings(0x1D,"ldrsb");
   setup_sdt_strings(0x4B,"strh");
   setup_sdt_strings(0xB,"strh");

	opcode_strings [0x9] = "mul";
	opcode_strings[0x19] = "muls";
	opcode_strings[0x29] = "mla";
	opcode_strings[0x39] = "mlas";
	opcode_strings[0x89] = "umull";
	opcode_strings[0x99] = "umulls";
	opcode_strings[0xC9] = "smull";
	opcode_strings[0xE9] = "smulls";
	opcode_strings[0xA9] = "umlal";
	opcode_strings[0xB9] = "umlals";
	opcode_strings[0xE9] = "smlal";
	opcode_strings[0xF9] = "smlals";
	opcode_strings[0x121] = "bx";

	for (i=0; i<0x100; i++)
		opcode_strings[0xF00|i] = "swi";
	for (i=0; i<0x100; i++) {
		opcode_strings[(0xA<<8)|i] = "b";
		opcode_strings[(0xB<<8)|i] = "bl";
	}
}
//-----------------------------------------------------------------------
void setup_dp_debug_handle (u32 base, DECODEARM handle)
{
	int i;

	for (i=0; i<0x10;i += (u8)2){
		debug_handles[base|i]        = handle;//ok
       debug_handles[base|0x200|i]  = handle;//ok
       debug_handles[base|0x201|i]  = handle;//ok
	}
   for(i=0;i<8;i++){
       if((i & 1) != 0)
           debug_handles[base|i]    = handle;
   }
}
//-----------------------------------------------------------------------
void setup_sdt_debug_handles (u32 base, DECODEARM handle)
{
	int i;

	for (i=0; i<0x10; i++) {
		debug_handles  [base|i]					= handle;
		debug_handles  [base|0x20|i]			= handle;
		debug_handles  [base|0x80|i]			= handle;
		debug_handles  [base|0x80|0x20|i]		= handle;
		debug_handles  [base|0x100|i]			= handle;
		debug_handles  [base|0x100|0x80|i]		= handle;
		debug_handles  [base|0x100|0x20|i]		= handle;
		debug_handles  [base|0x100|0x80|0x20|i] = handle;
	}
}
//-----------------------------------------------------------------------
void setup_hwdt_debug_handles (u32 base, DECODEARM handle)
{
	debug_handles [base] = handle;
	debug_handles [base|0x20] = handle;
	debug_handles [base|0x80] = handle;
	debug_handles [base|0x80|0x20] = handle;
	debug_handles [base|0x100] = handle;
	debug_handles [base|0x100|0x20] = handle;
	debug_handles [base|0x100|0x80] = handle;
	debug_handles [base|0x100|0x80|0x20] = handle;
}
//-----------------------------------------------------------------------
void setup_debug_handles (void)
{
	int i,n;

	for (i=0; i<0x1000; i++)
		debug_handles [i] = standard_debug_handle;

	setup_dp_debug_handle ( 0x00, dp_debug_handle);
	setup_dp_debug_handle ( 0x10, dp_debug_handle);
	setup_dp_debug_handle ( 0x20, dp_debug_handle);
	setup_dp_debug_handle ( 0x30, dp_debug_handle);
	setup_dp_debug_handle ( 0x40, dp_debug_handle);
	setup_dp_debug_handle ( 0x50, dp_debug_handle);
   setup_dp_debug_handle ( 0x60, dp_debug_handle);
	setup_dp_debug_handle ( 0x70, dp_debug_handle);
	setup_dp_debug_handle ( 0x80, dp_debug_handle);
	setup_dp_debug_handle ( 0x90, dp_debug_handle);
	setup_dp_debug_handle ( 0xA0, dp_debug_handle);
	setup_dp_debug_handle ( 0xB0, dp_debug_handle);
	setup_dp_debug_handle ( 0xC0, dp_debug_handle);
	setup_dp_debug_handle ( 0xD0, dp_debug_handle);
	setup_dp_debug_handle ( 0xE0, dp_debug_handle);
	setup_dp_debug_handle ( 0xF0, dp_debug_handle);
	setup_dp_debug_handle (0x110, dpnw_debug_handle);
	setup_dp_debug_handle (0x130, dpnw_debug_handle);
	setup_dp_debug_handle (0x150, dpnw_debug_handle);
	setup_dp_debug_handle (0x170, dpnw_debug_handle);
	setup_dp_debug_handle (0x180, dp_debug_handle);
	setup_dp_debug_handle (0x190, dp_debug_handle);
	setup_dp_debug_handle (0x1A0, dpsingle_debug_handle);
	setup_dp_debug_handle (0x1B0, dpsingle_debug_handle);
	setup_dp_debug_handle (0x1C0, dp_debug_handle);
	setup_dp_debug_handle (0x1D0, dp_debug_handle);
	setup_dp_debug_handle (0x1E0, dpsingle_debug_handle);
	setup_dp_debug_handle (0x1F0, dpsingle_debug_handle);

   for(i=0;i<0x10;i++){
       if(!(i & 1)){
           debug_handles[0x100|i] = msr_debug_handle;//ok
           debug_handles[0x120|i] = msr_debug_handle;//ok
           debug_handles[0x140|i] = msr_debug_handle;//ok
		    debug_handles[0x160|i] = msr_debug_handle;//ok
           debug_handles[0x320|i] = msr_debug_handle;//ok
           debug_handles[0x360|i] = msr_debug_handle;
       }
		for (n=0; n<0xF; n++) {
			debug_handles[0x800|(n<<5)|i] = mdt_debug_handle;
			debug_handles[0x810|(n<<5)|i] = mdt_debug_handle;
		}
   }

	setup_sdt_debug_handles (0x600, sdt_debug_handle);
	setup_sdt_debug_handles (0x640, sdt_debug_handle);
	setup_sdt_debug_handles (0x400, sdt_debug_handle);
	setup_sdt_debug_handles (0x440, sdt_debug_handle);
	setup_sdt_debug_handles (0x610, sdt_debug_handle);
	setup_sdt_debug_handles (0x650, sdt_debug_handle);
	setup_sdt_debug_handles (0x410, sdt_debug_handle);
	setup_sdt_debug_handles (0x450, sdt_debug_handle);

	setup_hwdt_debug_handles (0x5B|0x4,hwdt_debug_handle);
	setup_hwdt_debug_handles (0x5D,hwdt_debug_handle);
	setup_hwdt_debug_handles (0x1B,hwdt_debug_handle);
	setup_hwdt_debug_handles (0x1B|0x4,hwdt_debug_handle);
	setup_hwdt_debug_handles (0x1D,hwdt_debug_handle);
	setup_hwdt_debug_handles (0x4B,hwdt_debug_handle);
	setup_hwdt_debug_handles (0xB,hwdt_debug_handle);
	setup_hwdt_debug_handles (0x5B,hwdt_debug_handle);

        debug_handles [0x9] = (DECODEARM)mul_debug_handle;
        debug_handles[0x19] = (DECODEARM)mul_debug_handle;
        debug_handles[0x29] = (DECODEARM)mul_debug_handle;
        debug_handles[0x39] = (DECODEARM)mul_debug_handle;
	debug_handles[0x89] = mull_debug_handle;
	debug_handles[0x99] = mull_debug_handle;
	debug_handles[0xC9] = mull_debug_handle;
	debug_handles[0xE9] = mull_debug_handle;
	debug_handles[0xA9] = mull_debug_handle;
	debug_handles[0xB9] = mull_debug_handle;
	debug_handles[0xE9] = mull_debug_handle;
	debug_handles[0xF9] = mull_debug_handle;
   debug_handles[0x121] = bx_debug_handle;

	for (i=0; i<0x100; i++) {
       debug_handles[0xF00|i] = swi_debug_handle;
		debug_handles[(0xA<<8)|i] = b_debug_handle;
		debug_handles[(0xB<<8)|i] = b_debug_handle;
	}
}
#endif





