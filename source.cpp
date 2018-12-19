#include <windows.h>
#include <commctrl.h>
#pragma hdrstop

#include "gbaemu.h"
#include "gba.h"
#include "source.h"
#include "winedit.h"

#ifdef _DEBPRO
//---------------------------------------------------------------------------
HWND hwndSource;
static BOOL bRegister = FALSE;
static LWinEditList *pFileList;
static LSyntaxList *pSyntaxList;
static HWND hwndFindComboBox;
static HIMAGELIST imageListDebug[2];
//---------------------------------------------------------------------------
static BOOL CALLBACK DlgProcSource(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
static void CloseDialog(int result);
static LONG FAR PASCAL WindowProc(HWND win, UINT msg, WPARAM wparam, LPARAM lparam);
static BOOL InsertNewFile(char *lpszName);
//---------------------------------------------------------------------------
void InitDebugSourceWindow()
{
   WNDCLASS wc;

   hwndSource = NULL;
   if(!bRegister){
       wc.style = CS_HREDRAW | CS_VREDRAW;
       wc.lpfnWndProc = WindowProc;
       wc.cbClsExtra = 0;
       wc.cbWndExtra = 0;
       wc.hInstance = hInstance;
       wc.hIcon = NULL;
       wc.hCursor = LoadCursor(NULL, IDC_ARROW);
       wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
       wc.lpszMenuName = NULL;
       wc.lpszClassName = "RASCALBOYSOURCEWINDOW";
       if(!RegisterClass(&wc))
           bRegister = FALSE;
       else
           bRegister = TRUE;
   }
   pFileList = NULL;
   pSyntaxList = NULL;
}
//---------------------------------------------------------------------------
void DestroyDebugSourceWindow()
{
   if(hwndFindComboBox != NULL)
       ::DestroyWindow(hwndFindComboBox);
   hwndFindComboBox = NULL;
   if(pFileList != NULL)
       delete pFileList;
   pFileList = NULL;
   if(pSyntaxList != NULL)
       delete pSyntaxList;
   pSyntaxList = NULL;
   if(hwndSource != NULL)
       ::DestroyWindow(hwndSource);
   hwndSource = NULL;
   if(bRegister)
       UnregisterClass("RASCALBOYSOURCEWINDOW",hInstance);
   bRegister = FALSE;
}
//---------------------------------------------------------------------------
u8 CreateDebugSourceWindow(HWND win)
{
   if(!bRegister)
       return 0;
   if(hwndSource == NULL){
       if((hwndSource = CreateDialog(hInstance,MAKEINTRESOURCE(IDD_DIALOG10),win,(DLGPROC)DlgProcSource)) == NULL)
           return 0;
   }
   else{
       BringWindowToTop(hwndSource);
       InvalidateRect(hwndSource,NULL,TRUE);
       UpdateWindow(hwndSource);
   }
   return 1;
}
//---------------------------------------------------------------------------
static BOOL InsertNewFile(char *lpszName)
{
   TC_ITEM tc_item;
   HWND hwnd;

   hwnd = GetDlgItem(hwndSource,IDC_TAB1);

   if(pFileList == NULL){
       if((pFileList = new LWinEditList(hwnd)) == NULL)
           return FALSE;
   }
   if(pSyntaxList == NULL)
       pSyntaxList = new LSyntaxList();
   pFileList->SetSyntaxList(pSyntaxList);

   ZeroMemory(&tc_item,sizeof(TC_ITEM));
   tc_item.mask = TCIF_TEXT;
   tc_item.pszText = lpszName;
   TabCtrl_InsertItem(hwnd,0,&tc_item);

   pFileList->Add(lpszName);

   return TRUE;
}
//---------------------------------------------------------------------------
static BOOL CreateSourceToolBar()
{
   TBBUTTON tbb[18];
   HWND hwndToolBar;
   RECT rc;
   TBBUTTONINFO tbi;
   HBITMAP bit;

   if(hwndSource == NULL)
       return FALSE;
   imageListDebug[0] = imageListDebug[1] = NULL;

   hwndToolBar = GetDlgItem(hwndSource,IDM_TOOLBAR);

   SendMessage(hwndToolBar, TB_BUTTONSTRUCTSIZE,(WPARAM) sizeof(TBBUTTON), 0);

   imageListDebug[0] = ImageList_Create(16,16,ILC_COLOR16|ILC_MASK,11,11);
   if(imageListDebug[0] == NULL)
       return FALSE;
   bit = LoadBitmap(hInstance,MAKEINTRESOURCE(IDI_TOOLBAR_DEBUG));
   if(bit == NULL)
       return FALSE;

   ImageList_AddMasked(imageListDebug[0],bit,RGB(192,192,192));
   SendMessage(hwndToolBar,TB_SETIMAGELIST,0,(LPARAM)imageListDebug[0]);
   ::DeleteObject(bit);

   tbb[0].iBitmap = 0;
   tbb[0].idCommand = ID_DEBUG_LOAD;
   tbb[0].fsState = TBSTATE_ENABLED;
   tbb[0].fsStyle = TBSTYLE_BUTTON;
   tbb[0].dwData = 0;
   tbb[0].iString = -1;

   tbb[1].iBitmap = 1;
   tbb[1].idCommand = ID_DEBUG_SAVE;
   tbb[1].fsState = TBSTATE_ENABLED;
   tbb[1].fsStyle = TBSTYLE_BUTTON;
   tbb[1].dwData = 0;
   tbb[1].iString = -1;

   tbb[2].iBitmap = 0;
   tbb[2].idCommand = 0;
   tbb[2].fsState = TBSTATE_ENABLED;
   tbb[2].fsStyle = TBSTYLE_SEP;
   tbb[2].dwData = 0;
   tbb[2].iString = -1;

   tbb[3].iBitmap = 2;
   tbb[3].idCommand = 0;
   tbb[3].fsState = TBSTATE_ENABLED;
   tbb[3].fsStyle = TBSTYLE_BUTTON;
   tbb[3].dwData = 0;
   tbb[3].iString = -1;

   tbb[4].iBitmap = 3;
   tbb[4].idCommand = 0;
   tbb[4].fsState = TBSTATE_ENABLED;
   tbb[4].fsStyle = TBSTYLE_BUTTON;
   tbb[4].dwData = 0;
   tbb[4].iString = -1;

   tbb[5].iBitmap = 4;
   tbb[5].idCommand = 0;
   tbb[5].fsState = TBSTATE_ENABLED;
   tbb[5].fsStyle = TBSTYLE_BUTTON;
   tbb[5].dwData = 0;
   tbb[5].iString = -1;

   tbb[6].iBitmap = 5;
   tbb[6].idCommand = 0;
   tbb[6].fsState = TBSTATE_ENABLED;
   tbb[6].fsStyle = TBSTYLE_BUTTON;
   tbb[6].dwData = 0;
   tbb[6].iString = -1;

   tbb[7].iBitmap = 0;
   tbb[7].idCommand = 0;
   tbb[7].fsState = TBSTATE_ENABLED;
   tbb[7].fsStyle = TBSTYLE_SEP;
   tbb[7].dwData = 0;
   tbb[7].iString = -1;

   tbb[8].iBitmap = 0;
   tbb[8].idCommand = 0x5555;
   tbb[8].fsState = 0;
   tbb[8].fsStyle = TBSTYLE_SEP;
   tbb[8].dwData = 0;
   tbb[8].iString = -1;

   tbb[9].iBitmap = 0;
   tbb[9].idCommand = 0;
   tbb[9].fsState = TBSTATE_ENABLED;
   tbb[9].fsStyle = TBSTYLE_SEP;
   tbb[9].dwData = 0;
   tbb[9].iString = -1;

   tbb[10].iBitmap = 6;
   tbb[10].idCommand = 0;
   tbb[10].fsState = TBSTATE_ENABLED;
   tbb[10].fsStyle = TBSTYLE_BUTTON;
   tbb[10].dwData = 0;
   tbb[10].iString = -1;

   tbb[11].iBitmap = 7;
   tbb[11].idCommand = 0;
   tbb[11].fsState = TBSTATE_ENABLED;
   tbb[11].fsStyle = TBSTYLE_BUTTON;
   tbb[11].dwData = 0;
   tbb[11].iString = -1;

   tbb[12].iBitmap = 0;
   tbb[12].idCommand = 0;
   tbb[12].fsState = TBSTATE_ENABLED;
   tbb[12].fsStyle = TBSTYLE_SEP;
   tbb[12].dwData = 0;
   tbb[12].iString = -1;

   tbb[13].iBitmap = 8;
   tbb[13].idCommand = 0;
   tbb[13].fsState = TBSTATE_ENABLED;
   tbb[13].fsStyle = TBSTYLE_BUTTON;
   tbb[13].dwData = 0;
   tbb[13].iString = -1;

   tbb[14].iBitmap = 0;
   tbb[14].idCommand = 0;
   tbb[14].fsState = TBSTATE_ENABLED;
   tbb[14].fsStyle = TBSTYLE_SEP;
   tbb[14].dwData = 0;
   tbb[14].iString = -1;

   tbb[15].iBitmap = 9;
   tbb[15].idCommand = 0;
   tbb[15].fsState = TBSTATE_ENABLED;
   tbb[15].fsStyle = TBSTYLE_BUTTON;
   tbb[15].dwData = 0;
   tbb[15].iString = -1;

   tbb[16].iBitmap = 0;
   tbb[16].idCommand = 0;
   tbb[16].fsState = TBSTATE_ENABLED;
   tbb[16].fsStyle = TBSTYLE_SEP;
   tbb[16].dwData = 0;
   tbb[16].iString = -1;

   tbb[17].iBitmap = 10;
   tbb[17].idCommand = 0;
   tbb[17].fsState = TBSTATE_ENABLED;
   tbb[17].fsStyle = TBSTYLE_BUTTON;
   tbb[17].dwData = 0;
   tbb[17].iString = -1;

   SendMessage(hwndToolBar, TB_ADDBUTTONS, (WPARAM)18,(LPARAM)&tbb);
   SendMessage(hwndToolBar,TB_GETITEMRECT,8,(LPARAM)&rc);

   hwndFindComboBox = ::CreateWindow("COMBOBOX",NULL,WS_VISIBLE|WS_CHILD|CBS_DROPDOWN,rc.left,rc.top,200,150,
       hwndToolBar,(HMENU)0x5555,hInstance,NULL);
   SendMessage(hwndFindComboBox,WM_SETFONT,(WPARAM)SendMessage(hwndSource,WM_GETFONT,0,0),MAKELPARAM(FALSE,0));

   ZeroMemory(&tbi,sizeof(TBBUTTONINFO));
   tbi.cbSize = sizeof(TBBUTTONINFO);
   tbi.dwMask = 0x40;
   tbi.cx = 200;
   SendMessage(hwndToolBar,TB_SETBUTTONINFO,0x5555,(LPARAM)&tbi);
   imageListDebug[1] = ImageList_LoadImage(hInstance,MAKEINTRESOURCE(IDI_TOOLBARD_DEBUG),16,11,RGB(192,192,192),IMAGE_BITMAP,LR_DEFAULTCOLOR);
   if(imageListDebug[1] == NULL)
       return 0;
   SendMessage(hwndToolBar,TB_SETDISABLEDIMAGELIST,0,(LPARAM)imageListDebug[1]);
   return 1;
}
//---------------------------------------------------------------------------
static void OnCommand(WORD notifyCode,WORD wID)
{
   switch(wID){
       case ID_DEBUG_LOAD:
           InsertNewFile("e:\\gba\\sources\\afire\\afire.c");
       break;
   }
}
//---------------------------------------------------------------------------
static void OnSelChangeTab1(LPNMHDR p)
{
   
}
//---------------------------------------------------------------------------
static BOOL CALLBACK DlgProcSource(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   BOOL res;
   RECT rc,rc1;

   res = FALSE;
   switch(uMsg){
       case WM_NOTIFY:
           if(wParam == IDC_TAB1 && ((LPNMHDR)lParam)->code == TCN_SELCHANGE)
               OnSelChangeTab1((LPNMHDR)lParam);
       break;
       case WM_COMMAND:
           OnCommand(HIWORD(wParam),LOWORD(wParam));
       break;
       case WM_INITDIALOG:
           hwndSource = hwndDlg;
           CreateSourceToolBar();
       break;
       case WM_SIZE:
           SendMessage(GetDlgItem(hwndDlg,IDM_TOOLBAR),uMsg,wParam,lParam);
           SendMessage(GetDlgItem(hwndDlg,IDM_STATUSBAR),uMsg,wParam,lParam);
       break;
       case WM_WINDOWPOSCHANGED:
           GetClientRect(hwndDlg,&rc);
           GetWindowRect(GetDlgItem(hwndDlg,IDM_STATUSBAR),&rc1);
           rc.bottom -= rc1.bottom - rc1.top;
           GetWindowRect(GetDlgItem(hwndDlg,IDM_TOOLBAR),&rc1);
           rc.top += rc1.bottom - rc1.top;
           rc.bottom -= rc1.bottom - rc1.top;           
           InflateRect(&rc,-((GetSystemMetrics(SM_CXEDGE)<<1)+2),-((GetSystemMetrics(SM_CYEDGE)<<1)+2));
           SetWindowPos(GetDlgItem(hwndDlg,IDC_TAB1),NULL,0,rc.top,rc.right,rc.bottom,SWP_NOREPOSITION|SWP_NOSENDCHANGING);
           if(pFileList != NULL)
               pFileList->RepositionWindows();
       break;
       case WM_CLOSE:
           CloseDialog(0);
           res = TRUE;
       break;
       case WM_DESTROY:
           hwndSource = NULL;
       break;
       default:
       break;
   }
   return res;
}
//---------------------------------------------------------------------------
static void CloseDialog(int result)
{
   if(hwndFindComboBox != NULL)
       ::DestroyWindow(hwndFindComboBox);
   hwndFindComboBox = NULL;
   if(pFileList != NULL)
       delete pFileList;
   pFileList = NULL;
   if(pSyntaxList != NULL)
       delete pSyntaxList;
   pSyntaxList = NULL;
   if(imageListDebug[0] != NULL)
       ImageList_Destroy(imageListDebug[0]);
   if(imageListDebug[1] != NULL)
       ImageList_Destroy(imageListDebug[1]);
   imageListDebug[0] = imageListDebug[1] = NULL;
   ::DestroyWindow(hwndSource);
   hwndSource = NULL;
}
//---------------------------------------------------------------------------
static LONG FAR PASCAL WindowProc(HWND win, UINT msg, WPARAM wparam, LPARAM lparam)
{
   LRESULT res;
   BOOL bFlag;
   PAINTSTRUCT ps;
   WinEdit *pWin;

   bFlag = FALSE;
   pWin = (WinEdit *)GetWindowLong(win,GWL_USERDATA);
   switch(msg){
       case WM_SIZE:
           if(pWin != NULL)
               pWin->OnSize(LOWORD(lparam),HIWORD(lparam));
       break;
       case WM_ERASEBKGND:
           if(pWin != NULL){
               bFlag = TRUE;
               res = 1;
               pWin->OnEraseBackground((HDC)wparam);
           }
       break;
       case WM_PAINT:
           res = 1;
           if(pWin != NULL){
               ::BeginPaint(win,&ps);
               pWin->OnDraw(ps.hdc,&ps.rcPaint);
               ::EndPaint(win,&ps);
               bFlag = TRUE;
               res = 0;
           }
       break;
       case WM_VSCROLL:
           if(pWin != NULL){
               pWin->OnVScroll(LOWORD(wparam),HIWORD(wparam),(HWND)lparam);
               bFlag = TRUE;
               res = 0;
           }
       break;
       case WM_HSCROLL:
           if(pWin != NULL){
               pWin->OnHScroll(LOWORD(wparam),HIWORD(wparam),(HWND)lparam);
               bFlag = TRUE;
               res = 0;
           }
       break;
   }
   if(!bFlag)
       res = DefWindowProc(win, msg, wparam, lparam);
   return res;
}
#endif
