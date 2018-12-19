#include <windows.h>
#include <commctrl.h>
#pragma hdrstop

#include "linkcontrol.h"

//---------------------------------------------------------------------------
extern HINSTANCE hInstance;
static DWORD dwCount = 0;
typedef struct {
   HFONT currentFont,hLinkFont;
   BOOL bUnder;
   HWND hToolTip;
} LNKCONTROLDATA,*LPLNKCONTROLDATA;
//---------------------------------------------------------------------------
static void StoreWindowDWORD(HWND hwnd,int offset,LPDWORD p);
static DWORD LoadWindowDWORD(HWND hwnd,int offset);
//---------------------------------------------------------------------------
static void OnPaint(HWND hwnd)
{
   PAINTSTRUCT ps;
   RECT rc;
   char s[100];
   BOOL bUnder;
   HFONT currentFont,hLinkFont;
   UINT dtFlags;
   LONG dwStyle;

   if(BeginPaint(hwnd,&ps) == NULL)
       return;
   GetClientRect(hwnd,&rc);
   SetBkMode(ps.hdc,TRANSPARENT);
   currentFont = (HFONT)LoadWindowDWORD(hwnd,0);
   hLinkFont = (HFONT)LoadWindowDWORD(hwnd,1);
   bUnder = (BOOL)LoadWindowDWORD(hwnd,2);
   if(bUnder && hLinkFont != NULL){
       SetTextColor(ps.hdc,GetSysColor(COLOR_HIGHLIGHT));
       SelectObject(ps.hdc,hLinkFont);
   }
   else{
       SetTextColor(ps.hdc,GetSysColor(COLOR_WINDOWTEXT));
       SelectObject(ps.hdc,currentFont);
   }
   ZeroMemory(s,50);
   GetWindowText(hwnd,s,49);
   dtFlags = DT_LEFT;
   dwStyle = GetWindowLong(hwnd,GWL_STYLE);
   if((dwStyle & SS_CENTER))
       dtFlags = DT_CENTER;
   else if((dwStyle & SS_RIGHT))
       dtFlags = DT_RIGHT;
   DrawText(ps.hdc,s,-1,&rc,dtFlags|DT_VCENTER|DT_NOCLIP);
   EndPaint(hwnd,&ps);
}
//---------------------------------------------------------------------------
static DWORD LoadWindowDWORD(HWND hwnd,int offset)
{
   if(!IsWindow(hwnd))
       return 0;
   offset <<= 2;
   return (DWORD)MAKELONG(GetWindowWord(hwnd,offset),GetWindowWord(hwnd,offset+2));
}
//---------------------------------------------------------------------------
static void StoreWindowDWORD(HWND hwnd,int offset,LPDWORD p)
{
   if(!IsWindow(hwnd))
       return;
   offset <<= 2;
   SetWindowWord(hwnd,offset,((LPWORD)p)[0]);
   SetWindowWord(hwnd,offset+2,((LPWORD)p)[1]);
}
//---------------------------------------------------------------------------
static void MessageToToolTip(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   HWND hwndTT;
   MSG msg;

   hwndTT = (HWND)LoadWindowDWORD(hwnd,3);
   if (hwndTT == NULL)
       return;
   msg.lParam = lParam;
   msg.wParam = wParam;
   msg.message = uMsg;
   msg.hwnd = hwnd;
   SendMessage(hwndTT, TTM_RELAYEVENT, 0,(LPARAM) (LPMSG) &msg);
}
//---------------------------------------------------------------------------
static void CreateSystemHook(HWND hwnd)
{
   dwCount++;
}
//---------------------------------------------------------------------------
static LRESULT OnEraseBkGnd(HWND hwnd,WPARAM wParam)
{
   RECT rc;
   HBRUSH hBr;
   HWND hwndParent;

   hwndParent = GetParent(hwnd);
   if(hwndParent != NULL)
       hBr = (HBRUSH)SendMessage(hwndParent,WM_CTLCOLORSTATIC,(WPARAM)wParam,(LPARAM)hwnd);
   if(hBr == NULL)
       hBr = (HBRUSH)GetClassLong(hwnd,GCL_HBRBACKGROUND);
   if(hBr == NULL)
       return 0;
   GetClientRect(hwnd,&rc);
   FillRect((HDC)wParam,&rc,hBr);
   return 1;
}
//---------------------------------------------------------------------------
static void OnClick(HWND hwnd,UINT uMsg)
{
   LONG lStyle;
   HWND hwndParent;
   char *s;
   int iLen;

   iLen = 0;
   StoreWindowDWORD(hwnd,2,(LPDWORD)&iLen);
   lStyle = GetWindowLong(hwnd,GWL_STYLE);
   if((lStyle & SS_NOTIFY)){
       if((hwndParent = GetParent(hwnd)) != NULL){
           SendMessage(hwndParent,WM_COMMAND,MAKEWPARAM(GetWindowLong(hwnd,GWL_ID),
               (uMsg == WM_LBUTTONDOWN ? STN_CLICKED : STN_DBLCLK)),(LPARAM)hwnd);
       }
   }
   else{
       if((iLen = GetWindowTextLength(hwnd)) < 1)
           return;
       iLen += 10;
       if((s = (char *)GlobalAlloc(GPTR,iLen)) == NULL)
           return;
       GetWindowText(hwnd,s,iLen);
       ::ShellExecute(hwnd,"open",s,NULL,NULL,SW_SHOW);
       GlobalFree((HGLOBAL)s);
   }
}
//---------------------------------------------------------------------------
static LRESULT CALLBACK WindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   LOGFONT lf;
   HWND hwndTT;
   TOOLINFO ti;
   int i;
   char *p;
   HFONT currentFont,hLinkFont;
   POINT pt;
   RECT rc;
      
   switch(uMsg){
       case WM_CREATE:
           CreateSystemHook(hwnd);
           if((((LPCREATESTRUCT)lParam)->style & 0x800) && (i = GetWindowTextLength(hwnd)) > 0 && (p = new char[i+2]) != NULL){
               hwndTT = CreateWindow(TOOLTIPS_CLASS, (LPSTR) NULL, TTS_ALWAYSTIP,
                   CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                   NULL, (HMENU) NULL, hInstance, NULL);
               if(hwndTT != NULL){
                   StoreWindowDWORD(hwnd,3,(LPDWORD)&hwndTT);
                   ti.cbSize = sizeof(TOOLINFO);
                   ti.uFlags = 0;
                   ti.hwnd = hwnd;
                   ti.hinst = hInstance;
                   ti.uId = (UINT) 0;
                   GetWindowText(hwnd,p,i+1);
                   ti.lpszText = (LPSTR)p;
                   ti.rect.left = 0;
                   ti.rect.top = 0;
                   ti.rect.right = ((LPCREATESTRUCT) lParam)->cx;
                   ti.rect.bottom = ((LPCREATESTRUCT) lParam)->cy;
                   SendMessage(hwndTT, TTM_ADDTOOL, 0,(LPARAM) (LPTOOLINFO) &ti);
               }
               delete []p;
           }
       break;
       case WM_ERASEBKGND:
           return OnEraseBkGnd(hwnd,wParam);
       case WM_MOUSEMOVE:
           MessageToToolTip(hwnd,uMsg,wParam,lParam);
           pt.x = (int)LOWORD(lParam);
           pt.y = (int)HIWORD(lParam);
           GetClientRect(hwnd,&rc);
           if(!PtInRect(&rc,pt)){
               ReleaseCapture();
               if(LoadWindowDWORD(hwnd,2) != 0){
                   i = 0;
                   StoreWindowDWORD(hwnd,2,(LPDWORD)&i);
                   InvalidateRect(hwnd,NULL,TRUE);
                   UpdateWindow(hwnd);
               }
           }
           else{
               if(!LoadWindowDWORD(hwnd,2)){
                   i = 1;
                   StoreWindowDWORD(hwnd,2,(LPDWORD)&i);
                   InvalidateRect(hwnd,NULL,TRUE);
                   UpdateWindow(hwnd);
               }
               SetCapture(hwnd);
           }
       break;
       case WM_LBUTTONUP:
           MessageToToolTip(hwnd,uMsg,wParam,lParam);
       break;
       case WM_LBUTTONDBLCLK:
       case WM_LBUTTONDOWN:
           MessageToToolTip(hwnd,uMsg,wParam,lParam);
           OnClick(hwnd,uMsg);
       break;
       case WM_DESTROY:
           hLinkFont = (HFONT)LoadWindowDWORD(hwnd,1);
           if(hLinkFont != NULL)
               DeleteObject(hLinkFont);
           hwndTT = (HWND)LoadWindowDWORD(hwnd,3);
           if(hwndTT != NULL)
               DestroyWindow(hwndTT);
           if(dwCount > 0)
               dwCount--;
       break;
       case WM_SETFONT:
           currentFont = (HFONT)wParam;
           GetObject(currentFont,sizeof(LOGFONT),&lf);
           lf.lfUnderline = TRUE;
           hLinkFont = CreateFontIndirect(&lf);
           StoreWindowDWORD(hwnd,0,(LPDWORD)&currentFont);
           StoreWindowDWORD(hwnd,1,(LPDWORD)&hLinkFont);
       break;
       case WM_PAINT:
           OnPaint(hwnd);
           return 0;
   }
   return DefWindowProc(hwnd,uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
void UnregisterLinkControlClass(HINSTANCE this_inst)
{
   ::UnregisterClass("RBALINKCONTROL",this_inst);
   dwCount = 0;
}
//---------------------------------------------------------------------------
BOOL RegisterLinkControlClass(HINSTANCE this_inst)
{
   WNDCLASS  wc;
   HCURSOR hCursor;

   if(!GetClassInfo(this_inst,"RBALINKCONTROL",&wc)){
       dwCount = 0;
       ZeroMemory(&wc,sizeof(WNDCLASS));
       wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS |CS_GLOBALCLASS;
       wc.lpfnWndProc = WindowProc;
       wc.hInstance = this_inst;
	    hCursor = ::LoadCursor(NULL,MAKEINTRESOURCE(32649));
       if(hCursor == NULL)
   	    hCursor = ::LoadCursor(NULL,IDC_ARROW);
       wc.hCursor = hCursor;
       wc.hbrBackground = (HBRUSH)COLOR_3DFACE;
       wc.lpszClassName = "RBALINKCONTROL";
       wc.cbWndExtra = sizeof(LNKCONTROLDATA) + 2;
       if(!RegisterClass(&wc))
           return FALSE;
   }
   return TRUE;
}



