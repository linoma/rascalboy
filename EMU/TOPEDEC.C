#include "gbaemu.h"
#include "memory.h"
#include "debug.h"
#include "topedec.h"
#include "debmem.h"

#define STANDARD_INSTRUCTION_DECODING\
	lstrcpy (dest, opcode_strings_t [op>>6]);\
	lstrcat (dest, " ");
                                                        
#ifdef _DEBUG
//---------------------------------------------------------------------------
void unknown_debug_handle_t(u16 op, u32 adress, char *dest)
{
   STANDARD_INSTRUCTION_DECODING
}
//---------------------------------------------------------------------------
void standard_debug_handle_t(u16 op, u32 adress, char *dest)
{
   char s[30];

   STANDARD_INSTRUCTION_DECODING

   wsprintf(s,"%s,%s",register_strings[(op&0x7)],register_strings[(op >> 3) & 0x7]);
   lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void FillMultipleRegisterString(BYTE *s1,char *dest)
{
   BYTE *p,enterLoop;

   enterLoop = 0;
   lstrcat(dest,"{");
   p = s1;
   while(*p != 0xFF){
       if(!enterLoop){
           if(((int)p - (int)s1) > 0)
               lstrcat(dest,",");
           lstrcat(dest,register_strings[*p]);
       }
       if(abs(*(p+1) - *p) == 1)
           enterLoop = 1;
       else{
           if(enterLoop){
               lstrcat(dest,"-");
               lstrcat(dest,register_strings[*p]);
               enterLoop=0;
           }
       }
       p++;
   }
   lstrcat(dest,"}");
}
//---------------------------------------------------------------------------
void stack_debug_handle_t(u16 op, u32 adress, char *dest)
{
   int i;
   BYTE *p,s1[20];

   STANDARD_INSTRUCTION_DECODING
   p = s1;
   if((op & 0x800)){
       for(i=0;i<8;i++){
           if((op & (1 << i)))
               *p++ = (BYTE)i;
       }
       if((op & 0x100))
           *p++ = 15;
   }
   else{
       if((op & 0x100))
           *p++ = 14;
       for(i=0;i<8;i++){
           if((op & (1 << i)))
               *p++ = (BYTE)i;
       }
   }
   *p = 0xFF;
   FillMultipleRegisterString(s1,dest);
}
//---------------------------------------------------------------------------
void mdt_debug_handle_t(u16 op, u32 adress, char *dest)
{
   int i;
   BYTE *p,s1[20];

   STANDARD_INSTRUCTION_DECODING
   lstrcat(dest,register_strings[(op>>8)&0x7]);
   lstrcat(dest,"!,");

   p = s1;
   for(i=0;i<8;i++){
       if((op & (1 << i)))
           *p++ = (BYTE)i;
   }
   *p = 0xFF;
   FillMultipleRegisterString(s1,dest);
}
//---------------------------------------------------------------------------
void bx_debug_handle_t(u16 op, u32 adress, char *dest)
{
   STANDARD_INSTRUCTION_DECODING
   lstrcat(dest,register_strings[(op >> 3) & 0xF]);
}
//---------------------------------------------------------------------------
void hireg_debug_handle_t(u16 op, u32 adress, char *dest)
{
   char s[30];

   STANDARD_INSTRUCTION_DECODING
   wsprintf(s,"%s,%s",register_strings[((op & 0x80) >> 4)|(op&0x7)],register_strings[(op >> 3) & 0xF]);
   lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void bcond_debug_handle_t(u16 op, u32 adress, char *dest)
{
   int offset;
   char s[30];

   lstrcpy (dest, opcode_strings_t[op>>6]);
   lstrcat(dest,condition_strings[(op>>8)& 0xF]);
   if(((offset = ((u8)op) << 1) & 0x100))
       offset = -(512 - offset);
   adress += offset + 4;
   wsprintf(s," 0x%08X",adress);
   lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void immshort_debug_handle_t(u16 op, u32 adress, char *dest)
{
   char s[30];

   STANDARD_INSTRUCTION_DECODING
   wsprintf(s,"%s,%s,0x%1X",register_strings[op & 0x7],register_strings[(op >> 3) & 0x7],(op >> 6) & 0x7);
   lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void immlong_debug_handle_t(u16 op, u32 adress, char *dest)
{
   char s[30];

   STANDARD_INSTRUCTION_DECODING
   wsprintf(s,"%s,0x%02X",register_strings[(op>>8)&0x7],(u8)op);
   lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void reg_debug_handle_t(u16 op, u32 adress, char *dest)
{
   char s[30];

   STANDARD_INSTRUCTION_DECODING
   wsprintf(s,"%s,%s+%s",register_strings[(op&0x7)],register_strings[(op >> 3) & 0x7],register_strings[(op >> 6) & 0x7]);
   lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void immediate_debug_handle_t(u16 op, u32 adress, char *dest)
{
   char s[30];
   int i;

   STANDARD_INSTRUCTION_DECODING
   wsprintf(s,"%s,%s",register_strings[(op&0x7)],register_strings[(op >> 3) & 0x7]);
   lstrcat(dest,s);
   i = (op >> 6) & 0x1F;
   if(i != 0)
       wsprintf(s,",0x%02X",i);
   else
       lstrcpy(s,",0");
   lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void b_debug_handle_t(u16 op,u32 adress,char *dest)
{
   char s[30];
   int offset;

   STANDARD_INSTRUCTION_DECODING
   if(((offset = ((op&0x7ff) << 1)) & 0x800))
       offset = -(4096 - offset);
   (int)adress += offset + 4;
   wsprintf(s,"0x%08X",adress);
   lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void rs_debug_handle_t(u16 op,u32 adress,char *dest)
{
   char s[30];

   STANDARD_INSTRUCTION_DECODING

   if((op >> 11) == 0x9)
       wsprintf(s,"%s,PC,0x%03X",register_strings[(op >> 8)&0x7],((u8)op) << 2);
   else if(!(op & 0x2000)) //ldr,str PC
       wsprintf(s,"%s,SP,0x%03X",register_strings[(op >> 8)&0x7],((u8)op) << 2);
   else{
       if((op & 0x1000))
           wsprintf(s,"SP,SP,0x%03X",(op & 0x7F)<<2);
       else{
           if((op & 0x800))
               wsprintf(s,"%s,SP,0x%03X",register_strings[(op >> 8)&0x7],((u8)op) << 2);
           else
               wsprintf(s,"%s,PC,0x%03X",register_strings[(op >> 8)&0x7],((u8)op) << 2);
       }
   }
   lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void swi_debug_handle_t(u16 op,u32 adress,char *dest)
{
   char s[30];

   STANDARD_INSTRUCTION_DECODING
   wsprintf(s,"0x%02X",(u8)op);
   lstrcat(dest,s);
}
//---------------------------------------------------------------------------
void bl_debug_handle_t(u16 op, u32 adress, char *dest)
{
   int temp;
   char s[30];
   char h;                                    

   STANDARD_INSTRUCTION_DECODING
   h = (char)((op >> 11) & 3);
   if(h == 2){
       temp = ((op & 0x7ff) << 12);
       op = (u16)(ReadMem(adress+2,AMM_HWORD));
       if(((temp |= ((op & 0x7ff)<<1)) & 0x400000))
           temp = -(0x800000-temp);
       (int)adress += temp + 4;
       wsprintf(s,"0x%08X",adress);
       lstrcat(dest,s);
   }
   else{
       lstrcpy(dest,"**blh 0x");
       wsprintf(s,"%08X**",(op & 0x7FF));
       lstrcat(dest,s);
   }
}

#endif