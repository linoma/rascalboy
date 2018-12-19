//---------------------------------------------------------------------------
#include <windows.h>
#pragma hdrstop

#include "inputtext.h"
#include "gba.h"

//---------------------------------------------------------------------------
static WNDPROC oldWndProc;
static int bExit;
static LONG lStyle;
//---------------------------------------------------------------------------
static LRESULT CALLBACK WindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	BOOL bFlag;
	LRESULT res;

   bFlag = FALSE;
   switch(uMsg){
   	case WM_KEYDOWN:
       	if(wParam == VK_RETURN && !(lStyle & ES_MULTILINE)){
               bExit = 0;
               res = 0;
               bFlag = TRUE;
           }
           else if(wParam == VK_ESCAPE){
           	res = 0;
               bExit = -1;
               bFlag = TRUE;
           }
           else if(wParam == VK_TAB){
               if(bExit > 0)
       		    bExit = 0;
               res = 0;
               bFlag = TRUE;
           }
       break;
       case WM_KILLFOCUS:
           if(bExit > 0)
               bExit = -1;
       break;
   }
	if(!bFlag)
   	res = CallWindowProc(oldWndProc,hwnd,uMsg,wParam,lParam);
   return res;
}
//---------------------------------------------------------------------------
BOOL InputText(HWND parent,LPRECT rcPos,DWORD dwStyle,char *string,int maxlen)
{
	HWND hwnd;
   HFONT hFont;
   HDC hdc;
   SIZE sz;

   rcPos->bottom -= rcPos->top;
   rcPos->right -= rcPos->left;
   lStyle = dwStyle;
   hwnd = CreateWindowEx(0,"EDIT",string,
       dwStyle|WS_GROUP|WS_BORDER|WS_TABSTOP|WS_CHILD|ES_LEFT|ES_AUTOHSCROLL|ES_MULTILINE|WS_CLIPCHILDREN,
       rcPos->left,rcPos->top,rcPos->right,rcPos->bottom,parent,NULL,hInstance,NULL);
   if(hwnd == NULL)
       return FALSE;
	bExit = 1;
   oldWndProc = (WNDPROC)::SetWindowLong(hwnd,GWL_WNDPROC,(LONG)WindowProc);
   hFont = (HFONT)SendMessage(parent,WM_GETFONT,0,0);
   SendMessage(hwnd,WM_SETFONT,(WPARAM)hFont,0);
   if((hdc = GetDC(hwnd)) != NULL){
       GetTextExtentPoint32(hdc,"X",1,&sz);
       ::ReleaseDC(hwnd,hdc);
       sz.cy += (GetSystemMetrics(SM_CYEDGE) * 2);
       if(sz.cy >= rcPos->bottom)
           MoveWindow(hwnd,rcPos->left,rcPos->top,rcPos->right,sz.cy,FALSE);
   }
   ShowWindow(hwnd,SW_SHOW);
   SetFocus(hwnd);
   SendMessage(hwnd,EM_SETSEL,0,-1);
   while(bExit > 0)
   	ProcessaMessaggi();
	if(bExit == 0)
   	GetWindowText(hwnd,string,maxlen);
   ::DestroyWindow(hwnd);
   if(bExit == 0)
   	return TRUE;
 	return FALSE;
}
