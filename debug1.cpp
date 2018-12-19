#include "gba.h"
#include "gbaemu.h"
#include "debug.h"
#include "memory.h"
#include "resource.h"
#include "lcd.h"
#include "debsprite.h"
#include "debmem.h"
#include "debpal.h"
#include "debpalobj.h"
#include "debug1.h"
#include "trad.h"
#include "inputtext.h"
#include "fstream.h"
#include "debbkg.h"
#include "source.h"
#include "brkmem.h"
#include "exec.h"
#include <commctrl.h>

#ifdef _DEBUG
//---------------------------------------------------------------------------
static void OnToolTipToolBar(LPNMHDR p);
//---------------------------------------------------------------------------
static WNDPROC OldComboBoxBreakPointWndProc,OldEditBreakPointWndProc;
static WNDPROC OldListBoxMessageWndProc,OldCaptionBarWndProc,OldListBoxAssemblerWndProc,OldListBoxStackWndProc;
static WNDPROC OldListBoxProcedureWndProc;
static HWND hwndListBoxComboBox1,hwndCaptionBar,hwndTemp;
static UINT dwCountTimer,wTimerSB,IDmessage;
static HIMAGELIST imageListDebug[2];
static FINDREPLACE fr;
extern LFile *fpDebug;
WNDPROC OldListBoxBreakPointWndProc;
HMENU hDebugPopupMenu;
UINT wTimerBreakPointID;
HWND hwndDebugStatusBar,hwndDebugToolBar,hwndDebugComboBox1;
//---------------------------------------------------------------------------
void FillComboBreakPoint()
{
   u8 i;

   SendMessage(hwndDebugComboBox1,CB_RESETCONTENT,0,0);
   for(i=0;i<MAX_BREAK;i++){
       if(listBreakPoint[i].adress != 0 && listBreakPoint[i].Type == BT_PROGRAM)
           SendMessage(hwndDebugComboBox1,CB_ADDSTRING,0,(LPARAM)i + 1);
   }
}
//---------------------------------------------------------------------------
WORD GetBreakPointFromPos(int y)
{
   u8 i,i1;

   i = (u8)SendMessage(hwndDebugComboBox1,CB_GETTOPINDEX,0,0);
   i1 = (u8)SendMessage(hwndDebugComboBox1,CB_GETITEMHEIGHT,i,0);
   i += (u8)(y / i1);

   return (WORD)i;
}
#ifdef _DEBPRO
//---------------------------------------------------------------------------
void EnableBreak(HMENU hMenu,u8 nBreak,WORD wID)
{
   MENUITEMINFO mii;

   ZeroMemory(&mii,sizeof(MENUITEMINFO));
   mii.cbSize = sizeof(MENUITEMINFO);
   mii.fMask = MIIM_STATE;
   if(!MAIN_MESSAGE.bBreak[nBreak]){
       MAIN_MESSAGE.bBreak[nBreak] = TRUE;
       mii.fState = MFS_CHECKED;
   }
   else{
       MAIN_MESSAGE.bBreak[nBreak] = FALSE;
       mii.fState = MFS_UNCHECKED;
   }
   SetMenuItemInfo(hMenu,wID,FALSE,&mii);
}
//---------------------------------------------------------------------------
void EnableMessage(HMENU hMenu,u8 message,WORD wID)
{
   MENUITEMINFO mii;

   ZeroMemory(&mii,sizeof(MENUITEMINFO));
   mii.cbSize = sizeof(MENUITEMINFO);
   mii.fMask = MIIM_STATE;
   if(!MAIN_MESSAGE.bValue[message]){
       MAIN_MESSAGE.bValue[message] = TRUE;
       mii.fState = MFS_CHECKED;
   }
   else{
       MAIN_MESSAGE.bValue[message] = FALSE;
       mii.fState = MFS_UNCHECKED;
   }
   SetMenuItemInfo(hMenu,wID,FALSE,&mii);
}
#endif
//---------------------------------------------------------------------------
LRESULT CALLBACK EditBreakPointWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   LRESULT res;
   u8 bFlag,s[30];

   bFlag = 1;
   switch(uMsg){
       case WM_SETFOCUS:
           EnableWindow(GetDlgItem(hwndDebug,IDC_BUTTON1),FALSE);
       break;
       case WM_KILLFOCUS:
           EnableWindow(GetDlgItem(hwndDebug,IDC_BUTTON1),TRUE);
       break;
       case WM_KEYUP:
           if(wParam == VK_RETURN){
               ::GetWindowText(hwnd,(LPTSTR)s,20);
               InsertBreakPoint(StrToHex((char *)s));
               SetFocus(GetDlgItem(hwndDebug,IDC_LIST1));
               res = 0;
               bFlag = 0;
           }
       break;
   }
   if(bFlag != 0)
       res = (LRESULT)CallWindowProc(OldEditBreakPointWndProc,hwnd,uMsg,wParam,lParam);
   return res;
}
#ifdef _DEBPRO
//---------------------------------------------------------------------------
static void WriteAllToFile()
{
   LFile *fp;
   DWORD d,dw;
   u32 opcode;
   u8 inc;
   char riga[300];

   fp = new LFile("debug.txt");
   if(fp == NULL || !fp->Open(GENERIC_WRITE,OPEN_ALWAYS)){
       if(fp != NULL)
           delete fp;
       return;
   }
   d = arm.ra;
   if((CPSR & T_BIT) != 0){
       if((REG_PC - arm.ra) > 4)
           d = REG_PC - 2;
       d &= ~1;
   }
   else{
       if((REG_PC - arm.ra) > 8)
           d = REG_PC - 4;
   }
   dw = ((d & 0xF000000) | bin.rom_size_u8);
   inc = (u8)((CPSR & T_BIT) ? 2 : 4);
   for(;d < dw;d += inc){
       riga[0] = 0;
       if(inc == 2){
           opcode = ReadMem(d,AMM_HWORD);
           debug_handles_t[opcode >> 6]((u16)opcode,d,riga);
       }
       else{
           opcode = ReadMem(d,AMM_WORD);
           debug_handles[((opcode&0xFF00000)>>16)|((opcode&0xF0)>>4)](opcode, d,riga);
       }
       fp->WriteF("%08X : %08X %s\r\n",d,opcode,riga);
   }
   delete fp;
}
//---------------------------------------------------------------------------
LRESULT CALLBACK ListBoxProcedureWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   LRESULT res;
   u8 bFlag;
   POINT pt;
   WORD wID;
   int i,i1;
	LString s;

   bFlag = 1;
   switch(uMsg){
       case WM_RBUTTONDOWN:
               hDebugPopupMenu = ::LoadMenu(hInstance,MAKEINTRESOURCE(IDR_PROCEDURE));
               pt.x = LOWORD(lParam);
               pt.y = HIWORD(lParam);
               ::ClientToScreen(hwnd,&pt);
               TranslateMenu(hDebugPopupMenu,0);
               ::TrackPopupMenuEx(GetSubMenu(hDebugPopupMenu,0),TPM_LEFTALIGN|TPM_LEFTBUTTON,pt.x,pt.y,hwnd,NULL);
               bFlag = 0;
       break;
       case WM_COMMAND:
           wID = LOWORD(wParam);
           if(HIWORD(wParam) < 2){
               switch(wID){
                   case ID_PROCEDURE_EDIT:
                       ShowEditProcedure();
                   break;
                   case ID_PROCEDURE_DEL:
                       i = SendMessage(hwnd,LB_GETCURSEL,0,0);
                       if(i != LB_ERR)
                           SendMessage(hwnd,LB_DELETESTRING,(WPARAM)i,0);
                   break;
                   case ID_PROCEDURE_DELALL:
                       SendMessage(hwnd,LB_RESETCONTENT,0,0);
                   break;
                   case ID_PROCEDURE_MESSAGE:
                       SendMessage(hwndDebug,WM_COMMAND,MAKEWPARAM(ID_DEBUG_PROCEDURE,0),0);
                   break;
                   case ID_PROCEDURE_GO:
                   	i = SendMessage(hwnd,LB_GETCURSEL,0,0);
                       if(i != LB_ERR){
                       	i1 = SendMessage(hwnd,LB_GETTEXTLEN,i,0);
                           if(i1 > 0){
                               s.Capacity(i1+1);
                               SendMessage(hwnd,LB_GETTEXT,i,(LPARAM)s.c_str());
                               s.Length(8);
                               SendDlgItemMessage(hwndDebug,IDC_EDIT2,WM_SETTEXT,0,(LPARAM)s.c_str());
                       		SendMessage(hwndDebug,WM_COMMAND,MAKEWPARAM(IDC_BUTTON1,BN_CLICKED),(LPARAM)GetDlgItem(hwndDebug,IDC_BUTTON1));
                           }
                       }
                   break;
               }
               if(hDebugPopupMenu != NULL)
                   ::DestroyMenu(hDebugPopupMenu);
               hDebugPopupMenu = NULL;
           }
       break;
   }
   if(bFlag != 0)
       res = CallWindowProc(OldListBoxProcedureWndProc,hwnd,uMsg,wParam,lParam);
   return res;
}
//---------------------------------------------------------------------------
u8 ShowEditProcedure()
{
   int Index,i;
   POINT pt;
   RECT rc;
   LString s,s1;

   if((Index = SendDlgItemMessage(hwndDebug,IDC_LIST4,LB_GETCURSEL,0,0)) == LB_ERR)
       return 0;
   SendDlgItemMessage(hwndDebug,IDC_LIST4,LB_GETITEMRECT,(WPARAM)Index,(LPARAM)&rc);
   pt.x = rc.left;
   pt.y = rc.top;
   ClientToScreen(GetDlgItem(hwndDebug,IDC_LIST4),&pt);
   ScreenToClient(hwndDebug,&pt);
   i = SendDlgItemMessage(hwndDebug,IDC_LIST4,LB_GETTEXTLEN,(WPARAM)Index,0);
   s.Capacity(i + 1);
   SendDlgItemMessage(hwndDebug,IDC_LIST4,LB_GETTEXT,(WPARAM)Index,(LPARAM)s.c_str());
   if(i > 11){
       s1 = (char *)(s.c_str() + 11);
       s.Length(11);
   }
   else
       s1 = "";
   s1.Capacity(200);
	rc.left = pt.x + 60;
   rc.right -= rc.left;
	rc.bottom -= rc.top;
   rc.top = pt.y;
   rc.bottom += rc.top;
	if(!InputText(hwndDebug,&rc,0,s1.c_str(),199)){
       if(i < 12)
   		SendDlgItemMessage(hwndDebug,IDC_LIST4,LB_DELETESTRING,(WPARAM)Index,0);
   	return 0;
   }
   SendDlgItemMessage(hwndDebug,IDC_LIST4,LB_DELETESTRING,(WPARAM)Index,0);
   s += s1;
   SendDlgItemMessage(hwndDebug,IDC_LIST4,LB_ADDSTRING,0,(LPARAM)s.c_str());

   return 1;
}
#endif
//---------------------------------------------------------------------------
LRESULT CALLBACK ListBoxAssemblerWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   u8 bFlag;
   LRESULT res;
   WORD wID;
   POINT pt;
   int i,i1;
   LString s,s1;
   HGLOBAL handle;
   HWND hWnd;
   
   bFlag = 1;
   switch(uMsg){
       case WM_MOUSEWHEEL:
           i = (int)(short)HIWORD(wParam) / -120.0;
           i *= GetCurrentIncPipe(DBV_VIEW);
           SetScrollPos(GetDlgItem(hwndDebug,IDC_VSBDIS),SB_CTL,dbg.yScroll+i,TRUE);
           SendMessage(hwndDebug,WM_VSCROLL,MAKEWPARAM(SB_THUMBPOSITION,0),(LPARAM)GetDlgItem(hwndDebug,IDC_VSBDIS));
       break;
       case WM_KEYDOWN:
           switch(wParam){
               case VK_HOME:
                   SendMessage(hwndDebug,WM_VSCROLL,MAKEWPARAM(SB_TOP,0),(LPARAM)GetDlgItem(hwndDebug,IDC_VSBDIS));
               break;
               case VK_END:
                   SendMessage(hwndDebug,WM_VSCROLL,MAKEWPARAM(SB_BOTTOM,0),(LPARAM)GetDlgItem(hwndDebug,IDC_VSBDIS));
               break;
               case VK_UP:
                   i = SendMessage(hwnd,LB_GETCARETINDEX,0,0);
                   if(i == 0)
                       SendMessage(hwndDebug,WM_VSCROLL,MAKEWPARAM(SB_LINEUP,0),(LPARAM)GetDlgItem(hwndDebug,IDC_VSBDIS));
               break;
               case VK_DOWN:
                   i = SendMessage(hwnd,LB_GETCARETINDEX,0,0);
                   if(i == (dbg.iMaxItem-1)){
                       SendMessage(hwndDebug,WM_VSCROLL,MAKEWPARAM(SB_LINEDOWN,0),(LPARAM)GetDlgItem(hwndDebug,IDC_VSBDIS));
                       SendMessage(hwnd,LB_SETCARETINDEX,dbg.iMaxItem-1,0);
                   }
               break;
               case VK_PRIOR:
                   SendMessage(hwndDebug,WM_VSCROLL,MAKEWPARAM(SB_PAGEUP,0),(LPARAM)GetDlgItem(hwndDebug,IDC_VSBDIS));
               break;
               case VK_NEXT:
                   SendMessage(hwndDebug,WM_VSCROLL,MAKEWPARAM(SB_PAGEDOWN,0),(LPARAM)GetDlgItem(hwndDebug,IDC_VSBDIS));
               break;
           }
       break;
       case WM_RBUTTONDOWN:
               hDebugPopupMenu = ::LoadMenu(hInstance,MAKEINTRESOURCE(IDR_ASSEMBLER));
               pt.x = LOWORD(lParam);
               pt.y = HIWORD(lParam);
               ::ClientToScreen(hwnd,&pt);
               TranslateMenu(hDebugPopupMenu,0xC000 + ((IDR_ASSEMBLER - IDR_MAINFRAME) << 10));
               ::TrackPopupMenuEx(GetSubMenu(hDebugPopupMenu,0),TPM_LEFTALIGN|TPM_LEFTBUTTON,pt.x,pt.y,hwnd,NULL);
               bFlag = 0;
       break;
       case WM_COMMAND:
           wID = LOWORD(wParam);
           if(HIWORD(wParam) < 2){
               switch(wID){
#ifdef _DEBPRO
                   case ID_ASSEMBLER_PROCEDURE:
                       i = SendMessage(hwnd,LB_GETCURSEL,0,0);
                       if(i == LB_ERR)
                           break;
                       i1 = TabCtrl_GetCurSel((hWnd = GetDlgItem(hwndDebug,IDC_TAB1)));
                       if(i1 == 0){
							TabCtrl_SetCurSel(hWnd,1);
                           OnChangePageTab1();
                       }
                       wID = (WORD)SendMessage(hwnd,LB_GETTEXTLEN,(WPARAM)i,0);
                       s.Capacity(wID + 1);
                       SendMessage(hwnd,LB_GETTEXT,(WPARAM)i,(LPARAM)s.c_str());
                       s1.Capacity(10);
                       lstrcpyn(s1.c_str(),s.c_str(),9);
                       s1 += " : ";
                       i1 = SendDlgItemMessage(hwndDebug,IDC_LIST4,LB_ADDSTRING,0,(LPARAM)s1.c_str());
                       SendDlgItemMessage(hwndDebug,IDC_LIST4,LB_SETCURSEL,(WPARAM)i1,0);
                       ShowEditProcedure();
                   break;
#endif
                   case ID_ASSEMBLER_COPY:
                       i = SendMessage(hwnd,LB_GETCURSEL,0,0);
                       if(i != LB_ERR){
                           wID = (WORD)SendMessage(hwnd,LB_GETTEXTLEN,(WPARAM)i,0);
                           s.Capacity(wID + 1);
                           SendMessage(hwnd,LB_GETTEXT,(WPARAM)i,(LPARAM)s.c_str());
                           handle = GlobalAlloc(GPTR|GMEM_SHARE,13);
                           lstrcpy((char *)handle,"0x");
                           strncat((char *)handle,s.c_str(),8);
                           OpenClipboard(hwnd);
                           EmptyClipboard();
                           SetClipboardData(CF_TEXT,handle);
                           CloseClipboard();
                           GlobalFree(handle);
                       }
                   break;
               }
               if(hDebugPopupMenu != NULL)
                   DestroyMenu(hDebugPopupMenu);
               hDebugPopupMenu = NULL;
           }
       break;
   }
   if(bFlag != 0)
       res = (LRESULT)CallWindowProc(OldListBoxAssemblerWndProc,hwnd,uMsg,wParam,lParam);
   return res;
}
//---------------------------------------------------------------------------
int GetBreakpointIndexFromIndex(int index)
{
   int i;

   for(i = 0;index >= 0 && i < MAIN_MESSAGE.nBreak;i++){
       if(listBreakPoint[i].Type == BT_PROGRAM)
           index--;
   }
   return (i-1);
}
//---------------------------------------------------------------------------
LRESULT CALLBACK ListBoxBreakPointWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   WORD wID;
   LRESULT res;
   u8 bFlag,i;
#ifdef _DEBPRO
   int i1;
   RECT rc;
#endif
   POINT pt;
   HGLOBAL pString;
   HWND win;

   bFlag = 1;
   switch(uMsg){
       case WM_COMMAND:
           wID = LOWORD(wParam);
           if(HIWORD(wParam) < 2){
               switch(wID){
#ifdef _DEBPRO
                   case ID_BREAKPOINT_CONDITON:
                       ::DialogBox(hInstance,MAKEINTRESOURCE(IDD_DIALOG6),hwndDebug,(DLGPROC)DlgProcBRK);
                       bFlag = 0;
                   break;
                   case ID_BREAKPOINT_DISABLEALL:
                       for(i=0;i<MAX_BREAK;i++){
                           if(listBreakPoint[i].Type == BT_PROGRAM)
                               listBreakPoint[i].bEnable = FALSE;
                       }
                       RefillBreakPointList();
                       FillComboBreakPoint();
                       SetCapture(hwnd);
                   break;
#endif
                   case ID_BREAKPOINT_DEL:
                       ZeroMemory(&listBreakPoint[dwBreakPointItem],sizeof(BREAKPOINT));
                       RefillBreakPointList();
                       FillComboBreakPoint();
                       UpdateDebugToolBar();
                       SetCapture(hwnd);
                   break;
                   case ID_BREAKPOINT_DELALL:
                       for(i=0;i<MAX_BREAK;i++){
                           if(listBreakPoint[i].Type == BT_PROGRAM)
                               ZeroMemory(&listBreakPoint[i],sizeof(BREAKPOINT));
                       }
                       RefillBreakPointList();
                       FillComboBreakPoint();
                       UpdateDebugToolBar();
                       SetCapture(hwnd);
                   break;
                   case ID_BREAKPOINT_GO:
                       pString = GlobalAlloc(GPTR|GMEM_DDESHARE,15);
                       wsprintf((char *)pString,"0x%08X",listBreakPoint[dwBreakPointItem].adress);
                       SendDlgItemMessage(hwndDebug,IDC_EDIT2,WM_SETTEXT,0,(LPARAM)pString);
                       SendMessage(hwndDebug,WM_COMMAND,MAKEWPARAM(IDC_BUTTON1,BN_CLICKED),(LPARAM)GetDlgItem(hwndDebug,IDC_BUTTON1));
                       GlobalFree(pString);
                   break;
                   case ID_BREAKPOINT_COPY:
                       if(OpenClipboard(hwndDebug)){
                           EmptyClipboard();
                           pString = GlobalAlloc(GPTR|GMEM_DDESHARE,15);
                           wsprintf((char *)pString,"0x%08X\r\n",listBreakPoint[dwBreakPointItem].adress);
                           SetClipboardData(CF_TEXT,pString);
                           CloseClipboard();
                           GlobalFree(pString);
                       }
                       SetCapture(hwnd);
                   break;
               }
               ::DestroyMenu(hDebugPopupMenu);
               hDebugPopupMenu = NULL;
               win = GetDlgItem(hwndDebug,IDC_EDIT1);
               InvalidateRect(win,NULL,FALSE);
               UpdateWindow(win);
           }
       break;
#ifdef _DEBPRO
       case WM_LBUTTONDOWN:
           pt.x = LOWORD(lParam);
           pt.y = HIWORD(lParam);
           if(pt.x  < 16){
               i1 = GetBreakPointFromPos(pt.y);
               i = GetBreakpointIndexFromIndex(i1);
               listBreakPoint[i].bEnable = !listBreakPoint[i].bEnable;
               listBreakPoint[i].PassCount = 0;
               i = SendMessage(hwndDebugComboBox1,CB_GETITEMHEIGHT,i1,0);
               rc.left = 0;
               rc.top = rc.bottom = i * i1;
               rc.right = 200;
               rc.bottom += i;
               InvalidateRect(hwnd,&rc,FALSE);
               UpdateWindow(hwnd);
               res = 0;
               bFlag = 0;
               win = GetDlgItem(hwndDebug,IDC_LIST1);
               InvalidateRect(win,NULL,FALSE);
               UpdateWindow(win);
           }
       break;
#endif
       case WM_CAPTURECHANGED:
           if(hDebugPopupMenu != NULL){
               bFlag = 0;
               res = 0;
           }
       break;
       case WM_RBUTTONDOWN:
           if(MAIN_MESSAGE.nBreak != 0){
               hDebugPopupMenu = ::LoadMenu(hInstance,MAKEINTRESOURCE(IDR_BREAKPOINT));
               pt.x = LOWORD(lParam);
               pt.y = HIWORD(lParam);
               dwBreakPointItem = GetBreakpointIndexFromIndex(GetBreakPointFromPos(pt.y));
               ::ClientToScreen(hwnd,&pt);
               TranslateMenu(hDebugPopupMenu,0xC000 + ((IDR_BREAKPOINT - IDR_MAINFRAME) << 10));
               ::TrackPopupMenuEx(GetSubMenu(hDebugPopupMenu,0),TPM_LEFTALIGN|TPM_LEFTBUTTON,pt.x,pt.y,hwnd,NULL);
               bFlag = 0;
           }
       break;
   }
   if(bFlag != 0)
       res = CallWindowProc(OldListBoxBreakPointWndProc,hwnd,uMsg,wParam,lParam);
   return res;
}
#ifdef _DEBPRO
//---------------------------------------------------------------------------
LRESULT CALLBACK CaptionBarWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   LRESULT res;
   u8 bFlag;
   PAINTSTRUCT ps;
   RECT rc;
   HFONT hFont;

   switch(uMsg){
       case WM_PAINT:
           ::BeginPaint(hwnd,&ps);
           ::GetClientRect(hwnd,&rc);
           DrawEdge(ps.hdc,&rc,EDGE_RAISED,BF_RECT);
           SetBkMode(ps.hdc,TRANSPARENT);
           SetTextColor(ps.hdc,GetSysColor(COLOR_CAPTIONTEXT));
           ::InflateRect(&rc,-2,-2);
           FillRect(ps.hdc,&rc,GetSysColorBrush(COLOR_INACTIVECAPTION));
           hFont = (HFONT)::SendMessage(hwndDebug,WM_GETFONT,0,0);
           ::SelectObject(ps.hdc,hFont);
           ::DrawText(ps.hdc," Ena.    Address          Pass Count              Condition",-1,&rc,DT_VCENTER|DT_NOCLIP|DT_LEFT);
           ::EndPaint(hwnd,&ps);
           res = 0;
           bFlag = 0;
       break;
   }
   if(bFlag != 0)
       res = CallWindowProc(OldCaptionBarWndProc,hwnd,uMsg,wParam,lParam);
   return res;
}
//---------------------------------------------------------------------------
LRESULT CALLBACK ListBoxMessageWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   LRESULT res;
   u8 bFlag;
   POINT pt;
   WORD wID;
   int i,*pInt,i1,i2;
   char *p,s[10];
   LPFINDREPLACE lpfr;

   bFlag = 1;
   switch(uMsg){
       case WM_RBUTTONDOWN:
           hDebugPopupMenu = ::LoadMenu(hInstance,MAKEINTRESOURCE(IDR_MESSAGELIST));
           i = SendMessage(hwnd,LB_GETCURSEL,0,0);
           if(i == LB_ERR){
               EnableMenuItem(hDebugPopupMenu,ID_MESSAGE_TOBREAKPOINT,MF_BYCOMMAND|MF_DISABLED);
               EnableMenuItem(hDebugPopupMenu,ID_MESSAGE_COPY,MF_BYCOMMAND|MF_DISABLED);
               EnableMenuItem(hDebugPopupMenu,ID_MESSAGE_DELETESELECTED,MF_BYCOMMAND|MF_DISABLED);
               EnableMenuItem(hDebugPopupMenu,ID_MESSAGE_DESELECTALL,MF_BYCOMMAND|MF_DISABLED);
           }
           pt.x = LOWORD(lParam);
           pt.y = HIWORD(lParam);
           ::ClientToScreen(hwnd,&pt);
           TranslateMenu(hDebugPopupMenu,0);
           ::TrackPopupMenuEx(GetSubMenu(hDebugPopupMenu,0),TPM_LEFTALIGN|TPM_LEFTBUTTON,pt.x,pt.y,hwnd,NULL);
           bFlag = 0;
       break;
/*       case WM_GETDLGCODE:
           res = 0;
           bFlag = 0;
       break;*/
       case WM_COMMAND:
           wID = LOWORD(wParam);
           if(HIWORD(wParam) < 2){
               switch(wID){
                   case ID_MESSAGE_GO:
                       i = SendMessage(hwnd,LB_GETCURSEL,0,0);
                       if(i != LB_ERR){
                       	i1 = SendMessage(hwnd,LB_GETTEXTLEN,i,0);
                           if(i1 > 0){
                               p = (char *)GlobalAlloc(GMEM_FIXED|GMEM_SHARE,i1+1);
                               SendMessage(hwnd,LB_GETTEXT,(WPARAM)i,(LPARAM)p);
                               p[10]=0;
                               SendDlgItemMessage(hwndDebug,IDC_EDIT2,WM_SETTEXT,0,(LPARAM)p);
                       		SendMessage(hwndDebug,WM_COMMAND,MAKEWPARAM(IDC_BUTTON1,BN_CLICKED),(LPARAM)GetDlgItem(hwndDebug,IDC_BUTTON1));
                               GlobalFree((HGLOBAL)p);
                           }
                       }
                   break;
                   case ID_MESSAGE_CLEARLIST:
                       SendMessage(hwndDebug,WM_COMMAND,MAKEWPARAM(ID_MESSAGE_CLEARLIST,0),0);
                   break;
                   case ID_MESSAGE_COPY:
                       i = SendMessage(hwnd,LB_GETCURSEL,0,0);
                       if(i != LB_ERR){
                           i1 = SendMessage(hwnd,LB_GETTEXTLEN,(WPARAM)i,0);
                           if(i1 != LB_ERR && i1 > 0){
                               p = (char *)GlobalAlloc(GMEM_FIXED|GMEM_SHARE,i1+1);
                               SendMessage(hwnd,LB_GETTEXT,(WPARAM)i,(LPARAM)p);
                               p[10]=0;
                               OpenClipboard(hwnd);
                               EmptyClipboard();
                               SetClipboardData(CF_TEXT,p);
                               CloseClipboard();
                               GlobalFree((HGLOBAL)p);
                           }
                       }
                   break;
                   case ID_MESSAGE_TOBREAKPOINT:
                       i = SendMessage(hwnd,LB_GETCURSEL,0,0);
                       if(i != LB_ERR){
                           i1 = SendMessage(hwnd,LB_GETTEXTLEN,(WPARAM)i,0);
                           if(i1 != LB_ERR && i1 > 0){
                               p = (char *)GlobalAlloc(GMEM_FIXED,i1+1);
                               SendMessage(hwnd,LB_GETTEXT,(WPARAM)i,(LPARAM)p);
                               ZeroMemory(s,10);
                               lstrcpyn(s,p+2,9);
                               GlobalFree((HGLOBAL)p);
                               InsertBreakPoint(StrToHex((char *)s) + 4);
                           }
                       }
                   break;
                   case ID_MESSAGE_SELECTALL:
                       SendMessage(hwnd,LB_SETSEL,(WPARAM)TRUE,(LPARAM)-1);
                   break;
                   case ID_MESSAGE_DESELECTALL:
                       SendMessage(hwnd,LB_SETSEL,(WPARAM)FALSE,(LPARAM)-1);
                   break;
                   case ID_MESSAGE_DELETESELECTED:
                       i = SendMessage(hwnd,LB_GETSELCOUNT,0,0);
                       if(i > 0){
                           pInt = (int *)GlobalAlloc(GMEM_FIXED,i *sizeof(int));
                           SendMessage(hwnd,LB_GETSELITEMS,(WPARAM)i,(LPARAM)pInt);
                           for(i--;i>=0;i--)
                               SendMessage(hwnd,LB_DELETESTRING,(WPARAM)pInt[i],0);
                           GlobalFree((HGLOBAL)pInt);
                       }
                   break;
                   case ID_MESSAGE_FIND:
                       ZeroMemory(&fr,sizeof(FINDREPLACE));
                       fr.lStructSize = sizeof(FINDREPLACE);
                       fr.hwndOwner = hwnd;
                       fr.hInstance = hInstance;
                       fr.Flags = FR_DOWN;
                       fr.lpstrFindWhat = (LPTSTR)GlobalAlloc(GPTR,100);
                       fr.wFindWhatLen = 99;
                       IDmessage = RegisterWindowMessage(FINDMSGSTRING);
                       hwndTemp = ::FindText(&fr);
                   break;
               }
               ::DestroyMenu(hDebugPopupMenu);
               hDebugPopupMenu = NULL;
           }
       break;
       case WM_DESTROY:
           if(fr.lpstrFindWhat != NULL)
               GlobalFree((HGLOBAL)fr.lpstrFindWhat);
           fr.lpstrFindWhat = NULL;
           if(hwndTemp != NULL)
               ::DestroyWindow(hwndTemp);
           hwndTemp = NULL;
       break;
   }
   if(uMsg == IDmessage){
       lpfr = (LPFINDREPLACE)lParam;
       if((lpfr->Flags & FR_FINDNEXT)){
           if((i1 = SendMessage(hwnd,LB_GETCOUNT,0,0)) > 0){
               if((i = SendMessage(hwnd,LB_GETCURSEL,0,0)) == LB_ERR)
                   i = -1;
               for(i++;i<i1;i++){
                   if((i2 = SendMessage(hwnd,LB_GETTEXTLEN,i,0)) > 0){
                       p = (char *)GlobalAlloc(GPTR|GMEM_SHARE,i2+1);
                       SendMessage(hwnd,LB_GETTEXT,(WPARAM)i,(LPARAM)p);
                       if(strstr(p,lpfr->lpstrFindWhat) != NULL){
                           GlobalFree((HGLOBAL)p);
                           break;
                       }
                       GlobalFree((HGLOBAL)p);
                   }
               }
               if(i < i1)
                   SendMessage(hwnd,LB_SETSEL,TRUE,i);
           }
       }
       else if((lpfr->Flags & FR_DIALOGTERM)){
           if(lpfr->lpstrFindWhat != NULL)
               GlobalFree((HGLOBAL)lpfr->lpstrFindWhat);
           hwndTemp = NULL;
       }
   }
   if(bFlag != 0)
       res = CallWindowProc(OldListBoxMessageWndProc,hwnd,uMsg,wParam,lParam);
   return res;
}
//---------------------------------------------------------------------------
LRESULT CALLBACK ListBoxStackWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   LRESULT res;
   u8 bFlag;
   POINT pt;
   WORD wID;
   int i,*pInt,i1;
   char *p,s[10];

   bFlag = 1;
   switch(uMsg){
       case WM_RBUTTONDOWN:
           hDebugPopupMenu = ::LoadMenu(hInstance,MAKEINTRESOURCE(IDR_MESSAGELIST));
           i = SendMessage(hwnd,LB_GETCURSEL,0,0);
           if(i == LB_ERR){
               EnableMenuItem(hDebugPopupMenu,ID_MESSAGE_TOBREAKPOINT,MF_BYCOMMAND|MF_DISABLED);
               EnableMenuItem(hDebugPopupMenu,ID_MESSAGE_COPY,MF_BYCOMMAND|MF_DISABLED);
               EnableMenuItem(hDebugPopupMenu,ID_MESSAGE_DELETESELECTED,MF_BYCOMMAND|MF_DISABLED);
               EnableMenuItem(hDebugPopupMenu,ID_MESSAGE_DESELECTALL,MF_BYCOMMAND|MF_DISABLED);
           }
           ::DeleteMenu(hDebugPopupMenu,ID_MESSAGE_FIND,MF_BYCOMMAND);
           pt.x = LOWORD(lParam);
           pt.y = HIWORD(lParam);
           ::ClientToScreen(hwnd,&pt);
           TranslateMenu(hDebugPopupMenu,0);
           ::TrackPopupMenuEx(GetSubMenu(hDebugPopupMenu,0),TPM_LEFTALIGN|TPM_LEFTBUTTON,pt.x,pt.y,hwnd,NULL);
           bFlag = 0;
       break;
       case WM_COMMAND:
           wID = LOWORD(wParam);
           if(HIWORD(wParam) < 2){
               switch(wID){
                   case ID_MESSAGE_GO:
                       i = SendMessage(hwnd,LB_GETCURSEL,0,0);
                       if(i != LB_ERR){
                       	i1 = SendMessage(hwnd,LB_GETTEXTLEN,i,0);
                           if(i1 > 0){
                               p = (char *)GlobalAlloc(GMEM_FIXED|GMEM_SHARE,i1+1);
                               SendMessage(hwnd,LB_GETTEXT,(WPARAM)i,(LPARAM)p);
                               p[10]=0;
                               SendDlgItemMessage(hwndDebug,IDC_EDIT2,WM_SETTEXT,0,(LPARAM)p);
                       		SendMessage(hwndDebug,WM_COMMAND,MAKEWPARAM(IDC_BUTTON1,BN_CLICKED),(LPARAM)GetDlgItem(hwndDebug,IDC_BUTTON1));
                               GlobalFree((HGLOBAL)p);
                           }
                       }
                   break;
                   case ID_MESSAGE_CLEARLIST:
                       SendMessage(hwnd,LB_RESETCONTENT,0,0);
                   break;
                   case ID_MESSAGE_COPY:
                       i = SendMessage(hwnd,LB_GETCURSEL,0,0);
                       if(i != LB_ERR){
                           i1 = SendMessage(hwnd,LB_GETTEXTLEN,(WPARAM)i,0);
                           if(i1 != LB_ERR && i1 > 0){
                               p = (char *)GlobalAlloc(GMEM_FIXED|GMEM_SHARE,i1+1);
                               SendMessage(hwnd,LB_GETTEXT,(WPARAM)i,(LPARAM)p);
                               p[10]=0;
                               OpenClipboard(hwnd);
                               EmptyClipboard();
                               SetClipboardData(CF_TEXT,p);
                               CloseClipboard();
                               GlobalFree((HGLOBAL)p);
                           }
                       }
                   break;
                   case ID_MESSAGE_TOBREAKPOINT:
                       i = SendMessage(hwnd,LB_GETCURSEL,0,0);
                       if(i != LB_ERR){
                           i1 = SendMessage(hwnd,LB_GETTEXTLEN,(WPARAM)i,0);
                           if(i1 != LB_ERR && i1 > 0){
                               p = (char *)GlobalAlloc(GMEM_FIXED,i1+1);
                               SendMessage(hwnd,LB_GETTEXT,(WPARAM)i,(LPARAM)p);
                               ZeroMemory(s,10);
                               lstrcpyn(s,p+2,9);
                               GlobalFree((HGLOBAL)p);
                               InsertBreakPoint(StrToHex((char *)s) + 4);
                           }
                       }
                   break;
                   case ID_MESSAGE_SELECTALL:
                       SendMessage(hwnd,LB_SETSEL,(WPARAM)TRUE,(LPARAM)-1);
                   break;
                   case ID_MESSAGE_DESELECTALL:
                       SendMessage(hwnd,LB_SETSEL,(WPARAM)FALSE,(LPARAM)-1);
                   break;
                   case ID_MESSAGE_DELETESELECTED:
                       i = SendMessage(hwnd,LB_GETSELCOUNT,0,0);
                       if(i > 0){
                           pInt = (int *)GlobalAlloc(GMEM_FIXED,i *sizeof(int));
                           SendMessage(hwnd,LB_GETSELITEMS,(WPARAM)i,(LPARAM)pInt);
                           for(i--;i>=0;i--)
                               SendMessage(hwnd,LB_DELETESTRING,(WPARAM)pInt[i],0);
                           GlobalFree((HGLOBAL)pInt);
                       }
                   break;
                   case ID_MESSAGE_FIND:
                       ZeroMemory(&fr,sizeof(FINDREPLACE));
                       fr.lStructSize = sizeof(FINDREPLACE);
                       fr.hwndOwner = hwnd;
                       fr.hInstance = hInstance;
                       fr.Flags = FR_DOWN;
                       fr.lpstrFindWhat = (LPTSTR)GlobalAlloc(GPTR,100);
                       fr.wFindWhatLen = 99;
                       IDmessage = RegisterWindowMessage(FINDMSGSTRING);
                       hwndTemp = ::FindText(&fr);
                   break;
               }
               ::DestroyMenu(hDebugPopupMenu);
               hDebugPopupMenu = NULL;
           }
       break;
   }
   if(bFlag != 0)
       res = CallWindowProc(OldListBoxMessageWndProc,hwnd,uMsg,wParam,lParam);
   return res;
}
#endif
//---------------------------------------------------------------------------
LRESULT CALLBACK ComboBoxBreakPointWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   LRESULT res;
   u8 bFlag;

   bFlag = 1;
   switch(uMsg){
       case WM_CTLCOLORLISTBOX:
           hwndListBoxComboBox1 = (HWND)lParam;
       break;
   }
   if(bFlag != 0)
       res = ::CallWindowProc(OldComboBoxBreakPointWndProc,hwndDlg,uMsg,wParam,lParam);
   return res;
}
//---------------------------------------------------------------------------
u8 OnInitDialog(HWND hwndDlg)
{
#ifdef _DEBPRO
   TC_ITEM tc_item;
#endif
   int i;
   HWND hwnd;
   POINT pt;
   char s[50];
   RECT rc,rc1;

   hwndTemp = NULL;
   ZeroMemory(&fr,sizeof(FINDREPLACE));

   hwndDebug = hwndDlg;
   imageListDebug[0] = imageListDebug[1] = NULL;
   CreateDebugToolBar();
   CreateDebugStatusBar();

   ZeroMemory(&dbg,sizeof(DEBUGGER));
   hwnd = GetDlgItem(hwndDlg,IDC_LIST1);
   GetWindowRect(hwnd,&rc);
   dbg.iMaxItem = (u8)((rc.bottom - rc.top) / SendMessage(hwnd,LB_GETITEMHEIGHT,0,0));
   OldListBoxAssemblerWndProc = (WNDPROC)::SetWindowLong(hwnd,GWL_WNDPROC,(LONG)ListBoxAssemblerWndProc);

   dwKey = 0;
   bInLoop = FALSE;
   FillComboBreakPoint();
   hwndDebugComboBox1 = GetDlgItem(hwndDlg,IDC_COMBOBOX1);
#ifdef _DEBPRO
   SendMessage(hwndDebugComboBox1,CB_SETDROPPEDWIDTH,(WPARAM)350,0);
   hwnd = GetDlgItem(hwndDlg,IDC_LIST3);
   OldListBoxMessageWndProc = (WNDPROC)::SetWindowLong(hwnd,GWL_WNDPROC,(LONG)ListBoxMessageWndProc);
   hwnd = GetDlgItem(hwndDlg,IDC_LIST4);
   OldListBoxProcedureWndProc = (WNDPROC)::SetWindowLong(hwnd,GWL_WNDPROC,(LONG)ListBoxProcedureWndProc);
   hwnd = GetDlgItem(hwndDlg,IDC_LIST5);
   OldListBoxStackWndProc = (WNDPROC)::SetWindowLong(hwnd,GWL_WNDPROC,(LONG)ListBoxStackWndProc);
#endif
   OldComboBoxBreakPointWndProc = (WNDPROC)::SetWindowLong(hwndDebugComboBox1,GWL_WNDPROC,(LONG)ComboBoxBreakPointWndProc);
   pt.x = pt.y = 4;
   hwnd = ChildWindowFromPoint(hwndDebugComboBox1,pt);
   OldEditBreakPointWndProc = (WNDPROC)::SetWindowLong(hwnd,GWL_WNDPROC,(LONG)EditBreakPointWndProc);
   for(i=0;i < (signed)SizeMemoryAddressStruct;i++){
       wsprintf(s,"0x%08X - %s",MemoryAddress[i].Address,MemoryAddress[i].Name);
       SendDlgItemMessage(hwndDlg,IDC_COMBOBOX2,CB_ADDSTRING,0,(LPARAM)s);
   }
   SendDlgItemMessage(hwndDlg,IDC_COMBOBOX3,CB_ADDSTRING,0,(LPARAM)"Arm");
   SendDlgItemMessage(hwndDlg,IDC_COMBOBOX3,CB_ADDSTRING,0,(LPARAM)"Thumb");
   SendDlgItemMessage(hwndDlg,IDC_COMBOBOX3,CB_ADDSTRING,0,(LPARAM)"Auto");
   SendDlgItemMessage(hwndDlg,IDC_COMBOBOX3,CB_SETCURSEL,2,0);
   dbg.bTrackMode = 2;
   FillComboBreakPoint();
   hwnd = GetDlgItem(hwndDlg,IDC_TAB1);
#ifdef _DEBPRO
   ZeroMemory(&tc_item,sizeof(TC_ITEM));
   tc_item.mask = TCIF_TEXT;
   tc_item.pszText = "Messaggi";
   TabCtrl_InsertItem(hwnd,0,&tc_item);
   tc_item.pszText = "Dettaglio breakpoint";
   TabCtrl_InsertItem(hwnd,1,&tc_item);
   tc_item.pszText = "Call stack";
   TabCtrl_InsertItem(hwnd,2,&tc_item);
   tc_item.pszText = "Exception";
   TabCtrl_InsertItem(hwnd,3,&tc_item);
#else
   GetWindowRect(hwndDebug,&rc);
   GetWindowRect(hwnd,&rc1);
   SetWindowPos(hwndDebug,NULL,0,0,rc.right-rc.left,rc.bottom - rc.top - (rc1.bottom - rc1.top),SWP_NOREPOSITION|SWP_NOSENDCHANGING|SWP_NOMOVE);
   ::DestroyWindow(hwnd);
   ::DestroyWindow(GetDlgItem(hwndDebug,IDC_LIST3));
   ::DestroyWindow(GetDlgItem(hwndDebug,IDC_LIST4));
   ::DestroyWindow(GetDlgItem(hwndDebug,IDC_LIST5));
   ::DestroyWindow(GetDlgItem(hwndDebug,IDC_LIST6));
#endif
   Translation(hwndDebug,IDR_DEBUG,IDD_DIALOG1);
   return 1;
}
//---------------------------------------------------------------------------
static void EditRegister()
{
	char s[50];
   int item;
   RECT  rc;
	u32 value;

   item = SendDlgItemMessage(hwndDebug,ID_DEBUG_DIS,LB_GETCARETINDEX,0,0);
	if(item == LB_ERR)
   	return;
	SendDlgItemMessage(hwndDebug,ID_DEBUG_DIS,LB_GETITEMRECT,item,(LPARAM)&rc);
	MapWindowPoints(GetDlgItem(hwndDebug,ID_DEBUG_DIS),hwndDebug,(LPPOINT)&rc,2);
   SendDlgItemMessage(hwndDebug,ID_DEBUG_DIS,LB_GETTEXT,item,(LPARAM)s);
	if(InputText(hwndDebug,&rc,0,&s[4],50)){
		value = StrToHex(&s[4]);
   	GP_REG[item] = value;
   }
   UpdateRegister();
}
//---------------------------------------------------------------------------
void OnChangePageTab1()
{
   int i,item[4];

   for(i=0;i<4;i++)
       item[i] = SW_HIDE;
	i = TabCtrl_GetCurSel(GetDlgItem(hwndDebug,IDC_TAB1));
   switch(i){
       case 0:
           item[0] = SW_SHOW;
           i = MF_UNCHECKED;
       break;
       case 1:
           item[1] = SW_SHOW;
           i = MF_CHECKED;
       break;
       case 2:
           item[2] = SW_SHOW;
           i = MF_UNCHECKED;
       break;
       case 3:
           item[3] = SW_SHOW;
           i = MF_UNCHECKED;
       break;
   }
   ShowWindow(GetDlgItem(hwndDebug,IDC_LIST3),item[0]);
   ShowWindow(GetDlgItem(hwndDebug,IDC_LIST4),item[1]);
   ShowWindow(GetDlgItem(hwndDebug,IDC_LIST5),item[2]);
   ShowWindow(GetDlgItem(hwndDebug,IDC_LIST6),item[3]);
   CheckMenuItem(GetMenu(hwndDebug),ID_DEBUG_PROCEDURE,MF_BYCOMMAND|i);
}
//---------------------------------------------------------------------------
BOOL CALLBACK DialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   int wID,i;
   char s[80];
   HMENU hMenu;
   LPMEASUREITEMSTRUCT lpMIS;
   LPDRAWITEMSTRUCT lpDIS;
   HWND hwnd;
   RECT rc;
   BOOL res;
   DWORD dwPos;
   LString *pString;
   LString string,string1;
   HBRUSH hBrush,hOldBrush;
   HPEN hOldPen;
   LPNMHDR nh;
#ifdef _DEBPRO
   MENUITEMINFO mii;
   POINT pt;
#endif
   switch(uMsg){
       case WM_SIZE:
		    SendMessage(hwndDebugStatusBar,uMsg,wParam,lParam);
       break;
       case WM_NOTIFY:
           nh = (LPNMHDR)lParam;
           if(nh->idFrom == IDC_TAB1 && nh->code == TCN_SELCHANGE)
           	OnChangePageTab1();
           else if(nh->code == TTN_NEEDTEXT)
               OnToolTipToolBar(nh);
       break;
       case WM_TIMER:
           switch(wParam){
               case TM_STATUSBAR:
                   KillTimer(hwndDlg,wTimerSB);
                   wTimerSB = NULL;
                   UpdateDebugStatusBar("",0,0);
               break;
               case TM_BREAKPOINT:
                   ::FlashWindow(hwndDebug,(dwCountTimer & 1) ? FALSE : TRUE);
                   dwCountTimer++;
                   if(dwCountTimer == 6){
                       KillTimer(hwndDlg,wTimerBreakPointID);
                       wTimerBreakPointID = dwCountTimer = 0;
                       SetActiveWindow(hwndDebug);
                       SetFocus(GetDlgItem(hwndDebug,IDC_LIST1));
                   }
               break;
           }
           res = TRUE;
       break;
       case WM_VSCROLL:
           OnDebugVScroll(wParam,lParam);
       break;
       case WM_INITDIALOG:
           OnInitDialog(hwndDlg);
           res = FALSE;
       break;
       case WM_CTLCOLORSTATIC:
           wID = (WORD)GetDlgCtrlID((HWND)lParam);
           switch(wID){
               case IDC_CYCLE:
               case IDC_IRQRET:
               case IDC_MODE:
               case IDC_RASTERLINE:
                   return (BOOL)GetStockObject(WHITE_BRUSH);
               default:
                   return (BOOL)GetClassLong((HWND)lParam,GCL_HBRBACKGROUND);
           }
       case WM_MEASUREITEM:
           lpMIS = (LPMEASUREITEMSTRUCT)lParam;
           switch(wParam){
               case IDC_COMBOBOX1:
                   hwnd = GetDlgItem(hwndDlg,IDC_MODE);
                   ::GetClientRect(hwnd,&rc);
                   lpMIS->itemHeight = rc.bottom - rc.top - GetSystemMetrics(SM_CYEDGE) + 1;
               break;
               case IDC_LIST1:
                   lpMIS->itemHeight = 14;
               break;
           }
           res = TRUE;
       break;
       case WM_DRAWITEM:
           lpDIS = (LPDRAWITEMSTRUCT)lParam;
           switch(wParam){
               case IDC_COMBOBOX1:
                   if(OldListBoxBreakPointWndProc == NULL && hwndListBoxComboBox1 != NULL){
                       ::GetWindowRect(hwndListBoxComboBox1,&rc);
                       OldListBoxBreakPointWndProc = (WNDPROC)::SetWindowLong(hwndListBoxComboBox1,GWL_WNDPROC,(LONG)ListBoxBreakPointWndProc);
#ifdef _DEBPRO
                       ::SetWindowPos(hwndListBoxComboBox1,NULL,rc.left,rc.top + 20,0,0,SWP_NOSIZE|SWP_NOREPOSITION|SWP_NOSENDCHANGING);
                       pt.x = rc.left-1;
                       pt.y = rc.top+1;
                       ScreenToClient(hwndDebug,&pt);
                       hwndCaptionBar = CreateWindow("STATIC","",WS_CHILD|WS_VISIBLE|SS_SIMPLE|WS_BORDER,pt.x,pt.y,
                           rc.right - rc.left+2,20,hwndDebug,NULL,hInstance,NULL);
                       OldCaptionBarWndProc = (WNDPROC)::SetWindowLong(hwndCaptionBar,GWL_WNDPROC,(LONG)CaptionBarWndProc);
#endif
                   }
                   if(lpDIS->itemData != 0){
                       if((lpDIS->itemState & ODS_SELECTED)){
                           SetBkColor(lpDIS->hDC,GetSysColor(COLOR_HIGHLIGHT));
                           SetTextColor(lpDIS->hDC,GetSysColor(COLOR_HIGHLIGHTTEXT));
                       }
                       else{
                           SetBkColor(lpDIS->hDC,GetSysColor(COLOR_WINDOW));
                           SetTextColor(lpDIS->hDC,GetSysColor(COLOR_WINDOWTEXT));
                       }
                       CopyRect(&rc,&lpDIS->rcItem);
                       rc.top++;
#ifdef _DEBPRO
                       rc.right = rc.bottom - rc.top;
                       rc.left += 2;
                       rc.bottom = rc.top + rc.right;
                       if(listBreakPoint[(int)lpDIS->itemData - 1].bEnable != 0)
                           i = DFCS_CHECKED;
                       else
                           i = 0;
                       DrawFrameControl(lpDIS->hDC,&rc,DFC_BUTTON,DFCS_BUTTONCHECK|i);
                       rc.left = rc.right + 13;
                       rc.right = lpDIS->rcItem.right - 1;
                       rc.bottom -= 2;
#elif defined(_DEBUG)
                       rc.left += 3;
#endif
                       ToBreakPointString(listBreakPoint[(int)lpDIS->itemData - 1].adress,s,30);
                       ::ExtTextOut(lpDIS->hDC,rc.left,rc.top,ETO_CLIPPED|ETO_OPAQUE,&rc,s,lstrlen(s),NULL);
#ifdef _DEBPRO
                       rc.left += 90;
                       if(listBreakPoint[(int)lpDIS->itemData - 1].PassCountCondition != 0){
                           wsprintf(s,"%03d",listBreakPoint[(int)lpDIS->itemData - 1].PassCountCondition);
                           ::ExtTextOut(lpDIS->hDC,rc.left,rc.top,ETO_CLIPPED|ETO_OPAQUE,&rc,s,lstrlen(s),NULL);
                       }
                       string = ConditionToString((u8)(lpDIS->itemData - 1));
                       rc.left += 90;
                       if(!string.IsEmpty())
                           ::ExtTextOut(lpDIS->hDC,rc.left,rc.top,ETO_CLIPPED|ETO_OPAQUE,&rc,string.c_str(),string.Length(),NULL);
#endif
                   }
               break;
               case IDC_LIST1:
                   if(lpDIS->itemID != -1){
                       CopyRect(&rc,&lpDIS->rcItem);
                       if((lpDIS->itemState & ODS_SELECTED)){
                           if((lpDIS->itemState & ODS_FOCUS)){
                               SetBkColor(lpDIS->hDC,GetSysColor(COLOR_WINDOW));
                               SetTextColor(lpDIS->hDC,GetSysColor(COLOR_WINDOWTEXT));
                               FillRect(lpDIS->hDC,&rc,GetSysColorBrush(COLOR_WINDOW));
                               DrawFocusRect(lpDIS->hDC,&rc);
                           }
                           else{
                               SetBkColor(lpDIS->hDC,GetSysColor(COLOR_HIGHLIGHT));
                               SetTextColor(lpDIS->hDC,GetSysColor(COLOR_HIGHLIGHTTEXT));
                               FillRect(lpDIS->hDC,&rc,GetSysColorBrush(COLOR_HIGHLIGHT));
                           }
                       }
                       else{
                           SetBkColor(lpDIS->hDC,GetSysColor(COLOR_WINDOW));
                           SetTextColor(lpDIS->hDC,GetSysColor(COLOR_WINDOWTEXT));
                           FillRect(lpDIS->hDC,&rc,GetSysColorBrush(COLOR_WINDOW));
                       }
                       i = SendDlgItemMessage(hwndDlg,IDC_LIST1,LB_GETTEXTLEN,(WPARAM)lpDIS->itemID,0);
                       string.Capacity(i+2);
                       SendDlgItemMessage(hwndDlg,IDC_LIST1,LB_GETTEXT,(WPARAM)lpDIS->itemID,(LPARAM)string.c_str());
                       sscanf(string.c_str(),"%08X",&dwPos);
                       for(i=0;i<MAX_BREAK;i++){
                           if(listBreakPoint[i].adress == dwPos && dwPos != 0)
                               break;
                       }
                       if(i < MAX_BREAK){
                           CopyRect(&rc,&lpDIS->rcItem);
                           rc.left += 5;
                           rc.right = rc.left + 7;
                           rc.top = rc.top + (((rc.bottom - rc.top) - 6) >> 1);
                           rc.bottom = rc.top + 7;
                           if(listBreakPoint[i].bEnable != 0)
                               hBrush = CreateSolidBrush(RGB(255,0,0));
                           else
                               hBrush = CreateSolidBrush(RGB(0,255,0));
                           hOldBrush = (HBRUSH)SelectObject(lpDIS->hDC,hBrush);
                           hOldPen = (HPEN)SelectObject(lpDIS->hDC,GetStockObject(BLACK_PEN));
                           Ellipse(lpDIS->hDC,rc.left,rc.top,rc.right,rc.bottom);
                           ::SelectObject(lpDIS->hDC,hOldBrush);
                           ::SelectObject(lpDIS->hDC,hOldPen);
                           ::DeleteObject(hBrush);
                       }
                       string.ResetToken();
                       CopyRect(&rc,&lpDIS->rcItem);
                       SetBkMode(lpDIS->hDC,TRANSPARENT);
                       string1 = string.NextToken(32);
                       rc.left += 18;
                       ::ExtTextOut(lpDIS->hDC,rc.left,rc.top,ETO_CLIPPED,&rc,string1.c_str(),string1.Length(),NULL);
                       string1 = string.NextToken(32);
                       rc.left += 60;
                       ::ExtTextOut(lpDIS->hDC,rc.left,rc.top,ETO_CLIPPED,&rc,string1.c_str(),string1.Length(),NULL);
                       string1 = string.RemainderToken();
                       rc.left += 60;
                       ::ExtTextOut(lpDIS->hDC,rc.left,rc.top,ETO_CLIPPED,&rc,string1.c_str(),string1.Length(),NULL);
                   }
               break;
           }
           res = TRUE;
       break;
       case WM_INITMENUPOPUP:
           if(LOWORD(lParam) == 14){
               hMenu = (HMENU)wParam;
               i = GetMenuItemCount(hMenu);
               for(;i > 0;i--)
                   ::DeleteMenu(hMenu,i-1,MF_BYPOSITION);
               wID = ID_DEBUG_RECENT;
               if(pDebugRecentFile != NULL){
                   pString = (LString *)pDebugRecentFile->GetFirstItem(&dwPos);
                   while(pString != NULL){
                       AppendMenu(hMenu,MF_STRING,wID++,pString->c_str());
                       pString = (LString *)pDebugRecentFile->GetNextItem(&dwPos);
                       i++;
                   }
               }
               TranslateLoadString(&string,ID_DEBUG_RECENT);
               for(;i<4;i++)
                   AppendMenu(hMenu,MF_STRING|MF_GRAYED,wID++,string.c_str());
           }
       break;
       case WM_COMMAND:
           wID = LOWORD(wParam);
           if(lParam == NULL || (lParam != NULL && GetDlgCtrlID((HWND)lParam) == IDM_TOOLBARDEBUG)){
                   if(wID >= ID_DEBUG_RECENT && wID < ID_DEBUG_RECENT+4){
                       pString = (LString *)pDebugRecentFile->GetItem(wID-ID_DEBUG_RECENT + 1);
                       if(pString != NULL)
                           LoadBreakPointListFromName(pString->c_str());
                       break;
                   }
                   switch(wID){
                       case ID_DEBUG_RESTART:
                           ::PostMessage(hWin,WM_COMMAND,MAKEWPARAM(ID_EMU_RESET,0),0);
                       break;
                       case ID_DEBUG_STOP:
                           ::PostMessage(hwndDebug,WM_CLOSE,0,0);
                       break;
                       case ID_DEBUG_WRITEFILE_START:
                           if(fpDebug == NULL){
                               fpDebug = new LFile("debug.txt");
                               if(fpDebug != NULL)
                                   fpDebug->Open(GENERIC_WRITE,OPEN_ALWAYS);
                           }
                           if(fpDebug != NULL)
                               fpDebug->WriteF("Start debug at %08X\r\n",REG_PC);
                           bWriteDebug = TRUE;
                           hMenu = GetMenu(hwndDlg);
                           EnableMenuItem(hMenu,ID_DEBUG_WRITEFILE_START,MF_BYCOMMAND|MF_GRAYED);
                           EnableMenuItem(hMenu,ID_DEBUG_WRITEFILE_STOP,MF_BYCOMMAND|MF_ENABLED);
                           EnableMenuItem(hMenu,ID_DEBUG_WRITEFILE_PAUSE,MF_BYCOMMAND|MF_ENABLED);
                       break;
                       case ID_DEBUG_WRITEFILE_PAUSE:
                           bWriteDebug = FALSE;
                           fpDebug->WriteF("Pause debug at %08X\r\n",REG_PC);
                           hMenu = GetMenu(hwndDlg);
                           EnableMenuItem(hMenu,ID_DEBUG_WRITEFILE_PAUSE,MF_BYCOMMAND|MF_GRAYED);
                           EnableMenuItem(hMenu,ID_DEBUG_WRITEFILE_RESUME,MF_BYCOMMAND|MF_ENABLED);
                       break;
                       case ID_DEBUG_WRITEFILE_RESUME:
                           bWriteDebug = TRUE;
                           fpDebug->WriteF("Resume debug at %08X\r\n",REG_PC);
                           hMenu = GetMenu(hwndDlg);
                           EnableMenuItem(hMenu,ID_DEBUG_WRITEFILE_PAUSE,MF_BYCOMMAND|MF_ENABLED);
                           EnableMenuItem(hMenu,ID_DEBUG_WRITEFILE_RESUME,MF_BYCOMMAND|MF_GRAYED);
                       break;
                       case ID_DEBUG_WRITEFILE_STOP:
                           bWriteDebug = FALSE;
                           if(fpDebug != NULL){
                               fpDebug->WriteF("Stop debug at %08X\r\n",REG_PC);
                               delete fpDebug;
                           }
                           fpDebug = NULL;
                           hMenu = GetMenu(hwndDlg);
                           EnableMenuItem(hMenu,ID_DEBUG_WRITEFILE_START,MF_BYCOMMAND|MF_ENABLED);
                           EnableMenuItem(hMenu,ID_DEBUG_WRITEFILE_STOP,MF_BYCOMMAND|MF_GRAYED);
                           EnableMenuItem(hMenu,ID_DEBUG_WRITEFILE_PAUSE,MF_BYCOMMAND|MF_GRAYED);
                           EnableMenuItem(hMenu,ID_DEBUG_WRITEFILE_RESUME,MF_BYCOMMAND|MF_GRAYED);
                       break;
                       case ID_DEBUG_BREAKPOINT:
                           InsertBreapointFromListBox();
                       break;
                       case ID_DEBUG_PAUSE:
                           dwKey = VK_F4;
                           EnterDebugMode();
                       break;
                       case ID_DEBUG_PALBG:
                           CreateDebugBgPalWindow(hwndDebug);
                       break;
                       case ID_DEBUG_PALOBJ:
                           CreateDebugObjPalWindow(hwndDebug);
                       break;
                       case ID_DEBUG_REDRAW:
                           BlitFrame(NULL);
                       break;
                       case ID_DEBUG_SAVE:
                           SaveBreakPointList();
                       break;
                       case ID_DEBUG_LOAD:
                           LoadBreakPointList();
                       break;
                       case ID_DEBUG_GO:
                           dwKey = VK_F5;
                       break;
                       case ID_DEBUG_DIS3:
                           CreateDebugWindow(1);
                       break;
                       case ID_DEBUG_DIS1:
                           dwKey = VK_F8;
                       break;
                       case ID_DEBUG_DIS2:
                           dwKey = VK_F11;
                       break;
#ifdef _DEBPRO
                       case ID_DEBUG_WRITEFILE_ALL:
                           WriteAllToFile();
                       break;
                       case ID_DEBUG_BKMEMORY:
                           ShowDialogBRKMEM(hwndDebug);
                       break;
                       case ID_DEBUG_SOURCE:
                           CreateDebugSourceWindow(hWin);
                       break;
                       case ID_DEBUG_BACKGROUND:
                           CreateDebugBkgWindow(hwndDebug);
                       break;
                       case ID_DEBUG_SPRITE:
                           CreateDebugSpriteWindow(hwndDebug);
                       break;
                       case ID_MESSAGE_CLEARLIST:
                           SendMessage(GetDlgItem(hwndDlg,IDC_LIST3),LB_RESETCONTENT,0,0);
                       break;
                       case ID_DEBUG_DMA:
                           CreateDebugDMAWindow(hwndDebug);
                       break;
                       case ID_DEBUG_PROCEDURE:
                           hMenu = GetMenu(hwndDebug);
                           dwPos = GetMenuState(hMenu,ID_DEBUG_PROCEDURE,MF_BYCOMMAND);
                           if(dwPos == MF_CHECKED)
                               i = 0;
                           else
                               i = 1;
                           TabCtrl_SetCurSel(GetDlgItem(hwndDlg,IDC_TAB1),i);
                           OnChangePageTab1();
                       break;
                       case ID_DEBUG_MESSAGE:
                           hMenu = GetMenu(hwndDlg);
                           ZeroMemory(&mii,sizeof(MENUITEMINFO));
                           mii.cbSize = sizeof(MENUITEMINFO);
                           mii.fMask = MIIM_STATE;
                           if(!MAIN_MESSAGE.bEnable){
                               EnableMenuItem(hMenu,2,MF_ENABLED|MF_BYPOSITION);
                               MAIN_MESSAGE.bEnable = TRUE;
                               mii.fState = MFS_CHECKED;
                           }
                           else{
                               EnableMenuItem(hMenu,2,MF_GRAYED|MF_BYPOSITION);
                               MAIN_MESSAGE.bEnable = FALSE;
                               mii.fState = MFS_UNCHECKED;
                           }
                           DrawMenuBar(hwndDlg);
                           SetMenuItemInfo(hMenu,ID_DEBUG_MESSAGE,FALSE,&mii);
                       break;
                       case ID_MESSAGE_IRQ:
                           hMenu = GetMenu(hwndDlg);
                           EnableMessage(hMenu,MESSAGE_IRQ,ID_MESSAGE_IRQ);
                       break;
                       case ID_MESSAGE_DMA:
                           hMenu = GetMenu(hwndDlg);
                           EnableMessage(hMenu,MESSAGE_DMA,ID_MESSAGE_DMA);
                       break;
                       case ID_MESSAGE_DMA0:
                       case ID_MESSAGE_DMA1:
                       case ID_MESSAGE_DMA2:
                       case ID_MESSAGE_DMA3:
                           hMenu = GetMenu(hwndDlg);
                           EnableMessage(hMenu,MESSAGE_DMA0 + (wID - ID_MESSAGE_DMA0),wID);
                       break;
                       case ID_MESSAGE_SWI:
                           hMenu = GetMenu(hwndDlg);
                           EnableMessage(hMenu,MESSAGE_SWI,ID_MESSAGE_SWI);
                       break;
                       case ID_MESSAGE_SWI5:
                           hMenu = GetMenu(hwndDlg);
                           EnableMessage(hMenu,MESSAGE_SWI5,ID_MESSAGE_SWI5);
                       break;
                       case ID_DEBUG_IRQE:
                           hMenu = GetMenu(hwndDlg);
                           EnableBreak(hMenu,BREAK_ENTER_IRQ,ID_DEBUG_IRQE);
                       break;
                       case ID_DEBUG_IRQX:
                           hMenu = GetMenu(hwndDlg);
                           EnableBreak(hMenu,BREAK_EXIT_IRQ,ID_DEBUG_IRQX);
                       break;
                       case ID_DEBUG_ENETER_SWI:
                           hMenu = GetMenu(hwndDlg);
                           EnableBreak(hMenu,BREAK_ENTER_SWI,ID_DEBUG_ENETER_SWI);
                       break;
                       case ID_DEBUG_IRQE1:
                       case ID_DEBUG_IRQE2:
                       case ID_DEBUG_IRQE3:
                       case ID_DEBUG_IRQE4:
                       case ID_DEBUG_IRQE5:
                       case ID_DEBUG_IRQE6:
                       case ID_DEBUG_IRQE7:
                       case ID_DEBUG_IRQE8:
                       case ID_DEBUG_IRQE9:
                       case ID_DEBUG_IRQE10:
                       case ID_DEBUG_IRQE11:
                       case ID_DEBUG_IRQE12:
                       case ID_DEBUG_IRQE13:
                       case ID_DEBUG_IRQE14:
                           hMenu = GetMenu(hwndDlg);
                           EnableBreak(hMenu,(u8)(BREAK_ENTER_IRQ1 + (wID - ID_DEBUG_IRQE1)),(WORD)wID);
                       break;
                       case ID_DEBUG_IRQX1:
                       case ID_DEBUG_IRQX2:
                       case ID_DEBUG_IRQX3:
                       case ID_DEBUG_IRQX4:
                       case ID_DEBUG_IRQX5:
                       case ID_DEBUG_IRQX6:
                       case ID_DEBUG_IRQX7:
                       case ID_DEBUG_IRQX8:
                       case ID_DEBUG_IRQX9:
                       case ID_DEBUG_IRQX10:
                       case ID_DEBUG_IRQX11:
                       case ID_DEBUG_IRQX12:
                       case ID_DEBUG_IRQX13:
                       case ID_DEBUG_IRQX14:
                           hMenu = GetMenu(hwndDlg);
                           EnableBreak(hMenu,(u8)(BREAK_EXIT_IRQ1 + (wID - ID_DEBUG_IRQX1)),(WORD)wID);
                       break;
                       case ID_MESSAGE_EXCEPTIONS:
                           hMenu = GetMenu(hwndDlg);
                           MAIN_MESSAGE.bExceptions = !MAIN_MESSAGE.bExceptions;
                           CheckMenuItem(hMenu,ID_MESSAGE_EXCEPTIONS,MF_BYCOMMAND|(MAIN_MESSAGE.bExceptions ? MF_CHECKED : MF_UNCHECKED));
                       break;
                       case ID_MESSAGE_STACK:
                           hMenu = GetMenu(hwndDlg);
                           MAIN_MESSAGE.bStack = !MAIN_MESSAGE.bStack;
                           CheckMenuItem(hMenu,ID_MESSAGE_STACK,MF_BYCOMMAND|(MAIN_MESSAGE.bStack ? MF_CHECKED : MF_UNCHECKED));
                       break;
                       case ID_MESSAGE_CPU:
                           hMenu = GetMenu(hwndDlg);
                           EnableMessage(hMenu,MESSAGE_CPU,ID_MESSAGE_CPU);
                       break;
                       case ID_MESSAGE_TIMER:
                           hMenu = GetMenu(hwndDlg);
                           EnableMessage(hMenu,MESSAGE_TIMER,ID_MESSAGE_TIMER);
                       break;
                       case ID_MESSAGE_POWER:
                           hMenu = GetMenu(hwndDlg);
                           EnableMessage(hMenu,MESSAGE_POWER,ID_MESSAGE_POWER);
                       break;
                       case ID_DEBUG_START_DMA:
                           hMenu = GetMenu(hwndDlg);
                           EnableMessage(hMenu,BREAK_START_DMA,ID_DEBUG_START_DMA);
                       break;
#endif//202e910
                   }
               break;
           }
           else{
               switch(HIWORD(wParam)){
                   case LBN_DBLCLK:
						if(wID == ID_DEBUG_DIS)
							EditRegister();
#ifdef _DEBPRO
                       else if(wID == IDC_LIST1)
                           InsertBreapointFromListBox();
#endif
                   break;
                   case CBN_SELENDOK:
                       switch(wID){
                           case IDC_COMBOBOX2:
                               i = SendDlgItemMessage(hwndDlg,IDC_COMBOBOX2,CB_GETCURSEL,0,0);
                               wsprintf(s,"0x%08X",MemoryAddress[i].Address);
                               ::SetWindowText(GetDlgItem(hwndDlg,IDC_EDIT2),s);
                               dwPos = StrToHex(s);
                               FillListDiss(&dwPos,DBV_VIEW,0);
                               UpdateAddressBar(dwPos,1);
                           break;
                           case IDC_COMBOBOX3:
                               dbg.bTrackMode = (u8)SendMessage((HWND)lParam,CB_GETCURSEL,0,0);
                               dwPos = dbg.views[dbg.iCurrentView].StartAddress;
                               FillListDiss(&dwPos,DBV_VIEW,1);
                               UpdateAddressBar(dwPos,1);
                           break;
                       }
                   break;
                   case BN_CLICKED:
                       switch(wID){
                           case IDC_CHECK1:
                               NFLAG = (u8)(SendMessage((HWND)lParam,BM_GETCHECK,0,0) == BST_CHECKED ? 1 : 0);
                           break;
                           case IDC_CHECK2:
                               CFLAG = (u8)(SendMessage((HWND)lParam,BM_GETCHECK,0,0) == BST_CHECKED ? 1 : 0);
                           break;
                           case IDC_CHECK3:
                               VFLAG = (u8)(SendMessage((HWND)lParam,BM_GETCHECK,0,0) == BST_CHECKED ? 1 : 0);
                           break;
                           case IDC_CHECK4:
                               ZFLAG = (u8)(SendMessage((HWND)lParam,BM_GETCHECK,0,0) == BST_CHECKED ? 1 : 0);
                           break;
                           case IDC_BUTTON1:
                               ::GetWindowText(GetDlgItem(hwndDlg,IDC_EDIT2),s,20);
                               dwPos = StrToHex(s);
                               i = GetCurrentIncPipe(DBV_VIEW);
                               dwPos = (dwPos / i) * i;
                               i = MemoryAddressToIndex(dwPos);
                               SendDlgItemMessage(hwndDlg,IDC_COMBOBOX2,CB_SETCURSEL,(WPARAM)i,0);
                               FillListDiss(&dwPos,DBV_VIEW,0);
                               UpdateAddressBar(dwPos,1);
                           break;
                       }
                   break;
                   case CBN_CLOSEUP:
                       if(OldListBoxBreakPointWndProc != NULL){
                           ::SetWindowLong(hwndListBoxComboBox1,GWL_WNDPROC,(LONG)OldListBoxBreakPointWndProc);
                           OldListBoxBreakPointWndProc = NULL;
                           if(hwndCaptionBar != NULL)
                               ::DestroyWindow(hwndCaptionBar);
                           hwndCaptionBar = NULL;
                           OldListBoxBreakPointWndProc = NULL;
                       }
                   break;
               }
           }
           res = TRUE;
       break;
       case WM_CLOSE:
           CloseDebugWindow();
           res = TRUE;
       break;
       default:
           res = FALSE;
       break;
   }
   return res;
}
//---------------------------------------------------------------------------
void CloseDebugWindow()
{
   ExitDebugMode();
   if(imageListDebug[0] != NULL)
       ImageList_Destroy(imageListDebug[0]);
   if(imageListDebug[1] != NULL)
       ImageList_Destroy(imageListDebug[1]);
   imageListDebug[0] = imageListDebug[1] = NULL;
   ::DestroyWindow(hwndDebug);
   DestroyDebugMemoryWindow();
   hwndDebug = NULL;
   bDebug = 0;
}
//---------------------------------------------------------------------------
static void OnToolTipToolBar(LPNMHDR p)
{
   LPTOOLTIPTEXT lpttt;
   UINT idButton;
   char s[80],s2[20];
   int i;
   LString c;

   lpttt = (LPTOOLTIPTEXT)p;
   idButton = lpttt->hdr.idFrom;
   lpttt->hinst = NULL;

   switch (idButton) {
       case ID_DEBUG_GO:
       case ID_DEBUG_LOAD:
       case ID_DEBUG_SAVE:
       case ID_DEBUG_PAUSE:
       case ID_DEBUG_DIS1:
       case ID_DEBUG_DIS2:
       case ID_DEBUG_DIS3:
       case ID_DEBUG_DIS4:
       case ID_DEBUG_BREAKPOINT:
       case ID_DEBUG_STOP:
       case ID_DEBUG_RESTART:
       case ID_DEBUG_REDRAW:
           c = GetStringFromMenu(hwndDebug,idButton);
           if(!c.IsEmpty()){
               lstrcpy(s,c.c_str());
               i = MenuStringToString(s);
               if(i > 1){
                   lstrcpy(s2,&s[lstrlen(s)+1]);
                   lstrcat(s,"  (");
                   lstrcat(s,s2);
                   lstrcat(s,")");
               }
           }
           else
               lstrcpy(s,"");
           lstrcpy(lpttt->szText,s);
       break;
       default:
           lstrcpy(lpttt->szText,"");
       break;
   }
   UpdateDebugStatusBar(lpttt->szText,0,1);
}
//---------------------------------------------------------------------------
void UpdateDebugToolBar()
{
   HMENU hMenu;
   u8 bRun,bRunDebug,bRunStep;

   hMenu = GetMenu(hwndDebug);
   bRun = (u8)bThreadRun;
   bRunDebug = bRun && bDebug && bPass;
   bRunStep = bRun && bDebug && !bPass;

   SendMessage(hwndDebugToolBar,TB_ENABLEBUTTON,ID_DEBUG_DIS1,MAKELPARAM(bRunStep,0));
   SendMessage(hwndDebugToolBar,TB_ENABLEBUTTON,ID_DEBUG_DIS2,MAKELPARAM(bRunStep,0));
   SendMessage(hwndDebugToolBar,TB_ENABLEBUTTON,ID_DEBUG_PAUSE,MAKELPARAM(bRunDebug,0));
   SendMessage(hwndDebugToolBar,TB_ENABLEBUTTON,ID_DEBUG_GO,MAKELPARAM(bRunStep,0));
   SendMessage(hwndDebugToolBar,TB_ENABLEBUTTON,ID_DEBUG_RESTART,MAKELPARAM(bRunStep,0));
   SendMessage(hwndDebugToolBar,TB_ENABLEBUTTON,ID_DEBUG_STOP,MAKELPARAM(bRunStep,0));
   SendMessage(hwndDebugToolBar,TB_ENABLEBUTTON,ID_DEBUG_BREAKPOINT,MAKELPARAM(bRunStep,0));
   SendMessage(hwndDebugToolBar,TB_ENABLEBUTTON,ID_DEBUG_DIS4,MAKELPARAM(bRunStep,0));
   SendMessage(hwndDebugToolBar,TB_ENABLEBUTTON,ID_DEBUG_SAVE,MAKELPARAM((MAIN_MESSAGE.bBreakChanged != 0),0));

   EnableMenuItem(hMenu,ID_DEBUG_DIS1,MF_BYCOMMAND|(bRunStep ? MF_ENABLED : MF_GRAYED));
   EnableMenuItem(hMenu,ID_DEBUG_DIS2,MF_BYCOMMAND|(bRunStep ? MF_ENABLED : MF_GRAYED));
   EnableMenuItem(hMenu,ID_DEBUG_PAUSE,MF_BYCOMMAND|(bRunDebug ? MF_ENABLED : MF_GRAYED));
   EnableMenuItem(hMenu,ID_DEBUG_GO,MF_BYCOMMAND|(bRunStep ? MF_ENABLED : MF_GRAYED));
   EnableMenuItem(hMenu,ID_DEBUG_STOP,MF_BYCOMMAND|(bRunStep ? MF_ENABLED : MF_GRAYED));
   EnableMenuItem(hMenu,ID_DEBUG_RESTART,MF_BYCOMMAND|(bRunStep ? MF_ENABLED : MF_GRAYED));
   EnableMenuItem(hMenu,ID_DEBUG_BREAKPOINT,MF_BYCOMMAND|(bRunStep ? MF_ENABLED : MF_GRAYED));
   EnableMenuItem(hMenu,ID_DEBUG_DIS4,MF_BYCOMMAND|(bRunStep ? MF_ENABLED : MF_GRAYED));
   EnableMenuItem(hMenu,ID_DEBUG_SAVE,MF_BYCOMMAND|(MAIN_MESSAGE.bBreakChanged ? MF_ENABLED : MF_GRAYED));
}
//---------------------------------------------------------------------------
BOOL CreateDebugToolBar()
{
   TBBUTTON tbb[16];
   HBITMAP bit;
#ifdef _DEBPRO
   TBADDBITMAP tbab;
#endif
   if(hwndDebug == NULL)
       return FALSE;
   hwndDebugToolBar = CreateWindowEx(0, TOOLBARCLASSNAME, (LPSTR) NULL,
       WS_CHILD |WS_VISIBLE|CCS_TOP|TBSTYLE_FLAT|TBSTYLE_TRANSPARENT|TBSTYLE_TOOLTIPS, 0, 0, 0, 0, hwndDebug, (HMENU) IDM_TOOLBARDEBUG,hInstance, NULL);
   if(hwndDebugToolBar == NULL)
       return FALSE;
   SendMessage(hwndDebugToolBar, TB_BUTTONSTRUCTSIZE,(WPARAM) sizeof(TBBUTTON), 0);

   imageListDebug[0] = ImageList_Create(16,16,ILC_COLOR16|ILC_MASK,11,11);
   if(imageListDebug[0] == NULL)
       return FALSE;
   bit = LoadBitmap(hInstance,MAKEINTRESOURCE(IDI_TOOLBAR_DEBUG));
   if(bit == NULL)
       return FALSE;

   ImageList_AddMasked(imageListDebug[0],bit,RGB(192,192,192));
   SendMessage(hwndDebugToolBar,TB_SETIMAGELIST,0,(LPARAM)imageListDebug[0]);
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
   tbb[3].idCommand = ID_DEBUG_GO;
   tbb[3].fsState = TBSTATE_ENABLED;
   tbb[3].fsStyle = TBSTYLE_BUTTON;
   tbb[3].dwData = 0;
   tbb[3].iString = -1;

   tbb[4].iBitmap = 3;
   tbb[4].idCommand = ID_DEBUG_RESTART;
   tbb[4].fsState = TBSTATE_ENABLED;
   tbb[4].fsStyle = TBSTYLE_BUTTON;
   tbb[4].dwData = 0;
   tbb[4].iString = -1;

   tbb[5].iBitmap = 4;
   tbb[5].idCommand = ID_DEBUG_STOP;
   tbb[5].fsState = TBSTATE_ENABLED;
   tbb[5].fsStyle = TBSTYLE_BUTTON;
   tbb[5].dwData = 0;
   tbb[5].iString = -1;

   tbb[6].iBitmap = 5;
   tbb[6].idCommand = ID_DEBUG_PAUSE;
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

   tbb[8].iBitmap = 6;
   tbb[8].idCommand = ID_DEBUG_DIS1;
   tbb[8].fsState = TBSTATE_ENABLED;
   tbb[8].fsStyle = TBSTYLE_BUTTON;
   tbb[8].dwData = 0;
   tbb[8].iString = -1;

   tbb[9].iBitmap = 7;
   tbb[9].idCommand = ID_DEBUG_DIS2;
   tbb[9].fsState = TBSTATE_ENABLED;
   tbb[9].fsStyle = TBSTYLE_BUTTON;
   tbb[9].dwData = 0;
   tbb[9].iString = -1;

   tbb[10].iBitmap = 0;
   tbb[10].idCommand = 0;
   tbb[10].fsState = TBSTATE_ENABLED;
   tbb[10].fsStyle = TBSTYLE_SEP;
   tbb[10].dwData = 0;
   tbb[10].iString = -1;

   tbb[11].iBitmap = 8;
   tbb[11].idCommand = ID_DEBUG_BREAKPOINT;
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

   tbb[13].iBitmap = 9;
   tbb[13].idCommand = ID_DEBUG_DIS3;
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

   tbb[15].iBitmap = 10;
   tbb[15].idCommand = ID_DEBUG_REDRAW;
   tbb[15].fsState = TBSTATE_ENABLED;
   tbb[15].fsStyle = TBSTYLE_BUTTON;
   tbb[15].dwData = 0;
   tbb[15].iString = -1;

   SendMessage(hwndDebugToolBar, TB_ADDBUTTONS, (WPARAM)16,(LPARAM)&tbb);

   imageListDebug[1] = ImageList_LoadImage(hInstance,MAKEINTRESOURCE(IDI_TOOLBARD_DEBUG),16,11,RGB(192,192,192),IMAGE_BITMAP,LR_DEFAULTCOLOR);
   if(imageListDebug[1] == NULL)
       return 0;
   SendMessage(hwndDebugToolBar,TB_SETDISABLEDIMAGELIST,0,(LPARAM)imageListDebug[1]);

   UpdateDebugToolBar();

   return 1;
}
//---------------------------------------------------------------------------
BOOL CreateDebugStatusBar()
{
   int item[3];
   RECT rc;

   hwndDebugStatusBar = NULL;
   if(hwndDebug == NULL)
       return FALSE;
   hwndDebugStatusBar = CreateStatusWindow(WS_CLIPSIBLINGS|WS_CHILD|WS_VISIBLE|WS_BORDER|CCS_BOTTOM,"",hwndDebug,IDM_STATUSBARDEBUG);
   if(hwndDebugStatusBar == NULL)
       return FALSE;
   ::GetClientRect(hwndDebugStatusBar,&rc);
   item[0] = rc.right - 80;
   item[1] = rc.right;
   ::SendMessage(hwndDebugStatusBar,SB_SETPARTS,2,(LPARAM)(LPINT)item);
   return TRUE;
}
//---------------------------------------------------------------------------
void UpdateDebugStatusBar(char *string,int index,char activeTimer)
{
   if(hwndDebugStatusBar == NULL)
       return;
   if(index == 0 && activeTimer != 0){
       if(wTimerSB != NULL)
           KillTimer(hwndDebug,wTimerSB);
       wTimerSB = SetTimer(hwndDebug,TM_STATUSBAR,1500,NULL);
   }
   SendMessage(hwndDebugStatusBar,SB_SETTEXT,(WPARAM)index,(LPARAM)string);
}
//---------------------------------------------------------------------------
#ifdef _DEBPRO
BOOL CALLBACK DlgProcBRK(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   WORD wID;
   u32 i,i1;
   char s[20];
   LString condition;

   switch(uMsg){
       case WM_INITDIALOG:
           ::SetFocus(GetDlgItem(hwndDlg,IDC_EDIT2));
           wsprintf(s,"0x%08X",listBreakPoint[dwBreakPointItem].adress);
           SetWindowText(GetDlgItem(hwndDlg,IDC_BREAKPOINTADRESS),s);
           for(i=0;i<16;i++){
               SendDlgItemMessage(hwndDlg,IDC_COMBOBOX1,CB_ADDSTRING,0,(LPARAM)register_strings[i]);
               SendDlgItemMessage(hwndDlg,IDC_COMBOBOX2,CB_ADDSTRING,0,(LPARAM)register_strings[i]);
           }
           SendDlgItemMessage(hwndDlg,IDC_COMBOBOX1,CB_ADDSTRING,0,(LPARAM)"Carry flag");
           SendDlgItemMessage(hwndDlg,IDC_COMBOBOX1,CB_ADDSTRING,0,(LPARAM)"Overflow flag");
           SendDlgItemMessage(hwndDlg,IDC_COMBOBOX1,CB_ADDSTRING,0,(LPARAM)"Negative flag");
           SendDlgItemMessage(hwndDlg,IDC_COMBOBOX1,CB_ADDSTRING,0,(LPARAM)"Zero flag");
           SendDlgItemMessage(hwndDlg,IDC_COMBOBOX2,CB_ADDSTRING,0,(LPARAM)"Carry flag");
           SendDlgItemMessage(hwndDlg,IDC_COMBOBOX2,CB_ADDSTRING,0,(LPARAM)"Overflow flag");
           SendDlgItemMessage(hwndDlg,IDC_COMBOBOX2,CB_ADDSTRING,0,(LPARAM)"Negative flag");
           SendDlgItemMessage(hwndDlg,IDC_COMBOBOX2,CB_ADDSTRING,0,(LPARAM)"Zero flag");

           if(listBreakPoint[dwBreakPointItem].PassCountCondition != 0){
               wsprintf(s,"%03d",listBreakPoint[dwBreakPointItem].PassCountCondition);
               SetWindowText(GetDlgItem(hwndDlg,IDC_EDIT2),s);
               SendDlgItemMessage(hwndDlg,IDC_EDIT2,EM_SETSEL,0,-1);
           }
           else if((wID = ConditionToValue(dwBreakPointItem,(int *)&i,s,&i1))){
               SendDlgItemMessage(hwndDlg,IDC_COMBOBOX1,CB_SETCURSEL,(WPARAM)i,0);
               CheckRadioButton(hwndDlg,IDC_RADIO1,IDC_RADIO6,IDC_RADIO1+s[0]-1);
               if(wID == 2)
                   SendDlgItemMessage(hwndDlg,IDC_COMBOBOX2,CB_SETCURSEL,(WPARAM)i1,0);
               else{
                   wsprintf(s,"0x%08X",i1);
                   SendDlgItemMessage(hwndDlg,IDC_COMBOBOX2,WM_SETTEXT,0,(LPARAM)s);
               }
           }
           else
               CheckRadioButton(hwndDlg,IDC_RADIO1,IDC_RADIO6,IDC_RADIO1);
       break;
       case WM_COMMAND:
           wID = LOWORD(wParam);
           switch(HIWORD(wParam)){
               case BN_CLICKED:
                   if(wID == IDOK){
                       i = SendDlgItemMessage(hwndDlg,IDC_COMBOBOX1,CB_GETCURSEL,0,0);
                       if(i != CB_ERR){
                           if(i < 16)
                               condition = "r";
                           else
                               condition = "f";
                           wsprintf(s,"%02d",i);
                           condition += s;
                           for(i=IDC_RADIO1;i<=IDC_RADIO6;i++){
                               i1 = IsDlgButtonChecked(hwndDlg,i);
                               if(i1 == BST_CHECKED)
                                   break;
                           }
                           wsprintf(s,"%02d",1 + i - IDC_RADIO1);
                           condition += s;
                           if((i = SendDlgItemMessage(hwndDlg,IDC_COMBOBOX2,CB_GETCURSEL,0,0)) == CB_ERR){
                               SendDlgItemMessage(hwndDlg,IDC_COMBOBOX2,WM_GETTEXT,19,(LPARAM)s);
                               i1 = StrToHex(s);
                               wsprintf(s,"0x%08X",i1);
                               condition += s;
                           }
                           else{
                               if(i < 16)
                                   condition += "r";
                               else
                                   condition += "f";
                               wsprintf(s,"%02d",i);
                               condition += s;
                           }
                           lstrcpy(listBreakPoint[dwBreakPointItem].Condition,condition.c_str());
                       }
                       else{
                           if((::GetWindowText(GetDlgItem(hwndDlg,IDC_EDIT2),s,9)) == 0)
                               i = 0;
                           else
                               i = atoi(s);
                           listBreakPoint[dwBreakPointItem].PassCountCondition = i;
                           listBreakPoint[dwBreakPointItem].Condition[0] = 0;
                       }
                       listBreakPoint[dwBreakPointItem].PassCount = 0;
                       ::EndDialog(hwndDlg,1);
                   }
                   else if(wID == IDCANCEL)
                       ::EndDialog(hwndDlg,0);
               break;
           }
       break;
       case WM_CTLCOLORSTATIC:
           wID = (WORD)GetDlgCtrlID((HWND)lParam);
           switch(wID){
               case IDC_BREAKPOINTADRESS:
                   return (BOOL)GetStockObject(WHITE_BRUSH);
               default:
                   return (BOOL)GetClassLong((HWND)lParam,GCL_HBRBACKGROUND);
           }
       case WM_CLOSE:
           ::EndDialog(hwndDlg,0);
           SetFocus(hwndDebugComboBox1);
       break;
   }
   return FALSE;
}
#endif

#endif





