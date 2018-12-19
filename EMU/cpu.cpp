#include "gba.h"
#include "cpu.h"
#include "memory.h"
#include "sound.h"
#include "debug.h"
#include "lcd.h"
#include "exec.h"
#include "bios.h"
#include <math.h>
//-----------------------------------------------------------------------
ARM7TDMI arm;
TIMER timer[4];
DMA dma[4];
PCM pcm;
LPFIFOCHANNEL fifo;
LPGBCCHANNEL gbcch;
CRITICAL_SECTION crSection;
u32 cpu_cycles,*gp_reg;
char *cpu_mode_strings[0x20];
u8 mbID;
static WORD nSpeedCPU;
//-----------------------------------------------------------------------
int IDToCpuInc(int value)
{
   switch(value){
       case ID_SKIP_CPU25:
           return 1;
       case ID_SKIP_CPU50:
           return 2;
       case ID_SKIP_CPU75:
           return 3;
   }
   return 0;
}
//-----------------------------------------------------------------------
int CpuIncToID(int value)
{
   switch(value){
       case 1:
           return ID_SKIP_CPU25;
       case 2:
           return ID_SKIP_CPU50;
       case 3:
           return ID_SKIP_CPU75;
   }
   return 0;
}
//-----------------------------------------------------------------------
int EnableSpeedCPU(WORD wID)
{
   UINT item[3];
   int i;
   
   if(wID == 0xFFFF)
       return IDToCpuInc(nSpeedCPU);
   item[0] = item[1] = item[2]  = 0;
   if(nSpeedCPU == wID)
       wID = 0;
   switch(wID){
       case ID_SKIP_CPU25:
           arm.bEnableSpeedCPU = 1;
           arm.bInc = 1;
           item[0] = 1;
       break;
       case ID_SKIP_CPU50:
           arm.bEnableSpeedCPU = 1;
           arm.bInc = 2;
           item[1] = 1;
       break;
       case ID_SKIP_CPU75:
           arm.bEnableSpeedCPU = 1;
           arm.bInc = 3;
           item[2] = 1;
       break;
       default:
           arm.bEnableSpeedCPU = 0;
           arm.bInc = 0;
       break;
   }
   nSpeedCPU = wID;
   for(i = ID_SKIP_CPUSTART;i <= ID_SKIP_CPUEND;i++)
       CheckMenuItem(GetMenu(hWin),i,MF_BYCOMMAND|(item[i-ID_SKIP_CPUSTART] != 0 ? MF_CHECKED : MF_UNCHECKED));
   return wID;
}
//-----------------------------------------------------------------------
void ResetCpu()
{
   WORD wID;

   ZeroMemory(&arm,sizeof(ARM7TDMI));
   arm.reg[3][13] = arm.reg[0][13] = 0x03007F00;//user
   arm.reg[1][13] = 0x03007FA0;//irq
   arm.reg[2][13] = 0x03007FE0;//supervisor
   //Funzioni di esecuzione pipeline

   arm.exec[0][0] = arm_exec;
   arm.exec[0][1] = thumb_exec;
   arm.exec[0][2] = arm_exec;

   arm.exec[1][0] = arm_exec_d;
   arm.exec[1][1] = thumb_exec_d;

   SetReadMemFunction(0,0);
   SetExecFunction(0);
   wID = nSpeedCPU;
   nSpeedCPU = -1;
   EnableSpeedCPU(wID);
   cpu_cycles = 0;
   arm.ws[0].u.cS = 2;
   arm.ws[0].u.cN = 4;
   arm.ws[1].value = 0x0909;
   arm.ws[2].value = 0x0909;   
   arm.ws[3].u.cN = 9;
   if(SkipBiosIntro(TRUE)){
       SwitchCpuMode(SUPERVISOR_MODE);
       CPSR |= IRQ_BIT|FIQ_BIT;
   }
   else{
       CPSR = SYSTEM_MODE;
       GP_REG = arm.reg[(arm.mode = 0)];
   }
}
//-----------------------------------------------------------------------
u8 InitCpu()
{
   u8 i;

	for(i=0;i<0x20;i++)
		cpu_mode_strings[i] = "Unknown";

   InitializeCriticalSection(&crSection);

	cpu_mode_strings[0x10] = "USER";
	cpu_mode_strings[0x11] = "FIQ";
	cpu_mode_strings[0x12] = "IRQ";
	cpu_mode_strings[0x13] = "SUPERVISOR";
	cpu_mode_strings[0x17] = "ABORT";
	cpu_mode_strings[0x1B] = "UNDEFINED";
	cpu_mode_strings[0x1F] = "SYSTEM";

   ResetCpu();
   return 1;
}
//-----------------------------------------------------------------------
void DestroyCpu()
{                                  
   DeleteCriticalSection(&crSection);
}
//-----------------------------------------------------------------------
static u8 FASTCALL ModeToCpuMode(u8 value)
{
   switch(value & 0x1F){
       case 0:
           return USER_MODE;
       case 1:
           return IRQ_MODE;
       case 2:
           return SUPERVISOR_MODE;
       case 3:
           return SYSTEM_MODE;
       case 4:
           return FIQ_MODE;
       case 5:
           return ABORT_MODE;
       case 6:
           return UNDEFINED_MODE;
       default:
           return USER_MODE;
   }
}
//-----------------------------------------------------------------------
static u8 FASTCALL CpuModeToMode(u8 value)
{
   switch((value & 0x1F)){
       case USER_MODE:
       case SYSTEM_MODE:
           return 0;
       case FIQ_MODE:
           return 4;
       case IRQ_MODE:
           return 1;
       case SUPERVISOR_MODE:
           return 2;
       case ABORT_MODE:
           return 5;
       case UNDEFINED_MODE:
           return 6;
    }
    return 0;
}
//-----------------------------------------------------------------------
void SetInterruptSuspend(u16 value)
{
   EnterCriticalSection(&crSection);
   arm.irqSuspend |= value;
   arm.irq |= value;
   if((REG_IME & 1) && (REG_IE & value))
       arm.stop = 0;
   LeaveCriticalSection(&crSection);
}
//-----------------------------------------------------------------------
void SetInterrupt(u16 value)
{
   EnterCriticalSection(&crSection);
   arm.irq |= value;
   if((REG_IME & 1) && (REG_IE & value)){
       arm.stop = 0;
       if(!(CPSR & IRQ_BIT))
           CheckEnterInterrupt(value);
   }
//   else
//       arm.irqSuspend |= value;
   LeaveCriticalSection(&crSection);
}
//-----------------------------------------------------------------------
u8 SwitchCpuMode(u16 value)
{
   u32 *src,*dst,r,ra,lr,*p,*p1;
   u8 mode,irq,index,i;

   src = arm.reg[(mode = arm.mode)];
   index = (u8)((value >> 8) & 0x3F);
   if((value & 0x4000)){
       if((value & 0x1F) == 0)
           value |= ModeToCpuMode(arm.lrv[index].mode);
       if(arm.lrv[index].irq == 'H' && !(CPSR & IRQ_BIT))
           return 0;
   }
   arm.mode = CpuModeToMode((u8)value);
   GP_REG = dst = arm.reg[arm.mode];
#ifdef _DEBPRO
   WriteMessage(MESSAGE_CPU,"CPU switch %s -> %s VCOUNT : %02X SPSR : %u",cpu_mode_strings[ModeToCpuMode(mode)],cpu_mode_strings[ModeToCpuMode(arm.mode)],VCOUNT,src[16]);
#endif
   if((value & 0x8000)){
       r = (CPSR & 0xFFFFFF);
       if(VFLAG)
           r |= V_BIT;
       if(CFLAG)
           r |= C_BIT;
       if(ZFLAG)
           r |= Z_BIT;
       if(NFLAG)
           r |= N_BIT;
       p = dst;p1 = src;
       for(i=0;i<13;i++) *p++ = *p1++;
       dst[16] = r;
       ra = arm.ra;
       lr = 0x18;
       if((CPSR & T_BIT)){
           lr |= 1;
           ra += 2;
       }
       else
           ra += 4;
       arm.lrv[(index = arm.index)].lr = ra;
       arm.lrv[index].ra = arm.ra;
       arm.lrv[index].irq = irq = (u8)((value & 0x2000) ? 'H' : 'S');
       arm.lrv[index].mode = mode;
       CPSR = (CPSR & ~0x1F) | (u8)value;
       if(irq == 'H'){
           dst[13] -= 4;
           WRITEWORD(dst[13],src[0]);
           dst[13] -= 4;
           WRITEWORD(dst[13],src[1]);
           dst[13] -= 4;
           WRITEWORD(dst[13],src[2]);
           dst[13] -= 4;
           WRITEWORD(dst[13],src[3]);
           dst[13] -= 4;
           WRITEWORD(dst[13],src[12]);
           dst[13] -= 4;
           WRITEWORD(dst[13],src[14]);
           GP_REG[14] = lr;
       }
       else{
           dst[13] -= 4;
           WRITEWORD(dst[13],dst[16]);
           dst[13] -= 4;
           WRITEWORD(dst[13],src[11]);
           dst[13] -= 4;
           WRITEWORD(dst[13],src[12]);
           dst[13] -= 4;
           WRITEWORD(dst[13],src[14]);
           dst[13] -= 4;
           WRITEWORD(dst[13],src[2]);
           GP_REG[14] = lr;
           arm.mode = CpuModeToMode(SYSTEM_MODE);
           CPSR = (CPSR & ~0x1F) | SYSTEM_MODE;
           GP_REG = arm.reg[arm.mode];
           p = GP_REG;p1 = dst;
           for(i=0;i<13;i++) *p++ = *p1++;
           GP_REG[14] = lr;
           dst[16] = r;
       }
       arm.index++;
   }
   else if((value & 0x4000)){
       if(arm.lrv[index].irq == 'H'){
           p = dst;p1 = src;
           for(i=0;i<13;i++) *p++ = *p1++;
           dst[14] = READWORD(src[13]);
           src[13] += 4;
           dst[12] = READWORD(src[13]);
           src[13] += 4;
           dst[3] = READWORD(src[13]);
           src[13] += 4;
           dst[2] = READWORD(src[13]);
           src[13] += 4;
           dst[1] = READWORD(src[13]);
           src[13] += 4;
           dst[0] = READWORD(src[13]);
           src[13] += 4;
           r = src[16];
       }
       else{
           p = dst;p1 = src;
           for(i=0;i<13;i++) *p++ = *p1++;
           dst[2] = READWORD(arm.reg[2][13]);
           arm.reg[2][13] += 4;
           dst[14] = READWORD(arm.reg[2][13]);
           arm.reg[2][13] += 4;
           dst[12] = READWORD(arm.reg[2][13]);
           arm.reg[2][13] += 4;
           dst[11] = READWORD(arm.reg[2][13]);
           arm.reg[2][13] += 4;
           r = arm.reg[2][16] = READWORD(arm.reg[2][13]);
           arm.reg[2][13] += 4;
       }
       CPSR = (r & 0xFFFFFF);
       VFLAG = (r & V_BIT) ? 1 : 0;
       CFLAG = (r & C_BIT) ? 1 : 0;
       ZFLAG = (r & Z_BIT) ? 1 : 0;
       NFLAG = (r & N_BIT) ? 1 : 0;
       arm.lrv[index].lr = 0;
       arm.lrv[index].irq = 0;
       arm.lrv[index].mode = 0;
       arm.index--;
       dst[15] = src[15];
   }
   else{
       dst[15] = src[15];
//       dst[14] = src[14];
       CPSR = (CPSR & ~0x1F) | (u8)value;
       p = dst;p1 = src;
       for(i=0;i<13;i++) *p++ = *p1++;
   }

#ifdef _DEBPRO
   //WriteMessage(MESSAGE_CPU,"Exit : CPU %s",cpu_mode_strings[ModeToCpuMode(arm->mode)]);
#endif
   return 1;
}
//-----------------------------------------------------------------------
void ResetTimer()
{
   u8 i;
   PTIMER p;

   for(i=0;i<4;i++){
       p = &timer[i];
       p->Control = &io_ram_u16[0x81 + (i << 1)];
       p->Count = &io_ram_u16[0x80 + (i << 1)];
       p->CountOffset = (u16)(0x100 + (i << 2));
       p->Cascade = 0;
       p->Value = 0;
       p->Irq = 0;
       p->Freq = 1;
       p->ResetValue = 0;
       p->Enable = 0;
       p->Index = i;
       p->Diff = 0;
       p->Remainder = 0;
   }
}
//-----------------------------------------------------------------------
void RenderTimer(void)
{
   u8 i,ovr;
   u16 res,div;
   PTIMER p;

   ovr = 0;
   for(i=0;i<4;i++){
       if(!(p = &timer[i])->Enable)
           continue;
       if(p->Cascade){
           if(!ovr)
               continue;
           res = *p->Count;
           (*p->Count) += ovr;
           if(*p->Count < res){
               *p->Count += p->ResetValue;
               if(p->Irq)
                   SetInterrupt((u16)(1 << (i + 3)));
               ovr = 1;
           }
           else
               ovr = 0;
       }
       else{
           p->Value += arm.timerCycles;
           if(p->Value < (u32)p->Freq)
               continue;
           res = *p->Count;
           *p->Count += (div = (u16)(p->Value / p->Freq));
           p->Value -= (p->Freq * div);
           if(*p->Count < res){
               if(p->Irq)
                   SetInterrupt((u16)(1 << (i + 3)));
               p->Remainder += *p->Count;
               *p->Count = p->ResetValue;
               ovr = 1;
               if(p->Remainder >= p->Diff && p->Diff >= 2){
                   p->Remainder -= (u16)((div = (u16)(p->Remainder / p->Diff)) * p->Diff);
                   p->Inc += (u8)div;
                   ovr += (u8)div;
               }
               if(++p->Inc >= 16){
                   p->Inc -= (u8)16;
                   if(fifo[0].Enable > 0 && fifo[0].Dma != 0 && fifo[0].Timer == i)
                       ExecDMA(1);
                   if(fifo[1].Enable > 0 && fifo[1].Dma != 0 && fifo[1].Timer == i)
                       ExecDMA(2);
               }
           }
           else
               ovr = 0;
       }
   }
   arm.timerCycles = 0;
}
//---------------------------------------------------------------------------
void ResetDMA()
{
   u8 i,offset;
   LPDMA p;

   for(i=0;i<4;i++){
       p = &dma[i];
       ZeroMemory(p,sizeof(DMA));
       offset = (u8)(0xB0 + (i * 12));
       p->Source = (u32 *)(&io_ram_u8[offset]);
       p->offSrc = offset;
       offset += (u8)4;
       p->Dest = (u32 *)(&io_ram_u8[offset]);
       p->offDst = offset;
       offset += (u8)4;
       p->Control = (u32 *)(&io_ram_u8[offset]);
       p->Index = i;
       p->MaxCount = 0x3FFF;
   }
   dma[3].MaxCount = 0xFFFF;
}
//---------------------------------------------------------------------------
void ExecDMA(u8 i)
{
   LPDMA p;
   int i1;

   p = &dma[i];
#ifdef _DEBPRO
   if(!IsMessageEnable((i1 = i + MESSAGE_DMA0)))
       i1 = MESSAGE_DMA;
   WriteMessage(i1,"DMA %1d D:0x%08X S:0x%08X C:0x%04X R:%1d L:%1d I:%1d M:%2d",i,p->Dst,p->Src,
           p->InternalCount,p->Repeat,p->Reload,p->Irq,p->Mode != 0 ? 32 : 16);
#endif
   if(p->Enable == 0)
       return;
   arm.useDMA = 1;
	if(p->Mode != 0){
       p->Dst &= ~3;
       p->Src &= ~3;
	    for (i1 = p->InternalCount;i1 > 0;i1--){
		    WRITEWORD(p->Dst,READWORD(p->Src));
           p->Dst += p->IncD;
           p->Src += p->IncS;
       }
   }
	else{
       p->Dst &= ~1;
       p->Src &= ~1;
	    for(i1 = p->InternalCount;i1 > 0;i1--){           //8003f0c
		    WRITEHWORD(p->Dst,READHWORD(p->Src));
           p->Dst += p->IncD;
           p->Src += p->IncS;
       }
   }
   if(!p->Repeat){
       p->Enable = 0;
       *p->Control &= 0x7FFFFFFF;
       p->InternalCount = (u16)(*p->Control & p->MaxCount);
       p->Dst = *p->Dest;
       p->Src = *p->Source;
   }
   if(p->Reload != 0){
       p->Dst = *p->Dest;
//       p->Src = *p->Source;
       p->InternalCount = (u16)(*p->Control & p->MaxCount);
       p->Enable = 1;
   }
   if(p->Irq != 0)
       SetInterrupt((u16)(0x100 << i));
   arm.useDMA = 0;       
}
//---------------------------------------------------------------------------
void SetDMAGBA(u8 i,u16 adress)
{
   LPDMA p;
   u16 cnt;
   u8 bEnable;

   p = &dma[i];
   bEnable = p->Enable;
   p->Enable = (u8)((cnt = (u16)(*p->Control >> 16)) >> 15);
   p->Irq = (u8)((cnt >> 14) & 1);
   p->Repeat = (u8)((cnt >> 9) & 1);
   if(!p->Enable && bEnable && p->Repeat){
       p->Dst = *p->Dest;
       p->Src = *p->Source;
   }
   p->Reload = 0;
   switch((cnt & 0x60) >> 5){
       case 0:
           p->IncD = 4;
       break;
       case 1:
           p->IncD = -4;
       break;
		case 2:
           p->IncD = 0;
       break;
       case 3:
           p->Reload = 1;
           p->IncD = 4;
       break;
   }
   switch((cnt & 0x180) >>7){
       case 0:
           p->IncS = 4;
       break;
       case 1:
           p->IncS = -4;
       break;
		default:
           p->IncS = 0;
       break;
   }
   if((p->Start = (u8)((cnt >> 12) & 3)) == 3){
       switch(i){
           case 1:
           case 2:
               p->InternalCount = 4;
               p->IncD = 0;
               p->Mode = 1;
               i--;
               if(fifo[i].Enable != 0){
                   fifo[i].Dma = 1;
                   CalcFifoFreq(&timer[fifo[i].Timer]);
               }
           break;
           default:
           break;
       }
   }
   else{
       if((p->InternalCount = (u16)(*p->Control & p->MaxCount)) == 0)
           p->InternalCount = p->MaxCount;
       p->Mode = (u8)((cnt >> 10) & 1);
   }
   if(!p->Mode){
       p->IncS >>= 1;
       p->IncD >>= 1;
   }
   if(adress == p->offDst || p->Start == 0)
       p->Dst = *p->Dest;
   if(adress == p->offSrc || p->Start == 0)
       p->Src = *p->Source;
   if(p->Enable && p->Start == 0){
       if((p->Index == 1 && fifo[0].Dma) || (p->Index == 2 && fifo[1].Dma))
           return;
       ExecDMA(i);
   }
}
//---------------------------------------------------------------------------
void DestroyFifo()
{
   if(fifo == NULL)
       return;
   if(fifo[0].lpBuffer != NULL)
       GlobalFree((HGLOBAL)fifo[0].lpBuffer);
   fifo[0].lpBuffer = NULL;
   fifo[1].lpBuffer = NULL;
}
//---------------------------------------------------------------------------
u8 InitFifo()
{
   fifo = pcm.fifo;
   gbcch = pcm.gbcch;
   DestroyFifo();
   if((fifo[0].lpBuffer = (u8 *)GlobalAlloc(GMEM_FIXED,FIFOBUFFER_SIZE * 2)) == NULL)
       return 0;
   fifo[0].Inc = 0;
   fifo[1].lpBuffer = fifo[0].lpBuffer + FIFOBUFFER_SIZE;
   fifo[1].Inc = 0;
   ResetFifo();
   return 1;
}
//---------------------------------------------------------------------------
void ResetFifo()
{
   HMENU hMenu;
   MENUITEMINFO mii;
   u8 i;

   pcm.Enable = 1;
   pcm.Freq = 32768;
   fifo = pcm.fifo;

   for(i=0;i<2;i++){
       fifo[i].Enable = (u8)((fifo[i].Enable & 0x80) | 3);
       fifo[i].Dma = 0;
       fifo[i].Resample = 0;
       fifo[i].Pos = 0;
       fifo[i].Index = i;
       fifo[i].Freq = 0;
       fifo[i].Timer = 0;
       fifo[i].Inc = 0;
       fifo[i].Volume = 0;
       fifo[i].Start = 0;
       fifo[i].Max = 4;
       fifo[i].lpAddress = &io_ram_u8[0xa0 + (i << 2)];
   }
   ZeroMemory(fifo[0].lpBuffer,FIFOBUFFER_SIZE * 2);

   hMenu = GetMenu(hWin);
   ZeroMemory(&mii,sizeof(MENUITEMINFO));
   mii.cbSize = sizeof(MENUITEMINFO);
   mii.fMask = MIIM_STATE;
   GetMenuItemInfo(hMenu,ID_SOUND,FALSE,&mii);
   if(mii.fState != MFS_CHECKED)
       pcm.Enable |= 0x80;
}
//---------------------------------------------------------------------------
void SetFifoFreq(u8 i,u32 value)
{
   LPFIFOCHANNEL p;

   p = &fifo[i];
   if(p->Freq != value){
       p->Freq = value;
       p->Pos = 0;
       if(value < 65536){
           if((p->Resample = (((float)value / (float)pcm.hz) * 256.0)) < 1)
               p->Resample = 1;
       }
       else
           p->Resample = 256;
       p->MaxPos = FIFOBUFFER_SIZE - ceil((10.0 / p->Resample));
   }
}
//---------------------------------------------------------------------------
void SetTimerCount(LPTIMER p)
{
   p->ResetValue = *p->Count;
   p->Diff = (u16)(65536 - p->ResetValue);               //802fefA
   p->Inc = 0;
   p->Remainder = 0;
   p->Value = 0;
   switch(p->Index){
       case 0:
       case 1:
           CalcFifoFreq(p);
       break;
       default:
       break;
   }
}
//---------------------------------------------------------------------------
void SetTimerGBA(LPTIMER p)
{
   u16 Control;

#ifdef _DEBPRO
   WriteMessage(MESSAGE_TIMER,"Timer %d",p->Index);
#endif
   p->Enable = (u8)((Control = *p->Control) >> 7);
   p->Irq = (u8)((Control & 0x40) >> 6);
   p->Cascade = (u8)((Control & 0x4) >> 2);
   switch((Control & 0x3)){
       case 0:
           p->Freq = 1;
       break;
       case 1:
           p->Freq = 64;
       break;
       case 2:
           p->Freq = 256;
       break;
       case 3:
           p->Freq = 1024;
       break;
   }
   if(p->Index < 2)
       CalcFifoFreq(p);
}
//---------------------------------------------------------------------------
void CalcFifoFreq(PTIMER p)
{
   u8 i;
   float f;

   f = ((float)CPU_FREQ / (float)p->Freq);
   if(fifo[0].Timer == (i = p->Index))
       SetFifoFreq(0,f / (float)(65536 - p->ResetValue));
   if(fifo[1].Timer == i)
       SetFifoFreq(1,f / (float)(65536 - p->ResetValue));
}




