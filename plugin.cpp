//---------------------------------------------------------------------------
#include <windows.h>
#pragma hdrstop

#include "plugin.h"
#include "trad.h"
#include "lregkey.h"
#include "lcd.h"
#include "pluginctn.h"

//---------------------------------------------------------------------------
PlugIn::PlugIn()
{
   wID = 0xFFFF;
   wType = 0;
   ZeroMemory(&guid,sizeof(GUID));
   isLicensed = isReload = isLoad = bEnable = 0;
   index = bExclusive = 1;
   bDynamic = 0;
   pResetFunc = NULL;
   pDeleteFunc = NULL;
   pGetInfoFunc = NULL;
   pSetInfoFunc = NULL;
   pRunFunc = NULL;
   pSetPropertyFunc = NULL;
   hLib = NULL;
   pathLibrary = "";
   name = "";
   dwFlags = 0;
}
//---------------------------------------------------------------------------
PlugIn::~PlugIn()
{
   Unload(TRUE);
}
//---------------------------------------------------------------------------
BOOL PlugIn::NotifyState(DWORD dwState,DWORD dwStateMask)
{
   PLUGININFO pi;
   BOOL res;

   if(!InitPlugInInfo(&pi,dwState,dwStateMask))
       return FALSE;
   res = TRUE;
   if(pSetInfoFunc != NULL)
       res = pSetInfoFunc(&pi);
   return res;
}
//---------------------------------------------------------------------------
BOOL PlugIn::SetInfo(LPPLUGININFO p)
{
   if(p == NULL)
       return FALSE;
   wID = p->wID;
   if(pSetInfoFunc != NULL)
       return pSetInfoFunc(p);
   return TRUE;
}
//---------------------------------------------------------------------------
int PlugIn::GetInfo(LPPLUGININFO p,BOOL bReloadAll)
{
   LString s;
   int res;

   if(p == NULL)
       return FALSE;
   p->wID = wID;
   if(bReloadAll){
       if(!Load())
           return FALSE;
       if(pGetInfoFunc != NULL)
           res = pGetInfoFunc(p);
       else{
           res = TRUE;
           if(p->pszText != NULL){
               s.Capacity(p->cchTextMax);
               TranslateLoadString(&s,wID);
               lstrcpyn((LPSTR)p->pszText,s.c_str(),p->cchTextMax);
           }
           *(&p->guidID) = *(&guid);
       }
       if(res){
           if(!p->guidID.Data1 || !p->guidID.Data2)
               res = FALSE;
           else{
               wType = (WORD)(BYTE)p->wType;
               if((wType & 0xF) == PIT_VIDEO || (wType & 0xF) == PIT_AUDIO ||
               	(wType & 0xF) == PIT_SIO || (wType & 0xF) == PIT_BACKUP){
                   *(&guid) = *(&p->guidID);
                   name = (LPSTR)p->pszText;
                   dwFlags = (DWORD)p->wType;
                   bDynamic = (char)((p->wType & PIT_DYNAMIC) >> 8);
                   bExclusive = !((p->wType & PIT_NOEXCLUSIVE) >> 9);
                   isLicensed = (wType >> 4) == ((guid.Data3 >> 4) & 0xF);
               }
               else{
                   wType = 0;
                   res = FALSE;
               }
           }
       }
   }
   else{
       p->wType = wType;
       *(&p->guidID) = *(&guid);
       if(p->pszText != NULL)
           lstrcpyn((LPSTR)p->pszText,name.c_str(),p->cchTextMax);
       res = TRUE;
   }
   return res;
}
//---------------------------------------------------------------------------
BOOL PlugIn::Load()
{
   BOOL res;

   if(isLoad)
       return TRUE;
   Unload();
   res = FALSE;
   if(pathLibrary.IsEmpty())
       return TRUE;
   hLib = GetModuleHandle(pathLibrary.c_str());
   if(hLib != NULL)
       isReload = TRUE;
   else
       hLib = LoadLibrary(pathLibrary.c_str());
   if(hLib == NULL)
       return FALSE;
   (FARPROC)pResetFunc = GetProcAddress(hLib,"ResetFunc");
   (FARPROC)pDeleteFunc = GetProcAddress(hLib,"DeleteFunc");
   (FARPROC)pGetInfoFunc = GetProcAddress(hLib,"GetInfoFunc");
   (FARPROC)pSetInfoFunc = GetProcAddress(hLib,"SetInfoFunc");
   (FARPROC)pSetPropertyFunc = GetProcAddress(hLib,"SetPropertyFunc");
   pRunFunc = GetProcAddress(hLib,"RunFunc");
   if(pGetInfoFunc == NULL || pRunFunc == NULL || pSetInfoFunc == NULL)
       Unload();
   else{
       res = TRUE;
       isLoad = 1;
   }
   return res;
}
//---------------------------------------------------------------------------
int PlugIn::GetFileInfo(char *pOut)
{
   DWORD dwLen,lpPointer;
   LPBYTE lpBuf;
   UINT dwByte;
   WORD lID,slID;
   char string[100];
   int i;

   *((LPDWORD)pOut) = 0;
   i = -1;
   if(pathLibrary.IsEmpty())
       return i;
   if((dwLen = GetFileVersionInfoSize(pathLibrary.c_str(),&dwLen)) < 1)
       return i;
   if((lpBuf = new BYTE[dwLen+10]) == NULL)
       return i;
   if(GetFileVersionInfo(pathLibrary.c_str(),0,dwLen,(LPVOID)lpBuf)){
       if(VerQueryValue((LPVOID)lpBuf,"\\VarFileInfo\\Translation",(LPVOID *)&lpPointer,&dwByte)){
           lID =  ((LPWORD)lpPointer)[0];
           slID = ((LPWORD)lpPointer)[1];
           wsprintf(string,"\\StringFileInfo\\%04x%04x\\CompanyName",lID,slID);
           if(VerQueryValue((LPVOID)lpBuf,string,(LPVOID *)&lpPointer,&dwByte))
               lstrcpy(pOut,(LPSTR)lpPointer);
           else{
               lstrcpy(pOut,"Unknown");
               dwByte = 8;
           }
           pOut += dwByte;
           *pOut++ = 0;
           i = dwByte + 1;
           wsprintf(string,"\\StringFileInfo\\%04x%04x\\ProductVersion",lID,slID);
           if(VerQueryValue((LPVOID)lpBuf,string,(LPVOID *)&lpPointer,&dwByte))
               lstrcpy(pOut,(LPSTR)lpPointer);
           else{
               lstrcpy(pOut,"Unknown");
               dwByte = 8;
           }
           pOut += dwByte;
           *((LPWORD)pOut) = 0;
           i += dwByte + 2;
       }
   }
   delete []lpBuf;
   return i;
}
//---------------------------------------------------------------------------
BOOL PlugIn::InitPlugInInfo(LPPLUGININFO p,DWORD dwState,DWORD dwStateMask)
{
   if(p == NULL)
       return FALSE;
   ZeroMemory(p,sizeof(PLUGININFO));
   p->cbSize = sizeof(PLUGININFO);
   p->wID = wID;
   p->wIndex = (WORD)index;
   p->wType = (WORD)(wType & 0xFF0F);
   p->dwState = dwState;
   p->dwStateMask = dwStateMask;
   if((dwState & PIS_ENABLEMASK))
       p->dwStateMask |= PIS_ENABLEMASK;
   if((dwState & PIS_RUNMASK))
       p->dwStateMask |= PIS_RUNMASK;
   *(&p->guidID) = *(&guid);
   p->dwLanguageID = MAKELONG(pLanguageList->GetLanguageID(),pLanguageList->GetCodePage());
   p->pServiceInterface = pPlugInContainer->GetServiceInterface();
   return TRUE;
}
//---------------------------------------------------------------------------
void PlugIn::Unload(BOOL bForced)
{
   if(!bForced && !bDynamic)
       return;
   if(!isReload){
       if(isLoad == 2 && pDeleteFunc != NULL)
           pDeleteFunc();
       if(hLib != NULL)
           FreeLibrary(hLib);
   }
   hLib = NULL;
   pRunFunc = NULL;
   pResetFunc = NULL;
   pDeleteFunc = NULL;
   pGetInfoFunc = NULL;
   pSetInfoFunc = NULL;
   pSetPropertyFunc = NULL;
   isLoad = 0;
}
//---------------------------------------------------------------------------
BOOL PlugIn::Enable(BOOL bFlag)
{
   BOOL res;
   PLUGININFO p;
   DWORD dw;

   res = TRUE;
   if((bEnable = (char)(bFlag & 1)) == 0)
       Unload();
   else{
       if((res = Load()) != 0){
           SetUsed();
           dw = lcd.BlitMode == BM_DIRECTDRAWFULLSCREEN ? PIS_FULLSCREEN|PIS_DDRAW : (lcd.BlitMode == BM_GDI ? 0 : PIS_DDRAW);
           dw |= (bEnable & 1);
           InitPlugInInfo(&p,dw,PIS_RUNMASK|PIS_ENABLEMASK);
           res = SetInfo(&p);
       }
   }
   return res;
}
//---------------------------------------------------------------------------
PlugInList::PlugInList(char *p) : LList()
{
   name = p;
//   pListCallBack = NULL;
}
//---------------------------------------------------------------------------
PlugInList::~PlugInList()
{
   SaveSetConfig();
/*   if(pListCallBack != NULL)
       delete pListCallBack;
   pListCallBack = NULL;*/
   LList::Clear();
}
//---------------------------------------------------------------------------
void PlugInList::DeleteElem(LPVOID ele)
{
   if(ele == NULL)
       return;
   ((PlugIn *)ele)->Destroy();
   delete (PlugIn *)ele;
}
//---------------------------------------------------------------------------
void PlugInList::SaveSetConfig()
{
   LRegKey reg;
   GUID guid;

   GetSelectedGUID(&guid);
   reg.Open("Software\\RascalBoy");
   reg.WriteBinaryData(name.c_str(),(char *)&guid,sizeof(GUID));
   reg.Close();
}
//---------------------------------------------------------------------------
void PlugInList::LoadSetConfig()
{
   LRegKey reg;
   GUID guid;

   reg.Open("Software\\RascalBoy");
   reg.ReadBinaryData(name.c_str(),(char *)&guid,sizeof(GUID));
   Enable(&guid,TRUE);
   reg.Close();
}
//---------------------------------------------------------------------------
PlugIn *PlugInList::BuildPlugIn(char *path)
{
   return NULL;
}
//---------------------------------------------------------------------------
PlugIn *PlugInList::GetItemFromGUID(LPGUID p)
{
   DWORD dwPos;
   PlugIn *pPlugIn,*res;
   PLUGININFO pi;

   res = NULL;
   if((pPlugIn = (PlugIn *)GetFirstItem(&dwPos)) ==  NULL)
       return res;
   do{
       ZeroMemory(&pi,sizeof(PLUGININFO));
       pi.cbSize = sizeof(PLUGININFO);
       if(!pPlugIn->GetInfo(&pi))
           continue;
       if(memcmp(&pi.guidID,p,sizeof(GUID)) == 0){
           res = pPlugIn;
           break;
       }
   }while((pPlugIn = (PlugIn *)GetNextItem(&dwPos)) != NULL);
   return res;
}
//---------------------------------------------------------------------------
BOOL PlugInList::PreLoad(WORD *wID)
{
   return TRUE;
}
//---------------------------------------------------------------------------
/*BOOL PlugInList::AddCallback(LPPLUGINCALLBACK p)
{
   if(p == NULL || p->pPlugIn == NULL)
       return FALSE;
   if(pListCallBack == NULL)
       pListCallBack = new PlugInCallBackList();
   if(pListCallBack == NULL)
       return FALSE;
   if(!pListCallBack->Add(p))
       return FALSE;
   arm.nPluginCallback = 1;
   return TRUE;
}*/
//---------------------------------------------------------------------------
BOOL PlugInList::Enable(LPGUID p,BOOL bEnable)
{
   PlugIn *pPlugIn,*p1;
   DWORD dwPos;

   if(p == NULL || (pPlugIn = GetItemFromGUID(p)) == NULL)
       return FALSE;
   if(!pPlugIn->Enable(bEnable) && bEnable){
       Delete(IndexFromEle((LPVOID)pPlugIn));
       return FALSE;
   }
   if(!pPlugIn->IsExclusive() || !bEnable)
       return TRUE;
   p1 = pPlugIn;
   dwPos = 0;
   if((pPlugIn = (PlugIn *)GetFirstItem(&dwPos)) ==  NULL)
       return FALSE;
   do{
       if(p1 != pPlugIn){
           if(pPlugIn->IsExclusive())
               pPlugIn->Enable(FALSE);
       }
   }while((pPlugIn = (PlugIn *)GetNextItem(&dwPos)) != NULL);
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL PlugInList::Add(PlugIn *ele)
{
   GUID guid;

   if(ele == NULL)
       return FALSE;
   ele->GetGuid(&guid);
   if(GetItemFromGUID(&guid) != NULL)
       return FALSE;
   return LList::Add((LPVOID)ele);
}
//---------------------------------------------------------------------------
BOOL PlugInList::GetSelectedGUID(LPGUID p)
{
   DWORD dwPos;
   PlugIn *pPlugIn;
   PLUGININFO pi;
   BOOL bres;

   if(p == NULL)
       return FALSE;
   ZeroMemory(p,sizeof(GUID));
   bres = FALSE;
   if((pPlugIn = (PlugIn *)GetFirstItem(&dwPos)) ==  NULL)
       return bres;
   do{
       if(!pPlugIn->IsEnable())
           continue;
       ZeroMemory(&pi,sizeof(PLUGININFO));
       pi.cbSize = sizeof(PLUGININFO);
       if(!pPlugIn->GetInfo(&pi))
           break;
       *p = *(&pi.guidID);
       bres = TRUE;
   }while(!bres && (pPlugIn = (PlugIn *)GetNextItem(&dwPos)) != NULL);
   return bres;
}
//---------------------------------------------------------------------------
BOOL PlugInList::OnEnablePlug(WORD wID)
{
   BOOL res;
   PlugIn *p;
   MENUITEMINFO mi;
   GUID guid;

   if(nCount == 0)
       return FALSE;
   res = FALSE;
   ZeroMemory(&mi,sizeof(MENUITEMINFO));
   mi.cbSize = sizeof(MENUITEMINFO);
   mi.fMask = MIIM_DATA;
   GetMenuItemInfo(GetMenu(hWin),wID,FALSE,&mi);
   if((p = (PlugIn *)GetItem(mi.dwItemData + 1)) != NULL){
       p->GetGuid(&guid);
       res = Enable(&guid,!p->IsEnable());
   }
   return res;
}
//---------------------------------------------------------------------------
BOOL PlugInList::OnInitMenu(HMENU menu)
{
   int i;
   MENUITEMINFO mii;
   PlugIn *p;
   DWORD dwPos;
   LString s;
   PLUGININFO pi;

   i = GetMenuItemCount(menu)-1;
   for(;i>=0;i--)
       ::DeleteMenu(menu,i,MF_BYPOSITION);
   if(nCount < 1)
       return FALSE;
   ZeroMemory(&mii,sizeof(MENUITEMINFO));
   mii.cbSize = sizeof(MENUITEMINFO);
   p = (PlugIn *)GetFirstItem(&dwPos);
   i = 0;
   do{
       if(p->IsAttribute(0x8000))
           continue;
       mii.fMask = MIIM_TYPE|MIIM_STATE|MIIM_ID|MIIM_DATA;
       mii.fType = MFT_STRING;
       mii.fState = MFS_ENABLED;
       mii.dwItemData = i;
       if(p->IsEnable() && !p->IsAttribute(PIT_ENABLERUN))
           mii.fState |= MFS_CHECKED;
       p->InitPlugInInfo(&pi);
       s.Capacity(250);
       pi.pszText = (LPWSTR)s.c_str();
       pi.cchTextMax = 249;
       p->GetInfo(&pi,TRUE);
       mii.dwTypeData = s.c_str();
       mii.wID = pi.wID;
       InsertMenuItem(menu,i++,TRUE,&mii);
       if(!p->IsEnable())
           p->Unload();
   }while((p = (PlugIn *)GetNextItem(&dwPos)) != NULL);
   return TRUE;
}
//---------------------------------------------------------------------------
int PlugInList::Load(WORD wID,WORD wType)
{
   HANDLE handle;
   WIN32_FIND_DATA wf;
   BOOL bFlag,res;
   PlugIn *p,*p1;
   PLUGININFO pi;
   int i1,i;
   LString curDir,prgDir;

   if(!PreLoad(&wID))
       return FALSE;
   curDir.Capacity(MAX_PATH+1);
   prgDir.Capacity(MAX_PATH+1);
   GetModuleFileName(NULL,prgDir.c_str(),MAX_PATH);
   prgDir = prgDir.Path();
   GetCurrentDirectory(MAX_PATH,curDir.c_str());
   SetCurrentDirectory(prgDir.c_str());
   handle = FindFirstFile("*.dll",&wf);
   if(handle != INVALID_HANDLE_VALUE)
       bFlag = TRUE;
   else
       bFlag = FALSE;
   while(bFlag){
       if((p = BuildPlugIn(wf.cFileName)) != NULL){
           res = FALSE;
           if(p->Load()){
               p->InitPlugInInfo(&pi);
               if((i = p->GetInfo(&pi,TRUE)) != 0 && (pi.wType & 0xF) == wType){
                   p->InitPlugInInfo(&pi);
                   pi.wID = wID;
                   if(p->SetInfo(&pi) && Add(p)){
                       p->SetUsed();
                       res = TRUE;
                       wID++;
                       for(i1=2;i1<=i;i1++){
                           if((p1 = BuildPlugIn(wf.cFileName)) == NULL)
                               break;
                           p1->SetUsed();
                           p1->SetIndex((char)i1);
                           p1->InitPlugInInfo(&pi);
                           p1->GetInfo(&pi,TRUE);
                           p1->InitPlugInInfo(&pi);
                           pi.wID = wID;
                           if(p1->SetInfo(&pi) && Add(p1)){
                               wID++;
                               p1->Unload();
                           }
                           else
                               delete p1;
                       }
                   }
               }
           }
           if(!res)
               delete p;
           else
               p->Unload();
       }
       bFlag = ::FindNextFile(handle,&wf);
   }
   if(handle != INVALID_HANDLE_VALUE)
       FindClose(handle);
   SetCurrentDirectory(curDir.c_str());
   LoadSetConfig();
   return (int)wID;
}
//---------------------------------------------------------------------------
int PlugInList::Reset()
{
   int i;
   elem_list *p;

   if(nCount == 0)
       return -1;
   i = 0;
   p = First;
   do{
       i++;
       ((PlugIn *)p->Ele)->Reset();
   }while((p = p->Next) != NULL);
   return i;
}
//---------------------------------------------------------------------------
int PlugInList::NotifyState(DWORD dwState,DWORD dwStateMask)
{
   int i;
   elem_list *p;

   if(nCount == 0)
       return -1;
   i = 0;
   p = First;
   do{
       i++;
       ((PlugIn *)p->Ele)->NotifyState(dwState,dwStateMask);
   }while((p = p->Next) != NULL);
   return i;
}
//---------------------------------------------------------------------------
/*void PlugInList::IncCycles(int cycles)
{
   if(pListCallBack != NULL)
       pListCallBack->IncCycles(cycles);
}
//---------------------------------------------------------------------------
PlugInCallBackList::PlugInCallBackList() : LList()
{
}
//---------------------------------------------------------------------------
PlugInCallBackList::~PlugInCallBackList()
{
   LList::Clear();
}
//---------------------------------------------------------------------------
BOOL PlugInCallBackList::Add(LPPLUGINCALLBACK p)
{
   if(p == NULL || p->pPlugIn == NULL || p->cyclesElapsed == 0)
       return FALSE;
   if(!p->pPlugIn->IsLicensed())
       return FALSE;
   p->cyclesCounter = 0;
   return LList::Add((LPVOID)p);
}
//---------------------------------------------------------------------------
void PlugInCallBackList::IncCycles(int cycles)
{
   elem_list *p;
   LPPLUGINCALLBACK p1;

   p = First;
   while(p != NULL){
       p1 = (LPPLUGINCALLBACK)p->Ele;
       if((p1->cyclesCounter += cycles) >= p1->cyclesElapsed){
           p1->pPlugIn->NotifyState(0x400000,0x400000);
           p = p->Next;
           Delete(IndexFromEle(p1));
           if(Count() == 0)
               arm.nPluginCallback = 0;
       }
       else
           p = p->Next;
   }
}*/
