#include "trad.h"
#include "lregkey.h"
#include "unit1.h"

static HINSTANCE hLib;
LTradList *pLanguageList;
//---------------------------------------------------------------------------
LTradList::LTradList() : LList()
{
   iLanguageID = -1;
   wCodePage = -1;
}
//---------------------------------------------------------------------------
LTradList::~LTradList()
{
   Clear();
}
//---------------------------------------------------------------------------
void LTradList::DeleteElem(LPVOID ele)
{
   if(ele != NULL)
       delete (LPTRAD *)ele;
}
//---------------------------------------------------------------------------
static int tradSort(LPVOID ele1,LPVOID ele2)
{
   LPTRAD p,p1;

   p = (LPTRAD)ele1;
   p1 = (LPTRAD)ele2;
   return lstrcmp(p->Name.c_str(),p1->Name.c_str());
}
//---------------------------------------------------------------------------
static BOOL CALLBACK EnumThreadWindowProc(HWND hwnd,LPARAM)
{
   DWORD dw;

   dw = GetWindowLong(hwnd,GWL_USERDATA);
   Translation(hwnd,LOWORD(dw),HIWORD(dw));
   InvalidateRect(hwnd,NULL,TRUE);
   UpdateWindow(hwnd);
   DrawMenuBar(hwnd);
   return TRUE;
}
//---------------------------------------------------------------------------
HINSTANCE FindResourceInternal(WORD wID,LPCTSTR lpType)
{
   if(hLib == NULL)
       return hInstance;
   if(FindResource(hLib,MAKEINTRESOURCE(wID),lpType))
       return hLib;
   return hInstance;
}
//---------------------------------------------------------------------------
BOOL LTradList::FindLanguage(WORD wID,LPDWORD item)
{
   LPTRAD lpTrad;
   DWORD dwPos,i;
   BOOL bFlag;

   lpTrad = (LPTRAD)pLanguageList->GetFirstItem(&dwPos);
   i = 0;
   bFlag = FALSE;
   while(lpTrad != NULL){
       i++;
       if(wID == (WORD)lpTrad->wLanguageID){
           bFlag = TRUE;
           break;
       }
       lpTrad = (LPTRAD)pLanguageList->GetNextItem(&dwPos);
   }
   if(!bFlag)
       *item = 0;
   else
       *item = i;
   return bFlag;
}
//---------------------------------------------------------------------------
BOOL LTradList::SetLanguageID(WORD id,BOOL bRedraw)
{
   LPTRAD lpTrad;
   DWORD dw;
   LString s;

   ResetMainPlugInMenu(GetMenu(hWin));
   if(!FindLanguage(id,&dw))
       FindLanguage((id = 0x410),&dw);
   if(id == iLanguageID)
       return TRUE;
   lpTrad = (LPTRAD)pLanguageList->GetItem(dw);
   if(lpTrad == NULL)
       return FALSE;
   if(hLib != NULL)
       FreeLibrary(hLib);
   hLib = NULL;
   if(!lpTrad->Path.IsEmpty()){
       hLib = LoadLibrary(lpTrad->Path.c_str());
       if(hLib == NULL){
           SetSubCodeError(-10);
           ShowMessageError(TE_MAIN,lpTrad->Path.c_str());
           pLanguageList->Delete(dw);
           FindLanguage((id = 0x410),&dw);
           lpTrad = (LPTRAD)pLanguageList->GetItem(dw);
       }
   }
   iLanguageID = lpTrad->wLanguageID;
   wCodePage = lpTrad->wCodePage;
   if(bRedraw)
       EnumThreadWindows(GetCurrentThreadId(),(WNDENUMPROC)EnumThreadWindowProc,NULL);
   return TRUE;
}
//---------------------------------------------------------------------------
LPTRAD CreateNewTrad(char *path)
{
   LPTRAD lpTrad;
   DWORD dwLen,lpPointer,dwByte;
   LPBYTE lpBuf;
   WORD plID;
   char *string;
   BOOL WINAPI (*pCheckValidation)(LPVOID);
   HINSTANCE hLibrary;

   lpTrad = NULL;
   lpBuf = NULL;
   hLibrary = NULL;
   if((string = new char[MAX_PATH]) == NULL)
       goto Ex_CreateNewTrad;
   if(path == NULL)
       GetModuleFileName(GetModuleHandle(NULL),string,MAX_PATH-1);
   else
       lstrcpy(string,path);
   dwLen = GetFileVersionInfoSize(string,&dwLen);
   if(dwLen == 0)
       goto Ex_CreateNewTrad;
   if((lpBuf = new BYTE[dwLen+10]) == NULL)
       goto Ex_CreateNewTrad;
   if(::GetFileVersionInfo(string,0,dwLen,(LPVOID)lpBuf)){
       if(::VerQueryValue((LPVOID)lpBuf,"\\VarFileInfo\\Translation",(LPVOID *)&lpPointer,(UINT*)&dwByte))
           plID =  ((LPWORD)lpPointer)[0];
       else
           goto Ex_CreateNewTrad;
   }
   else
       goto Ex_CreateNewTrad;
   GetLocaleInfo(MAKELCID(plID,SORT_DEFAULT),LOCALE_NOUSEROVERRIDE|LOCALE_SLANGUAGE,(LPTSTR)lpBuf,dwLen);
   if(pLanguageList->FindLanguage(plID,&dwByte))
       goto Ex_CreateNewTrad;
   if(path != NULL){
       if((hLibrary = ::LoadLibrary(path)) == NULL)
           goto Ex_CreateNewTrad;
       (FARPROC)pCheckValidation = GetProcAddress(hLibrary,"CheckValidation");
       if(pCheckValidation == NULL || !pCheckValidation(NULL))
           goto Ex_CreateNewTrad;
   }
   if((lpTrad = new TRAD) == NULL)
       goto Ex_CreateNewTrad;
   lpTrad->Path = path;
   lpTrad->Name = (char *)lpBuf;
   lpTrad->wLanguageID = plID;
   lpTrad->wCodePage = ((LPWORD)lpPointer)[1];
Ex_CreateNewTrad:
   if(hLibrary != NULL)
       ::FreeLibrary(hLibrary);
   if(lpBuf != NULL)
       delete []lpBuf;
   if(string != NULL)
       delete []string;
   return lpTrad;
}
//---------------------------------------------------------------------------
BOOL TranslateLoadString(LString *string,UINT wID)
{
   BOOL res;

   if((res = string->LoadString(wID,hLib)) == 0 && hLib != NULL)
       res = string->LoadString(wID);
   return res;
}
//---------------------------------------------------------------------------
BOOL InitTranslation()
{
   HANDLE handle;
   WIN32_FIND_DATA wf;
   BOOL res,bFlag;
   LPTRAD lpTrad;
   LRegKey KeyReg;
   DWORD dw;
   LString curDir,prgDir,file;

   hLib = NULL;
   pLanguageList = new LTradList();
   if(pLanguageList == NULL)
       return FALSE;
   if((lpTrad = CreateNewTrad(NULL)) == NULL)
       return FALSE;
   pLanguageList->Add((LPVOID)lpTrad);
   curDir.Capacity(MAX_PATH+1);
   prgDir.Capacity(MAX_PATH+1);
   GetModuleFileName(NULL,prgDir.c_str(),MAX_PATH);
   prgDir = prgDir.Path();
   GetCurrentDirectory(MAX_PATH,curDir.c_str());
   SetCurrentDirectory(prgDir.c_str());
   res = TRUE;
   handle = FindFirstFile("*.dll",&wf);
   if(handle != INVALID_HANDLE_VALUE)
       bFlag = TRUE;
   else
       bFlag = FALSE;
   while(bFlag){
       file = prgDir;
       file += "\\";
       file += wf.cFileName;
       if((lpTrad = CreateNewTrad(file.c_str())) != NULL){
           pLanguageList->Add((LPVOID)lpTrad);
           res = TRUE;
       }
       bFlag = ::FindNextFile(handle,&wf);
   }
   if(handle != INVALID_HANDLE_VALUE)
       FindClose(handle);
   SetCurrentDirectory(curDir.c_str());
   pLanguageList->Sort(1,pLanguageList->Count(),tradSort);
   hLib = NULL;
   if(res){
       dw = GetSystemDefaultLCID();
       if(KeyReg.Open("Software\\RascalBoy"))
           dw = KeyReg.ReadLong("LanguageID",dw);
       pLanguageList->SetLanguageID(LANGIDFROMLCID(dw),FALSE);
   }
   return TRUE;
}
//---------------------------------------------------------------------------
void DestroyTranslation()
{
   LRegKey KeyReg;

   if(hLib != NULL)
       ::FreeLibrary(hLib);
   hLib = NULL;
   if(KeyReg.Open("Software\\RascalBoy")){
       KeyReg.WriteLong("LanguageID",pLanguageList->GetLanguageID());
       KeyReg.Close();
   }
   if(pLanguageList != NULL)
       delete pLanguageList;
   pLanguageList = NULL;
}
//---------------------------------------------------------------------------
static BOOL CALLBACK EnumChildProc(HWND hwnd,LPARAM lParam)
{
   LString s;
   int wID;

   (*((LPDWORD)lParam))++;
   wID = GetDlgCtrlID(hwnd);
   if(wID != NULL){
       if(TranslateLoadString(&s,wID))
           ::SetWindowText(hwnd,s.c_str());
   }
   return TRUE;
}
//---------------------------------------------------------------------------
static DWORD TranslateControl(HWND win)
{
   DWORD dwWindow;

   dwWindow = 0;
   EnumChildWindows(win,(WNDENUMPROC)EnumChildProc,(LPARAM)&dwWindow);
   return dwWindow;
}
//---------------------------------------------------------------------------
LString GetStringFromMenu(HWND hwnd,UINT wID,BOOL bPosition)
{
   LString s;
   MENUITEMINFO mi;
   HMENU hMenu;

   s = "";
   hMenu = GetMenu(hwnd);
   if(hMenu == NULL)
       return s;
   ZeroMemory(&mi,sizeof(MENUITEMINFO));
   mi.cbSize = sizeof(MENUITEMINFO);
   mi.fMask = MIIM_TYPE;
   s.Capacity(100);
   mi.dwTypeData = s.c_str();
   mi.cch = 99;
   GetMenuItemInfo(hMenu,wID,bPosition,&mi);

   return s;
}
//---------------------------------------------------------------------------
LString TranslateGetMessage(UINT wID)
{
   LString s;

   TranslateLoadString(&s,wID);
   return s;
}
//---------------------------------------------------------------------------
static int TranslateMenuItem(HMENU hMenu,int wID)
{
   LString s;
   MENUITEMINFO mi;

   if(hMenu == NULL || wID < 0)
       return 0;
   if(TranslateLoadString(&s,wID)){
       ZeroMemory(&mi,sizeof(MENUITEMINFO));
       mi.cbSize = sizeof(MENUITEMINFO);
       mi.fMask = MIIM_TYPE;
       mi.dwTypeData = s.c_str();
       SetMenuItemInfo(hMenu,wID,FALSE,&mi);
   }
   return 1;
}
//---------------------------------------------------------------------------
int TranslateMenu(HMENU hMenu,WORD wID)
{
   int itemCount,i,nItem;
   LString s;
   MENUITEMINFO mi;

   if(hMenu == NULL)
       return 0;
   itemCount = GetMenuItemCount(hMenu);
   for(nItem = i = 0;i<itemCount;i++){
       ZeroMemory(&mi,sizeof(MENUITEMINFO));
       mi.cbSize = sizeof(MENUITEMINFO);
       mi.fMask = MIIM_SUBMENU|MIIM_ID;
       GetMenuItemInfo(hMenu,i,TRUE,&mi);
       if(mi.hSubMenu == NULL){
           TranslateMenuItem(hMenu,mi.wID);
           continue;
       }
       s = TranslateGetMessage(wID + i + nItem);
       nItem += TranslateMenu(mi.hSubMenu,(WORD)(wID+ i + nItem + 1));
       if(s.IsEmpty())
           continue;
       ZeroMemory(&mi,sizeof(MENUITEMINFO));
       mi.cbSize = sizeof(MENUITEMINFO);
       mi.fMask = MIIM_TYPE;
       mi.dwTypeData = s.c_str();
       SetMenuItemInfo(hMenu,i,TRUE,&mi);
   }
   return itemCount + nItem;
}
//---------------------------------------------------------------------------
static int TranslateMainMenu(HWND win,UINT idMenu)
{
   HMENU hMenu;
   int itemCount,i;
   LString s;
   MENUITEMINFO mi;
   WORD wID;

   if((hMenu = GetMenu(win)) == NULL)
       return 0;
   itemCount = GetMenuItemCount(hMenu);
   for(i=0;i<itemCount;i++){
       wID = (WORD)(0xc000 + ((idMenu - IDR_MAINFRAME) << 10) + (i << 7));
       s = TranslateGetMessage(wID++);
       if(!s.IsEmpty()){
           ZeroMemory(&mi,sizeof(MENUITEMINFO));
           mi.cbSize = sizeof(MENUITEMINFO);
           mi.fMask = MIIM_TYPE;
           mi.fType = MFT_STRING;
           mi.dwTypeData = s.c_str();
           SetMenuItemInfo(hMenu,i,TRUE,&mi);
       }
       TranslateMenu(GetSubMenu(hMenu,i),wID);
   }
   return itemCount;
}
//---------------------------------------------------------------------------
BOOL Translation(HWND win,UINT idMenu,UINT iID)
{                     
   LString s;
   DWORD dw;

   if(win == NULL || !IsWindow(win))
       return FALSE;
   dw = MAKELONG(idMenu,iID);
   ::SetWindowLong(win,GWL_USERDATA,dw);
   TranslateMainMenu(win,idMenu);
   TranslateControl(win);
   s = TranslateGetMessage(iID);
   if(!s.IsEmpty())
       SetWindowText(win,s.c_str());
   return TRUE;
}

