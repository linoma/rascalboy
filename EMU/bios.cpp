#include <math.h>
#include "gba.h"
#include "fstream.h"
#include "memory.h"
#include "bios.h"
#include "exec.h"
#include "debug.h"

//---------------------------------------------------------------------------
#define FREQ_MIDI  1.0594630944
#define OCT_MIDI   32.70319566
//---------------------------------------------------------------------------
static void Halt();
static void RegisterRamReset();
static void CpuSet();
static void CpuFastSet();
static void ObjAffineSet();
static void LZ77UnComp();
static void RLUnComp();
static void ResetMem(u32 dst,u32 count);
static void HuffUnComp();
//---------------------------------------------------------------------------
// BIOS FUNCTION
//---------------------------------------------------------------------------
//  0 Thumb Mode
//  1 Arm Mode
//---------------------------------------------------------------------------
static BIOSFUNC biosfunc[] = {
//   {0x00B4,1},
   {0x00B4,1},             //  0
   {0x09C2,0},      //  1
   {0x01a0,1},
   {0x0,1},
/*   {0x01A0,1,"Halt"},//  2
   {0x01A8,1,"Stop"},//  3*/
   {0x0330,1},              //  4
   {0x0328,1},        //  5
   {0x03B4,1},                   //  6
   {0x03A8,1},                //  7
   {0x0404,1},                  //  8
   {0x0474,1},                //  9
   {0x04FC,0},               // 10
   {0x0B4C,0},                // 11
   {0x0BC4,1},            // 12
   {0x0000,0},                      // 13
   {0x0C2C,1},           // 14
   {0x0CE0,1},          // 15
   {0x0F60,1},             // 16
   {0x10FC,1},        // 17
   {0x1194,1},        // 18
   {0x1014,1},            // 19
   {0x1278,0},          // 20
   {0x12C0,0},          // 21
   {0x1332,0},  // 22
   {0x135C,0},  // 23
   {0x1398,0},     // 24
   {0x800,0},        // 25
   {0x1664,0},       // 26
   {0x179C,0},       // 27
   {0x1DC4,0},       // 28
   {0x210C,0},      // 29
   {0x1824,0},     // 30
   {0x18D8,0},          // 31
   {0x0,0},                    // 32
   {0x0,0},                    // 33
   {0x0,0},                    // 34
   {0x0,0},                    // 35
   {0x0,0},                    // 36
   {0x0,0},                // 37
   {0x1878,0},   // 38
   {0x18C8,0},  // 39
   {0x18D8,0}                  // 40
};

static BOOL bLoadBios = FALSE,bUseBios = FALSE,bSkipBiosIntro = FALSE;
static int iSubCodeError;
static LString biosFileName;
static u32 GP_REG12;
//---------------------------------------------------------------------------
#define MAX_BIOS sizeof(biosfunc) / sizeof(BIOSFUNC)
//---------------------------------------------------------------------------
int GetBiosSubCodeError()
{
   return iSubCodeError;
}
//---------------------------------------------------------------------------
void bios(u16 func)
{
   u8 mode;
   BIOSFUNC *p;

   iSubCodeError = 0;
   if(func >= MAX_BIOS){
       iSubCodeError = -1;
       ShowMessageError(TE_BIOS,func);
       SendMessage(hWin,WM_CLOSE,0,0);
       return;
   }
   p = &biosfunc[func];
#ifdef _DEBPRO
   if(func == 5 || func == 2 || func == 4)
       WriteMessage(MESSAGE_SWI5,"swi %02d",func);
   else{
       WriteMessage(MESSAGE_SWI,"swi %02d %08X %08X %08X",func,GP_REG[0],GP_REG[1],GP_REG[2]);
       ControlBreakOn(BREAK_ENTER_SWI);
   }
#endif
   if(!UseBiosFile(TRUE)){
       if(biosEmulation(func) == 0){
           iSubCodeError = -2;
           ShowMessageError(TE_BIOS,func);
           SendMessage(hWin,WM_CLOSE,0,0);
       }
       return;
   }
   if(p->adress == 0)
       return;
   if((CPSR & T_BIT)){
       tadvance_instruction_pipe();
       mode = 1;
   }
   else{
       mode = 0;
       advance_instruction_pipe();
   }
   SwitchCpuMode(0x8000|SUPERVISOR_MODE);
   if(func == 0){
       arm.index--;
       ZeroMemory(&arm.lrv[arm.index],sizeof(LR));
   }
   REG_PC = p->adress;
   if(p->mode){
       REG_PC += 4;
       if(mode)
           advance_instruction_pipe();
       CPSR &= ~T_BIT;
       SetExecFunction(0);
   }
   else{
       REG_PC += 2;
       if(!mode)
           tadvance_instruction_pipe();
       CPSR |= T_BIT;
       SetExecFunction(1);
   }
}
//---------------------------------------------------------------------------
BOOL ResetBios(char *lpNameFile)
{
   LFile *fp;
   DWORD dw;
   BOOL res;

   if(zero_page_u8 == NULL)
       return FALSE;
   if(lpNameFile == NULL && bLoadBios && bUseBios)
       return TRUE;
   bLoadBios = FALSE;
   if(lpNameFile == NULL && (biosFileName.Length() < 1 || !bUseBios)){
       ZeroMemory(zero_page_u8,16384);
       return TRUE;
   }
   res = FALSE;
   fp = new LFile(lpNameFile != NULL ? lpNameFile : biosFileName.c_str());
   if(fp == NULL)
       goto ex_resetbios;
   if(!fp->Open())
       goto ex_resetbios;
   if((dw = fp->Size()) > 16384)
       goto ex_resetbios;
   if(bUseBios && fp->Read(zero_page_u8,dw) != dw)
       goto ex_resetbios;
   res = TRUE;
ex_resetbios:
   if(fp != NULL)
       delete fp;
   if(lpNameFile != NULL && res)
       biosFileName = lpNameFile;
   else if(lpNameFile == NULL && !res)
       biosFileName = "";
   bLoadBios = bUseBios && res;
   return res;
}
//---------------------------------------------------------------------------
void ResetBiosParam()
{
   bSkipBiosIntro = bUseBios = FALSE;
   biosFileName = "";
   SetSkipBiosIntro(FALSE);
   SetUseBios(FALSE);
}
//---------------------------------------------------------------------------
BOOL SkipBiosIntro(int flag)
{
   if(flag)
       return UseBiosFile(TRUE) && !bSkipBiosIntro;
   return bSkipBiosIntro;
}
//---------------------------------------------------------------------------
void SetSkipBiosIntro(BOOL flag)
{
   bSkipBiosIntro = flag;
   CheckMenuItem(GetMenu(hWin),ID_SKIPBIOSINTRO,MF_BYCOMMAND|(flag ? MF_CHECKED : MF_UNCHECKED));
}
//---------------------------------------------------------------------------
void SetUseBios(BOOL flag)
{
   bUseBios = flag;
   CheckMenuItem(GetMenu(hWin),ID_APP_USEBIOS,MF_BYCOMMAND|(flag ? MF_CHECKED : MF_UNCHECKED));
   ResetBios(NULL);
}
//---------------------------------------------------------------------------
void SetBiosFileName(char *lpFileName)
{
   biosFileName = lpFileName;
}
//---------------------------------------------------------------------------
int GetBiosFileName(char *dst,int len)
{
   if(biosFileName.c_str() == NULL)
       return -1;
   if(dst == NULL)
       return biosFileName.Length();
   len = biosFileName.Length() < len ? biosFileName.Length() : len;
   lstrcpyn(dst,biosFileName.c_str(),len+1);
   return len;
}
//---------------------------------------------------------------------------
BOOL UseBiosFile(int flag)
{
   if(flag)
       return bLoadBios && bUseBios;
   else
       return bUseBios;
}
//---------------------------------------------------------------------------
static void Halt()
{
   WRITEBYTE(0x04000301,0);
}
//---------------------------------------------------------------------------
BOOL CheckIntrWait()
{
   BOOL res;

   if((INTR_CHECK & arm.IntrWait)){
       INTR_CHECK &= (u16)~arm.IntrWait;
       arm.IntrWait = 0;
       res = TRUE;
       GP_REG[12] = GP_REG12;
   }
   else
       res = FALSE;
   REG_IME = 1;
   return res;
}
//---------------------------------------------------------------------------
static void IntrWait()
{
   arm.IntrWait = (u16)GP_REG[1];
   if(GP_REG[0])
       INTR_CHECK &= (u16)~GP_REG[1];
   REG_IME = 1;
   GP_REG12 = GP_REG[12];
   GP_REG[12] = 0x4000000;
   WRITEBYTE(0x04000301,0);
}
//---------------------------------------------------------------------------
static void VBlankIntrWait()
{
   GP_REG[0] = GP_REG[1] = 1;
   IntrWait();
}
//---------------------------------------------------------------------------
static void CpuSet()
{
   u8 DataSize;
   int count;
   u32 dst,src,i;

   count = (int)((u16)GP_REG[2]);
   dst = GP_REG[1];
   src = GP_REG[0];
   if((DataSize = (u8)((GP_REG[2] >> 26) & 1)) != 0){
       dst &= ~3;
       src &= ~3;
   }
   else{
       dst &= ~1;
       src &= ~1;
   }
   switch((GP_REG[2] >> 24) & 1){
       case 0:
           if(DataSize != 0){
               for(;count>0;count--){
                   WRITEWORD(dst,READWORD(src));
                   dst += 4;
                   src += 4;
               }
           }
           else{
               for(;count>0;count--){
                   WRITEHWORD(dst,READHWORD(src));
                   dst += 2;
                   src += 2;
               }
           }
       break;
       case 1:
           if(DataSize != 0){
               i = READWORD(src);
               for(;count>0;count--){
                   WRITEWORD(dst,i);
                   dst += 4;
               }
           }
           else{
               i = (u32)READHWORD(src);
               for(;count>0;count--){
                   WRITEHWORD(dst,(u16)i);
                   dst += 2;
               }
           }
       break;
   }
}
//---------------------------------------------------------------------------
static void CpuFastSet()
{
   int count;
   u32 dst,src,i;

   count = (int)((u16)GP_REG[2]);
   dst = GP_REG[1];
   src = GP_REG[0];
   switch((GP_REG[2] >> 24) & 1){
       case 0:
           for(;count>0;count--){
               WRITEWORD(dst,READWORD(src));
               dst += 4;
               src += 4;
           }
       break;
       case 1:
           i = READWORD(src);
           for(;count>0;count--){
               WRITEWORD(dst,i);
               dst += 4;
           }
       break;
   }
}
//---------------------------------------------------------------------------
static void Diff8bitUnfilterWRAM()
{
   u32 dst,src,DataHeader;
   int DataSize;
   u8 value8;

   dst = GP_REG[1];
   src = GP_REG[0] & ~3;
   DataHeader = READWORD(src);
   src += 4;
   DataSize = (DataHeader >> 8);
   value8 = READBYTE(src++);
   WRITEBYTE(dst++,value8);
   while(DataSize > 1){
       value8 += READBYTE(src++);
       WRITEBYTE(dst++,value8);
       DataSize--;
   }
}
//---------------------------------------------------------------------------
static void RLUnComp()
{
   u32 dst,src,DataHeader;
   int DataSize;
   u8 FlagData,DataLength,b;

   dst = GP_REG[1];
   src = GP_REG[0];

   DataHeader = READWORD(src);
   src += 4;
   DataSize = (DataHeader >> 8);
   while(DataSize > 0){
       FlagData = READBYTE(src++);
       DataLength = (u8)(FlagData & 0x7F);
       if((FlagData & 0x80) != 0){
           DataLength += (u8)3;
           DataSize -= DataLength;
           b = READBYTE(src++);
           for(;DataLength > 0;DataLength--)
               WRITEBYTE(dst++,b);
       }
       else{
           DataLength += (u8)1;
           DataSize -= DataLength;
           for(;DataLength > 0;DataLength--)
               WRITEBYTE(dst++,READBYTE(src++));
       }
   }
}
//---------------------------------------------------------------------------
static void LZ77UnComp()
{
   u32 dst,src,DataHeader,i1,i3;
   int DataSize;
   u32 b;
   u8 i,i4;

   src = GP_REG[0];
   dst = GP_REG[1];

   DataHeader = READWORD(src);
   src += 4;
   DataSize = DataHeader >> 8;
   while(DataSize > 0){
       b = (u32)READBYTE(src++);
       for(i=8;i > 0 && DataSize > 0;i--){
           if((b & 0x80) == 0){
               WRITEBYTE(dst++,READBYTE(src++));
               DataSize--;
           }
           else{
               i4 = READBYTE(src++);
               i1 = 3 + (i4 >> 4);
               i3 = (((i4 & 0xF) << 8) | READBYTE(src++)) + 1;
               DataSize -= i1;
               for(;i1 > 0;i1--)
                   WRITEBYTE(dst++,READBYTE(dst-i3));
           }
           b <<= 1;
       }
   }
}
//---------------------------------------------------------------------------
static void HuffUnComp()
{
   u32 dst,src,r3,tableAddress,value32,adr;
   int count;
   u8 i,i1,prog,max,sl,sr,carry;

   dst = GP_REG[1];
   adr = (src = GP_REG[0] & ~3) + 4;
   tableAddress = adr + 1;
   value32 = READWORD(src);
   count = (int)(value32 >> 8);

   sr = (u8)(value32 & 0xF);
   sl = (u8)(0x20 - sr);
   max = (u8)((sr & 7) + 4);

   src = ((READBYTE(adr) + 1) << 1) + adr;
   adr = tableAddress;
   for(r3 = prog = 0;count > 0;){
       value32 = READWORD(src);
       src += 4;
       for(i = 32;i > 0 && count > 0;i--,value32 <<= 1){
           carry = (u8)(value32 >> 31);
           i1 = READBYTE(adr);
           adr = (adr & ~1) + (((i1 & 0x3F) + 1) << 1) + carry;
           i1 <<= carry;
           if(!(i1 & 0x80))
               continue;
           r3 = (r3 >> sr) | (READBYTE(adr) << (u8)sl);
           adr = tableAddress;
           prog++;
           if(prog != max)
               continue;
           WRITEWORD(dst,r3);
           dst += 4;
           count -= 4;
           prog = 0;
       }
   }
}
//---------------------------------------------------------------------------
static void ResetMem(u32 dst,u32 count)
{
   for(;count > 0;count--){
       WRITEWORD(dst,0);
       dst += 4;
   }
}
//---------------------------------------------------------------------------
static void RegisterRamReset()
{
   u8 value;

   value = (u8)GP_REG[0];
   WRITEWORD(0x04000000,0x80);
   if((value & 0x80)){
       ResetMem(0x04000200,8);
       WRITEHWORD(0x04000202,0xFFFF);
       ResetMem(0x04000004,8);
       ResetMem(0x04000020,16);
       ResetMem(0x040000B0,24);
       WRITEWORD(0x04000130,0x0000FFFF);
       WRITEHWORD(0x04000020,0x0100);
       WRITEHWORD(0x04000026,0x0100);
       WRITEHWORD(0x04000030,0x0100);
       WRITEHWORD(0x04000036,0x0100);
   }
   if((value & 0x40)){
       WRITEBYTE(0x04000084,0x80);
       WRITEWORD(0x04000080,0);
       WRITEBYTE(0x04000070,0);
       ResetMem(0x04000090,8);
   }
   if((value & 0x20)){
       ResetMem(0x04000110,8);
       WRITEHWORD(0x04000130,0xFFFF);
       WRITEBYTE(0x04000140,0x7);
       ResetMem(0x04000140,7);
   }
   if((value & 0x10))
       ResetMem(0x07000000,0x0100);
   if((value & 0x8))
       ResetMem(0x06000000,0x6000);
   if((value & 0x4))
       ResetMem(0x05000000,0x0100);
   if((value & 0x2))
       ResetMem(0x03000000,0x1F7F);
   if((value & 0x1))
       ResetMem(0x02000000,0x10000);
}
//---------------------------------------------------------------------------
static void BgAffineSet()
{
   int i,sn,cs,i1,i2,i3,i4,i5,i6,i9,i10,i12,i11;
   u16 w;
   u32 dst,src;

   dst = GP_REG[1];
   src = GP_REG[0];
   for(i=GP_REG[2];i>0;i--){
       w = (u16)(READHWORD(src+0x10) >> 8);
       sn = (int)(sin(((u8)(w + 0x40)) * M_PI / 128.0) * 16384.0);
       cs = (int)(sin(w * M_PI / 128.0) * 16384.0);
       i1 = (int)((s16)READHWORD(src+12));
       i2 = (int)((s16)READHWORD(src+14));
       i3 = (sn * i1) >> 14;
       i4 = (cs * i1) >> 14;
       i5 = (cs * i2) >> 14;
       i6 = (sn * i2) >> 14;
       i9 = READWORD(src);
       i10 = READWORD(src + 4);
       i12 = READWORD(src + 8);
       i11 = (int)((u16)i12);
       i12 >>= 16;
       i9 += i3 * -i11;
       WRITEWORD(dst + 8,i4 * i12 + i9);
       i10 += i5 * -i11;
       WRITEWORD(dst+12,i6 * -i12 + i10);
       WRITEHWORD(dst,(u16)i3);
       WRITEHWORD(dst+2,(u16)(0-i4));
       WRITEHWORD(dst+4,(u16)i5);
       WRITEHWORD(dst+6,(u16)i6);
       src += 20;
       dst += 16;
   }
}
//---------------------------------------------------------------------------
static void ObjAffineSet()
{
   int i,sn,cs,i1,i2,i3,offset;
   u16 w;
   u32 dst,src;

   dst = GP_REG[1];
   src = GP_REG[0];
   offset = GP_REG[3];
   for(i=GP_REG[2];i>0;i--){
       w = (u16)(READHWORD(src+4) >> 8);
       sn = (int)(sin(((u8)(w + 0x40)) * M_PI / 128.0) * 16384.0);
       cs = (int)(sin(w * M_PI / 128.0) * 16384.0);
       i1 = (int)((s16)READHWORD(src));
       i2 = (int)((s16)READHWORD(src+2));
       i3 = (sn * i1) >> 14;
       WRITEHWORD(dst,(u16)i3);
       dst += offset;
       i3 = 0 - ((cs * i1) >> 14);
       WRITEHWORD(dst,(u16)i3);
       dst += offset;
       i3 = (cs * i2) >> 14;
       WRITEHWORD(dst,(u16)i3);
       dst += offset;
       i3 = (sn * i2) >> 14;
       WRITEHWORD(dst,(u16)i3);
       dst += offset;
       src += 8;
   }
}
//---------------------------------------------------------------------------
static void SoftReset()
{
   ResetCpu();
   REG_PC = READBYTE(0x03FFFFFA) != 0 ? 0x02000004 : 0x08000004;
   ResetMem(0x03FFFE00,128);
   SetExecFunction(0);
   advance_instruction_pipe();
}
//---------------------------------------------------------------------------
static void ArcTan2()
{
   float theta;
   s16 x,y;

   y = (s16)GP_REG[1];
   x = (s16)GP_REG[0];
   if(x != 0)
       theta = atan2(y,x);
   else if(y != 0)
       theta = atan(y);
   else
       theta = 0;
   GP_REG[0] = (u32)((s16)(theta * 65535.0 / 2.0 / M_PI));
}
//---------------------------------------------------------------------------
static void Midi2Key()
{
   double freq,oct,nota;
   u8 f,o,i;

   o = (u8)GP_REG[1];
   f = (u8)(o & 0xF);
   o >>= 4;
   freq = 1.0;
   for(i=0;i<=f;i++)
       freq = freq * FREQ_MIDI;
   oct = OCT_MIDI;
   for(i=0;i<=o;i++)
       oct = oct * 2.0;
   nota = freq * oct;
   if(f > 0xE){
       freq = 1;
       oct *= 2.0;
   }
   else
       freq *= FREQ_MIDI;
   nota = (oct * freq) - nota;
   if(GP_REG[2] != 0)
       nota /= (double)GP_REG[2];
   GP_REG[0] = (u32)((READWORD(GP_REG[0] + 4) >> 16) * nota);
}
//---------------------------------------------------------------------------
u8 biosEmulation(u8 func)
{
   int i;

   switch(func){
       case 0:
           SoftReset();
           return 1;
       case 1:
           RegisterRamReset();
           return 1;
       case 2:
           Halt();
           return 1;
       case 4:
           IntrWait();
           return 1;
       case 5:
           VBlankIntrWait();
           return 1;
       case 6:
           if(GP_REG[1] != 0){
               i = (int)GP_REG[0] / (int)GP_REG[1];
               GP_REG[1] = (int)GP_REG[0] % (int)GP_REG[1];
               GP_REG[0] = i;
               GP_REG[3] = abs(i);
           }
           return 1;
       case 7:
           i = (int)GP_REG[1] / (int)GP_REG[0];
           GP_REG[1] = (int)GP_REG[1] % (int)GP_REG[0];
           GP_REG[0] = i;
           GP_REG[3] = abs(i);
           return 1;
       case 8:
           if(GP_REG[0] != 0)
               GP_REG[0] = (u32)sqrt(GP_REG[0]);
           return 1;
       case 10:
           ArcTan2();
           return 1;
       case 11:
           CpuSet();
           return 1;
       case 12:
           CpuFastSet();
           return 1;
       case 14:
           BgAffineSet();
           return 1;
       case 15:
           ObjAffineSet();
           return 1;
       case 17:
       case 18:
           LZ77UnComp();
           return 1;
       case 19:
           HuffUnComp();
           return 1;
       case 20:
       case 21:
           RLUnComp();
           return 1;
       case 22:
           Diff8bitUnfilterWRAM();
           return 1;
       case 31:
           Midi2Key();
           return 1;
   }
   return 0;
}





