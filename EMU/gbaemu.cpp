#include <windows.h>
#pragma hdrstop
#if defined(__BORLANDC__)
#include <condefs.h>
#endif
#include "gbaemu.h"
#include "memory.h"
#include "graphics.h"
#include "exec.h"
#include "opcodes.h"
#include "topcodes.h"
#include "io.h"
#include "opedec.h"
#include "tables.h"
#include "ttables.h"
#include "sound.h"
#include "sprite.h"
#include "gba.h"
#include "debug.h"
#include "lcd.h"
#include "input.h"      
#include "bios.h"
#include "pluginctn.h"
#include "cheat.h"
//---------------------------------------------------------------------------
BINARY_IMAGE bin;
CPUEXEC exec;
ARMOPCODE opcode_handles[0x1002];
THUMBOPCODE opcode_handles_t[0x1002];
IOROUTINE io_write_handles[0x400];

static u8 nSkip;
u8 nSkipMaster;
static int iSubCodeError;
static DWORD cpuCycles[229];
static SioPlugList *pSioPlugList;
//---------------------------------------------------------------------------
int FASTCALL log2(u32 value)
{
   signed char i;
   u32 pow;

   if(!value)
       return 0;
   pow = 1 << 31;
   for(i=31;i>=0;i--,pow >>= 1){
       if(value < pow)
           continue;
       else
           return i;
   }
   return 0;
}
//---------------------------------------------------------------------------
int init_gbaemu()
{
	int i;
   u32 value;

   ZeroMemory(&bin,sizeof(BINARY_IMAGE));
/*   iSubCodeError = -1;
   if((opcode_handles_t = (THUMBOPCODE *)GlobalAlloc(GMEM_FIXED,sizeof(THUMBOPCODE) * 0x402)) == NULL)
       return 0;
   iSubCodeError = -2;
   if((opcode_handles = (ARMOPCODE *)GlobalAlloc(GMEM_FIXED,sizeof(ARMOPCODE) * 0x1002)) == NULL)
       return 0;
   iSubCodeError = -3;
   if((io_write_handles = (IOROUTINE *)GlobalAlloc(GMEM_FIXED,sizeof(IOROUTINE) * 0x400)) == NULL)
       return 0;*/
   iSubCodeError = -4;
 	if(!InitCpu())
       return 0;
   iSubCodeError = -5;
   if(!InitMmu())
       return 0;
   iSubCodeError = -6;
   if(!InitSprite())
       return 0;
   iSubCodeError = -7;
   if(!InitFifo())
       return 0;
   iSubCodeError = TE_LCD;
   if(!InitLcd())
       return 0;
   iSubCodeError = TE_SOUND;
   if(!EnableSound())
       return 0;
   iSubCodeError = TE_INPUT;
   if(!InitInput()){
       if(GetInputSubCodeError() > -2)
           return 0;
   }
   if(!InitSio())
       return 0;
   if(!InitCheatSystem())
       return 0;
   iSubCodeError = 0;
	setup_tables();
   nSkipMaster = 0;
   value = LINE_CYCLES;
   for(i=1;i<230;i++){
   	cpuCycles[i-1] = value >> CYCLES_SHIFT;
       value += LINE_CYCLES;
   }
   return 1;
}
//---------------------------------------------------------------------------
int GetMainSubCodError()
{
   return iSubCodeError;
}
//---------------------------------------------------------------------------
int SettaVelocita(u8 value)
{
   HMENU hMenu;
   UINT item[5];
   int i;

   if(value > 10)
       return nSkipMaster;
   nSkipMaster = value;
   nSkip = 0;
   hMenu = GetMenu(hWin);
   for(i=0;i<5;i++)
       item[i] = MF_UNCHECKED;
   switch(value){
       case 0:
           item[0] = MF_CHECKED;
       break;
       case 1:
           item[1] = MF_CHECKED;
       break;
       case 2:
           item[2] = MF_CHECKED;
       break;
       case 3:
           item[3] = MF_CHECKED;
       break;
       case 4:
           item[4] = MF_CHECKED;
       break;
   }
   CheckMenuItem(hMenu,ID_SKIP_0FRAME,MF_BYCOMMAND|item[0]);
   CheckMenuItem(hMenu,ID_SKIP_1FRAME,MF_BYCOMMAND|item[1]);
   CheckMenuItem(hMenu,ID_SKIP_2FRAME,MF_BYCOMMAND|item[2]);
   CheckMenuItem(hMenu,ID_SKIP_3FRAME,MF_BYCOMMAND|item[3]);
   CheckMenuItem(hMenu,ID_SKIP_4FRAME,MF_BYCOMMAND|item[4]);

   return value;
}
//---------------------------------------------------------------------------
void reset_gbaemu ()
{
   ZeroMemory(translated_palette,0x400);
   ResetCpu();
   ResetMmu();
   ResetTimer();
   ResetDMA();
   ResetFifo();
   ResetLcd();
   ResetSprite();
   sound_reset();
   ResetSio();
   ResetCheatSystem();
   REG_IF = 0;
   WAITCNT = 0x80;
   RCNT = 0x8000;
   SGCNT0_H = 0x3300;
   DISPCNT = 0x80;
   BG3PA = BG3PD = BG2PA = BG2PD = 0x100;
   if(SkipBiosIntro(TRUE))
       REG_PC = 4;
   else
	    REG_PC = 0x08000004;
   advance_instruction_pipe();
	KEYINPUT = 0xFFFF;
   KeyField = 0x3FF;
   nSkip = 0;
   pSioPlugList = pPlugInContainer->GetSioPlugInList();
}
//---------------------------------------------------------------------------
void setup_tables (void)
{
	setup_handle_tables();
	setup_io_handle_tables();
	setup_handle_tables_t();
#ifdef _DEBUG
	setup_debug_handles();
	setup_string_tables();
	setup_debug_handles_t();
	setup_string_tables_t();
#endif
}
//---------------------------------------------------------------------------
void CheckExitInterrupt(void)
{
   u8 i;
   u16 irq;
#ifdef _DEBPRO
   char riga[20];

   EnterCriticalSection(&crSection);
   if(bDebug){
       ControlBreakOn(BREAK_EXIT_IRQ1 + log2(0) - 1);
       wsprintf(riga,"X%d",0);
       SendDlgItemMessage(hwndDebug,IDC_IRQRET,WM_SETTEXT,0,(LPARAM)riga);
//       WriteMessage(MESSAGE_IRQ,"%s %0X",irq_strings[n],VCOUNT);
   }
#else
   EnterCriticalSection(&crSection);
#endif
   REG_PC = arm.lrv[i = (u8)(arm.index - 1)].lr;
   arm.ra = arm.lrv[i].ra;
	SwitchCpuMode((u16)(0x4000|((i & 0x3F) << 8)));
   if(arm.irq){
       i = 16;
       if(!(CPSR & IRQ_BIT) && (REG_IME & 1)){
           for(i = 0,irq=1;i < 16;i++,irq <<= 1){
               if(!(arm.irq & irq) || !(REG_IE & irq))
                   continue;
               CheckEnterInterrupt(irq);
               break;
           }
       }
       if(i != 16) goto ex_CheckExitInterrupt;
   }
   if(!arm.index && arm.IntrWait && !CheckIntrWait()){
       WRITEBYTE(0x04000301,0);
       goto ex_CheckExitInterrupt;
   }
   if((CPSR & T_BIT)){
       SetExecFunction(1);
       tadvance_instruction_pipe();
       goto ex_CheckExitInterrupt;
   }
   SetExecFunction(0);
   advance_instruction_pipe();
ex_CheckExitInterrupt:
   LeaveCriticalSection(&crSection);
}
//---------------------------------------------------------------------------
void CheckEnterInterrupt(u16 r)
{
#ifdef _DEBPRO
   char riga[100];
#endif
   SwitchCpuMode(0xA000 | IRQ_BIT | IRQ_MODE);
   CPSR &= ~T_BIT;
   REG_PC = (u32)INTR_VCR_BUF + 4;
#ifdef _DEBPRO
   if(bDebug){
       ControlBreakOn(BREAK_ENTER_IRQ1 + (r != 1 ? log2(r) : 0));
       wsprintf(riga,"E%02d",r);
       SendDlgItemMessage(hwndDebug,IDC_IRQRET,WM_SETTEXT,0,(LPARAM)riga);
       WriteMessage(MESSAGE_IRQ,"Enter %s Raster Line %02X",irq_strings[log2(r)],VCOUNT);
   }
#endif
   SetExecFunction(0);
   advance_instruction_pipe();
   REG_IF = r;
   arm.irqSuspend &= (u16)~r;
}
//---------------------------------------------------------------------------
void SwapBuffer(u8 xStart,u8 xEnd)
{
   if(lcd.winObj.Enable)
       DrawLineObjWindow(xStart,xEnd);
   else
       lcd.swapBuffer[lcd.iBlend](xStart,xEnd);
}
//---------------------------------------------------------------------------
void CheckSuspendInterrupt(void)
{
   u16 i,irq;

   EnterCriticalSection(&crSection);
   if(!(CPSR & IRQ_BIT) && (REG_IME & 1)){
       for(i = 0,irq=1;i < 16;i++,irq <<= 1){
           if(!(arm.irqSuspend & irq) || !(REG_IE & irq))
               continue;
           CheckEnterInterrupt(irq);
           break;
       }
   }
   LeaveCriticalSection(&crSection);
}
//---------------------------------------------------------------------------
void run_frame(void)
{
   u32 n,n1;
   u8 bSkip,res;
	DWORD *p;

   lcd.pBuffer = screen;
   if(lcd.layers[2].Enable != 0){
       lcd.layers[2].CurrentX = lcd.layers[2].bgrot[0];
       lcd.layers[2].CurrentY = lcd.layers[2].bgrot[1];
   }
   if(lcd.layers[3].Enable != 0){
       lcd.layers[3].CurrentX = lcd.layers[3].bgrot[0];
       lcd.layers[3].CurrentY = lcd.layers[3].bgrot[1];
   }
   p = cpuCycles;
   if(nSkip > nSkipMaster || !nSkipMaster)
       nSkip = bSkip = 0;
   else
       bSkip = 1;
   pSioPlugList->NotifyState(0x300000,0x300000);
	for(VCOUNT = 0;VCOUNT < 161;VCOUNT++){
       pSioPlugList->NotifyState(0x100000,0x100000);
       if((DISPSTAT >> 8) == VCOUNT){
           if((DISPSTAT & 0x20)){
               arm.bEnableSpeedCPU = 0;
               SetInterrupt(4);
           }
           DISPSTAT |= 4;
           lcd.blankBit |= 4;
       }
       else{
           DISPSTAT &= ~4;
           lcd.blankBit &= ~4;
       }
       DISPSTAT &= ~3;
       lcd.blankBit &= ~3;
       n1 = (n = *p++) - 272;
       lcd.pCurrentZBuffer = lcd.pZBuffer;
       lcd.pCurSB = (u16 *)lcd.SourceBuffer;
       lcd.pCurOB = (u16 *)lcd.SpriteBuffer;
       for(;cpu_cycles < n1;){
           if(!arm.stop){
               res = exec();
               if(arm.index > 0 && REG_PC <= 0x20)
                   CheckExitInterrupt();
               else if(arm.irqSuspend)
                   CheckSuspendInterrupt();
           }
           else
               res = 32;
           if(arm.bEnableSpeedCPU)
               res += arm.bInc;
//           if(arm.nPluginCallback)
//               pPlugInContainer->IncCycles((int)res);
           cpu_cycles += res;
           pcm.cycles += res;
           arm.timerCycles += res;
       }
       DISPSTAT |= 2;
       lcd.blankBit |= 2;
       if((DISPSTAT & 16))
           SetInterrupt(2);
       if(lcd.Enable && !bSkip)
           lcd.Render();
       exec_dma(2);
       for(;cpu_cycles < n;){
           if(!arm.stop){
               res = exec();
               if(arm.index > 0 && REG_PC <= 0x20)
                   CheckExitInterrupt();
               else if(arm.irqSuspend)
                   CheckSuspendInterrupt();
           }
           else
               res = 32;
           if(arm.bEnableSpeedCPU)
               res += arm.bInc;
//           if(arm.nPluginCallback)
//               pPlugInContainer->IncCycles((int)res);
           cpu_cycles += res;
           pcm.cycles += res;
           arm.timerCycles += res;
       }
       RenderTimer();
       pcm.fpSoundMix(0);
   }
   Acquire();
   if(!bSkip)
       BlitFrame(NULL);
   DISPSTAT |= 0x1;
   lcd.blankBit |= 1;
   arm.bEnableSpeedCPU = 1;
   exec_dma(1);
   UpdateFifo();
   if((DISPSTAT & 8))
       SetInterrupt(1);
   pSioPlugList->NotifyState(0x100000,0x100000);
   DISPSTAT &= ~2;
   lcd.blankBit &= ~2;
   n1 = (n = *p++) - 272;
   for(;cpu_cycles < n1;){
       if(!arm.stop){
           res = exec();
           if(arm.index > 0 && REG_PC <= 0x20)
               CheckExitInterrupt();
           else if(arm.irqSuspend)
               CheckSuspendInterrupt();
       }
       else
           res = 32;
       if(arm.bEnableSpeedCPU)
           res += arm.bInc;
//       if(arm.nPluginCallback)
//           pPlugInContainer->IncCycles((int)res);
       cpu_cycles += res;
       pcm.cycles += res;
       arm.timerCycles += res;
   }
   if((DISPSTAT >> 8) == VCOUNT){
       if((DISPSTAT & 0x20)){
           arm.bEnableSpeedCPU = 0;
           SetInterrupt(4);
       }
       DISPSTAT |= 4;
       lcd.blankBit |= 4;
   }
   else{
       DISPSTAT &= ~4;
       lcd.blankBit &= ~4;
   }
   if((DISPSTAT & 16))
       SetInterrupt(2);
   DISPSTAT |= 2;
   lcd.blankBit |= 2;
   for(;cpu_cycles < n;){
       if(!arm.stop){
           res = exec();
           if(arm.index > 0 && REG_PC <= 0x20)
               CheckExitInterrupt();
           else if(arm.irqSuspend)
               CheckSuspendInterrupt();
       }
       else
           res = 32;
       if(arm.bEnableSpeedCPU)
           res += arm.bInc;
//       if(arm.nPluginCallback)
//           pPlugInContainer->IncCycles((int)res);
       cpu_cycles += res;
       pcm.cycles += res;
       arm.timerCycles += res;
   }
   RenderTimer();
   pcm.fpSoundMix(0);
   for(VCOUNT++;VCOUNT < 228;VCOUNT++){
       pSioPlugList->NotifyState(0x100000,0x100000);
       if((DISPSTAT >> 8) == VCOUNT){
           if((DISPSTAT & 0x20)){
               arm.bEnableSpeedCPU = 0;
               SetInterrupt(4);
           }
           DISPSTAT |= 4;
           lcd.blankBit |= 4;
       }
       else{
           DISPSTAT &= ~4;
           lcd.blankBit &= ~4;
       }
       DISPSTAT &= ~2;
       lcd.blankBit &= ~2;
       n1 = (n = *p++) - 272;
       for(;cpu_cycles < n1;){
           if(!arm.stop){
               res = exec();
               if(arm.index > 0 && REG_PC <= 0x20)
                   CheckExitInterrupt();
               else if(arm.irqSuspend)
                   CheckSuspendInterrupt();
           }
           else
               res = 32;
           if(arm.bEnableSpeedCPU)
               res += arm.bInc;
           cpu_cycles += res;
           pcm.cycles += res;
           arm.timerCycles += res;
       }
       DISPSTAT |= 2;
       lcd.blankBit |= 2;
       if((DISPSTAT & 16))
           SetInterrupt(2);
       for(;cpu_cycles < n;){
           if(!arm.stop){
               res = exec();
               if(arm.index > 0 && REG_PC <= 0x20)
                   CheckExitInterrupt();
               else if(arm.irqSuspend)
                   CheckSuspendInterrupt();
           }
           else
               res = 32;
           if(arm.bEnableSpeedCPU)
               res += arm.bInc;
//           if(arm.nPluginCallback)
//               pPlugInContainer->IncCycles((int)res);
           cpu_cycles += res;
           pcm.cycles += res;
           arm.timerCycles += res;
       }
       RenderTimer();
       pcm.fpSoundMix(0);
   }
   nSkip++;
   KEYINPUT = KeyField;
   if(cpu_cycles > *p)
       return;
   cpu_cycles -= p[-1];
   lcd.iFps++;
   if(lcd.iFps == 7){
       cpu_cycles += 3;
       lcd.iFps = 0;
   }

}
//---------------------------------------------------------------------------
void clean_up (void)
{
/*   if(io_write_handles != NULL)
       GlobalFree((HGLOBAL)io_write_handles);
   io_write_handles = NULL;
/*   if(opcode_handles != NULL)
       GlobalFree((HGLOBAL)opcode_handles);
   opcode_handles = NULL;
   if(opcode_handles_t != NULL)
       GlobalFree((HGLOBAL)opcode_handles_t);
   opcode_handles_t = NULL;*/
   DestroyInput();
   DestroyFifo();
   DestroySprite();
   DestroyMmu();
   DestroyLcd();
   DestroySio();
   DestroyCpu();
   DestroyCheatSystem();
}
//---------------------------------------------------------------------------
void advance_instruction_pipe()
{
   u32 i;

   if(bDataAbort){
       bDataAbort = FALSE;
       GP_REG[(OPCODE >> 12) & 0xF] = ValueDataAbort;
   }
   i = LastDataRead;
   OPCODE = READWORD((arm.ra = REG_PC - 4));
   LastDataRead=i;
   REG_PC += 4;
   arm.memCyclesPipe.value = arm.memCycles.value;
   if(arm.bRefillPipe){
       arm.bRefillPipe = 0;
       cycP += (u8)(cycS + cycN);
   }
}
//---------------------------------------------------------------------------
void tadvance_instruction_pipe()
{
   u32 i;

   if(bDataAbort){
       bDataAbort = FALSE;
       GP_REG[OPCODE_T & 0x7] = ValueDataAbort;
   }
   i = LastDataRead;OPCODE_T = READHWORD((arm.ra = REG_PC - 2));LastDataRead = i;
	REG_PC += 2;
   arm.memCyclesPipe.value = arm.memCycles.value;
   if(arm.bRefillPipe){
       arm.bRefillPipe = 0;
       cycP += (u8)(cycS + cycN);
   }
}
//---------------------------------------------------------------------------

