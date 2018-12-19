#include "gbaemu.h"
#include "memory.h"
#include "debug.h"
#include "topedec.h"

#define STANDARD_INSTRUCTION_DECODING  \
       lstrcpy (dest, opcode_strings [((op&0xFF00000)>>16)|((op&0xF0)>>4)]);\
       lstrcat (dest, condition_strings [(op>>28)]);\
       lstrcat (dest, " ");

#ifdef _DEBUG
//---------------------------------------------------------------------------
void second_operand (u32 op, u32 adress, char *dest)
{
   u32 temp;
	u32 shift_amount;
	char string [33], string2 [33];

	if((op&0x2000000)){
		shift_amount = ((op>>8)&0xF)<<1;
		temp = ((op&0xFF)<<(32-shift_amount))|((op&0xFF)>>shift_amount);
       wsprintf(string,"0x%08X",temp);
		lstrcat (dest, string);
	}
	else {
       lstrcat (dest, register_strings[(op&0xF)]);
		lstrcpy (string, ", ");
		lstrcat (string, shift_strings[(op>>5)&0x3]);
		if((op&0x10)) {
			lstrcat(string, " ");
			lstrcat(string, register_strings[(op>>8)&0xF]);
			lstrcat(dest, string);
		}
		else {
			temp = (op>>7)&0x1F;
			if(!temp) {
				switch ((op>>5)&0x3) {
					case 0:
                   break;
					case 1:
                       lstrcat (dest,", lsr 0x20");
                   break;
					case 2:
						lstrcat (dest,", asr 0x20");
                   break;
					case 3:
						lstrcat (dest,", rrx");
                   break;
				}
			}
           else {
				lstrcat (string, " ");
               wsprintf(string2,"%d",temp);
				lstrcat (string, string2);
				lstrcat (dest, string);
			}
		}
	}
}
//---------------------------------------------------------------------------
void standard_debug_handle (u32 op, u32 adress, char *dest)
{
	STANDARD_INSTRUCTION_DECODING;
}
//---------------------------------------------------------------------------
void bx_debug_handle (u32 op, u32 adress, char *dest)
{
   STANDARD_INSTRUCTION_DECODING;
   lstrcat(dest,register_strings[op&0xF]);
}
//---------------------------------------------------------------------------
void b_debug_handle (u32 op, u32 adress, char *dest)
{
	u32 temp;
	char string [33];

	STANDARD_INSTRUCTION_DECODING;

   if (op&0x800000)
       temp = adress + (((op&0xFFFFFF)<<2)-0x4000000) + 8;
	else
       temp = adress + ((op&0x7FFFFF)<<2) + 8;
   wsprintf(string,"0x%08X",temp);
	lstrcat (dest, string);
}
//---------------------------------------------------------------------------
void dpsingle_debug_handle (u32 op, u32 adress, char * dest)
{
	STANDARD_INSTRUCTION_DECODING;

	lstrcat (dest, " ");
	lstrcat (dest, register_strings[(op>>12)&0xF]);
	lstrcat (dest, ", ");
	second_operand (op, adress, dest);
}
//---------------------------------------------------------------------------
void swi_debug_handle (u32 op, u32 adress, char * dest)
{
   char s[30];

	STANDARD_INSTRUCTION_DECODING;
   wsprintf(s,"0x%02X",((op & 0x00FF0000) >> 16));
   lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void dpnw_debug_handle (u32 op, u32 adress, char * dest)
{
	STANDARD_INSTRUCTION_DECODING;

	lstrcat (dest, "  ");
	lstrcat (dest, register_strings[(op>>16)&0xF]);
	lstrcat (dest, ", ");
	second_operand (op, adress, dest);
}
//---------------------------------------------------------------------------
void dp_debug_handle (u32 op, u32 adress, char * dest)
{
	STANDARD_INSTRUCTION_DECODING;

	lstrcat (dest, "  ");
	lstrcat (dest, register_strings[(op>>12)&0xF]);
	lstrcat (dest, ", ");
	lstrcat (dest, register_strings[(op>>16)&0xF]);
	lstrcat (dest, ", ");

	second_operand (op, adress, dest);
}
//---------------------------------------------------------------------------
void mul_debug_handle (u32 op, u32 adress, char *dest)
{
	STANDARD_INSTRUCTION_DECODING;

	lstrcat (dest, "  ");
	lstrcat (dest, register_strings[(op>>16)&0xF]);
	lstrcat (dest, ", ");
	lstrcat (dest, register_strings[op&0xF]);
	lstrcat (dest, ", ");
	lstrcat (dest, register_strings[(op>>8)&0xF]);
	if (op&0x200000) {
		lstrcat (dest, ", ");
		lstrcat (dest, register_strings[(op>>12)&0xF]);
	}
}
//---------------------------------------------------------------------------
void mull_debug_handle (u32 op, u32 adress, char *dest)
{
	STANDARD_INSTRUCTION_DECODING;
	lstrcat (dest, "  ");
	lstrcat (dest, register_strings[(op>>16)&0xF]);
	lstrcat (dest, ", ");
   lstrcat (dest, register_strings[(op>>12)&0xF]);
	lstrcat (dest, ",[");
   lstrcat (dest, register_strings[(op>>8)&0xF]);
   lstrcat (dest, ",");
   lstrcat (dest, register_strings[op&0xF]);
   lstrcat (dest, "]");
}
//---------------------------------------------------------------------------
void sdt_debug_handle (u32 op, u32 adress, char *dest)
{
	u32 temp;
	char string [33], string2 [33];

	STANDARD_INSTRUCTION_DECODING;

	lstrcat (dest, "  ");
	lstrcat (dest, register_strings[(op>>12)&0xF]);

	if ((op&0x2000000)) {
		lstrcat (dest, ", [");
		lstrcat (dest, register_strings[(op>>16)&0xF]);
		if(!(op&0x1000000))
           lstrcat (dest, "]");
		lstrcat(dest, ", ");
		lstrcat(dest, register_strings[(op&0xF)]);
		lstrcpy(string, ", ");
		lstrcat(string, shift_strings[(op>>5)&0x3]);
		temp = (op>>7)&0x1F;
 		if (!temp) {
			switch ((op>>5)&0x3) {
				case 0:
               break;
				case 1:
                   lstrcpy (string2, ", ");
                   lstrcat (string2, "LSR 0x20");
                   lstrcat (dest, string2);
               break;
				case 2:
                   lstrcpy (string2, ", ");
                   lstrcat (string2, "ASR 0x20");
                   lstrcat (dest, string2);
               break;
				case 3:
                   lstrcpy (string2, ", ");
                   lstrcat (string2, "RRX");
                   lstrcat (dest, string2);
               break;
			}
		}
       else {
			lstrcat (string, " ");
           wsprintf(string2,"0x%08X",temp);
			lstrcat (string, string2);
			lstrcat (dest, string);
		}
		if ((op&0x1000000)) {
			lstrcat (dest, "]");
			if((op&0x200000))
               lstrcat(dest, "!");
		}
	}
	else {
		temp = (op&0xFFF);
		if (temp) {
			if ((((op>>16)&0xF)==15)&&(op&0x100000)) {
				lstrcat (dest, ", 0x");
				if (op&0x800000)
                   temp = adress+8+temp;
               else
                   temp = adress+8-temp;
				if(op&0x400000)
                   temp = (u32)READBYTE(temp);
               else
                   temp = READWORD(temp);
               wsprintf(string,"%X",temp);
				lstrcat (dest, string);
			}
           else {
				lstrcat (dest, ", [");
				lstrcat (dest, register_strings[(op>>16)&0xF]);
				if(!(op&0x1000000))
                   lstrcat (dest, "]");
				if((op&0x800000))
                   lstrcat (dest, ", ");
               else
                   lstrcat (dest, ", -");
               wsprintf(string,"0x%08X",temp);
				lstrcat (dest, string);
				if ((op&0x1000000)) {
					lstrcat(dest , "]");
					if((op&0x200000))
                       lstrcat(dest, "!");
				}
			}
		}
		else {
			lstrcat (dest, ", [");
			lstrcat (dest, register_strings[(op>>16)&0xF]);
			lstrcat (dest, "]");
		}
	}
}
//---------------------------------------------------------------------------
void hwdt_debug_handle (u32 op, u32 adress, char *dest)
{
   char string[33];
   int i;

	STANDARD_INSTRUCTION_DECODING;
	lstrcat(dest,"  ");
	lstrcat(dest,register_strings[(op>>12)&0xF]);
   lstrcat (dest, ", [");
	lstrcat (dest, register_strings[(op>>16)&0xF]);
   if((op & 0x400000)){
       if((i = (((op&0xF00)>>4)|(op&0xF))) != 0){
           wsprintf(string,"%X",i);
           if((op & 0x800000) != 0)
               lstrcat(dest,"+");
           else
               lstrcat(dest,"-");
           lstrcat(dest,"0x");
           lstrcat(dest,string);
       }
   }
   else{
       if((op & 0x800000) != 0)
           lstrcat(dest,"+");
       else
           lstrcat(dest,"-");
       lstrcat(dest,register_strings[op&0xF]);
   }
   lstrcat(dest,"]");
}
//---------------------------------------------------------------------------
void mdt_debug_handle (u32 op, u32 adress, char *dest)
{
   int i;
   BYTE s[20],*p;

	lstrcpy(dest,opcode_strings [((op&0xFF00000)>>16)|((op&0xF0)>>4)]);
   if((op & 0x800000)){//up
       if((op & 0x1000000))
           lstrcat(dest,"ib");
       else
           lstrcat(dest,"ia");
   }
   else{
       if((op & 0x1000000))
           lstrcat(dest,"db");
       else
           lstrcat(dest,"da");
   }
	lstrcat(dest,condition_strings[(op>>28)]);
   lstrcat(dest," ");
   lstrcat(dest,register_strings[(op>>16)&0xF]);
   p = s;
   for(i=0;i<16;i++){
       if((op & (1 << i)))
           *p++ = (BYTE)i;
   }
   *p = 0xFF;
   if((op & 0x200000))
       lstrcat(dest,"!");
   lstrcat(dest,",");
   FillMultipleRegisterString(s,dest);
}
//---------------------------------------------------------------------------
void msr_debug_handle (u32 op, u32 adress, char *dest)
{
	STANDARD_INSTRUCTION_DECODING;
   if(!(op & (1 << 25)) && !(op & (1 << 21))){
       lstrcat(dest,register_strings[(op >> 12) & 0xF]);
       if((op & (1 << 22)))
           lstrcat(dest,",SPSR");
       else
           lstrcat(dest,",CPSR");
   }
   else{
       if((op & (1 << 22)))
           lstrcat(dest,"SPSR,");
       else
           lstrcat(dest,"CPSR,");
       lstrcat(dest,register_strings[op & 0xF]);
   }
}
//---------------------------------------------------------------------------
void mrs_debug_handle (u32 op, u32 adress, char *dest)
{
	STANDARD_INSTRUCTION_DECODING;
}
//---------------------------------------------------------------------------
void swp_debug_handle (u32 op, u32 adress, char *dest)
{
	STANDARD_INSTRUCTION_DECODING;
}

#endif
