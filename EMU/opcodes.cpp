#include "gbaemu.h"
#include "opcodes.h"
#include "exec.h"
#include "cpu.h"
#include "bios.h"

#define DEST_REG_INDEX         ((u16)OPCODE >> 12)
#define BASE_REG_INDEX         ((OPCODE >> 16) & 0xF)
#define DEST_REG		        GP_REG[DEST_REG_INDEX]
#define BASE_REG		        GP_REG[BASE_REG_INDEX]
#define LO_REG                 DEST_REG
#define HI_REG                 BASE_REG
#define OP_REG			        GP_REG[OPCODE & 0xF]
#define SHFT_AMO_REG	        GP_REG[(OPCODE >> 8) & 0xF]
#define IMM_SHIFT	            ((OPCODE >> 7) & 0x1F)
//---------------------------------------------------------------
u32 FASTCALL CalcAddcFlags(u32 a,u32 b)
{
   __asm{
       bt  word ptr[_arm+18],0
       adc  eax,edx
       setz byte ptr[_arm+16]
       sets byte ptr[_arm+17]
       setc byte ptr[_arm+18]
       seto byte ptr[_arm+19]
   }
   return _EAX;
}
//---------------------------------------------------------------
u32 FASTCALL CalcAddFlags(u32 a,u32 b)
{
   __asm{
       add  eax,edx
       setz byte ptr[_arm+16]
       sets byte ptr[_arm+17]
       setc byte ptr[_arm+18]
       seto byte ptr[_arm+19]
   }
   return _EAX;
}
//---------------------------------------------------------------
u32 FASTCALL CalcSubFlags(u32 a,u32 b)
{
   __asm{
       sub  eax,edx
       setz  byte ptr[_arm+16]
       sets  byte ptr[_arm+17]
       setnc byte ptr[_arm+18]
       seto  byte ptr[_arm+19]
   }
   return _EAX;
}
//---------------------------------------------------------------
void FASTCALL SET_DP_LOG_FLAGS(u32 a)
{
   __asm{
       test eax,eax
       setz byte ptr[_arm+16]
       sets byte ptr[_arm+17]
   }
}
//---------------------------------------------------------------
u32 FASTCALL DP_IMM_OPERAND(void)
{
   __asm{
       mov  ecx,dword ptr[_arm]
       movzx eax,byte ptr[_arm]
       and  ecx,0F00h
       shr  ecx,7
       ror  eax,cl
   }
   return _EAX;
}
//---------------------------------------------------------------
u32 FASTCALL DP_IMM_OPERAND_UPD(void)
{
   __asm{
       mov  ecx,dword ptr[_arm]
       movzx  eax,byte ptr[_arm]
       and  ecx,0F00h
       shr  ecx,7
       jz .noupt
       ror  eax,cl
       setc byte ptr[_arm+18]
       .noupt:
   }
   return _EAX;
}
//---------------------------------------------------------------
u32 DP_REG_OPERAND_IMM(void)
{
   u8 shift;

   switch((OPCODE & 0x60)){
       case 0: //LSL
           return OP_REG << IMM_SHIFT;
       case 0x20: // LSR
           if((shift = (u8)IMM_SHIFT) == 0)
               return 0;
           return OP_REG >> shift;
       case 0x40://asr
           if((shift = (u8)IMM_SHIFT) == 0){
               if((OP_REG & 0x80000000))
                   return 0xFFFFFFFF;
               else
                   return 0;
           }
           return (signed long)OP_REG >> shift;
       case 0x60://ROR
           if((shift = (u8)IMM_SHIFT) == 0)
               return ((OP_REG >> 1)|(CFLAG << 31));
#if defined(__BORLANDC__)
           _EAX = OP_REG;
           _CL = shift;
           __asm ror eax,cl
           return _EAX;
#else
           return ((OP_REG << (32-IMM_SHIFT))|(OP_REG>>IMM_SHIFT));
#endif
   }
   return _EAX;
}
//---------------------------------------------------------------
u32 DP_REG_OPERAND(void)
{
   u8 shift;

   switch((OPCODE & 0x60)){
       case 0: //LSL
           return OP_REG << (u8)SHFT_AMO_REG;
       case 0x20: // LSR
           if((shift = (u8)SHFT_AMO_REG) == 0)
               return OP_REG;
           else if(shift < 32)
               return OP_REG >> shift;
           else
               return 0;
       case 0x40://asr
           if((shift = (u8)SHFT_AMO_REG) == 0)
               return OP_REG;
           else if(shift < 32)
               return (signed long)OP_REG >> shift;
           else{
               if((OP_REG & 0x80000000))
                   return 0xFFFFFFFF;
               else
                   return 0;
           }
       case 0x60://ROR
#if defined(__BORLANDC__)
           if((shift = (u8)SHFT_AMO_REG) == 0)
               return OP_REG;
           else if(shift < 32){
               _EAX = OP_REG;
               _CL = shift;
               __asm ror eax,cl
               return _EAX;
           }
           else if(shift == 32)
               return OP_REG;
           else{
               shift &= 0x1F;
               _EAX = OP_REG;
               _CL = shift;
               __asm ror eax,cl
               return _EAX;
           }
#else
           if(!(shift = (u8)SHFT_AMO_REG))
               return OP_REG;
           else if(shift < 32)
               return ((OP_REG << (32-shift))|(OP_REG>>shift));
           else if(shift == 32)
               return OP_REG;
           else{
               shift &= 0x1F;
               return ((OP_REG << (32-shift))|(OP_REG>>shift));
           }
#endif
   }
   return _EAX;
}
//---------------------------------------------------------------
u32 DP_REG_OPERAND_UPD(u8 shift)
{
   u32 res;
#if defined(__BORLANDC__)
   switch((OPCODE & 0x60)){
       case 0: //LSL
           if(!shift)
               res = OP_REG;
           else if(shift < 32){
               res = OP_REG << shift;
               __asm setc byte ptr[CFLAG]
           }
           else if(shift == 32){
               res = 0;
               CFLAG = (u8)(OP_REG & 1);
           }
           else{
               res = 0;
               CFLAG = (u8)0;
           }
       break;
       case 0x20: // LSR
           if(!shift){
               if(!(OPCODE & 0x10)){
                   res = 0;
                   CFLAG = (u8)(OP_REG >> 31);
               }
               else
                   res = OP_REG;
           }
           else if(shift < 32){
               res = OP_REG >> shift;
               __asm setc byte ptr[CFLAG]
           }
           else if(shift == 32){
               res = 0;
               CFLAG = (u8)(OP_REG >> 31);
           }
           else{
               res = 0;
               CFLAG = 0;
           }
       break;
       case 0x40://asr
           if(!shift){
               if(!(OPCODE & 0x10)){
                   if((OP_REG & 0x80000000)){
                       res = 0xFFFFFFFF;
                       CFLAG = (u8)1;
                   }
                   else{
                       res = 0;
                       CFLAG = (u8)0;
                   }
               }
               else
                   res = OP_REG;
           }
           else if(shift < 32){
               res = (signed long)OP_REG >> shift;
               __asm setc byte ptr[CFLAG]
           }
           else{
               if((OP_REG & 0x80000000)){
                   res = 0xFFFFFFFF;
                   CFLAG = (u8)1;
               }
               else{
                   res = 0;
                   CFLAG = (u8)0;
               }
           }
       break;
       case 0x60://ROR
           if(!shift){
               if(!(OPCODE & 0x10)){ //RRX
                   res = ((OP_REG>>1)|(CFLAG<<31));
                   CFLAG = (u8)(OP_REG & 1);
               }
               else
                   res = OP_REG;
           }
           else if(shift < 32){
               _EAX = OP_REG;
               _CL = shift;
               __asm ror eax,cl
               __asm setc byte ptr[CFLAG]
               res = _EAX;
           }
           else if(shift == 32){
               res = OP_REG;
               CFLAG = (u8)(res >> 31);
           }
           else{
               shift &= 0x1F;
               _EAX = OP_REG;
               _CL = shift;
               __asm ror eax,cl
               __asm setc byte ptr[CFLAG]
               res = _EAX;
           }
       break;
   }
#else
   switch((OPCODE & 0x60)){
       case 0: //LSL
           if(!shift)
               res = OP_REG;
           else if(shift <32){
               res = OP_REG << shift;
               CFLAG = (u8)((OP_REG << (shift - 1)) >> 31);
           }
           else if(shift == 32){
               res = 0;
               CFLAG = (u8)(OP_REG & 1);
           }
           else{
               res = 0;
               CFLAG = (u8)0;
           }
       break;
       case 0x20: // LSR
           if(!shift){
               if(!(OPCODE & 0x10)){
                   res = 0;
                   CFLAG = (u8)(OP_REG >> 31);
               }
               else
                   res = OP_REG;
           }
           else if(shift < 32){
               res = OP_REG >> shift;
               CFLAG = (u8)((OP_REG >> (shift - 1)) & 1);
           }
           else if(shift == 32){
               res = 0;
               CFLAG = (u8)(OP_REG >> 31);
           }
           else{
               res = 0;
               CFLAG = 0;
           }
       break;
       case 0x40://asr
           if(!shift){
               if(!(OPCODE & 0x10)){
                   if((OP_REG & 0x80000000)){
                       res = 0xFFFFFFFF;
                       CFLAG = (u8)1;
                   }
                   else{
                       res = 0;
                       CFLAG = (u8)0;
                   }
               }
               else
                   res = OP_REG;
           }
           else if(shift < 32){
               res = (signed long)OP_REG >> shift;
               CFLAG = (u8)((OP_REG >> (shift - 1)) & 1);
           }
           else{
               if((OP_REG & 0x80000000)){
                   res = 0xFFFFFFFF;
                   CFLAG = (u8)1;
               }
               else{
                   res = 0;
                   CFLAG = (u8)0;
               }
           }
       break;
       case 0x60://ROR
           if(!shift){
               if(!(OPCODE & 0x10)){ //RRX
                   res = ((OP_REG>>1)|(CFLAG<<31));
                   CFLAG = (u8)(OP_REG & 1);
               }
               else
                   res = OP_REG;
           }
           else if(shift < 32){
               res = ((OP_REG << (32-shift)) | (OP_REG>>shift));
               CFLAG = (u8)(res >> 31);
           }
           else if(shift == 32){
               res = OP_REG;
               CFLAG = (u8)(OP_REG >> 31);
           }
           else{
               shift &= 0x1F;
               res = ((OP_REG << (32-shift))|(OP_REG>>shift));
               CFLAG = (u8)((OP_REG >> (shift - 1)) & 1);
           }
       break;
   }
#endif
   return res;
}
//--------------------------------------------------------------------------------------
u8 unknown_opcode(void)
{
	return 0;
}
//--------------------------------------------------------------------------------------
u8 ins_bmi(void)
{
   int adr;

   if(((adr = (OPCODE & 0xFFFFFF)) & 0x800000))
       adr = 0 - (0x1000000 - adr);
	REG_PC += (adr << 2) + 4;
   arm.bRefillPipe = 1;
   return cycP;
}
//--------------------------------------------------------------------------------------
u8 ins_blmi(void)
{
   int adr;

	GP_REG[14] = REG_PC - 4;
   if(((adr = (OPCODE&0xFFFFFF)) & 0x800000))
       adr = 0 - (0x1000000 - adr);
	REG_PC += (adr << 2) + 4;
   arm.bRefillPipe = 1;
   return cycP;
}
//--------------------------------------------------------------------------------------
u8 ins_bx(void)
{
   u32 value;
   u8 res;

	REG_PC = (value = OP_REG) & ~0x1;
   arm.bRefillPipe = 1;
   res = cycP;
	if((value & 1)) {
		CPSR |= T_BIT;
       REG_PC += 2;
       SetExecFunction(1);
		tadvance_instruction_pipe();
	}
	else
       REG_PC += 4;
	return res;
}
//--------------------------------------------------------------------------------------
u8 ins_mul(void)
{
   u8 m;
   u32 value;

	BASE_REG = OP_REG * (value = SHFT_AMO_REG);
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
//--------------------------------------------------------------------------------------
u8 ins_muls(void)
{
   u8 m;
   u32 value;

	SET_DP_LOG_FLAGS((BASE_REG = OP_REG * (value = SHFT_AMO_REG)));
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
//--------------------------------------------------------------------------------------
u8 ins_mla(void)
{
   u32 value;
   u8 m;

	BASE_REG = (u32)OP_REG * (value = (u32)SHFT_AMO_REG) + DEST_REG;
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
   return (u8)(cycP + m + 1);
}
//--------------------------------------------------------------------------------------
u8 ins_mlas(void)
{
   u32 value;
   u8 m;

	SET_DP_LOG_FLAGS(BASE_REG = OP_REG * (value = SHFT_AMO_REG) + DEST_REG);
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
//---------------------------------------------------------------
u8 ins_mull(void)
{
   u32 op1,op2,value;
   u8 m;

   op1 = OP_REG;
   value = op2 = SHFT_AMO_REG;
#if defined(__BORLANDC__)
   __asm{
       mov eax,op1
       imul dword ptr[op2]
       mov dword ptr[op1],eax
       mov dword ptr[op2],edx
   }
#endif
	LO_REG = (u32)op1;
	HI_REG = (u32)op2;
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
//---------------------------------------------------------------
u8 ins_mulls(void)
{
   u32 op1,op2,value;
   u8 m;

   op1 = OP_REG;
   value = op2 = SHFT_AMO_REG;
#if defined(__BORLANDC__)
   __asm{
       mov eax,op1
       imul dword ptr[op2]
       mov dword ptr[op1],eax
       mov dword ptr[op2],edx
   }
#endif
	LO_REG = (u32)op1;
	HI_REG = (u32)op2;
	if(!op1 && !op2)
       ZFLAG = 1;
   else
       ZFLAG = 0;
   NFLAG = (u8)(op2 >> 31);
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
//---------------------------------------------------------------
u8 ins_mull_unsigned(void)
{
   u32 op1,op2,value,m;

   op1 = OP_REG;
   value = op2 = SHFT_AMO_REG;
#if defined(__BORLANDC__)
   __asm{
       mov eax,op1
       mul dword ptr[op2]
       mov dword ptr[op1],eax
       mov dword ptr[op2],edx
   }
#endif
	LO_REG = op1;
	HI_REG = op2;
   value >>= 8;
   if(value == 0)
       m = 1;
   else{
       value >>= 8;
       if(value == 0)
           m = 2;
       else{
           value >>= 8;
           if(value == 0)
               m = 3;
           else
               m = 4;
       }
   }
   return (u8)(cycP + m + 1);
}
//---------------------------------------------------------------
u8 ins_mulls_unsigned(void)
{
   u32 op1,op2,value;
   u8 m;

   op1 = OP_REG;
   value = op2 = SHFT_AMO_REG;
#if defined(__BORLANDC__)
   __asm{
       mov eax,op1
       mul dword ptr[op2]
       mov dword ptr[op1],eax
       mov dword ptr[op2],edx
   }
#endif
	LO_REG = (u32)op1;
	HI_REG = (u32)op2;
	if(!op1 && !op2)
       ZFLAG = 1;
   else
       ZFLAG = 0;
   NFLAG = (u8)(op2 >> 31);
   value >>= 8;
   if(value == 0)
       m = 1;
   else{
       value >>= 8;
       if(value == 0)
           m = 2;
       else{
           value >>= 8;
           if(value == 0)
               m = 3;
           else
               m = 4;
       }
   }
   return (u8)(cycP + m);
}
//---------------------------------------------------------------
u8 ins_mlal(void)
{
   s32 op1,op2,d_reg,b_reg;
   u32 value;
   u8 m;

   op1 = OP_REG;
   value = (u32)(op2 = SHFT_AMO_REG);
   d_reg = DEST_REG;
   b_reg = BASE_REG;
#if defined(__BORLANDC__)
   __asm{
       mov eax,op1
       imul dword ptr[op2]
       add d_reg,eax
       adc b_reg,edx
   }
#endif
	LO_REG = (u32)d_reg;
	HI_REG = (u32)b_reg;
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
   return (u8)(cycP + m + 1);
}
//---------------------------------------------------------------
u8 ins_mlals(void)
{
   u32 op1,op2,d_reg,b_reg,value;
   u8 m;

   op1 = OP_REG;
   value = op2 = SHFT_AMO_REG;
   d_reg = DEST_REG;
   b_reg = BASE_REG;
#if defined(__BORLANDC__)
   __asm{
       mov eax,op1
       imul dword ptr[op2]
       add d_reg,eax
       adc b_reg,edx
   }
#endif
	LO_REG = (u32)d_reg;
	HI_REG = (u32)b_reg;
	if(!d_reg && !b_reg)
       ZFLAG = 1;
   else
       ZFLAG = 0;
   NFLAG = (u8)(b_reg >> 31);
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
   return (u8)(cycP + 1 + m);
}
//---------------------------------------------------------------
u8 ins_mlal_unsigned(void)
{
   u32 op1,op2,d_reg,b_reg,value;
   u8 m;

   op1 = OP_REG;
   value = op2 = SHFT_AMO_REG;
   d_reg = DEST_REG;
   b_reg = BASE_REG;
#if defined(__BORLANDC__)
   __asm{
       mov eax,op1
       mul dword ptr[op2]
       add d_reg,eax
       adc b_reg,edx
   }
#endif
	LO_REG = (u32)d_reg;
	HI_REG = (u32)b_reg;
   value >>= 8;
   if(value == 0)
       m = 1;
   else{
       value >>= 8;
       if(value == 0)
           m = 2;
       else{
           value >>= 8;
           if(value == 0)
               m = 3;
           else
               m = 4;
       }
   }
   return (u8)(cycP + 1 + m);
}
//---------------------------------------------------------------
u8 ins_mlals_unsigned(void)
{
   u32 op1,op2,d_reg,b_reg,value;
   u8 m;

   op1 = OP_REG;
   value = op2 = SHFT_AMO_REG;
   d_reg = DEST_REG;
   b_reg = BASE_REG;
#if defined(__BORLANDC__)
   __asm{
       mov eax,op1
       mul dword ptr[op2]
       add d_reg,eax
       adc b_reg,edx
   }
#endif
	LO_REG = (u32)d_reg;
	HI_REG = (u32)b_reg;
	if(!d_reg && !b_reg)
       ZFLAG = 1;
   else
       ZFLAG = 0;
   NFLAG = (u8)(b_reg >> 31);
   value >>= 8;
   if(value == 0)
       m = 1;
   else{
       value >>= 8;
       if(value == 0)
           m = 2;
       else{
           value >>= 8;
           if(value == 0)
               m = 3;
           else
               m = 4;
       }
   }
   return (u8)(cycP + 1 + m);
}
//---------------------------------------------------------------------
u8 ins_mrs_cpsr(void)
{
   u32 r;

   r = (CPSR & 0xFFFFFF);
   if(VFLAG)
       r |= V_BIT;
   if(CFLAG)
       r |= C_BIT;
   if(ZFLAG)
       r |= Z_BIT;
   if(NFLAG)
       r |= N_BIT;
	DEST_REG = r;
   return cycP;
}
//-----------------------------------------------------------------------
u8 ins_msr_cpsr(void)
{
   u32 value;
   u8 field,thb;

   EnterCriticalSection(&crSection);
   if((OPCODE & 0x2000000))
       value = DP_IMM_OPERAND();
   else
       value = OP_REG;
   field = (u8)((OPCODE >> 16) & 0xF);
   if((CPSR & 0x1F) != USER_MODE){
       if((field & 1)){
           thb = (u8)(CPSR & T_BIT);
           SwitchCpuMode(((u16)(value & 0x1F)));
           CPSR = (CPSR & 0xFFFFFF3F) | (value & 0x000000E0);
           SetExecFunction((u8)((CPSR & T_BIT) >> 5));
           if(thb != (CPSR & T_BIT)){
               if((CPSR & T_BIT)){
                   REG_PC += 2;
                   tadvance_instruction_pipe();
               }
               else{
                   REG_PC += 4;
                   advance_instruction_pipe();
               }
           }
       }
       if((field & 2))
           CPSR = (CPSR & 0xFFFF00FF) | (value & 0x0000FF00);
       if((field & 4))
           CPSR = (CPSR & 0xFF00FFFF) | (value & 0x00FF0000);
   }
   if((field & 8)){
       if((value & V_BIT))
           VFLAG = 1;
       else
           VFLAG = 0;
       if((value & C_BIT))
           CFLAG = 1;
       else
           CFLAG = 0;
       if((value & Z_BIT))
           ZFLAG = 1;
       else
           ZFLAG = 0;
       if((value & N_BIT))
           NFLAG = 1;
       else
           NFLAG = 0;
   }
   LeaveCriticalSection(&crSection);
   return cycP;
}
//-----------------------------------------------------------------------
u8 ins_mrs_spsr(void)
{
	DEST_REG = GP_REG[16];
   return cycP;
}
//-----------------------------------------------------------------------
u8 ins_msr_spsr(void)
{
   u32 value;
   u8 field;

   if((OPCODE & 0x2000000))
       value = DP_IMM_OPERAND();
   else
       value = OP_REG;
   field = (u8)((OPCODE >> 16) & 0xF);
   if((field & 8))
       GP_REG[16] = (GP_REG[16] & ~0xFF000000) | (0xFF000000 & value);
   if((field & 4))
       GP_REG[16] = (GP_REG[16] & ~0x00FF0000) | (0x00FF0000 & value);
   if((field & 2))
       GP_REG[16] = (GP_REG[16] & ~0x0000FF00) | (0x0000FF00 & value);
   if((field & 1))
       GP_REG[16] = (GP_REG[16] & ~0x000000FF) | ((u8)value);
   return cycP;
}
//-----------------------------------------------------------------------
u8 ins_swi (void)
{
   bios((u8)(OPCODE >> 16));
	return cycP;
}
//-----------------------------------------------------------------------
u8 ins_swp (void)
{
	u32 temp;

	temp = READWORD(BASE_REG);
	WRITEWORD(BASE_REG, OP_REG);
	DEST_REG = temp;
   return (u8)(cycI + cycS + cycN + cycN);
}
//-----------------------------------------------------------------------
u8 ins_swpb(void)
{
	u8 temp;

	temp = READBYTE (BASE_REG);
	WRITEBYTE(BASE_REG,OP_REG);
	DEST_REG = temp;
   return (u8)(cycI + cycS + cycN + cycN);
}
//-----------------------------------------------------------------------
//LDRSB
//-----------------------------------------------------------------------
u8 ldrsb_preup(void)
{
   DEST_REG = (u32)((s8)READBYTE(BASE_REG + OP_REG));
   return LDR_RET;
}
//-----------------------------------------------------------------------
u8 ldrsb_predown(void)
{
   DEST_REG = (u32)((s8)READBYTE(BASE_REG - OP_REG));
   return LDR_RET;
}
//-----------------------------------------------------------------------
u8 ldrsb_preupwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) + OP_REG;
   DEST_REG = (u32)((s8)READBYTE(base));
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrsb_predownwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - OP_REG;
   DEST_REG = (u32)((s8)READBYTE(base));
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrsb_postup(void)
{
   u32 *regs;

   DEST_REG = (u32)((s8)READBYTE(*(regs = &BASE_REG)));
   *regs += OP_REG;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrsb_postdown(void)
{
   u32 *regs;

   DEST_REG = (u32)((s8)READBYTE(*(regs = &BASE_REG)));
   *regs -= OP_REG;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrsb_preupimm(void)
{
   DEST_REG = (u32)((s8)READBYTE(BASE_REG + (((OPCODE&0xF00)>>4)|(OPCODE&0xF))));
   return LDR_RET;
}
//-----------------------------------------------------------------------
u8 ldrsb_predownimm(void)
{
   DEST_REG = (u32)((s8)READBYTE(BASE_REG - (((OPCODE&0xF00)>>4)|(OPCODE&0xF))));
   return LDR_RET;
}
//-----------------------------------------------------------------------
u8 ldrsb_preupimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) + (((OPCODE&0xF00)>>4)|(OPCODE&0xF));
   DEST_REG = (u32)((s8)READBYTE(base));
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrsb_predownimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - (((OPCODE&0xF00)>>4)|(OPCODE&0xF));
   DEST_REG = (u32)((s8)READBYTE(base));
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrsb_postupimm(void)
{
   u32 *regs;

   DEST_REG = (u32)((s8)READBYTE(*(regs = &BASE_REG)));
   *regs += ((OPCODE&0xF00)>>4)|(OPCODE&0xF);
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrsb_postdownimm(void)
{
   u32 *regs;

   DEST_REG = (u32)((s8)READBYTE(*(regs = &BASE_REG)));
   *regs -= ((OPCODE&0xF00)>>4)|(OPCODE&0xF);
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
//LDRSH
//-----------------------------------------------------------------------
u8 ldrsh_preup(void)
{
   DEST_REG = (u32)((s16)READHWORD(BASE_REG + OP_REG));
   return LDR_RET;
}
//-----------------------------------------------------------------------
u8 ldrsh_predown(void)
{
   DEST_REG = (u32)((s16)READHWORD(BASE_REG - OP_REG));
   return LDR_RET;
}
//-----------------------------------------------------------------------
u8 ldrsh_preupwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) + OP_REG;
   DEST_REG = (u32)((s16)READHWORD(base));
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrsh_predownwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - OP_REG;
   DEST_REG = (u32)((s16)READHWORD(base));
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrsh_postup(void)
{
   u32 *regs;

   DEST_REG = (u32)((s16)READHWORD(*(regs = &BASE_REG)));
   *regs += OP_REG;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrsh_postdown(void)
{
   u32 *regs;

   DEST_REG = (u32)((s16)READHWORD(*(regs = &BASE_REG)));
   *regs -= OP_REG;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrsh_preupimm(void)
{
   DEST_REG = (u32)((s16)READHWORD(BASE_REG + (((OPCODE&0xF00)>>4)|(OPCODE&0xF))));
   return LDR_RET;
}
//-----------------------------------------------------------------------
u8 ldrsh_predownimm(void)
{
   DEST_REG = (u32)((s16)READHWORD(BASE_REG - (((OPCODE&0xF00)>>4)|(OPCODE&0xF))));
   return LDR_RET;
}
//-----------------------------------------------------------------------
u8 ldrsh_preupimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) + (((OPCODE&0xF00)>>4)|(OPCODE&0xF));
   DEST_REG = (u32)((s16)READHWORD(base));
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrsh_predownimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - (((OPCODE&0xF00)>>4)|(OPCODE&0xF));
   DEST_REG = (u32)((s16)READHWORD(base));
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrsh_postupimm(void)
{
   u32 *regs;

   DEST_REG = (u32)((s16)READHWORD(*(regs = &BASE_REG)));
   *regs += ((OPCODE&0xF00)>>4)|(OPCODE&0xF);
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrsh_postdownimm(void)
{
   u32 *regs;

   DEST_REG = (u32)((s16)READHWORD(*(regs = &BASE_REG)));
   *regs -= ((OPCODE&0xF00)>>4)|(OPCODE&0xF);
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
//STM
//-----------------------------------------------------------------------
u8 stm(void) //ldm-stm
{
   u32 *regs,base,stack,reg[16],*p2,*p3;
   u8 r,i;
   u16 p1;

   stack = *(regs = &(p3 = GP_REG)[(OPCODE >> 16) & 0xF]) & ~3;
   p2 = reg;
   for(p1 = 1;p1 > 0;p1 <<= 1,p3++){
       if((OPCODE & p1))
           *p2++ = *p3;
   }
   r = (u8)(p2 - reg);
   if((OPCODE & 0x800000)){
       base = stack + (r << 2);
       if((OPCODE & 0x1000000))
           stack += 4;
   }
   else{
       stack = (base = stack - (r << 2));
       if(!(OPCODE & 0x1000000))
           stack += 4;
   }
   if((OPCODE & 0x200000))
       *regs = base;
   p2 = reg;
   for(i = r,r=0;i > 0;i--){
       WRITEWORD(stack,*p2++);
       stack += 4;
       r += cycS;
   }
   return (u8)((r - cycS) + cycN + cycPN);
}
//-----------------------------------------------------------------------
//LDM
//-----------------------------------------------------------------------
u8 ldm(void) //ldm-stm
{
   u32 *regs,base,stack,reg[16],*p2,*p3;
   u8 r,i;
   u16 p1;

   stack = *(regs = &(p3 = GP_REG)[(OPCODE >> 16) & 0xF]) & ~3;
   p2 = reg;
   for(p1=1;p1 > 0;p1 <<= 1,p3++){
       if((OPCODE & p1))
           *p2++ = (u32)p3;
   }
   r = (u8)(p2 - reg);
   if((OPCODE & 0x800000)){
       base = stack + (r << 2);
       if((OPCODE & 0x1000000))
           stack += 4;
   }
   else{
       stack = (base = stack - (r << 2));
       if(!(OPCODE & 0x1000000))
           stack += 4;
   }
   if((OPCODE & 0x200000))
       *regs = base;
   for(p2 = reg,i = r,r=0;i > 0;i--){
       *((u32 *)(*p2++)) = READWORD(stack);
       stack += 4;
       r += cycS;
   }
   if((OPCODE & 0x8000)){
       REG_PC += 4;
       arm.bRefillPipe = 1;
   }
   return (u8)(cycI + r + cycN);
}
//-----------------------------------------------------------------------
//LDRH
//-----------------------------------------------------------------------
u8 ldrh_preup(void)
{
   DEST_REG = READHWORD(BASE_REG + OP_REG);
   return LDR_RET;
}
//-----------------------------------------------------------------------
u8 ldrh_predown(void)
{
   DEST_REG = READHWORD(BASE_REG - OP_REG);
   return LDR_RET;
}
//-----------------------------------------------------------------------
u8 ldrh_preupwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) + OP_REG;
   DEST_REG = (u32)READHWORD(base);
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrh_predownwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - OP_REG;
   DEST_REG = READHWORD(base);
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrh_postup(void)
{
   u32 *regs;

   DEST_REG = READHWORD(*(regs = &BASE_REG));
   *regs += OP_REG;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrh_postdown(void)
{
   u32 *regs;

   DEST_REG = READHWORD(*(regs = &BASE_REG));
   *regs -= OP_REG;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrh_preupimm(void)
{
   DEST_REG = READHWORD(BASE_REG + (((OPCODE&0xF00)>>4)|(OPCODE&0xF)));
   return LDR_RET;
}
//-----------------------------------------------------------------------
u8 ldrh_predownimm(void)
{
   DEST_REG = READHWORD(BASE_REG - (((OPCODE&0xF00)>>4)|(OPCODE&0xF)));
   return LDR_RET;
}
//-----------------------------------------------------------------------
u8 ldrh_preupimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) + (((OPCODE&0xF00)>>4)|(OPCODE&0xF));
   DEST_REG = READHWORD(base);
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrh_predownimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - (((OPCODE&0xF00)>>4)|(OPCODE&0xF));
   DEST_REG = READHWORD(base);
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrh_postupimm(void)
{
   u32 *regs;

   DEST_REG = READHWORD(*(regs = &BASE_REG));
   *regs += ((OPCODE&0xF00)>>4)|(OPCODE&0xF);
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrh_postdownimm(void)
{
   u32 *regs;

   DEST_REG = READHWORD(*(regs = &BASE_REG));
   *regs -= ((OPCODE&0xF00)>>4)|(OPCODE&0xF);
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
//STRH
//-----------------------------------------------------------------------
u8 strh_preup(void)
{
   WRITEHWORD(BASE_REG + OP_REG,(u16)DEST_REG);
   return STR_RET;
}
//-----------------------------------------------------------------------
u8 strh_predown(void)
{
   WRITEHWORD(BASE_REG - OP_REG,(u16)DEST_REG);
   return STR_RET;
}
//-----------------------------------------------------------------------
u8 strh_preupwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) + OP_REG;
   WRITEHWORD(base,(u16)DEST_REG);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
u8 strh_predownwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - OP_REG;
   WRITEHWORD(base,(u16)DEST_REG);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
u8 strh_postup(void)
{
   u32 *regs;

   WRITEHWORD(*(regs = &BASE_REG),(u16)DEST_REG);
   *regs += OP_REG;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
u8 strh_postdown(void)
{
   u32 *regs;

   WRITEHWORD(*(regs = &BASE_REG),(u16)DEST_REG);
   *regs -= OP_REG;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
u8 strh_preupimm(void)
{
   WRITEHWORD(BASE_REG + (((OPCODE&0xF00)>>4)|(OPCODE&0xF)),(u16)DEST_REG);
   return STR_RET;
}
//-----------------------------------------------------------------------
u8 strh_predownimm(void)
{
   WRITEHWORD(BASE_REG - (((OPCODE&0xF00)>>4)|(OPCODE&0xF)),(u16)DEST_REG);
   return STR_RET;
}
//-----------------------------------------------------------------------
u8 strh_preupimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) + (((OPCODE&0xF00)>>4)|(OPCODE&0xF));
   WRITEHWORD(base,(u16)DEST_REG);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
u8 strh_predownimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - (((OPCODE&0xF00)>>4)|(OPCODE&0xF));
   WRITEHWORD(base,(u16)DEST_REG);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
u8 strh_postupimm(void)
{
   u32 *regs;

   WRITEHWORD(*(regs = &BASE_REG),(u16)DEST_REG);
   *regs += ((OPCODE&0xF00)>>4)|(OPCODE&0xF);
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
u8 strh_postdownimm(void)
{
   u32 *regs;

   WRITEHWORD(*(regs = &BASE_REG),(u16)DEST_REG);
   *regs -= ((OPCODE&0xF00)>>4)|(OPCODE&0xF);
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
//STRB
//-----------------------------------------------------------------------
u8 strb_postup(void)
{
   u32 *regs;

   regs = &BASE_REG;
   WRITEBYTE(*regs,(u8)DEST_REG);
   *regs += DP_REG_OPERAND_IMM();
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
u8 strb_postdown(void)
{
   u32 *regs;

   regs = &BASE_REG;
   WRITEBYTE(*regs,(u8)DEST_REG);
   *regs -= DP_REG_OPERAND_IMM();
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
u8 strb_preup(void)
{
   WRITEBYTE(BASE_REG + DP_REG_OPERAND_IMM(),(u8)DEST_REG);
   return STR_RET;
}
//-----------------------------------------------------------------------
u8 strb_predown(void)
{
   WRITEBYTE(BASE_REG - DP_REG_OPERAND_IMM(),(u8)DEST_REG);
   return STR_RET;
}
//-----------------------------------------------------------------------
u8 strb_preupwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) + DP_REG_OPERAND_IMM();
   WRITEBYTE(base,(u8)DEST_REG);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
u8 strb_predownwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - DP_REG_OPERAND_IMM();
   WRITEBYTE(base,(u8)DEST_REG);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
u8 strb_postupimm(void)
{
   u32 *regs;

   WRITEBYTE(*(regs = &BASE_REG),(u8)DEST_REG);
   *regs += OPCODE & 0xFFF;
   return STRBASE_RET;//lino
}
//-----------------------------------------------------------------------
u8 strb_postdownimm(void)
{
   u32 *regs;

   regs = &BASE_REG;
   WRITEBYTE(*regs,(u8)DEST_REG);
   *regs -= OPCODE & 0xFFF;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
u8 strb_preupimm(void)
{
   WRITEBYTE(BASE_REG + (OPCODE & 0xFFF),(u8)DEST_REG);
   return STR_RET;
}
//-----------------------------------------------------------------------
u8 strb_predownimm(void)
{
   WRITEBYTE(BASE_REG - (OPCODE & 0xFFF),(u8)DEST_REG);
   return STR_RET;
}
//-----------------------------------------------------------------------
u8 strb_preupimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) + (OPCODE & 0xFFF);
   WRITEBYTE(base,(u8)DEST_REG);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
u8 strb_predownimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - (OPCODE & 0xFFF);
   WRITEBYTE(base,(u8)DEST_REG);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
//LDRB
//-----------------------------------------------------------------------
u8 ldrb_postup(void)
{
   u32 *regs;

   DEST_REG = READBYTE(*(regs = &BASE_REG));
   *regs += DP_REG_OPERAND_IMM();
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrb_postdown(void)
{
   u32 *regs;

   DEST_REG = READBYTE(*(regs = &BASE_REG));
   *regs -= DP_REG_OPERAND_IMM();
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrb_preup(void)
{
   DEST_REG = READBYTE(BASE_REG + DP_REG_OPERAND_IMM());
   return LDR_RET;
}
//-----------------------------------------------------------------------
u8 ldrb_predown(void)
{
   DEST_REG = READBYTE(BASE_REG - DP_REG_OPERAND_IMM());
   return LDR_RET;
}
//-----------------------------------------------------------------------
u8 ldrb_preupwb(void)
{
   u32 base,*regs;

   DEST_REG = READBYTE(base = *(regs = &BASE_REG) + DP_REG_OPERAND_IMM());
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrb_predownwb(void)
{
   u32 base,*regs;

   DEST_REG = READBYTE(base = *(regs = &BASE_REG) - DP_REG_OPERAND_IMM());
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrb_postupimm(void)
{
   u32 *regs;

   DEST_REG = READBYTE(*(regs = &BASE_REG));
   *regs += OPCODE & 0xFFF;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrb_postdownimm(void)
{
   u32 *regs;

   DEST_REG = READBYTE(*(regs = &BASE_REG));
   *regs -= OPCODE & 0xFFF;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrb_preupimm(void)
{
   DEST_REG = READBYTE(BASE_REG + (OPCODE & 0xFFF));
   return LDR_RET;
}
//-----------------------------------------------------------------------
u8 ldrb_predownimm(void)
{
   DEST_REG = READBYTE(BASE_REG - (OPCODE & 0xFFF));
   return LDR_RET;
}
//-----------------------------------------------------------------------
u8 ldrb_preupimmwb(void)
{
   u32 base,*regs;

   DEST_REG = READBYTE(base = *(regs = &BASE_REG) + (OPCODE & 0xFFF));
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
u8 ldrb_predownimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - (OPCODE & 0xFFF);
   DEST_REG = READBYTE(base);
   *regs = base;
   return LDRBASE_RET;
}
//-----------------------------------------------------------------------
//STR
//-----------------------------------------------------------------------
u8 str_postup(void)
{
   u32 *regs;
   u8 rd;

   regs = &BASE_REG;
   if((rd = (u8)DEST_REG_INDEX) != 15)
       WRITEWORD(*regs,GP_REG[rd]);
   else
       WRITEWORD(*regs,REG_PC + 4);
   *regs += DP_REG_OPERAND_IMM();
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
u8 str_postdown(void)
{
   u32 *regs;
   u8 rd;

   regs = &BASE_REG;
   if((rd = (u8)DEST_REG_INDEX) != 15)
       WRITEWORD(*regs,GP_REG[rd]);
   else
       WRITEWORD(*regs,REG_PC+4);
   *regs -= DP_REG_OPERAND_IMM();
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
u8 str_preup(void)
{
   u8 rd;

   if((rd = (u8)DEST_REG_INDEX) != 15)
       WRITEWORD(BASE_REG + DP_REG_OPERAND_IMM(),GP_REG[rd]);
   else
       WRITEWORD(BASE_REG + DP_REG_OPERAND_IMM(),REG_PC+4);
   return STR_RET;
}
//-----------------------------------------------------------------------
u8 str_predown(void)
{
   u8 rd;

   if((rd = (u8)DEST_REG_INDEX) != 15)
       WRITEWORD(BASE_REG - DP_REG_OPERAND_IMM(),GP_REG[rd]);
   else
       WRITEWORD(BASE_REG - DP_REG_OPERAND_IMM(),REG_PC+4);
   return STR_RET;
}
//-----------------------------------------------------------------------
u8 str_preupwb(void)
{
   u32 base,*regs;
   u8 rd;

   base = *(regs = &BASE_REG) + DP_REG_OPERAND_IMM();
   if((rd = (u8)DEST_REG_INDEX) != 15)
       WRITEWORD(base,GP_REG[rd]);
   else
       WRITEWORD(base,REG_PC+4);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
u8 str_predownwb(void)
{
   u32 base,*regs;
   u8 rd;

   base = *(regs = &BASE_REG) - DP_REG_OPERAND_IMM();
   if((rd = (u8)DEST_REG_INDEX) != 15)
       WRITEWORD(base,GP_REG[rd]);
   else
       WRITEWORD(base,REG_PC+4);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
u8 str_postupimm(void)
{
   u32 *regs;
   u8 rd;                        

   regs = &BASE_REG;
   if((rd = (u8)DEST_REG_INDEX) != 15)
       WRITEWORD(*regs,GP_REG[rd]);
   else
       WRITEWORD(*regs,REG_PC + 4);
   *regs += OPCODE & 0xFFF;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
u8 str_postdownimm(void)
{
   u32 *regs;
   u8 rd;

   regs = &BASE_REG;
   if((rd = (u8)DEST_REG_INDEX) == 15)
       WRITEWORD(*regs,REG_PC + 4);
   else
       WRITEWORD(*regs,GP_REG[rd]);
   *regs -= OPCODE & 0xFFF;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
u8 str_preupimm(void)
{
   u8 rd;

   if((rd = (u8)DEST_REG_INDEX) != 15)
       WRITEWORD(BASE_REG + (OPCODE & 0xFFF),GP_REG[rd]);
   else
       WRITEWORD(BASE_REG + (OPCODE & 0xFFF),REG_PC+4);
   return STR_RET;
}
//-----------------------------------------------------------------------
u8 str_predownimm(void)
{
   u8 rd;

   if(((rd = (u8)DEST_REG_INDEX)) != 15)
       WRITEWORD(BASE_REG - (OPCODE & 0xFFF),GP_REG[rd]);
   else
       WRITEWORD(BASE_REG - (OPCODE & 0xFFF),REG_PC+4);
   return STR_RET;
}
//-----------------------------------------------------------------------
u8 str_preupimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) + (OPCODE & 0xFFF);
   WRITEWORD(base,GP_REG[DEST_REG_INDEX]);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
u8 str_predownimmwb(void)
{
   u32 base,*regs;

   base = *(regs = &BASE_REG) - (OPCODE & 0xFFF);
   WRITEWORD(base,GP_REG[DEST_REG_INDEX]);
   *regs = base;
   return STRBASE_RET;
}
//-----------------------------------------------------------------------
//LDR
//-----------------------------------------------------------------------
u8 ldr_postup(void)
{
   u32 *regs;
   u8 rd;

   regs = &BASE_REG;
   GP_REG[(rd = (u8)DEST_REG_INDEX)] = READWORD(*regs);
   *regs += DP_REG_OPERAND_IMM();
   if(rd != 15) return LDRBASE_RET;
   REG_PC += 4;
   arm.bRefillPipe = 1;
   return LDRBASEPC_RET;
}
//-----------------------------------------------------------------------
u8 ldr_postdown(void)
{
   u32 *regs;
   u8 rd;

   regs = &BASE_REG;
   GP_REG[(rd = (u8)DEST_REG_INDEX)] = READWORD(*regs);
   *regs -= DP_REG_OPERAND_IMM();
   if(rd != 15) return LDRBASE_RET;
   REG_PC += 4;
   arm.bRefillPipe = 1;   
   return LDRBASEPC_RET;
}
//-----------------------------------------------------------------------
u8 ldr_preupwb(void)
{
   u32 base,*regs;
   u8 rd;

   base = *(regs = &BASE_REG) + DP_REG_OPERAND_IMM();
   GP_REG[(rd = (u8)DEST_REG_INDEX)] = READWORD(base);
   *regs = base;
   if(rd != 15)
       return LDRBASE_RET;
   arm.bRefillPipe = 1;
   REG_PC += 4;
   return LDRBASEPC_RET;
}
//-----------------------------------------------------------------------
u8 ldr_predown(void)
{
   u8 rd;

   GP_REG[(rd = (u8)DEST_REG_INDEX)] = READWORD(BASE_REG - DP_REG_OPERAND_IMM());
   if(rd != 15)
       return LDR_RET;
   arm.bRefillPipe = 1;
   REG_PC += 4;
   return LDRPC_RET;
}
//-----------------------------------------------------------------------
u8 ldr_preup(void)
{
   u8 rd;

   GP_REG[(rd = (u8)DEST_REG_INDEX)] = READWORD(BASE_REG + DP_REG_OPERAND_IMM());
   if(rd != 15)
       return LDR_RET;
   arm.bRefillPipe = 1;
   REG_PC += 4;
   return LDRPC_RET;
}
//-----------------------------------------------------------------------
u8 ldr_predownwb(void)
{
   u32 base,*regs;
   u8 rd;

   base = *(regs = &BASE_REG) - DP_REG_OPERAND_IMM();
   GP_REG[(rd = (u8)DEST_REG_INDEX)] = READWORD(base);
   *regs = base;
   if(rd != 15)
       return LDRBASE_RET;
   arm.bRefillPipe = 1;
   REG_PC += 4;
   return LDRBASEPC_RET;
}
//-----------------------------------------------------------------------
u8 ldr_postupimm(void)
{
   u32 *regs;
   u8 rd;

   regs = &BASE_REG;
   GP_REG[(rd = (u8)DEST_REG_INDEX)] = READWORD(*regs);
   *regs += OPCODE & 0xFFF;
   if(rd != 15) return LDRBASE_RET;
   arm.bRefillPipe = 1;
   REG_PC += 4;
   return LDRBASEPC_RET;
}
//-----------------------------------------------------------------------
u8 ldr_postdownimm(void)
{
   u32 *regs;
   u8 rd;

   regs = &BASE_REG;
   GP_REG[(rd = (u8)DEST_REG_INDEX)] = READWORD(*regs);
   *regs -= OPCODE & 0xFFF;
   if(rd != 15) return LDRBASE_RET;
   arm.bRefillPipe = 1;
   REG_PC += 4;
   return LDRBASEPC_RET;
}
//-----------------------------------------------------------------------
u8 ldr_preupimm(void)
{
   u8 rd;

   GP_REG[(rd = (u8)DEST_REG_INDEX)] = READWORD(BASE_REG + (OPCODE & 0xFFF));
   if(rd != 15) return LDR_RET;
   arm.bRefillPipe = 1;
   REG_PC += 4;
   return LDRPC_RET;
}
//-----------------------------------------------------------------------
u8 ldr_predownimm(void)
{
   u8 rd;

   GP_REG[(rd = (u8)DEST_REG_INDEX)] = READWORD(BASE_REG - (OPCODE & 0xFFF));
   if(rd != 15) return LDR_RET;
   arm.bRefillPipe = 1;
   REG_PC += 4;
   return LDRPC_RET;
}
//-----------------------------------------------------------------------
u8 ldr_preupimmwb(void)
{
   u32 *regs,base;
   u8 rd;

   GP_REG[(rd = (u8)DEST_REG_INDEX)] = READWORD((base = *(regs = &BASE_REG) + (OPCODE & 0xFFF)));
   *regs = base;
   if(rd != 15)
       return LDRBASE_RET;
   arm.bRefillPipe = 1;
   REG_PC += 4;
   return LDRBASEPC_RET;
}
//-----------------------------------------------------------------------
u8 ldr_predownimmwb(void)
{
   u32 base,*regs;
   u8 rd;

   GP_REG[(rd = (u8)DEST_REG_INDEX)] = READWORD((base = *(regs = &BASE_REG) - (OPCODE & 0xFFF)));
   *regs = base;
   if(rd != 15)
       return LDRBASE_RET;
   arm.bRefillPipe = 1;
   REG_PC += 4;
   return LDRBASEPC_RET;
}
//-----------------------------------------------------------------------
// TEQ
//-----------------------------------------------------------------------
u8 teq_imm(void)
{
   SET_DP_LOG_FLAGS(BASE_REG ^ DP_IMM_OPERAND_UPD());
   return cycP;
}
//-----------------------------------------------------------------------
u8 teq(void)
{
   SET_DP_LOG_FLAGS(BASE_REG ^ DP_REG_OPERAND_UPD((u8)IMM_SHIFT));
   return cycP;
}
//-----------------------------------------------------------------------
u8 teq_reg(void)
{
   SET_DP_LOG_FLAGS(BASE_REG ^ DP_REG_OPERAND_UPD((u8)SHFT_AMO_REG));
   return (u8)(cycI + cycP);
}
//-----------------------------------------------------------------------
//TST
//-----------------------------------------------------------------------
u8 tst_imm(void)
{
   SET_DP_LOG_FLAGS(BASE_REG & DP_IMM_OPERAND_UPD());
   return cycP;
}
//-----------------------------------------------------------------------
u8 tst(void)
{
   SET_DP_LOG_FLAGS(BASE_REG & DP_REG_OPERAND_UPD((u8)IMM_SHIFT));
   return cycP;
}
//-----------------------------------------------------------------------
u8 tst_reg(void)
{
   SET_DP_LOG_FLAGS(BASE_REG & DP_REG_OPERAND_UPD((u8)SHFT_AMO_REG));
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
//CMN
//-----------------------------------------------------------------------
u8 cmn_imm(void)
{
   CalcAddFlags(BASE_REG,DP_IMM_OPERAND());
   return cycP;
}
//-----------------------------------------------------------------------
u8 cmn(void)
{
   CalcAddFlags(BASE_REG,DP_REG_OPERAND_IMM());
   return cycP;
}
//-----------------------------------------------------------------------
u8 cmn_reg(void)
{
   CalcAddFlags(BASE_REG,DP_REG_OPERAND());
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
//CMP
//-----------------------------------------------------------------------
u8 cmp_imm(void)
{
   CalcSubFlags(BASE_REG,DP_IMM_OPERAND());
   return cycP;
}
//-----------------------------------------------------------------------
u8 cmp_reg(void)
{
   CalcSubFlags(BASE_REG,DP_REG_OPERAND());
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
u8 cmp(void)
{
   CalcSubFlags(BASE_REG,DP_REG_OPERAND_IMM());
   return cycP;
}
//-----------------------------------------------------------------------
//ADC
//-----------------------------------------------------------------------
u8 adc(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG + DP_REG_OPERAND_IMM() + CFLAG;
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 adc_reg(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG + DP_REG_OPERAND() + CFLAG;
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
u8 adc_imm(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG + DP_IMM_OPERAND() + CFLAG;
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 adcs(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = CalcAddcFlags(BASE_REG,DP_REG_OPERAND_IMM());
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 adcs_reg(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = CalcAddcFlags(BASE_REG,DP_REG_OPERAND());
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
u8 adcs_imm(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = CalcAddcFlags(BASE_REG,DP_IMM_OPERAND());
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
//RS
//-----------------------------------------------------------------------
u8 rss(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = CalcSubFlags(DP_REG_OPERAND_IMM(),BASE_REG);
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 rss_reg(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = CalcSubFlags(DP_REG_OPERAND(),BASE_REG);
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
u8 rss_imm(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = CalcSubFlags(DP_IMM_OPERAND(),BASE_REG);
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 rs(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = DP_REG_OPERAND_IMM() - BASE_REG;
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 rs_reg(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = DP_REG_OPERAND() - BASE_REG;
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
u8 rs_imm(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = DP_IMM_OPERAND() - BASE_REG;
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
//SUB
//-----------------------------------------------------------------------
u8 subs(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = CalcSubFlags(BASE_REG,DP_REG_OPERAND_IMM());
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 subs_reg(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = CalcSubFlags(BASE_REG,DP_REG_OPERAND());
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
u8 subs_imm(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = CalcSubFlags(BASE_REG,DP_IMM_OPERAND());
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 sub(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG - DP_REG_OPERAND_IMM();
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 sub_reg(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG - DP_REG_OPERAND();
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
u8 sub_imm(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG - DP_IMM_OPERAND();
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 subcs(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = CalcSubFlags(BASE_REG,DP_REG_OPERAND_IMM() + (CFLAG ? 0 : 1));
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 subcs_reg(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = CalcSubFlags(BASE_REG,DP_REG_OPERAND() + (CFLAG ? 0 : 1));
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
u8 subcs_imm(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = CalcSubFlags(BASE_REG,DP_IMM_OPERAND() + (CFLAG ? 0 : 1));
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 subc(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG - (DP_REG_OPERAND_IMM() + (CFLAG ? 0 : 1));
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 subc_reg(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG - (DP_REG_OPERAND() + (CFLAG ? 0 : 1));
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
u8 subc_imm(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG - (DP_IMM_OPERAND() + (CFLAG ? 0 : 1));
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
//ADD
//-----------------------------------------------------------------------
u8 adds(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = CalcAddFlags(BASE_REG,DP_REG_OPERAND_IMM());
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 adds_reg(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = CalcAddFlags(BASE_REG,DP_REG_OPERAND());
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
u8 adds_imm(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = CalcAddFlags(BASE_REG,DP_IMM_OPERAND());
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 add(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG + DP_REG_OPERAND_IMM();
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 add_reg(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG + DP_REG_OPERAND();
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
u8 add_imm(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG + DP_IMM_OPERAND();
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
//RSC
//-----------------------------------------------------------------------
u8 rsc(void)
{
   u32 op1,op2;
   u8 res,pipe;

   res = cycP;
   op1 = BASE_REG;
   if((OPCODE & 0x2000000))
       op2 = DP_IMM_OPERAND();
   else{
       if(!(OPCODE & 0x10))
           op2 = DP_REG_OPERAND_IMM();
       else{
           op2 = DP_REG_OPERAND();
           res += (u8)cycI;
       }
   }
   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = (op2 += (CFLAG ? 0 : 1)) - op1;
   if((OPCODE & 0x100000))
       CalcSubFlags(op2,op1);
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return res;
}
//-----------------------------------------------------------------------
//MOV
//-----------------------------------------------------------------------
u8 mov(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = DP_REG_OPERAND_IMM();
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 mov_reg(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = DP_REG_OPERAND();
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
u8 mov_imm(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = DP_IMM_OPERAND();
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 movs(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((GP_REG[(pipe = (u8)DEST_REG_INDEX)] = DP_REG_OPERAND_UPD((u8)IMM_SHIFT)));
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 movs_reg(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((GP_REG[(pipe = (u8)DEST_REG_INDEX)] = DP_REG_OPERAND_UPD((u8)SHFT_AMO_REG)));
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
u8 movs_imm(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((GP_REG[(pipe = (u8)DEST_REG_INDEX)] = DP_IMM_OPERAND_UPD()));
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
//MVN
//-----------------------------------------------------------------------
u8 mvn(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = ~DP_REG_OPERAND_IMM();
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 mvn_reg(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)(((u16)OPCODE>> 12)))] = ~DP_REG_OPERAND();
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
u8 mvn_imm(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = ~DP_IMM_OPERAND();
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 mvns(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((GP_REG[(pipe = (u8)DEST_REG_INDEX)] = ~DP_REG_OPERAND_UPD((u8)IMM_SHIFT)));
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 mvns_reg(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((GP_REG[(pipe = (u8)DEST_REG_INDEX)] = ~DP_REG_OPERAND_UPD((u8)SHFT_AMO_REG)));
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
u8 mvns_imm(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((GP_REG[(pipe = (u8)DEST_REG_INDEX)] = ~DP_IMM_OPERAND_UPD()));
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
//BIC
//-----------------------------------------------------------------------
u8 bic(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)(((u16)OPCODE>> 12)))] = BASE_REG & ~DP_REG_OPERAND_IMM();
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 bic_reg(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG & ~DP_REG_OPERAND();
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
u8 bic_imm(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG & ~DP_IMM_OPERAND();
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 bics(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG & ~DP_REG_OPERAND_UPD((u8)IMM_SHIFT)));
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 bics_reg(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG & ~DP_REG_OPERAND_UPD((u8)SHFT_AMO_REG)));
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
u8 bics_imm(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG & ~DP_IMM_OPERAND_UPD()));
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
// AND
//-----------------------------------------------------------------------
u8 and_(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG & DP_REG_OPERAND_IMM();
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 and_reg(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG & DP_REG_OPERAND();
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
u8 and_imm(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = (BASE_REG & DP_IMM_OPERAND());
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 ands(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG & DP_REG_OPERAND_UPD((u8)IMM_SHIFT)));
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 ands_reg(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG & DP_REG_OPERAND_UPD((u8)SHFT_AMO_REG)));
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
u8 ands_imm(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG & DP_IMM_OPERAND_UPD()));
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
// EOR
//-----------------------------------------------------------------------
u8 eor(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG ^ DP_REG_OPERAND_IMM();
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 eor_reg(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG ^ DP_REG_OPERAND();
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
u8 eor_imm(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG ^ DP_IMM_OPERAND();
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 eors(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG ^ DP_REG_OPERAND_UPD((u8)IMM_SHIFT)));
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 eors_reg(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG ^ DP_REG_OPERAND_UPD((u8)SHFT_AMO_REG)));
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return (u8)(cycI+cycP);
}
//-----------------------------------------------------------------------
u8 eors_imm(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG ^ DP_IMM_OPERAND_UPD()));
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
// ORR
//-----------------------------------------------------------------------
u8 orr(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG | DP_REG_OPERAND_IMM();
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 orr_reg(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG | DP_REG_OPERAND();
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return (u8)(cycP + cycI);
}
//-----------------------------------------------------------------------
u8 orr_imm(void)
{
   u8 pipe;

   GP_REG[(pipe = (u8)DEST_REG_INDEX)] = (BASE_REG | DP_IMM_OPERAND());
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 orrs(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG | DP_REG_OPERAND_UPD((u8)IMM_SHIFT)));
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
u8 orrs_reg(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG | DP_REG_OPERAND_UPD((u8)SHFT_AMO_REG)));
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return (u8)(cycP + cycI);
}
//-----------------------------------------------------------------------
u8 orrs_imm(void)
{
   u8 pipe;

   SET_DP_LOG_FLAGS((GP_REG[(pipe = (u8)DEST_REG_INDEX)] = BASE_REG | DP_IMM_OPERAND_UPD()));
   if(pipe == 15){
       arm.bRefillPipe = 1;
       REG_PC += 4;
   }
   return cycP;
}
//-----------------------------------------------------------------------
/*u8 dp(void)
{
   u32 *regd,op2,op1;
   u8 opcode,pipe,s1,s;

   op1 = BASE_REG;
   regd = &GP_REG[(pipe = DEST_REG_INDEX & 0xF)];
   s = (OPCODE & 0x100000) ? 1 : 0;
   if((opcode = (u8)((OPCODE >> 21) & 0xF)) > 4 && opcode < 8)
       s1 = 0;
   else
       s1 = s;
   if((OPCODE & 0x2000000))
       op2 = DP_IMM_OPERAND(s1);
   else{
       if(!(OPCODE & 0x10))
           op2 = DP_REG_OPERAND(((OPCODE>>7)&0x1F),s1);
       else
           op2 = DP_REG_OPERAND((u8)(SHFT_AMO_REG),s1);
   }
   switch(opcode){
       case 0://AND
           *regd = op1 & op2;
           if(s)
               SET_DP_LOG_FLAGS(*regd);
       break;
       case 1://eor
           *regd = op1 ^ op2;
           if(s)
               SET_DP_LOG_FLAGS(*regd);
       break;
       case 2://sub
           *regd = op1 - op2;
           if(s)
               CalcSubFlags(op1,op2);
       break;
       case 3://rs
           *regd = op2 - op1;
           if(s)
               CalcSubFlags(op2,op1);
       break;
       case 4://add
           *regd = op1 + op2;
           if(s)
               CalcAddFlags(op1,op2);
       break;
       case 5://adc
           op2 += CFLAG;
           *regd = op1 + op2;
           if(s)
               CalcAddFlags(op1,op2);
       break;
       case 6://subc
           *regd = op1 - (op2 + (CFLAG ? 0 : 1));
           if(s)
               CalcSubFlags(op1,op2);
       break;
       case 7://rsc
           *regd = op2 - (op1 + (CFLAG ? 0 : 1));
           if(s)
               CalcSubFlags(op2,op1);
       break;
       case 8://TST
           SET_DP_LOG_FLAGS(op1 & op2);
       break;
       case 9: // TEQ
           SET_DP_LOG_FLAGS(op1 ^ op2);
       break;
       case 10: // CMP
           CalcSubFlags(op1,op2);
       break;
       case 11: //CMN
           CalcAddFlags(op1,op2);
       break;
       case 12: //ORR
           *regd = op1 | op2;
           if(s)
               SET_DP_LOG_FLAGS(*regd);
       break;
       case 13: //MOV
           *regd = op2;
           if(s)
               SET_DP_LOG_FLAGS(op2);
       break;
       case 14: // BIC
           *regd = op1 & ~op2;
           if(s)
               SET_DP_LOG_FLAGS(*regd);
       break;
       case 15://MVN
           *regd = ~op2;
           if(s)
               SET_DP_LOG_FLAGS(*regd);
       break;
   }
   if(pipe == 0xF){
       REG_PC += 4;
       return 3;
   }
   return 2;
}
//-----------------------------------------------------------------------
u8 bdt(void) //ldm-stm
{
   u32 *regs,p,base,stack;
   u8 i,r,reg[15],*p2;
   u16 *p1;

   p = (OPCODE & 0x1000000);
   stack = *(regs = &BASE_REG);
   p1 = pow2;
   p2 = reg;
   for(i=0;i<16;i++){
       if((OPCODE & *p1++))
           *p2++ = i;
   }
   r = (u8)(p2 - reg);
   if((OPCODE & 0x800000)){
       base = stack + (r<<2);
       if(p)
           stack += 4;
   }
   else{
       base = stack - (r<<2);
       stack = base;
       if(!p)
           stack += 4;
   }
   p2 = reg;
   if((OPCODE & 0x200000))
       *regs = base;
   if(!(OPCODE & 0x100000)){//Store
       for(i=0;i<r;i++){
           WRITEWORD(stack,GP_REG[*p2++]);
           stack += 4;
       }
   }
   else{//Load
       for(i=0;i<r;i++){
           GP_REG[*p2++] = READWORD(stack);
           stack += 4;
       }
      	if((OPCODE & 0x8000)){
           REG_PC += 4;
           r += (u8)2;
       }
   }
   return (u8)(2 + r);
}
//-----------------------------------------------------------------------
u8 sdt(void) //ldr-str
{
   u32 offset,base,*regd,*regs;
   u32 p,u;
   u8 rd,res;

   if(!(OPCODE & 0x2000000))
       offset = OPCODE & 0xFFF;
   else
       offset = DP_REG_OPERAND((OPCODE >> 7) & 0x1F,FALSE);
   base = *(regs = &BASE_REG);
   regd = &GP_REG[(rd = (u8)(DEST_REG_INDEX & 0xF))];
   u = (OPCODE & 0x800000);
   if((p = (OPCODE & 0x1000000)) != 0){
       if(u)
           base += offset;
       else
           base -= offset;
   }
   res = 3;
   if((OPCODE & 0x100000)){//Load
       if((OPCODE & 0x400000))
           *regd = READBYTE(base);
       else{
           *regd = READWORD(base);
           if(rd == 15){
               REG_PC += 4;
               res += (u8)2;
           }
       }
   }
   else{//Store
       if((OPCODE & 0x400000))
           WRITEBYTE(base,(u8)*regd);
       else{
           if(rd == 15)
               WRITEWORD(base,*regd+4);
           else
               WRITEWORD(base,*regd);
       }
   }
   if(!p){
       if(u)
           base += offset;
       else
           base -= offset;
       *regs = base;
   }
   else if((OPCODE & 0x200000))
       *regs = base;
   return res;
}
//-----------------------------------------------------------------------
u8 hsdt(void) //ldrh-strh-ldrbs
{
   u32 offset,base,*regd,*regs,p,u;

   if((OPCODE & 0x400000))
       offset = ((OPCODE&0xF00)>>4)|(OPCODE&0xF);
   else
       offset = OP_REG;
   base = *(regs = &BASE_REG);
   regd = &GP_REG[DEST_REG_INDEX & 0xF];
   u = (OPCODE & 0x800000);
   if((p = (OPCODE & 0x1000000)) != 0){
       if(u)
           base += offset;
       else
           base -= offset;
   }
   if((OPCODE & 0x100000)){//Load
       switch((OPCODE & 0x60) >> 5){
           case 0:
           break;
           case 1:
               *regd = READHWORD(base);
           break;
           case 2:
               *regd = (u32)((s8)READBYTE(base));
           break;
           case 3:
               *regd = (u32)((s16)READHWORD(base));
           break;
       }
   }
   else{//Store
       if(((OPCODE & 0x60) >> 5) == 1)
           WRITEHWORD(base,(u16)*regd);
   }
   if(!p){
       if(u)
           base += offset;
       else
           base -= offset;
       *regs = base;
   }
   else if((OPCODE & 0x200000))
       *regs = base;
   return 3;
}*/