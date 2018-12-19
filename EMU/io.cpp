#include "io.h"
#include "pluginctn.h"
#ifdef _DEBPRO
#include "debug.h"
#endif
//---------------------------------------------------------------------------
void standard_io_write_handle(u16 adress,u8 accessMode)
{
}
//---------------------------------------------------------------------------
void wait_io_handle(u16 adress,u8 accessMode)
{
   u8 ws0[]={4,3,2,8};
   u8 ws1[][2]={{2,1},{4,1},{8,1}};

   arm.ws[0].value = MAKEWORD(ws1[0][(WAITCNT >> 4) & 1]+1,ws0[(WAITCNT >> 2) & 3]+1);
   arm.ws[1].value = MAKEWORD(ws1[1][(WAITCNT >> 7) & 1]+1,ws0[(WAITCNT >> 5) & 3]+1);
   arm.ws[2].value = MAKEWORD(ws1[2][(WAITCNT >> 10) & 1]+1,ws0[(WAITCNT >> 8) & 3]+1);
   arm.ws[3].value = MAKEWORD(0,ws0[WAITCNT & 3]+1);
}
//---------------------------------------------------------------------------
void power_mode(u16 adress,u8 accessMode)
{
   arm.stop = (u8)(io_ram_u8[0x301] & 0x80 ? 2 : 1);
#ifdef _DEBPRO
   WriteMessage(MESSAGE_POWER,"Power Mode Mode %d",arm.stop);
#endif
}
//---------------------------------------------------------------------------
void if_io_handle(u16 adress,u8 accessMode)
{
   ::EnterCriticalSection(&crSection);
   arm.irqSuspend &= (u16)~REG_IF;
 	arm.irq &= (u16)~REG_IF;
   REG_IF = 0;
   ::LeaveCriticalSection(&crSection);
}
//---------------------------------------------------------------------------
void ime_io_handle(u16 adress,u8 accessMode)
{
//   REG_IME &= 1;
}
//---------------------------------------------------------------------------
void ie_io_handle(u16 adress,u8 accessMode)
{
   if(accessMode == AMM_WORD)
       if_io_handle(adress,accessMode);
}
//---------------------------------------------------------------------------
void com_io_handle(u16 adress,u8 accessMode)
{
   DWORD w;
   SioPlugList *pSioPlugList;

   if((pSioPlugList = pPlugInContainer->GetSioPlugInList()) != NULL && pSioPlugList->Count() > 0){
       w = 1 << (adress - 0x120);
       if(accessMode == AMM_HWORD)
           w |= (w << 1);
       if(accessMode == AMM_WORD)
           w |= (w << 1) | (w << 2);
       if((w & 0x100) && (SCCNT_L & 0x80))
           arm.irqSuspend &= ~0x80;
       if(pSioPlugList->Run(w) > 0)
           return;
   }
   if((SCCNT_L & 0x80))
       RCNT = 0;
   SCCNT_L &= ~0x80;
   if(SCD0)
       SCD0 |= 1;
}
//---------------------------------------------------------------------------
void joy_io_handle(u16 adress,u8 accessMode)
{
/*   int i;

   i = JOYCNT;*/
}
//---------------------------------------------------------------------------
void reg_keyinput_write(u16 adress,u8 accessMode)
{
   KEYINPUT = KeyField;
}
//---------------------------------------------------------------------------
void reg_dm3cnt_write(u16 adress,u8 accessMode)
{
   SetDMAGBA(3,adress);
}
//---------------------------------------------------------------------------
void reg_dm1cnt_write(u16 adress,u8 accessMode)
{
   SetDMAGBA(1,adress);
}
//---------------------------------------------------------------------------
void reg_dm2cnt_write(u16 adress,u8 accessMode)
{
   SetDMAGBA(2,adress);
}
//---------------------------------------------------------------------------
void reg_dm0cnt_write(u16 adress,u8 accessMode)
{
   SetDMAGBA(0,adress);
}
//---------------------------------------------------------------------------
void exec_dma(u16 value)
{
   LPDMA p;
   u8 i;

   p = dma;
   for(i=0;i<4;i++,p++){
       if(!p->Enable || p->Start != value)
           continue;
       ExecDMA(i);
       arm.bEnableSpeedCPU = 0;
   }
}
//---------------------------------------------------------------------------
void timer_io_handle(u16 adress,u8 accessMode)
{
   LPTIMER p;

   p = &timer[(adress - 0x100) >> 2];
   switch(accessMode){
       case AMM_WORD:
           SetTimerGBA(p);
           SetTimerCount(p);
       break;
       default:
           if((adress - p->CountOffset) != 0)
               SetTimerGBA(p);
           else
               SetTimerCount(p);
       break;
   }
}


