//---------------------------------------------------------------------------
#include <windows.h>
//#include <mmsystem.h>

#pragma hdrstop

#include "about.h"
#include "gba.h"
#include "resource.h"
#include "trad.h"

#define INTERVALLO	40

//---------------------------------------------------------------------------
static BOOL CALLBACK AboutProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
static BOOL OnInitDialog(HWND hwndDlg);
//---------------------------------------------------------------------------
static HWND hwndAbout;
static HBITMAP hbitLogo,hbitText;
static HDC hdcText;
static int nIndexText,nMode,yScroll;
static RECT rcClient;
static HANDLE handleThread;
static BOOL bQuitPrivate;
static int xPos;
//static SIZE szText;
static DWORD dwWait;

char textAbout[][40] = {
 	{"Version 1.3.0.0"},
   {"by Lino"},
};
//static char textScroll[]={"Un grazie particolare a Luca Antignano e al network N!ZONE, a Marco Lago per il supporto grafico e la realizzazione web.\0"};
//---------------------------------------------------------------------------
int ShowAbout()
{
	return ::DialogBox(hInstance,MAKEINTRESOURCE(IDD_DIALOG2),hWin,(DLGPROC)AboutProc);
}
//---------------------------------------------------------------------------
/*static void DrawScroll()
{
   HDC hdc;
   HWND hwnd;
   int y,x,x1;
   RECT rc;
   SIZE sz;

   if((hwnd = GetDlgItem(hwndAbout,IDC_LOGO)) == NULL)
       return;
   hdc = GetDC(hwnd);
   if(hdc == NULL)
       return;
/*   if(hbitText == NULL){
       ::SelectObject(hdc,(HFONT)SendMessage(hwndAbout,WM_GETFONT,0,0));
       GetTextExtentPoint32(hdc,textScroll,lstrlen(textScroll),&szText);
       szText.cx += rcClient.right + 10;
       hbitText = CreateBitmap(szText.cx+10,szText.cy+10,1,1,NULL);
       if(hbitText == NULL)
           goto ex_DrawScroll;
       if((hdcText = CreateCompatibleDC(NULL)) == NULL){
           ::DeleteObject(hbitText);
           hbitText = NULL;
           goto ex_DrawScroll;
       }
       ::SelectObject(hdcText,hbitText);
       ::SelectObject(hdcText,(HFONT)SendMessage(hwndAbout,WM_GETFONT,0,0));
       ::SetTextColor(hdcText,RGB(255,255,255));
       ::SetBkMode(hdcText,OPAQUE);
       ::SetBkColor(hdcText,0);
       ::SetRect(&rc,0,0,szText.cx+10,szText.cy+10);
       ::FillRect(hdcText,&rc,(HBRUSH)GetStockObject(BLACK_BRUSH));
       ::TextOut(hdcText,1,1,textScroll,lstrlen(textScroll));
       xPos = 0;
       xPos = -1;
   }
   if(xPos >= 0){
       y = rcClient.bottom - szText.cy - 2;
       if(xPos == 0){
           ::SetRect(&rc,0,y,rcClient.right,rcClient.bottom);
           ::FillRect(hdc,&rc,(HBRUSH)GetStockObject(BLACK_BRUSH));
       }
       if(xPos >= rcClient.right){
           x = 0;
           x1 = xPos - rcClient.right;
       }
       else{
           x = rcClient.right - xPos;
           x1 = 0;
       }
       BitBlt(hdc,x,y,rcClient.right-x,rcClient.bottom - y,hdcText,x1,0,SRCCOPY);
       xPos++;
       if(xPos >= szText.cx)
           xPos = -1;
   }
   if(xPos < 0){
       xPos = -1;
       ::SelectObject(hdc,(HFONT)SendMessage(hwndAbout,WM_GETFONT,0,0));
       GetTextExtentPoint32(hdc,textAbout[0],lstrlen(textAbout[0]),&sz);
       ::SetTextColor(hdc,RGB(255,255,255));
       ::SetBkMode(hdc,OPAQUE);
       ::SetBkColor(hdc,0);
       TextOut(hdc,rcClient.right - sz.cx - 5,rcClient.bottom - sz.cy - 3,textAbout[0],lstrlen(textAbout[0]));
//   }
ex_DrawScroll:
   ::ReleaseDC(hwnd,hdc);
}*/
//---------------------------------------------------------------------------
static void DestroyLateralScroll()
{
   DWORD res;

   if(handleThread == NULL)
       return;
   bQuitPrivate = TRUE;
   res = dwWait = 0;
   do{
       WaitForSingleObject(handleThread,500);
   }while(GetExitCodeThread(handleThread,&res) && res != 1);
   CloseHandle(handleThread);
   handleThread = NULL;
}
//---------------------------------------------------------------------------
static DWORD WINAPI ThreadFunc(LPVOID)
{
   DWORD dwTick,dwoldTick;

   dwWait = INTERVALLO;
   dwoldTick = GetTickCount();
   while(!bQuitPrivate){
       do{
           SleepEx(INTERVALLO - 20,FALSE);
           dwTick = GetTickCount();
       }while((dwTick - dwoldTick) < dwWait);
       dwWait = INTERVALLO;
       if(xPos < 0)
           xPos = 0;
       dwoldTick = dwTick;
//       DrawScroll();
       if(xPos < 0)
           dwWait = 10000;
   }
   return 1;
}
//---------------------------------------------------------------------------
static void EnableLateralScroll()
{
   DWORD id;

   DestroyLateralScroll();
   bQuitPrivate = TRUE;
   handleThread = CreateThread(NULL,0,ThreadFunc,NULL,0,&id);
}
//---------------------------------------------------------------------------
static BOOL OnInitDialog(HWND hwndDlg)
{
   hdcText = NULL;
   handleThread = NULL;
   hbitText = hbitLogo = NULL;
	yScroll = 0;
   hwndAbout = hwndDlg;
   Translation(hwndDlg,0,IDD_DIALOG2);
   hbitLogo = (HBITMAP)LoadImage(hInstance,MAKEINTRESOURCE(IDI_LOGO),IMAGE_BITMAP,0,0,LR_COLOR);
	if(hbitLogo == NULL)
   	return FALSE;
   nIndexText = 0;
   nMode = 0;
	GetClientRect(GetDlgItem(hwndDlg,IDC_LOGO),&rcClient);
   EnableLateralScroll();
   return TRUE;
}
//---------------------------------------------------------------------------
static BOOL CALLBACK AboutProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   BOOL res;
   LPDRAWITEMSTRUCT lpdis;
	char *s1;
   int x,y;
   HDC hdc;
   BITMAP bm;
   SIZE sz;

   res = TRUE;
   switch(uMsg){
       case WM_INITDIALOG:
       	OnInitDialog(hwndDlg);
       break;
       case WM_DRAWITEM:
           lpdis = (LPDRAWITEMSTRUCT)lParam;
           switch(wParam){
               case IDC_LOGO:
               	FillRect(lpdis->hDC,&lpdis->rcItem,(HBRUSH)GetStockObject(BLACK_BRUSH));
                   s1 = textAbout[1];
                   GetTextExtentPoint32(lpdis->hDC,s1,lstrlen(s1),&sz);
			        GetObject(hbitLogo,sizeof(BITMAP),&bm);
                   x = (rcClient.right - bm.bmWidth) >> 1;
                   y = sz.cy + 5;
                   hdc = CreateCompatibleDC(NULL);
                   SelectObject(hdc,hbitLogo);
                   BitBlt(lpdis->hDC,x+5,y+5,bm.bmWidth,bm.bmHeight,hdc,0,0,SRCCOPY);
                   DeleteDC(hdc);
                   SetBkMode(lpdis->hDC,TRANSPARENT);
                   SetTextColor(lpdis->hDC,RGB(255,255,255));
                   TextOut(lpdis->hDC,2,1,s1,lstrlen(s1));
                   s1 = textAbout[0];
                   GetTextExtentPoint32(lpdis->hDC,s1,lstrlen(s1),&sz);
//                   DrawScroll();
                   TextOut(lpdis->hDC,rcClient.right - sz.cx - 5,rcClient.bottom - sz.cy - 3,textAbout[0],lstrlen(textAbout[0]));
               break;
           }
       break;
       case WM_DESTROY:
           DestroyLateralScroll();
           if(hdcText != NULL)
               ::DeleteDC(hdcText);
           hdcText = NULL;
           if(hbitText != NULL)
               ::DeleteObject(hbitText);
           hbitText = NULL;
           if(hbitLogo != NULL)
           	::DeleteObject(hbitLogo);
           hbitLogo = NULL;
       break;
       case WM_CLOSE:
           bQuitPrivate = TRUE;
           ::EndDialog(hwndDlg,1);
       break;
       default:
           res = FALSE;
       break;
   }
   return res;
}




