//---------------------------------------------------------------------------
#include <windows.h>
#pragma hdrstop
#include <commctrl.h>
#include <math.h>

#include "resource.h"
#include "trad.h"
#include "lregkey.h"
#include "pluginctn.h"
#include "propplug.h"
#include "fstream.h"
#include "savestate.h"

//---------------------------------------------------------------------------
static HBITMAP hbitLogo;
static HWND hwndPropertySheet;
extern UINT RGSInterleave;
extern LSaveState rewindState;
extern DWORD dwPriorityClass,bApplyIPSPath;
extern BOOL bUsePNG;
//---------------------------------------------------------------------------
static void LoadPlugIn(HWND hwnd1)
{
   PlugIn *p;
   SETPROPPLUGIN p1;
   int i,i1;
   char s[MAX_PATH],s1[MAX_PATH];
   LVITEM item;
   AudioPlugList *pAudioPlugList;
   VideoPlugList *pVideoPlugList;
   SioPlugList *pSioPlugList;

   ListView_DeleteAllItems(hwnd1);
   p1.hwndOwner = hwndPropertySheet;
   pAudioPlugList = pPlugInContainer->GetAudioPlugInList();
   pVideoPlugList = pPlugInContainer->GetVideoPlugInList();
   pSioPlugList = pPlugInContainer->GetSioPlugInList();

   for(i1=i=0;i < (signed)pAudioPlugList->Count();i++,i1++){
       p = (PlugIn *)pAudioPlugList->GetItem(i+1);
       p->InitPlugInInfo(&p1.info,p->IsEnable() & 1,PIS_ENABLEMASK);
       p1.info.pszText = (LPWSTR)s;
       p1.info.cchTextMax = 49;
       p->GetInfo(&p1.info,TRUE);

       ZeroMemory(&item,sizeof(LVITEM));
       item.mask = LVIF_TEXT;
       item.iItem = i1;
       item.pszText = s;
       ListView_InsertItem(hwnd1,&item);
       ListView_SetItemText(hwnd1,i1,1,"Audio");
       p->GetFileInfo(s1);
       ListView_SetItemText(hwnd1,i1,2,s1);
       ListView_SetItemText(hwnd1,i1,3,&s1[lstrlen(s1) + 2]);
       p->SetProperty(&p1);
   }
   for(i=0;i<(signed)pVideoPlugList->Count();i++,i1++){
       p = (PlugIn *)pVideoPlugList->GetItem(i+1);
       p->InitPlugInInfo(&p1.info,p->IsEnable() & 1,PIS_ENABLEMASK);
       p1.info.pszText = (LPWSTR)s;
       p1.info.cchTextMax = 49;
       p->GetInfo(&p1.info,TRUE);

       ZeroMemory(&item,sizeof(LVITEM));
       item.mask = LVIF_TEXT;
       item.iItem = i1;
       item.pszText = s;
       ListView_InsertItem(hwnd1,&item);
       ListView_SetItemText(hwnd1,i1,1,"Video");
       p->GetFileInfo(s1);
       ListView_SetItemText(hwnd1,i1,2,s1);
       ListView_SetItemText(hwnd1,i1,3,&s1[lstrlen(s1) + 2]);
       p->SetProperty(&p1);
   }
   for(i=0;i<(signed)pSioPlugList->Count();i++,i1++){
       p = (PlugIn *)pSioPlugList->GetItem(i+1);
       p->InitPlugInInfo(&p1.info,p->IsEnable() & 1,PIS_ENABLEMASK);
       p1.info.pszText = (LPWSTR)s;
       p1.info.cchTextMax = 49;
       p->GetInfo(&p1.info,TRUE);

       ZeroMemory(&item,sizeof(LVITEM));
       item.mask = LVIF_TEXT;
       item.iItem = i1;
       item.pszText = s;
       ListView_InsertItem(hwnd1,&item);
       ListView_SetItemText(hwnd1,i1,1,"S.I.O.");
       p->GetFileInfo(s1);
       ListView_SetItemText(hwnd1,i1,2,s1);
       ListView_SetItemText(hwnd1,i1,3,&s1[lstrlen(s1) + 2]);
       p->SetProperty(&p1);
   }
}
//---------------------------------------------------------------------------
static void OnInitDialog(HWND hwnd)
{
   HWND hwnd1;
   LVCOLUMN col;
   int i;
   char s[MAX_PATH];
   MEMORYSTATUS ms;

   Translation(hwnd,0,IDD_DIALOG14);
#if defined(__BORLANDC__)
   __asm{
       mov dword ptr[s],0
       mov dword ptr[s+4],0
       mov dword ptr[s+8],0
       mov dword ptr[s+12],0
       push ebx
       xor eax,eax
       cpuid
       mov dword ptr[s],ebx
       mov dword ptr[s+4],edx
       mov dword ptr[s+8],ecx
       pop ebx
   }
#endif
   SetWindowText(GetDlgItem(hwnd,IDC_CPU),s);
   ZeroMemory(&ms,sizeof(MEMORYSTATUS));
   ms.dwLength = sizeof(MEMORYSTATUS);
   ::GlobalMemoryStatus(&ms);
   wsprintf(s,"%d MB RAM",(int)ceil(ms.dwTotalPhys / 1048576.0));
   SetWindowText(GetDlgItem(hwnd,IDC_MEM),s);

   hbitLogo = (HBITMAP)LoadImage(hInstance,MAKEINTRESOURCE(IDI_LOGO),IMAGE_BITMAP,0,0,LR_COLOR);

   hwnd1 = GetDlgItem(hwnd,IDC_LIST1);
   i = ListView_GetExtendedListViewStyle(hwnd1);
   i |= LVS_EX_FULLROWSELECT;
   ListView_SetExtendedListViewStyle(hwnd1,i);

   ZeroMemory(&col,sizeof(LVCOLUMN));
   col.mask = LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM;
   col.fmt = LVCFMT_LEFT;
   col.cx = 150;
   col.pszText = "Plugin";
   ListView_InsertColumn(hwnd1,0,&col);
   col.cx = 50;
   col.pszText = "Type";
   ListView_InsertColumn(hwnd1,1,&col);
   col.cx = 150;
   col.pszText = "Maker";
   ListView_InsertColumn(hwnd1,3,&col);
   col.cx = 50;
   col.pszText = "Version";
   ListView_InsertColumn(hwnd1,4,&col);

   LoadPlugIn(hwnd1);
}
//---------------------------------------------------------------------------
static void OnDestroy(HWND hwnd)
{
   if(hbitLogo != NULL)
       ::DeleteObject((HGDIOBJ)hbitLogo);
   hbitLogo = NULL;
}
//---------------------------------------------------------------------------
static void KeyFileRegister(const char *key)
{
   LRegKey regKey;
   LString c,c1;

   c.Capacity(MAX_PATH+1);
   GetModuleFileName(NULL,c.c_str(),MAX_PATH);
   if(regKey.Open("RascalBoy.File\\DefaultIcon",HKEY_CLASSES_ROOT)){
       c1 = c;
       c1 += ",0";
       regKey.WriteString(NULL,c1.c_str());
       regKey.Close();
   }
   if(regKey.Open("RascalBoy.File",HKEY_CLASSES_ROOT)){
       regKey.WriteString(NULL,"RascalBoy Advance File");
       regKey.Close();
   }
   if(regKey.Open("RascalBoy.File\\Shell\\Open\\Command",HKEY_CLASSES_ROOT)){
       c += " %1";
       regKey.WriteString(NULL,c.c_str());
       regKey.Close();
   }
   if(regKey.Open((char *)key,HKEY_CLASSES_ROOT))
       regKey.WriteString(NULL,"RascalBoy.File");
}
//---------------------------------------------------------------------------
static BOOL KeyFileExists(const char *key)
{
   LRegKey regKey;
   LString c;
   BOOL res;

   res = FALSE;
   if(regKey.Open((char *)key,HKEY_CLASSES_ROOT,FALSE)){
       c = regKey.ReadString(NULL,"");
       c.LowerCase();
       if(c == "rascalboy.file")
           res = TRUE;
       regKey.Close();
   }
   return res;
}
//---------------------------------------------------------------------------
static void KeyFileRemove(const char *key)
{
   LRegKey regKey;

   if(!KeyFileExists(key))
       return;
   regKey.Delete(key,HKEY_CLASSES_ROOT);
}
//---------------------------------------------------------------------------
static BOOL CALLBACK WndProc5(HWND hwnd,UINT uMessage,WPARAM wparam,LPARAM lparam)
{
   BOOL res;
   char s[20];
   int i;
   LRegKey regKey;

   res = FALSE;
   switch(uMessage){
       case WM_CTLCOLORSTATIC:
           if(GetDlgCtrlID((HWND)lparam) == IDC_EDIT1 || GetDlgCtrlID((HWND)lparam) == IDC_EDIT2){
               SetTextColor((HDC)wparam,GetSysColor(COLOR_WINDOWTEXT));
               return (BOOL)GetSysColorBrush(COLOR_WINDOW);
           }
       break;
       case WM_INITDIALOG:
           SendDlgItemMessage(hwnd,IDC_TRACK1,TBM_SETRANGE,0,MAKELPARAM(5,20));
           SendDlgItemMessage(hwnd,IDC_TRACK1,TBM_SETPOS,(WPARAM)TRUE,rewindState.get_IndexMax());
           wsprintf(s,"%d",rewindState.get_IndexMax());
           SendDlgItemMessage(hwnd,IDC_EDIT1,WM_SETTEXT,0,(LPARAM)s);
           SendDlgItemMessage(hwnd,IDC_TRACK2,TBM_SETRANGE,0,MAKELPARAM(1,15));
           SendDlgItemMessage(hwnd,IDC_TRACK2,TBM_SETPOS,(WPARAM)TRUE,RGSInterleave);
           wsprintf(s,"%d",RGSInterleave);
           SendDlgItemMessage(hwnd,IDC_EDIT2,WM_SETTEXT,0,(LPARAM)s);
           Translation(hwnd,0,IDD_DIALOG15);
           switch(GetPriorityClass(GetCurrentProcess())){
               case IDLE_PRIORITY_CLASS:
                   SendDlgItemMessage(hwnd,IDC_RADIO11,BM_SETCHECK,BST_CHECKED,0);
               break;
               case NORMAL_PRIORITY_CLASS:
                   SendDlgItemMessage(hwnd,IDC_RADIO12,BM_SETCHECK,BST_CHECKED,0);
               break;
               case HIGH_PRIORITY_CLASS:
                   SendDlgItemMessage(hwnd,IDC_RADIO13,BM_SETCHECK,BST_CHECKED,0);
               break;
               case REALTIME_PRIORITY_CLASS:
                   SendDlgItemMessage(hwnd,IDC_RADIO14,BM_SETCHECK,BST_CHECKED,0);
               break;
           }
           if(KeyFileExists(".gba"))
               SendDlgItemMessage(hwnd,IDC_CHECK1,BM_SETCHECK,BST_CHECKED,0);
           if(KeyFileExists(".agb"))
               SendDlgItemMessage(hwnd,IDC_CHECK2,BM_SETCHECK,BST_CHECKED,0);
           if(KeyFileExists(".bin"))
               SendDlgItemMessage(hwnd,IDC_CHECK3,BM_SETCHECK,BST_CHECKED,0);
//           if((bApplyIPSPath >> 16) != 0)
//               SendDlgItemMessage(hwnd,IDC_CHECK4,BM_SETCHECK,BST_CHECKED,0);
           SendDlgItemMessage(hwnd,bUsePNG ? IDC_RADIO16 : IDC_RADIO15,BM_SETCHECK,BST_CHECKED,0);
       break;
       case WM_NOTIFY:
           switch(((LPNMHDR)lparam)->code){
               case PSN_APPLY:
                   rewindState.set_IndexMax(SendDlgItemMessage(hwnd,IDC_TRACK1,TBM_GETPOS,0,0));
                   RGSInterleave = SendDlgItemMessage(hwnd,IDC_TRACK2,TBM_GETPOS,0,0);
					rewindState.Reset();
                   EnableMenuItem(GetMenu(hWin),ID_FILE_STATE_REWINDGAME,MF_BYCOMMAND|MF_GRAYED);
                   if(SendDlgItemMessage(hwnd,IDC_RADIO11,BM_GETCHECK,0,0) == BST_CHECKED)
                       dwPriorityClass = IDLE_PRIORITY_CLASS;
                   else if(SendDlgItemMessage(hwnd,IDC_RADIO12,BM_GETCHECK,0,0) == BST_CHECKED)
                       dwPriorityClass = NORMAL_PRIORITY_CLASS;
                   else if(SendDlgItemMessage(hwnd,IDC_RADIO13,BM_GETCHECK,0,0) == BST_CHECKED)
                       dwPriorityClass = HIGH_PRIORITY_CLASS;
                   else
                       dwPriorityClass = REALTIME_PRIORITY_CLASS;
                   SetPriorityClass(GetCurrentProcess(),dwPriorityClass);
                   i = 3;
                   if(SendDlgItemMessage(hwnd,IDC_CHECK1,BM_GETCHECK,0,0) == BST_CHECKED)
                       KeyFileRegister(".gba");
                   else{
                       KeyFileRemove(".gba");
                       i--;
                   }
                   if(SendDlgItemMessage(hwnd,IDC_CHECK2,BM_GETCHECK,0,0) == BST_CHECKED)
                       KeyFileRegister(".agb");
                   else{
                       KeyFileRemove(".agb");
                       i--;
                   }
                   if(SendDlgItemMessage(hwnd,IDC_CHECK3,BM_GETCHECK,0,0) == BST_CHECKED)
                       KeyFileRegister(".bin");
                   else{
                       KeyFileRemove(".bin");
                       i--;
                   }
                   if(!i){
                       regKey.Delete("RascalBoy.File\\DefaultIcon",HKEY_CLASSES_ROOT);
                       regKey.Delete("RascalBoy.File\\Shell\\Open\\Command",HKEY_CLASSES_ROOT);
                       regKey.Delete("RascalBoy.File\\Shell\\Open",HKEY_CLASSES_ROOT);
                       regKey.Delete("RascalBoy.File\\Shell",HKEY_CLASSES_ROOT);
                       regKey.Delete("RascalBoy.File",HKEY_CLASSES_ROOT);
                   }
//                   i = SendDlgItemMessage(hwnd,IDC_CHECK4,BM_GETCHECK,0,0) == BST_CHECKED ? 1 : 0;
//                   bApplyIPSPath = (i << 16) | ((WORD)bApplyIPSPath);
                   bUsePNG = SendDlgItemMessage(hwnd,IDC_RADIO15,BM_GETCHECK,0,0) == BST_CHECKED ? FALSE : TRUE;
               break;
           }
       break;
       case WM_HSCROLL:
           switch(LOWORD(wparam)){
               case TB_THUMBTRACK:
               case TB_ENDTRACK:
                   wsprintf(s,"%d",(int)SendMessage((HWND)lparam,TBM_GETPOS,0,0));
                   switch(GetDlgCtrlID((HWND)lparam)){
                       case IDC_TRACK1:
                           SendDlgItemMessage(hwnd,IDC_EDIT1,WM_SETTEXT,0,(LPARAM)s);
                       break;
                       case IDC_TRACK2:
                           SendDlgItemMessage(hwnd,IDC_EDIT2,WM_SETTEXT,0,(LPARAM)s);
                       break;
                   }
               break;
               default:
               break;
           }
       break;
   }
   return res;
}
//---------------------------------------------------------------------------
static BOOL CALLBACK WndProc4(HWND hwnd,UINT uMessage,WPARAM wparam,LPARAM lparam)
{
   BOOL res;
   LPDRAWITEMSTRUCT lpdis;
   HDC hdc;
   BITMAP bm;
   WORD wID;
   int i1;

   res = FALSE;
   switch(uMessage){
       case WM_INITDIALOG:
           OnInitDialog(hwnd);
       break;
       case WM_COMMAND:
           wID = LOWORD(wparam);
           res = TRUE;
           switch(HIWORD(wparam)){
               case BN_CLICKED:
               case STN_DBLCLK:
                   switch(wID){
                       case IDC_RELOAD:
                           i1 = TabCtrl_GetItemCount(PropSheet_GetTabControl(hwndPropertySheet)) - 1;
                           for(;i1 > 1;i1--)
                               PropSheet_RemovePage(hwndPropertySheet,i1,NULL);
                           pPlugInContainer->Destroy();
                           pPlugInContainer->Init();
                           LoadPlugIn(GetDlgItem(hwnd,IDC_LIST1));
                       break;
                   }
               break;
           }
       break;
       case WM_DESTROY:
           OnDestroy(hwnd);
       break;
       case WM_DRAWITEM:
           lpdis = (LPDRAWITEMSTRUCT)lparam;
           switch(wparam){
               case IDC_LOGO:
               	FillRect(lpdis->hDC,&lpdis->rcItem,(HBRUSH)GetStockObject(BLACK_BRUSH));
                   if(hbitLogo != NULL){
			            GetObject(hbitLogo,sizeof(BITMAP),&bm);
                       hdc = CreateCompatibleDC(NULL);
                       SelectObject(hdc,hbitLogo);
                       BitBlt(lpdis->hDC,((lpdis->rcItem.right - bm.bmWidth) >> 1),((lpdis->rcItem.bottom- bm.bmHeight) >> 1)
                           ,bm.bmWidth,bm.bmHeight,hdc,0,0,SRCCOPY);
                       DeleteDC(hdc);
                   }
                   res = TRUE;
               break;
           }
       break;
   }
   return res;
}

//---------------------------------------------------------------------------
static int CALLBACK PropSheetProc(HWND hwndDlg,UINT uMsg,LPARAM lParam)
{
   if(uMsg == PSCB_INITIALIZED)
       hwndPropertySheet = hwndDlg;
   return 0;
}
//---------------------------------------------------------------------------
void ShowPlugInProperty()
{
   PROPSHEETHEADER psh;
   LString c,c1,c2;
   PROPSHEETPAGE psp[2];
   int i;
   PlugIn *p;
   AudioPlugList *pAudioPlugList;
   VideoPlugList *pVideoPlugList;
   SioPlugList *pSioPlugList;

   hwndPropertySheet = NULL;
   ZeroMemory(psp,sizeof(PROPSHEETPAGE)*2);
   psp[0].dwSize = sizeof(PROPSHEETPAGE);
   psp[0].dwFlags = PSP_USETITLE|PSP_DEFAULT;
   psp[0].hInstance = FindResourceInternal(IDD_DIALOG14,RT_DIALOG);
   psp[0].pszTemplate = MAKEINTRESOURCE(IDD_DIALOG14);
   psp[0].pfnDlgProc = (DLGPROC)WndProc4;
   c1 = TranslateGetMessage(IDD_DIALOG14);
   psp[0].pszTitle = c1.c_str();

   psp[1].dwSize = sizeof(PROPSHEETPAGE);
   psp[1].dwFlags = PSP_USETITLE|PSP_DEFAULT;
   psp[1].hInstance = FindResourceInternal(IDD_DIALOG15,RT_DIALOG);
   psp[1].pszTemplate = MAKEINTRESOURCE(IDD_DIALOG15);
   psp[1].pfnDlgProc = (DLGPROC)WndProc5;
   c2 = TranslateGetMessage(IDD_DIALOG15);
   psp[1].pszTitle = c2.c_str();
   ZeroMemory(&psh,sizeof(PROPSHEETHEADER));
   psh.dwSize = sizeof(PROPSHEETHEADER);
   psh.dwFlags = PSH_NOAPPLYNOW|PSH_USEICONID|PSH_PROPSHEETPAGE|PSH_USECALLBACK|PSH_DEFAULT;
   psh.pfnCallback = PropSheetProc;
   c = GetStringFromMenu(hWin,ID_SET_PROPERTY);
   MenuStringToString(c.c_str());
   psh.pszCaption = c.c_str();
   psh.pszIcon = MAKEINTRESOURCE(IDI_ICON1);
   psh.hInstance = hInstance;
   psh.hwndParent = hWin;
   psh.nPages = 2;
   psh.ppsp = psp;
   if(PropertySheet(&psh) == -1)
       return;
   pAudioPlugList = pPlugInContainer->GetAudioPlugInList();
   for(i=0;i<(signed)pAudioPlugList->Count();i++){
       p = (PlugIn *)pAudioPlugList->GetItem(i+1);
       if(!p->IsEnable())
           p->Unload();
   }
   pVideoPlugList = pPlugInContainer->GetVideoPlugInList();
   for(i=0;i<(signed)pVideoPlugList->Count();i++){
       p = (PlugIn *)pVideoPlugList->GetItem(i+1);
       if(!p->IsEnable())
           p->Unload();
   }
   pSioPlugList = pPlugInContainer->GetSioPlugInList();
   for(i=0;i<(signed)pSioPlugList->Count();i++){
       p = (PlugIn *)pSioPlugList->GetItem(i+1);
       if(!p->IsEnable())
           p->Unload();
   }
}
