//---------------------------------------------------------------------------
#include <windows.h>
#pragma hdrstop

#include "sio.h"
#include "trad.h"
#include "gbaemu.h"
#include "memory.h"
#include "pluginctn.h"

//---------------------------------------------------------------------------
static void WINAPI SioCallbackEx(LPSERIALIO p);
static BOOL bTrdRun,bThreadRun1;
static LPSERIALIO mem1;
//---------------------------------------------------------------------------
static LPSERIALIO InitSERIALIO(LPSERIALIO p1,DWORD mask)
{
   SioPlugList *pSioPlugList;

   if(p1 == NULL)
       p1 = (LPSERIALIO)GlobalAlloc(GPTR|GMEM_DDESHARE,sizeof(SERIALIO));
   if(p1 == NULL)
       return NULL;
   p1->REG_CNT.value = RCNT;
   p1->CNT.value = SCCNT_L;
   p1->d0 = SCD0;
   p1->d1 = SCD1;
   p1->d2 = SCD2;
   p1->d3 = SCD3;
   p1->DATA = SCCNT_H;
   p1->mMask = mask;
   if((pSioPlugList = pPlugInContainer->GetSioPlugInList()) != NULL)
       pSioPlugList->GetCallback(p1->dwCallback);
   return p1;
}
//---------------------------------------------------------------------------
SioPlug::SioPlug() : PlugIn()
{
   isASync = 0;
}
//---------------------------------------------------------------------------
int SioPlug::GetInfo(LPPLUGININFO p,BOOL bReloadAll)
{
   if(!PlugIn::GetInfo(p,bReloadAll))
       return FALSE;
   bExclusive = 1;
   if(bReloadAll)
       isASync = (BYTE)((p->wType & (PIT_ASYNC|PIT_ASYNCEVENT)) >> 10);
   if(isASync > 2)
       return FALSE;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL SioPlug::InitPlugInInfo(LPPLUGININFO p,DWORD dwState,DWORD dwStateMask)
{
   if(!PlugIn::InitPlugInInfo(p,dwState,dwStateMask))
       return FALSE;
   if((dwStateMask & 0x100000)){
       if(!isLicensed || !bEnable)
           return FALSE;
       if((dwStateMask & 0x200000))
           p->lParam = (LPARAM)&cpu_cycles;
       else
           p->lParam = VCOUNT;
   }
   else{
       InitSERIALIO(mem1,(DWORD)-1);
       p->lParam = (LPARAM)mem1;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL SioPlug::Run(LPSERIALIO p)
{
   if(!bEnable)
       return TRUE;
   if(pRunFunc == NULL || isASync)
       return FALSE;
   return ((LPSIOPLUGRUN)pRunFunc)(p);
}
//---------------------------------------------------------------------------
SioPlugList::SioPlugList() : PlugInList("SIOPlugIn")
{
   hThreadRun = hEventRun = hEvent = hThread = NULL;
   bThreadRun1 = bTrdRun = FALSE;
}
//---------------------------------------------------------------------------
SioPlugList::~SioPlugList()
{
   DestroyASyncMethod();
}
//---------------------------------------------------------------------------
static DWORD WINAPI ThreadFunc1(LPVOID p)
{
   MSG msg;
   HANDLE event;
   SioPlug *p1;

   event = ((SioPlugList *)p)->GetEventHandle(1);
   PeekMessage(&msg,NULL,SIO_NOTIFY+1,SIO_NOTIFY+1,PM_NOREMOVE);
   while(bThreadRun1){
       ::WaitForSingleObject(event,INFINITE);
       while(bThreadRun1 && PeekMessage(&msg,NULL,SIO_NOTIFY+1,SIO_NOTIFY+1,PM_REMOVE)){
           if(msg.wParam != NULL){
               if((p1 = (SioPlug *)msg.lParam) != NULL){
                   if(((LPSIOPLUGRUN)p1->GetRunFunc())((LPSERIALIO)msg.wParam))
                       SioCallbackEx((LPSERIALIO)msg.wParam);
               }
               GlobalFree((HGLOBAL)msg.wParam);
           }
       }
   }
   LeaveCriticalSection(&crSection);
   return 1;
}
//---------------------------------------------------------------------------
/*static DWORD WINAPI ThreadFunc(LPVOID p)
{
   MSG msg;
   HANDLE event;
   LPSERIALIO sio;

   event = ((SioPlugList *)p)->GetEventHandle();
   PeekMessage(&msg,NULL,SIO_NOTIFY,SIO_NOTIFY,PM_NOREMOVE);
   while(bThreadRun){
       ::WaitForSingleObject(event,INFINITE);
       while(bThreadRun && PeekMessage(&msg,NULL,SIO_NOTIFY,SIO_NOTIFY,PM_REMOVE)){
           sio = (LPSERIALIO)msg.lParam;
           if(sio == NULL)
               continue;
           if(sio->dwCallback[0] != NULL)
               ::PulseEvent((HANDLE)sio->dwCallback[0]);
           else
               ::PulseEvent(event);
           SioCallbackEx(sio);
       }
   }
   return 1;
}*/
//---------------------------------------------------------------------------
BOOL SioPlugList::EnableASyncMethod(int mode)
{
   char s[100];

   DestroyASyncMethod();
   if(mode < 1)
       return FALSE;
   wsprintf(s,"RBASIOEVENTRUN%u",GetTickCount());
   hEventRun = CreateEvent(NULL,FALSE,0,s);
   if(hEventRun == NULL)
       return FALSE;
   bThreadRun1 = TRUE;
   hThreadRun = CreateThread(NULL,0,ThreadFunc1,this,0,&dwThreadId[1]);
   if(hThreadRun == NULL){
       DestroyASyncMethod();
       return FALSE;
   }
   if(mode == 1)
       return TRUE;
/*   wsprintf(s,"RBASIOEVENT%u",GetTickCount());
   hEvent = CreateEvent(NULL,0,0,s);
   if(hEvent == NULL)
       return FALSE;
   hThread = CreateThread(NULL,0,ThreadFunc,this,CREATE_SUSPENDED,&dwThreadId[0]);
   if(hThread == NULL){
       ::CloseHandle(hEvent);
       hEvent = NULL;
       return FALSE;
   }
   bThreadRun = TRUE;
   ::ResumeThread(hThread);*/
   return TRUE;
}
//---------------------------------------------------------------------------
SioPlug *SioPlugList::BuildPlugIn(char *path)
{
   SioPlug *p;

   if(path == NULL || (p = new SioPlug()) == NULL)
       return NULL;
   p->SetLibraryPath(path);
   return p;
}
//---------------------------------------------------------------------------
void SioPlugList::DestroyASyncMethod()
{
   DWORD res;

   bTrdRun = FALSE;
   if(hEvent != NULL)
       ::SetEvent(hEvent);
   if(hThread != NULL){
       do{
           WaitForSingleObject(hThread,500);
       }while(GetExitCodeThread(hThread,&res) && res != 1);
       ::CloseHandle(hThread);
   }
   hThread = NULL;
   if(hEvent)
       CloseHandle(hEvent);
   hEvent = NULL;
   bThreadRun1 = FALSE;
   if(hThreadRun != NULL){
       do{
           PulseEvent(hEventRun);
           LeaveCriticalSection(&crSection);
           WaitForSingleObject(hThreadRun,500);
       }while(GetExitCodeThread(hThreadRun,&res) && res != 1);
       ::CloseHandle(hThreadRun);
   }
   hThreadRun = NULL;
   if(hEventRun)
       CloseHandle(hEventRun);
   hEventRun = NULL;
}
//---------------------------------------------------------------------------
BOOL SioPlugList::Enable(LPGUID p,BOOL bEnable)
{
   SioPlug *pPlugIn,*p1;
   DWORD dwPos;

   if(p == NULL || (pPlugIn = (SioPlug *)GetItemFromGUID(p)) == NULL)
       return FALSE;
   if(bEnable){
       dwCallback[0] = (DWORD)SioCallbackEx;
       dwCallback[1] = (DWORD)pPlugIn;
       if(!pPlugIn->IsSync()){
           if(!EnableASyncMethod(pPlugIn->GetASyncMethod()))
               return FALSE;
/*           if(pPlugIn->GetASyncMethod() == 2){
               dwCallback[0] = (DWORD)hEvent;
               dwCallback[1] = dwThreadId[0];
           }*/
       }
   }
   if(!pPlugIn->Enable(bEnable) && bEnable){
       dwCallback[0] = dwCallback[1] = 0;
       Delete(IndexFromEle((LPVOID)pPlugIn));
       return FALSE;
   }
   if(!bEnable)
       return TRUE;
   if(pPlugIn->IsAttribute(PIT_ENABLERUN))
   	pPlugIn->Run(NULL);
   p1 = pPlugIn;
   dwPos = 0;
   if((pPlugIn = (SioPlug *)GetFirstItem(&dwPos)) ==  NULL)
       return FALSE;
   do{
       if(p1 != pPlugIn)
           pPlugIn->Enable(FALSE);
   }while((pPlugIn = (SioPlug *)GetNextItem(&dwPos)) != NULL);

   return TRUE;
}
//---------------------------------------------------------------------------
int SioPlugList::Run(DWORD mask)
{
   DWORD dwPos;
   SioPlug *p;
   int i;
   BOOL res;
   LPSERIALIO p1;

   if((p = (SioPlug *)GetFirstItem(&dwPos)) ==  NULL)
       return -1;
   if((p1 = InitSERIALIO(NULL,mask)) == NULL)
       return -1;
   i = 0;
   do{
       if(p->IsEnable() && !p->IsAttribute(PIT_ENABLERUN)){
           if(p->IsSync()){
               res = p->Run(p1);
               CopyMemory(mem1,p1,sizeof(SERIALIO));
               GlobalFree((HGLOBAL)p1);
           }
           else if(hEventRun != NULL && hThreadRun != NULL && p->IsRunnable()){
               res = PostThreadMessage(dwThreadId[1],SIO_NOTIFY + 1,(WPARAM)p1,(LPARAM)p);
               if(res)
                   ::PulseEvent(hEventRun);
           }
           if(res){
               i = 1;
               if(p->IsSync())
                   SioCallbackEx((LPSERIALIO)mem1);
               else
                   i = 2;
           }
           break;
       }
   }while((p = (SioPlug *)GetNextItem(&dwPos)) != NULL);

   return i;
}
//---------------------------------------------------------------------------
BOOL InitSio()
{
   mem1 = (LPSERIALIO)::GlobalAlloc(GPTR|GMEM_SHARE,sizeof(SERIALIO));
   if(mem1 == NULL)
       return FALSE;
   return TRUE;
}
//---------------------------------------------------------------------------
void DestroySio()
{
   if(mem1 != NULL)
       ::GlobalFree(mem1);
   mem1 = NULL;
}
//---------------------------------------------------------------------------
int ResetSio()
{
   SioPlugList *pSioPlugList;

   if((pSioPlugList = pPlugInContainer->GetSioPlugInList()) == NULL)
       return -1;
   return pSioPlugList->Reset();
}
//---------------------------------------------------------------------------
BOOL EnableSioPlug(WORD wID)
{
   return pPlugInContainer->GetSioPlugInList()->OnEnablePlug(wID);
}
//---------------------------------------------------------------------------
BOOL OnInitSioPlugMenu(HMENU menu)
{
   return pPlugInContainer->GetSioPlugInList()->OnInitMenu(menu);
}
//---------------------------------------------------------------------------
/*static BOOL OnNotifyInterrupt(LPSERIALIO p)
{
   LPPLUGINCALLBACK p1;

   if((p->dwResult & 0x80000000) == 0){
       SetInterruptSuspend(0x80);
       return TRUE;
   }
   p1 = new PLUGINCALLBACK[1];
   if(p1 == NULL)
       return FALSE;
   p1->cyclesElapsed = (p->dwResult & 0xFFFFF);
   p1->pPlugIn = (PlugIn *)p->dwCallback[1];
   if(!pPlugInContainer->GetSioPlugInList()->AddCallback(p1)){
       delete p1;
       return FALSE;
   }
   return TRUE;
}*/
//---------------------------------------------------------------------------
static void WINAPI SioCallbackEx(LPSERIALIO p)
{
   if(p == NULL || !p->dwResult)
       return;
   if((p->mMask & 0x300000))
       RCNT = (u16)((RCNT & 0x8000) | (p->REG_CNT.value & 0x7FFF));
   if((p->mMask & 0x100)){
       SCCNT_L = (u16)((SCCNT_L & 0xFF00) | (u8)p->CNT.value);
       mbID = (u8)p->CNT.mp.id;
   }
   if((p->mMask & 0xFF)){
       SCD0    = p->d0;
       SCD1    = p->d1;
       SCD2    = p->d2;
       SCD3    = p->d3;
   }
   if((p->mMask & 0xC00))
       SCCNT_H = p->DATA;
   if(((SCCNT_L & 0x4000) && !(RCNT & 0x8000))){
       if(p->CNT.nm.irq)
           SetInterruptSuspend(0x80);
   }
   else if(((RCNT & 0xC000) == 0x8000 && (RCNT & 0x100))){
       if(p->REG_CNT.regg.irq)
           SetInterruptSuspend(0x80);
   }
}

