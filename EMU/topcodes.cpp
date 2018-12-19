#include "gbaemu.h"
#include "topcodes.h"
#include "bios.h"
#include "debug.h"

//---------------------------------------------------------------------------
u8 tins_unknown(void)
{
	return 1;
}
//---------------------------------------------------------------------------
u8 tins_swi(void)
{
   u8 res = cycP;
   bios((u8)OPCODE_T);
   return (u8)(res + cycS + cycN);
}
//---------------------------------------------------------------------------
// SBC
//---------------------------------------------------------------------------
u8 tins_sbc(void)
{
   u32 *regd;

   *regd = CalcSubFlags(*(regd = &GP_REG[OPCODE_T & 7]),GP_REG[(OPCODE_T >> 3) & 7] + (CFLAG ? 0 : 1));
   return cycP;
}
//---------------------------------------------------------------------------
// TST
//---------------------------------------------------------------------------
u8 tins_tst(void)
{
 	SET_DP_LOG_FLAGS(GP_REG[OPCODE_T & 7] & GP_REG[(OPCODE_T >> 3) & 7]);
	return cycP;
}
//---------------------------------------------------------------------------
// ROR
//---------------------------------------------------------------------------
u8 tins_ror_reg(void)
{
   u8 s;
   u32 value,*regd;

   value = *(regd = &GP_REG[OPCODE_T & 7]);
   s = (u8)GP_REG[(OPCODE_T >> 3) & 7];
   if(s){
       s &= 0x1F;
       if(s == 0)
           CFLAG = (u8)(value >> 31);
       else{
#if defined(__BORLANDC__)
       _EAX = value;
       _CL = s;
       __asm{
           ror eax,cl
           setc byte ptr[CFLAG]
       }
       value = _EAX;
#endif
       }
   }
   SET_DP_LOG_FLAGS(*regd = value);
	return (u8)(cycI + cycP);
}
//---------------------------------------------------------------------------
// MUL
//---------------------------------------------------------------------------
u8 tins_mul(void)
{
   u8 m;
   u32 value;

   SET_DP_LOG_FLAGS((GP_REG[OPCODE_T & 7] *= (value = GP_REG[(OPCODE_T >> 3) & 7])));
   value >>= 8;
   if(value == 0 || value == 0xFFFFFF)
       m = 1;
   else{
       value >>= 8;
       if(value == 0 || value == 0xFFFF)
           m = 2;
       else{
           value >>= 8;
           m = (u8)((value == 0 || value == 0xFF) ? 3 : 4);
       }
   }
   return (u8)(cycP + m);
}
//---------------------------------------------------------------------------
// ADD
//---------------------------------------------------------------------------
u8 tins_adc_reg(void)
{
   u32 *regd;

	*regd = CalcAddcFlags(GP_REG[(OPCODE_T >> 3) & 7],*(regd = &GP_REG[OPCODE_T & 7]));
	return cycP;
}
//---------------------------------------------------------------------------
u8 tins_add_pc(void)
{
   GP_REG[(OPCODE_T >> 8) & 7] = (REG_PC & ~3) + (((u8)OPCODE_T) << 2);
	return cycP;
}
//---------------------------------------------------------------------------
u8 tins_add_lo_hi (void)
{
   u8 rd;

   if((rd = (u8)(((OPCODE_T & 0x80) >> 4) | (OPCODE_T & 7))) == 15){
#ifdef _DEBPRO
       InsertStackAddress(REG_PC,"Add Hi registers");
#endif
       REG_PC += GP_REG[(OPCODE_T >> 3) & 0xF] + 2;
       arm.bRefillPipe = 1;
       return cycP;
   }
   GP_REG[rd] += GP_REG[(OPCODE_T >> 3) & 0xF];
	return cycP;
}
//---------------------------------------------------------------------------
u8 tins_add_rel_sp (void)
{
   GP_REG[13] += ((OPCODE_T&0x7F)<<2);
	return cycP;
}
//---------------------------------------------------------------------------
u8 tins_add_sp (void)
{
   GP_REG[(OPCODE_T >> 8) & 7] = GP_REG[13] + (((u8)OPCODE_T)<<2);
	return cycP;
}
//---------------------------------------------------------------------------
u8 tins_add_short_imm (void)
{
	GP_REG[OPCODE_T & 7] = CalcAddFlags(GP_REG[(OPCODE_T >> 3) & 7],(u32)((u8)((OPCODE_T >> 6) & 7)));
	return cycP;
}
//---------------------------------------------------------------------------
u8 tins_add(void)
{
	GP_REG[OPCODE_T & 7] = CalcAddFlags(GP_REG[(OPCODE_T >> 3) & 7],GP_REG[(OPCODE_T >> 6) & 7]);
	return cycP;
}
//---------------------------------------------------------------------------
u8 tins_add_imm (void)
{
	u32 *regd;

	*regd = CalcAddFlags(*(regd = &GP_REG[(OPCODE_T >> 8) & 7]),(u32)((u8)OPCODE_T));
	return cycP;
}
//---------------------------------------------------------------------------
// MOV
//---------------------------------------------------------------------------
u8 tins_mov_hi_lo (void)
{
   u8 r;

   r = (u8)(((OPCODE_T & 0x80) >> 4) | (OPCODE_T & 7));
#ifdef _DEBPRO
   if(r == 15)
       InsertStackAddress(REG_PC,"Mov Hi registers");
#endif
   GP_REG[r] = GP_REG[((OPCODE_T >> 3) & 0xF)];
   if(r == 15){
       REG_PC = (REG_PC & ~1) + 2;
       arm.bRefillPipe = 1;
   }
	return cycP;
}
//---------------------------------------------------------------------------
u8 tins_mov_imm (void)
{
	SET_DP_LOG_FLAGS((GP_REG[(OPCODE_T >> 8) & 7] = (u8)OPCODE_T));
	return cycP;
}
//---------------------------------------------------------------------------
// LDR
//---------------------------------------------------------------------------
u8 tins_ldmia(void)
{
   u8 n;
   u32 address,*regd,*reg;
   u16 p;

   address = *(regd = &(reg = GP_REG)[(OPCODE_T >> 8) & 7]) & ~3;
   for(p=1,n=0;p < 0x100;reg++,p <<= 1){
       if(!(OPCODE_T & p))
           continue;
       *reg = READWORD(address);
       if(reg == regd)
           regd = NULL;
       address += 4;
       n += cycS;
   }
   if(regd != NULL)
       *regd = address;
   return (u8)(cycI + n + cycN);
}
//---------------------------------------------------------------------------
u8 tins_sp_rel_ldr(void)
{
	GP_REG[(OPCODE_T >> 8) & 7] = READWORD(GP_REG[13] + (((u8)OPCODE_T)<<2));
	return LDR_RET;
}
//---------------------------------------------------------------------------
u8 tins_pc_rel_ldr(void)
{
	GP_REG[(OPCODE_T >> 8) & 7] = READWORD((REG_PC & ~3) + (((u8)OPCODE_T)<<2));
	return LDR_RET;
}
//---------------------------------------------------------------------------
u8 tins_ldr_reg(void)
{
	GP_REG[OPCODE_T & 7] = READWORD(GP_REG[(OPCODE_T >> 3) & 7] + GP_REG[(OPCODE_T >> 6) & 7]);
	return LDR_RET;
}
//---------------------------------------------------------------------------
u8 tins_ldr_imm(void)
{
	GP_REG[OPCODE_T & 7] = READWORD(GP_REG[(OPCODE_T >> 3) & 7] + (((OPCODE_T>>6)&0x1F)<<2));
	return LDR_RET;
}
//---------------------------------------------------------------------------
u8 tins_ldrb_imm(void)
{                                                                         //805640a
	GP_REG[OPCODE_T & 7] = READBYTE(GP_REG[(OPCODE_T >> 3) & 7] + (((OPCODE_T>>6)&0x1F)));
	return LDR_RET;
}
//---------------------------------------------------------------------------
u8 tins_ldrb_reg(void)
{
	GP_REG[OPCODE_T & 7] = READBYTE(GP_REG[(OPCODE_T >> 3) & 7] + GP_REG[(OPCODE_T >> 6) & 7]);
	return LDR_RET;
}
//---------------------------------------------------------------------------
u8 tins_ldrh_reg(void)
{
	GP_REG[OPCODE_T & 7] = READHWORD(GP_REG[(OPCODE_T >> 3) & 7] + GP_REG[(OPCODE_T >> 6) & 7]);
	return LDR_RET;
}
//---------------------------------------------------------------------------
u8 tins_ldrh_imm(void)
{
   u32 adr,*reg;

	*(reg = &GP_REG[OPCODE_T & 7]) = READHWORD((adr = GP_REG[(OPCODE_T >> 3) & 7] + (((OPCODE_T>>6)&0x1F)<<1)) & ~1);
   if((adr & 1))
       *reg = (*reg << 24) | ((u16)*reg >> 8);
	return LDR_RET;
}
//---------------------------------------------------------------------------
u8 tins_ldrsh(void)
{
	GP_REG[OPCODE_T & 7] = (u32)((s16)READHWORD(GP_REG[(OPCODE_T >> 3) & 7] + GP_REG[(OPCODE_T >> 6) & 7]));
	return LDR_RET;
}
//---------------------------------------------------------------------------
u8 tins_ldrsb_reg(void)
{
	GP_REG[OPCODE_T & 7] = (u32)((s8)READBYTE(GP_REG[(OPCODE_T >> 3) & 7] + GP_REG[(OPCODE_T >> 6) & 7]));
	return LDR_RET;
}
//---------------------------------------------------------------------------
// STR
//---------------------------------------------------------------------------
u8 tins_stmia(void)
{
   u8 n;
   u32 address,*regd,*reg;
   u16 p;

   address = *(regd = &(reg = GP_REG)[(OPCODE_T >> 8) & 7]) & ~3;
   n = 0;
   for(p=1;p < 0x100;p <<= 1,reg++){
       if(!(OPCODE_T & p))
           continue;
       WRITEWORD(address,*reg);
       address += 4;
       n += cycS;
   }
   *regd = address;
   return (u8)((n - cycS) + cycN + cycPN);
}
//---------------------------------------------------------------------------
u8 tins_str_reg(void)
{
	WRITEWORD(GP_REG[(OPCODE_T >> 3) & 7] + GP_REG[(OPCODE_T >> 6) & 7], GP_REG[OPCODE_T & 7]);
	return STR_RET;
}
//---------------------------------------------------------------------------
u8 tins_str_imm(void)
{
   WRITEWORD(GP_REG[(OPCODE_T >> 3) & 7] + (((OPCODE_T>>6)&0x1F)<<2), GP_REG[OPCODE_T & 7]);
	return STR_RET;
}
//---------------------------------------------------------------------------
u8 tins_sp_rel_str(void)
{
	WRITEWORD(GP_REG[13] + (((u8)OPCODE_T)<<2), GP_REG[(OPCODE_T >> 8) & 7]);
	return STR_RET;
}                           
//---------------------------------------------------------------------------
u8 tins_strh_reg(void)
{
   WRITEHWORD(GP_REG[(OPCODE_T >> 3) & 7] + GP_REG[(OPCODE_T >> 6) & 7],(u16)GP_REG[OPCODE_T & 7]);
	return STR_RET;
}
//---------------------------------------------------------------------------
u8 tins_strb_reg(void)
{
   WRITEBYTE(GP_REG[(OPCODE_T >> 3) & 7] + GP_REG[(OPCODE_T >> 6) & 7],GP_REG[OPCODE_T & 7]);
	return STR_RET;
}                                                                           //801f6f2
//---------------------------------------------------------------------------
u8 tins_strb_imm(void)
{
	WRITEBYTE(GP_REG[(OPCODE_T >> 3) & 7] + (((OPCODE_T>>6)&0x1F)),GP_REG[OPCODE_T & 7]);
	return STR_RET;
}
//---------------------------------------------------------------------------
u8 tins_strh_imm(void)
{
	WRITEHWORD(GP_REG[(OPCODE_T >> 3) & 7] + (((OPCODE_T>>6)&0x1F)<<1),(u16)GP_REG[OPCODE_T & 7]);
	return STR_RET;
}
//---------------------------------------------------------------------------
// lsl
//---------------------------------------------------------------------------
u8 tins_lsl_reg(void)
{
   u8 temp;
   register u32 temp1,*regd;

   temp1 = *(regd = &GP_REG[OPCODE_T & 7]);
   temp = (u8)GP_REG[(OPCODE_T >> 3) & 7];
   if(temp && temp < 32){
       temp1 <<= temp;
#if defined(__BORLANDC__)
       __asm {
           setc byte ptr[CFLAG]
           setz byte ptr[ZFLAG]
           sets byte ptr[NFLAG]
       }
#else
   __asm__ (
       "setcb _arm+18\r\n"
       "setzb _arm+16\r\n"
       "setsb _arm+17\r\n"
   );
#endif
       *regd = temp1;
       return (u8)(cycI + cycP);
   }
   else if(temp == 32){
       CFLAG = (u8)(temp1 & 1);
       temp1 = 0;
   }
   else if(temp > 0){
       temp1 = 0;
       CFLAG = 0;
   }
	SET_DP_LOG_FLAGS(*regd = temp1);
	return (u8)(cycI + cycP);
}
//---------------------------------------------------------------------------
u8 tins_lsl_imm(void)
{
   u32 temp1;

   temp1 = GP_REG[(OPCODE_T >> 3) & 7] << ((OPCODE_T>>6)&0x1F);
#if defined(__BORLANDC__)
       __asm {
           setc byte ptr[CFLAG]
           setz byte ptr[ZFLAG]
           sets byte ptr[NFLAG]
       }
#else
   __asm__ (
       "setcb _arm+18\r\n"
       "setzb _arm+16\r\n"
       "setsb _arm+17\r\n"
   );
#endif
   GP_REG[OPCODE_T & 7] = temp1;
	return cycP;
}
//---------------------------------------------------------------------------
// lsr
//---------------------------------------------------------------------------
u8 tins_lsr_reg(void)
{
   u8 temp;
   register u32 temp1,*regd;

   temp = (u8)(GP_REG[(OPCODE_T >> 3) & 7]);
   temp1 = *(regd = &GP_REG[OPCODE_T & 7]);
   if(temp && temp < 32){
       temp1 >>= temp;
#if defined(__BORLANDC__)
       __asm {
           setc byte ptr[CFLAG]
           setz byte ptr[ZFLAG]
           sets byte ptr[NFLAG]
       }
#else
   __asm__ (
       "setcb _arm+18\r\n"
       "setzb _arm+16\r\n"
       "setsb _arm+17\r\n"
   );
#endif
       GP_REG[OPCODE_T & 7] = temp1;
       return (u8)(cycP+cycI);
   }
   else if(temp == 32){
       CFLAG = (u8)(temp1 >> 31);
       temp1 = 0;
   }
   else if(temp > 32){
       CFLAG = 0;
       temp1 = 0;
   }
 	SET_DP_LOG_FLAGS(*regd = temp1);
	return (u8)(cycI + cycP);
}
//---------------------------------------------------------------------------
u8 tins_lsr_imm(void)
{
   register u32 temp1;

   temp1 = GP_REG[(OPCODE_T >> 3) & 7];
   temp1 >>= ((OPCODE_T>>6)&0x1F);
#if defined(__BORLANDC__)
       __asm {
           setc byte ptr[CFLAG]
           setz byte ptr[ZFLAG]
           sets byte ptr[NFLAG]
       }
#else
   __asm__ (
       "setcb _arm+18\r\n"
       "setzb _arm+16\r\n"
       "setsb _arm+17\r\n"
   );
#endif
   GP_REG[OPCODE_T & 7] = temp1;
	return cycP;
}
//---------------------------------------------------------------------------
//ASR
//---------------------------------------------------------------------------
u8 tins_asr_reg(void)
{
   u8 shift;
   u32 *regd;
   s32 temp1;

   temp1 = *(regd = &GP_REG[OPCODE_T & 7]);
   shift = (u8)GP_REG[(OPCODE_T >> 3) & 7];
   if(shift && shift < 32){
       temp1 >>= shift;
#if defined(__BORLANDC__)
       __asm {
           setc byte ptr[CFLAG]
           setz byte ptr[ZFLAG]
           sets byte ptr[NFLAG]
       }
#else
   __asm__ (
       "setcb _arm+18\r\n"
       "setzb _arm+16\r\n"
       "setsb _arm+17\r\n"
   );
#endif
       *regd = temp1;
   }
   else if(shift > 31){
       if((CFLAG = (u8)(temp1 >> 31)) != 0)
           temp1 = 0xFFFFFFFF;
       else
           temp1 = 0;
	    SET_DP_LOG_FLAGS(*regd = temp1);
   }
   return (u8)(cycI + cycP);
}
//---------------------------------------------------------------------------
u8 tins_asr_imm(void)
{
   u8 shift;
   s32 temp1;

   temp1 = GP_REG[(OPCODE_T >> 3) & 7];
   if((shift = (u8)((OPCODE_T>>6) & 0x1F)) == 0){
       if((CFLAG = (u8)(temp1 >> 31)) != 0)
           temp1 = 0xFFFFFFFF;
       else
           temp1 = 0;
	    SET_DP_LOG_FLAGS(GP_REG[OPCODE_T & 7] = temp1);
   }
   else{
       temp1 >>= shift;
#if defined(__BORLANDC__)
       __asm {
           setc byte ptr[CFLAG]
           setz byte ptr[ZFLAG]
           sets byte ptr[NFLAG]
       }
#else
   __asm__ (
       "setcb _arm+18\r\n"
       "setzb _arm+16\r\n"
       "setsb _arm+17\r\n"
   );
#endif
       GP_REG[OPCODE_T & 7] = temp1;
   }
	return cycP;
}
//---------------------------------------------------------------------------
// CMN
//---------------------------------------------------------------------------
u8 tins_cmn_reg(void)
{
   CalcAddFlags(GP_REG[OPCODE_T & 7],GP_REG[(OPCODE_T >> 3) & 7]);
	return cycP;
}
//---------------------------------------------------------------------------
// CMP
//---------------------------------------------------------------------------
u8 tins_cmp_reg(void)
{
	CalcSubFlags(GP_REG[((OPCODE_T & 0x80) >> 4)|(OPCODE_T & 0x7)],GP_REG[(OPCODE_T>>3)&0xf]);
	return cycP;
}
//---------------------------------------------------------------------------
u8 tins_cmp(void)
{
	CalcSubFlags(GP_REG[OPCODE_T & 7],GP_REG[(OPCODE_T >> 3) & 7]);
	return cycP;
}
//---------------------------------------------------------------------------
u8 tins_cmp_imm(void)
{
	CalcSubFlags(GP_REG[(OPCODE_T >> 8) & 7],(u8)OPCODE_T);
	return cycP;
}
//---------------------------------------------------------------------------
u8 tins_eor(void)
{
	SET_DP_LOG_FLAGS((GP_REG[OPCODE_T & 7] ^= GP_REG[(OPCODE_T >> 3) & 7]));
	return cycP;
}
//---------------------------------------------------------------------------
u8 tins_and(void)
{
	SET_DP_LOG_FLAGS((GP_REG[OPCODE_T & 7] &= GP_REG[(OPCODE_T >> 3) & 7]));
	return cycP;
}
//---------------------------------------------------------------------------
u8 tins_orr(void)
{
	SET_DP_LOG_FLAGS((GP_REG[OPCODE_T & 7] |= GP_REG[(OPCODE_T >> 3) & 7]));
	return cycP;
}
//---------------------------------------------------------------------------
u8 tins_bic(void)
{
	SET_DP_LOG_FLAGS((GP_REG[OPCODE_T & 7] &= ~GP_REG[(OPCODE_T >> 3) & 7]));
	return cycP;
}
//---------------------------------------------------------------------------
u8 tins_mvn(void)
{
	SET_DP_LOG_FLAGS ((GP_REG[OPCODE_T & 7] = ~GP_REG[(OPCODE_T >> 3) & 7]));
	return cycP;
}
//---------------------------------------------------------------------------
u8 tins_neg(void)
{
   GP_REG[OPCODE_T & 7] = CalcSubFlags(0,GP_REG[(OPCODE_T >> 3) & 7]);
	return cycP;
}
//---------------------------------------------------------------------------
// SUB
//---------------------------------------------------------------------------
u8 tins_sub_reg_imm(void)
{
   GP_REG[OPCODE_T & 7] = CalcSubFlags(GP_REG[(OPCODE_T >> 3) & 7],((OPCODE_T >> 6) & 7));
	return cycP;
}
//---------------------------------------------------------------------------
u8 tins_sub_sp(void)
{
	GP_REG[13] -= (OPCODE_T & 0x7F) << 2;
	return cycP;
}
//---------------------------------------------------------------------------
u8 tins_sub(void)
{
	GP_REG[OPCODE_T & 7] = CalcSubFlags(GP_REG[(OPCODE_T >> 3) & 7], GP_REG[(OPCODE_T >> 6) & 7]);
	return cycP;
}
//---------------------------------------------------------------------------
u8 tins_sub_imm(void)
{
	u32 *regd;

	*regd = CalcSubFlags(*(regd = &GP_REG[(OPCODE_T >> 8) & 7]), (u32)((u8)OPCODE_T));
	return cycP;
}
//---------------------------------------------------------------------------
// stack
//---------------------------------------------------------------------------
u8 tins_push(void)
{
   u8 p,n;
   u32 *reg,adress;

   adress = (reg = &GP_REG[7])[6];
   n=0;
   if((OPCODE_T & 0x100)){
       adress -= 4;
       WRITEWORD(adress,GP_REG[14]);
       n += cycS;
   }
   for(p = 0x80;p > 0;p >>= 1,reg--){
       if(!(OPCODE_T & p))
           continue;
       adress -= 4;
       WRITEWORD(adress,*reg);
       n += cycS;
   }
   GP_REG[13] = adress;
	return (u8)((n - cycS) + cycN + cycPN);
}
//---------------------------------------------------------------------------
u8 tins_pop(void)
{
   u8 n;
   u32 *reg,adress;
   u16 p;

   n = 0;
   adress = (reg = GP_REG)[13];
   for(p = 1;p < 0x100;p <<= 1,reg++) {
	    if(!(OPCODE_T & p))
           continue;
       *reg = READWORD(adress);
       adress += 4;
       n += cycS;
   }
   if((OPCODE_T & 0x100)){
#ifdef _DEBPRO
       InsertStackAddress(REG_PC,"Pop");
#endif
       REG_PC = READWORD(adress);
       adress += 4;
       arm.bRefillPipe = 1;
/*       if(!(REG_PC & 1)){
           CPSR &= ~T_BIT;
           REG_PC = (REG_PC & ~3) + 4;
           exec = arm_exec;                                                 //8030660
           advance_instruction_pipe();
       }
       else*/
       REG_PC = (REG_PC & ~1) + 2;
   }
   GP_REG[13] = adress;
	return (u8)(cycI + n + cycN + cycPN);
}
//---------------------------------------------------------------------------
// branch
//---------------------------------------------------------------------------
u8 tins_bx(void)
{
   u8 res;

   res = cycP;
   if((OPCODE_T & 0x80))
       GP_REG[14] = REG_PC | 1;
#ifdef _DEBPRO
   InsertStackAddress(REG_PC,"Branch and Exchange");
#endif
   arm.bRefillPipe = 1;
   if(!((REG_PC = GP_REG[(OPCODE_T>> 3) & 0xf]) & 1)){
       REG_PC = (REG_PC & ~3) + 4;
       CPSR &= ~T_BIT;
       SetExecFunction(0);
       advance_instruction_pipe();
   }
   else
       REG_PC = (REG_PC & ~1) + 2;
	return res;
}
//---------------------------------------------------------------------------
u8 tins_bl(void)
{
	int temp;
   u8 h,res;

#ifdef _DEBPRO
   InsertStackAddress(REG_PC,"Branch with Link");
#endif
   arm.bRefillPipe = 1;
   res = (u8)(cycP + cycS);
   h = (u8)((OPCODE_T >> 11) & 0x3);
   if(h == 3){
       temp = GP_REG[14];
       GP_REG[14] = (REG_PC - 2) | 1;
       REG_PC = (u32)temp + ((OPCODE_T & 0x7FF) << 1);
#ifdef _DEBPRO
       InsertException("Non normal Brach with Link 0x%08X to 0x%08X",GP_REG[14],REG_PC);
#endif
       if(!(REG_PC & 1)){
           REG_PC = (REG_PC & ~3) + 4;
           CPSR &= ~T_BIT;
           SetExecFunction(0);
           advance_instruction_pipe();
       }
       else
           REG_PC = (REG_PC & ~1) + 2;
       return res;
   }
   GP_REG[14] = REG_PC | 1;
   temp = ((OPCODE_T & 0x7ff) << 12);
   OPCODE_T = READHWORD(REG_PC-2);
   h = (u8)((OPCODE_T >> 11) & 0x3);
   if(((temp |= ((OPCODE_T & 0x7ff) << 1)) & 0x400000))
       temp = -(0x800000 - temp);
   REG_PC += temp + 2;
   if(h == 1){
       REG_PC = (REG_PC & ~3) + 4;
       CPSR &= ~T_BIT;
       SetExecFunction(0);
       advance_instruction_pipe();
   }
   return res;
}
//---------------------------------------------------------------------------
u8 tins_bu(void) // opcode 0x380
{
   int offset;

   if(((offset = ((OPCODE_T & 0x7ff) << 1)) & 0x800))
       offset = -(4096 - offset);
   REG_PC += offset + 2;
   arm.bRefillPipe = 1;
	return cycP;
}
//---------------------------------------------------------------------------
u8 tins_b(void) // opcode 0x340
{
   int offset;

   switch(((OPCODE_T>>8) & 0xF)){
       case 0: // beq
           if(!ZFLAG)
               return cycP;
       break;
       case 1: //BNE
           if(ZFLAG)
               return cycP;
       break;
       case 2: //BCS
           if(!CFLAG)
               return cycP;
       break;
       case 3: //BCC
           if(CFLAG)
               return cycP;
       break;
       case 4: //BMI
           if(!NFLAG)
               return cycP;
       break;
       case 5://BPL
           if(NFLAG)
               return cycP;
       break;
       case 6: //BVS
           if(!VFLAG)
               return cycP;
       break;
       case 7: //BVC
           if(VFLAG)
               return cycP;
       break;
       case 8: // BHI
           if(!(CFLAG && !ZFLAG))
               return cycP;
       break;
       case 9: //BLS
           if(!(!CFLAG || ZFLAG))
               return cycS;
       break;
       case 10: //BGE
           if(VFLAG != NFLAG)
               return cycP;
       break;
       case 11: // BLT
           if(VFLAG == NFLAG)
               return cycP;
       break;
       case 12: //BGT
           if(!(!ZFLAG && VFLAG == NFLAG))
               return cycP;
       break;
       case 13: // BLE
           if(!(ZFLAG || NFLAG != VFLAG))
               return cycP;
       break;
   }
   if(((offset = ((u8)OPCODE_T) << 1) & 0x100))
       offset = -(0x200 - offset);
   REG_PC += offset + 2;
   arm.bRefillPipe = 1;
	return cycP;
}

