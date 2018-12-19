//---------------------------------------------------------------------------
#include <windows.h>
#pragma hdrstop

#include "savestate.h"
#include "memory.h"
#include "sound.h"
#include "lcd.h"
#include "sprite.h"
#include "gba.h"
#include "list.h"
#include "trad.h"
#include "exec.h"
#include "cheat.h"

#define VER_SAVESTATE  3
//---------------------------------------------------------------------------
extern BOOL bLoadRecent;
static int iLoadRecentIndex;
static UINT idTimer,oldPos;
static BOOL bLoadAccel;
static WNDPROC oldTBWndProc,oldSBWndProc;
static HWND hwndTrack;
extern LList *pSaveStateList;
extern HACCEL hAccel;
extern DWORD ReadFileEEPROM(LStream *pFile);
extern BOOL WriteFileEEPROM(LStream *pFile);
extern BOOL WriteFileSRAM(LStream *pFile);
extern BOOL ReadFileSRAM(LStream *pFile);
extern HWND hWndStatusBar;
extern u8 nSkipMaster;
LSaveState rewindState;
//---------------------------------------------------------------------------
static void RebuildSaveStateAccelerator();
static void WINAPI ssCallBack(int progress);
//---------------------------------------------------------------------------
LSaveState::LSaveState()
{
	Index = 0;
   IndexMax = 1;
	bUseFile = FALSE;
   pMemoryFile = NULL;
}
//---------------------------------------------------------------------------
LSaveState::~LSaveState()
{
   if(pMemoryFile != NULL)
   	delete pMemoryFile;
}
//---------------------------------------------------------------------------
void LSaveState::Reset()
{
   if(pMemoryFile != NULL)
   	delete pMemoryFile;
	Index = 0;
   pMemoryFile = NULL;
}
//---------------------------------------------------------------------------
BOOL LSaveState::set_File(const char *fileName)
{
   zipFile.Close();
   zipFile.SetFileStream(NULL);
	if(fileName == NULL)
   	bUseFile = FALSE;
   else{
   	lstrcpy(cFileName,fileName);
   	bUseFile = TRUE;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LSaveState::Init(BOOL bOpenAlways)
{
	if(bUseFile){
   	if(!zipFile.Open(cFileName,bOpenAlways))
       	return FALSE;
		return TRUE;
   }
   if(pMemoryFile == NULL){
       if((pMemoryFile = new LMemoryFile(65536)) == NULL)
           return FALSE;
   }
   if(!pMemoryFile->IsOpen()){
       if(!pMemoryFile->Open())
           return FALSE;
       Index = 0;
   }
   zipFile.SetFileStream(pMemoryFile);
   if(!zipFile.Open(""))
       return FALSE;
	return TRUE;
}
//---------------------------------------------------------------------------
LPVOID LSaveState::Save(LPSAVESTATECALLBACK pCallBack,int iLevel,int index)
{
   LString s;
   LMemoryFile *pFile;
   int i;
   LPBYTE pBuffer;
   LPZIPFILEHEADER res;

	if(!Init())
   	return NULL;
   s = (int)(index != -1 ? index : Index + 1);
   s += ".sgf";
   res = NULL;
   if(zipFile.AddZipFile(s.c_str(),iLevel)){
       i = 1;
       if((pFile = new LMemoryFile()) != NULL){
           if(pFile->Open())
               i = VER_SAVESTATE;
       }
       zipFile.WriteZipFile(&i,sizeof(int));
       if(pCallBack) pCallBack(1);
       zipFile.WriteZipFile(&arm,SAVESTATE_SIZEARM7TDMI);
       if(pCallBack) pCallBack(2);
       zipFile.WriteZipFile(&pcm,sizeof(PCM));
       if(pCallBack) pCallBack(3);
       zipFile.WriteZipFile(dma,sizeof(DMA) * 4);
       if(pCallBack) pCallBack(4);
       zipFile.WriteZipFile(timer,sizeof(TIMER)*4);
       if(pCallBack) pCallBack(5);
       zipFile.WriteZipFile(translated_palette,1024);
       if(pCallBack) pCallBack(6);
       zipFile.WriteZipFile(wram_int_u8,428616);
       if(pCallBack) pCallBack(7);
       if(i > 1){
           if(UseSRAM())
               i = (int)WriteFileSRAM(pFile);
           else
               i = (int)WriteFileEEPROM(pFile);
           if(i){
               pFile->SeekToBegin();
               i = UseSRAM();
               zipFile.WriteZipFile(&i,sizeof(int));
               i = pFile->Size();
               zipFile.WriteZipFile(&i,sizeof(int));
               pBuffer = (LPBYTE)GlobalAlloc(GPTR,0x1000);
               while((i = pFile->Read(pBuffer,0xFFF)) != 0)
                   zipFile.WriteZipFile(pBuffer,i);
               GlobalFree(pBuffer);
           }
       }
       zipFile.WriteZipFile(io_ram_u8,0x400);
       if(pCallBack) pCallBack(8);
       if(pFile != NULL)
           delete pFile;
       res = zipFile.GetCurrentZipFileHeader();
       if(index == -1){
       	Index++;
       	if(Index > IndexMax){
           	zipFile.Close();
           	zipFile.Open("");
           	zipFile.DeleteZipFile(1);
       	}
       }
   }
   zipFile.Close();
   return res;
}
//---------------------------------------------------------------------------
BOOL LSaveState::Load(LPSAVESTATECALLBACK pCallBack,UINT index)
{
   LPBYTE pBuffer,p;
   int ver,i,size,i1;
   LMemoryFile *pFile;
   BOOL res;

	if(!Init())
   	return FALSE;
   res = FALSE;
   if(zipFile.OpenZipFile((WORD)index) != NULL){
       zipFile.ReadZipFile(&ver,sizeof(int));
       if(pCallBack) pCallBack(1);
       pBuffer = (LPBYTE)GlobalAlloc(GPTR,0x1000);
       if(pBuffer == NULL)
           return FALSE;
       p = pBuffer;
       zipFile.ReadZipFile(p,SAVESTATE_SIZEARM7TDMI + sizeof(PCM) + sizeof(DMA) * 4 + sizeof(TIMER) * 4);
		if(pCallBack) pCallBack(2);
       CopyMemory(&arm,p,((int)&arm.exec - (int)&arm));
       p += SAVESTATE_SIZEARM7TDMI;
       //PCM
       CopyMemory(pcm.gbcch,((LPPCM)p)->gbcch,sizeof(GBCCHANNEL) * 4);
       CopyMemory(&pcm.fifo[0],&((LPPCM)p)->fifo[0],sizeof(FIFOCHANNEL) - 8);
       CopyMemory(&pcm.fifo[1],&((LPPCM)p)->fifo[1],sizeof(FIFOCHANNEL) - 8);
       CopyMemory(pcm.wave,((LPPCM)p)->wave,16);
       pcm.LastWriteBytes = ((LPPCM)p)->LastWriteBytes;
       pcm.LastPlay = ((LPPCM)p)->LastPlay;
       pcm.LastWriteEnd = ((LPPCM)p)->LastWriteEnd;
       pcm.Update = ((LPPCM)p)->Update;
       pcm.Inc = ((LPPCM)p)->Inc;
       pcm.MaxPos = ((LPPCM)p)->MaxPos;
       pcm.cycles = ((LPPCM)p)->cycles;
       pcm.volL = ((LPPCM)p)->volL;
       pcm.volR = ((LPPCM)p)->volR;
       pcm.pos = ((LPPCM)p)->pos;
       p += sizeof(PCM);
		if(pCallBack) pCallBack(3);
       //DMA
       for(i=0;i<4;i++){
           CopyMemory(&dma[i].Dst,&((LPDMA)p)->Dst,sizeof(DMA) - ((int)&dma[0].Dst - (int)dma));
           p += sizeof(DMA);
       }
		if(pCallBack) pCallBack(4);
       //Timer
       for(i=0;i<4;i++){
           CopyMemory(&timer[i].Freq,&((LPTIMER)p)->Freq,sizeof(TIMER) - ((int)&timer[0].Freq - (int)timer));
           p += sizeof(TIMER);
       }
		if(pCallBack) pCallBack(5);
       zipFile.ReadZipFile(translated_palette,1024);
       if(pCallBack) pCallBack(6);
       zipFile.ReadZipFile(wram_int_u8,428616);
       if(pCallBack) pCallBack(7);
       if(ver > 1){
           zipFile.ReadZipFile(&i,sizeof(int));
           zipFile.ReadZipFile(&size,sizeof(int));
           if((pFile = new LMemoryFile()) != NULL)
               pFile->Open();
           while(size > 0){
               i1 = size > 0xFFF ? 0xFFF : size;
               zipFile.ReadZipFile(pBuffer,i1);
               if(pFile != NULL)
                   pFile->Write(pBuffer,i1);
               size -= i1;
           }
           if(pFile != NULL){
               pFile->SeekToBegin();
               if(i){
                   if(ReadFileSRAM(pFile)){
                       bin.bLoadRam &= ~6;
                       bin.bLoadRam |= 1;
                   }
               }
               else{
	                FreeRomPack(1);
                   FreeRomPack(2);
                   if((i = ReadFileEEPROM(pFile)) != 0){
                       bin.bLoadRam &= ~1;
                       bin.bLoadRam |= (u8)i;
                   }
               }
               delete pFile;
           }
       }
       ::GlobalFree(pBuffer);
       zipFile.ReadZipFile(io_ram_u8,0x400);
       if(pCallBack) pCallBack(8);
       lcd.DrawMode = 0xFF;
       lcd.blankBit = 0;
       ResetSprite();
       SetLcd(0,AMM_WORD);
       for(i=8;i<85;i+=2)
           SetLcd((u16)i,AMM_HWORD);
       for(i=0;i<128;i+=2)
           WriteSprite((u16)i,AMM_HWORD);
       SetExecFunction((u8)((CPSR & T_BIT) >> 5));
       res = TRUE;
   }
   return res;
}
//---------------------------------------------------------------------------
static LONG FAR PASCAL SBWndProc(HWND win, UINT msg, WPARAM wparam, LPARAM lparam)
{
   UINT pos,i;

   switch(msg){
       case WM_TIMER:
           DestroyWindow(hwndTrack);
           UpdateStatusBar(bin.Title,0);
           hwndTrack = NULL;
           SetWindowLong(hWndStatusBar,GWL_WNDPROC,(LONG)oldSBWndProc);
           if(bPause)
               SendMessage(hWin,WM_COMMAND,ID_EMU_PAUSE,0);
           return 0;
       case WM_HSCROLL:
           switch(LOWORD(wparam)){
               case TB_LINEUP:
               case TB_LINEDOWN:
               case TB_PAGEUP:
               case TB_PAGEDOWN:
               case TB_THUMBTRACK:
                   pos = (UINT)SendMessage((HWND)lparam,TBM_GETPOS,0,0);
                   if(pos == oldPos)
                       return 0;
					rewindState.Load(NULL,pos+1);
                   oldPos = pos;
                   i = (UINT)nSkipMaster;
                   nSkipMaster = 0;
                   run_frame();
                   nSkipMaster = (u8)i;
               break;
           }
           return 0;
   }
   return CallWindowProc(oldSBWndProc,win,msg,wparam,lparam);
}
//---------------------------------------------------------------------------
static LONG FAR PASCAL TBWndProc(HWND win, UINT msg, WPARAM wparam, LPARAM lparam)
{
   switch(msg){
       case WM_LBUTTONDOWN:
           if(idTimer != 0)
               KillTimer(hWndStatusBar,idTimer);
           idTimer = 0;
       break;
       case WM_LBUTTONUP:
           idTimer = SetTimer(hWndStatusBar,0xEEEE,1000,NULL);
       break;
       case WM_DESTROY:
           if(idTimer != 0)
               KillTimer(hWndStatusBar,idTimer);
           idTimer = 0;
       break;
   }
   return CallWindowProc(oldTBWndProc,win,msg,wparam,lparam);
}
//---------------------------------------------------------------------------
BOOL PlayGameState()
{
   RECT rc;
   int i;

   SendMessage(hWndStatusBar,SB_GETRECT,0,(LPARAM)&rc);
   if((i = rc.right - rc.left - 4) > 150)
       i = 150;
   rc.left = rc.left + (((rc.right - rc.left) - i) >> 1);
   hwndTrack = CreateWindow(TRACKBAR_CLASS,NULL,WS_CHILD|TBS_AUTOTICKS|TBS_HORZ|TBS_BOTTOM|TBS_TOOLTIPS,rc.left,rc.top+1,i,rc.bottom-rc.top - 2,hWndStatusBar,(HMENU)IDC_TRKBRIGHTNESS,hInstance,NULL);
   if(hwndTrack == NULL)
       return FALSE;
   if(bThreadRun || bPause)
       SendMessage(hWin,WM_COMMAND,ID_EMU_PAUSE,0);
   oldPos = -1;
   oldSBWndProc = (WNDPROC)SetWindowLong(hWndStatusBar,GWL_WNDPROC,(LONG)SBWndProc);
   UpdateStatusBar("",0);
   oldTBWndProc = (WNDPROC)SetWindowLong(hwndTrack,GWL_WNDPROC,(LONG)TBWndProc);
   SendMessage(hwndTrack, TBM_SETRANGE,(WPARAM)TRUE,(LPARAM) MAKELONG(0,rewindState.Count()-1));
   SendMessage(hwndTrack, TBM_SETPOS,(WPARAM)TRUE,(LPARAM)(rewindState.Count()-1));
   ShowWindow(hwndTrack,SW_SHOW);
   SetFocus(hwndTrack);
   idTimer = SetTimer(hWndStatusBar,0xEEEE,2000,NULL);
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL RecordGameState()
{
   if(!rewindState.Save(NULL))
   	return FALSE;
   EnableMenuItem(GetMenu(hWin),ID_FILE_STATE_REWINDGAME,MF_ENABLED);
   return TRUE;
}
//---------------------------------------------------------------------------
static void WINAPI ssCallBack(int progress)
{
	DrawProgressBar(MAKELONG(progress,8));
}
//---------------------------------------------------------------------------
BOOL LoadMostRecentSaveState()
{
   if(iLoadRecentIndex < 0 || !bLoadRecent)
       return FALSE;
   return LoadState(iLoadRecentIndex);
}
//---------------------------------------------------------------------------
void ResetSaveState()
{
   LString nameFile;
   HMENU menu;

   if((menu = GetSubMenu(GetSubMenu(GetMenu(hWin),0),POS_SM_SAVESTATE)) != NULL){
       EnableMenuItem(menu,POS_SM_SAVESTATE_LOAD,MF_BYPOSITION|MF_GRAYED);
       EnableMenuItem(menu,POS_SM_SAVESTATE_SAVE,MF_BYPOSITION|MF_GRAYED);
       EnableMenuItem(menu,ID_FILE_STATE_RESET,MF_GRAYED);
   }
   if(!nameFile.BuildFileName(bin.saveFileName,"sgs",mbID))
       return;
   DeleteFile(nameFile.c_str());
   RebuildSaveStateAccelerator();
   if(pSaveStateList != NULL)
       delete pSaveStateList;
   pSaveStateList = NULL;
}
//---------------------------------------------------------------------------
BOOL OnInitMenuSaveState(HMENU menu,int sub)
{
   int i;
   MENUITEMINFO mii;
   DWORD dwPos;
   char s[50],s1[30];
   LPSSFILE p;
   WORD wID;

   i = GetMenuItemCount(menu)-1;
   for(;i>=0;i--)
       ::DeleteMenu(menu,i,MF_BYPOSITION);
   i = 0;
   ZeroMemory(&mii,sizeof(MENUITEMINFO));
   mii.cbSize = sizeof(MENUITEMINFO);
   if(pSaveStateList == NULL || pSaveStateList->Count() < 1)
       return FALSE;
   p = (LPSSFILE)pSaveStateList->GetFirstItem(&dwPos);
   wID = (WORD)(ID_FILE_STATE_START + (sub == POS_SM_SAVESTATE_SAVE ? 0xF : 0));
   while(p != NULL && wID <= ID_FILE_STATE_END){
       mii.fMask = MIIM_TYPE|MIIM_STATE|MIIM_ID;
       mii.fType = MFT_STRING;
       mii.fState = MFS_ENABLED;
       mii.dwItemData = i+1;
       GetDateFormat(MAKELCID(pLanguageList->GetLanguageID(),SORT_DEFAULT),DATE_SHORTDATE,&p->time,NULL,s1,30);
       lstrcpy(s,s1);
       lstrcat(s,"  ");
       GetTimeFormat(MAKELCID(pLanguageList->GetLanguageID(),SORT_DEFAULT),LOCALE_NOUSEROVERRIDE,&p->time,NULL,s1,30);
       lstrcat(s,s1);
       if(bLoadAccel && iLoadRecentIndex == i && sub != POS_SM_SAVESTATE_SAVE)
           lstrcat(s,"\tShift+F7");
       mii.dwTypeData = s;
       mii.wID = wID++;
       InsertMenuItem(menu,i++,TRUE,&mii);
       p = (LPSSFILE)pSaveStateList->GetNextItem(&dwPos);
   }
   return TRUE;
}
//---------------------------------------------------------------------------
void ReadSaveStateList()
{
   LString nameFile,s1;
   int i;
   LPSSFILE p;
   HMENU menu;
   LZipFile zipFile;
   LPZIPFILEHEADER p1;
   FILETIME fileTime;

   bLoadAccel = FALSE;
   iLoadRecentIndex = -1;
   if((menu = GetSubMenu(GetSubMenu(GetMenu(hWin),0),POS_SM_SAVESTATE)) != NULL){
       EnableMenuItem(menu,POS_SM_SAVESTATE_LOAD,MF_BYPOSITION|MF_GRAYED);
       EnableMenuItem(menu,POS_SM_SAVESTATE_SAVE,MF_BYPOSITION|MF_GRAYED);
       EnableMenuItem(menu,ID_FILE_STATE_RESET,MF_GRAYED);
   }
   if(pSaveStateList != NULL)
       delete pSaveStateList;
   if((pSaveStateList = new LList()) == NULL)
       return;
   if(!nameFile.BuildFileName(bin.saveFileName,"sgs",mbID))
       return;
   if(!zipFile.Open(nameFile.c_str(),FALSE))
       return;
   zipFile.Rebuild();
   zipFile.Close();
   if(!zipFile.Open(nameFile.c_str(),FALSE))
       return;
   for(i=0;i< (int)zipFile.Count();i++){
       p1 = zipFile.GetZipFileHeader(i+1);
       s1 = p1->nameFile;
       s1.LowerCase();
       if(s1.Pos(".sgf") > 0){
           if((p = new SSFILE[1]) != NULL){
               lstrcpyn(p->fileName,s1.c_str(),12);
               DosDateTimeToFileTime(p1->m_uModDate,p1->m_uModTime,&fileTime);
               FileTimeToSystemTime(&fileTime,&p->time);
               pSaveStateList->Add((LPVOID)p);
           }
       }
   }
   zipFile.Close();
   if(menu != NULL && pSaveStateList->Count() > 0){
       EnableMenuItem(menu,POS_SM_SAVESTATE_LOAD,MF_BYPOSITION|MF_ENABLED);
       EnableMenuItem(menu,POS_SM_SAVESTATE_SAVE,MF_BYPOSITION|MF_ENABLED);
       EnableMenuItem(menu,ID_FILE_STATE_RESET,MF_ENABLED);
   }
   RebuildSaveStateAccelerator();
}
//---------------------------------------------------------------------------
static void RebuildSaveStateAccelerator()
{
   LString s,s1;
   int i;
   LPSSFILE p;
   DWORD dwPos;
   LPACCEL accelBuffer;
   HACCEL temphAccel;

   bLoadAccel = FALSE;
   iLoadRecentIndex = -1;
   p = (LPSSFILE)pSaveStateList->GetFirstItem(&dwPos);
   s.Capacity(200);
   s1 = "";
   i = 0;
   while(p != NULL){
       wsprintf(s.c_str(),"%04d%02d%02d%02d%02d%02d",p->time.wYear,p->time.wMonth,p->time.wDay,
           p->time.wHour,p->time.wMinute,p->time.wSecond);
       if(s1.IsEmpty() || lstrcmpi(s1.c_str(),s.c_str()) <= 0){
           s1 = s;
           iLoadRecentIndex = i;
       }
       i++;
       p = (LPSSFILE)pSaveStateList->GetNextItem(&dwPos);
   }
   if(hAccel != NULL)
       DestroyAcceleratorTable(hAccel);
   hAccel = ::LoadAccelerators(hInstance,MAKEINTRESOURCE(IDR_MAINFRAME));
   if(hAccel == NULL || iLoadRecentIndex == -1)
       return;
   if((i = CopyAcceleratorTable(hAccel,NULL,0)) < 1)
       return;
   if((accelBuffer = (LPACCEL)GlobalAlloc(GPTR,sizeof(ACCEL) * (i+1))) == NULL)
       return;
   CopyAcceleratorTable(hAccel,accelBuffer,i);
   accelBuffer[i].fVirt = FVIRTKEY|FNOINVERT|FSHIFT;
   accelBuffer[i].key = VK_F7;
   accelBuffer[i].cmd = (WORD)(ID_FILE_STATE_START+iLoadRecentIndex);
   temphAccel = CreateAcceleratorTable(accelBuffer,i+1);
   if(temphAccel != NULL){
       DestroyAcceleratorTable(hAccel);
       hAccel = temphAccel;
       bLoadAccel = TRUE;
   }
   GlobalFree(accelBuffer);
}
//---------------------------------------------------------------------------
BOOL LoadState(int index)
{
   LString nameFile;
   BOOL res;
	LSaveState saveState;

   if(pSaveStateList == NULL || index > 14)
       return FALSE;
   if(pSaveStateList->GetItem(index+1) == NULL)
       return FALSE;
   if(!nameFile.BuildFileName(bin.saveFileName,"sgs",mbID))
       return FALSE;
   saveState.set_File(nameFile.c_str());
	if((res = saveState.Load(ssCallBack,index+1)))
       ApplyAllCheatList();
   return res;
}
//---------------------------------------------------------------------------
static int DeleteLastSaveState()
{
   LString s,s1;
   LPSSFILE p;
   DWORD dwPos;
   int i,i1;

   if(pSaveStateList == NULL || pSaveStateList->Count() < 1)
       return 1;
   p = (LPSSFILE)pSaveStateList->GetFirstItem(&dwPos);
   s.Capacity(200);
   s1 = "";
   i = i1 = 0;
   while(p != NULL){
       wsprintf(s.c_str(),"%04d%02d%02d%02d%02d%02d",p->time.wYear,p->time.wMonth,p->time.wDay,
           p->time.wHour,p->time.wMinute,p->time.wSecond);
       if(s1.IsEmpty() || lstrcmpi(s1.c_str(),s.c_str()) > 0){
           s1 = s;
           i1 = i;
       }
       i++;
       p = (LPSSFILE)pSaveStateList->GetNextItem(&dwPos);
   }
   p = (LPSSFILE)pSaveStateList->GetItem(++i1);
   pSaveStateList->Delete(i1);
   return atoi(p->fileName);
}
//---------------------------------------------------------------------------
BOOL SaveState(int index)
{
   LString nameFile;
   LPZIPFILEHEADER p1;
   LPSSFILE p;
   FILETIME fileTime;
   HMENU menu;
   LSaveState saveState;

	if(!nameFile.BuildFileName(bin.saveFileName,"sgs",mbID))
       return FALSE;
   if(pSaveStateList == NULL){
       if((pSaveStateList = new LList()) == NULL)
           return FALSE;
   }
   p = NULL;
   if(index == -1)
       index = (int)(pSaveStateList->Count() + 1);
   else{
       if((p = (LPSSFILE)pSaveStateList->GetItem(++index)) == NULL)
           index = (int)(pSaveStateList->Count() + 1);
   }
   if(index > 15)
       index = DeleteLastSaveState();
   saveState.set_File(nameFile.c_str());
   if((p1 = (LPZIPFILEHEADER)saveState.Save(ssCallBack,9,index)) != NULL){
       if(p == NULL){
           p = new SSFILE[1];
           pSaveStateList->Add((LPVOID)p);
       }
       lstrcpy(p->fileName,p1->nameFile);
       DosDateTimeToFileTime(p1->m_uModDate,p1->m_uModTime,&fileTime);
       FileTimeToSystemTime(&fileTime,&p->time);
       if((menu = GetSubMenu(GetSubMenu(GetMenu(hWin),0),POS_SM_SAVESTATE)) != NULL){
           EnableMenuItem(menu,POS_SM_SAVESTATE_LOAD,MF_BYPOSITION|MF_ENABLED);
           EnableMenuItem(menu,POS_SM_SAVESTATE_SAVE,MF_BYPOSITION|MF_ENABLED);
           EnableMenuItem(menu,ID_FILE_STATE_RESET,MF_ENABLED);
       }
   }
   RebuildSaveStateAccelerator();
   return TRUE;
}





