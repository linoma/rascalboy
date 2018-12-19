//---------------------------------------------------------------------------
#include <windows.h>
#pragma hdrstop

#include "debpalobj.h"
#include "gba.h"
#include "resource.h"
#include "debug.h"
#include "memory.h"
#include "gbaemu.h"
#include "lcd.h"
#include "debpal.h"

#ifdef _DEBUG
//---------------------------------------------------------------------------
static BOOL CALLBACK DlgProcObjPal(HWND hwndDlg,UINT uMsg,WPARAM,LPARAM lParam);
//---------------------------------------------------------------------------
static DEBPALETTE dbgpalobj; 
static HCURSOR hCursorPicker = NULL;
static HWND hwndDlgOBJPal;
//---------------------------------------------------------------------------
u8 CreateDebugObjPalWindow(HWND win)
{
   if(hwndDlgOBJPal == NULL)
       if((hwndDlgOBJPal = CreateDialog(hInstance,MAKEINTRESOURCE(IDD_DIALOG4),win,(DLGPROC)DlgProcObjPal)) == NULL)
           return 0;
   else{
       BringWindowToTop(hwndDlgOBJPal);
       InvalidateRect(hwndDlgOBJPal,NULL,TRUE);
       UpdateWindow(hwndDlgOBJPal);
   }

   return 1;
}
//---------------------------------------------------------------------------
void InitDebugObjPalWindow()
{
   hwndDlgOBJPal = NULL;
}
//---------------------------------------------------------------------------
void DestroyDebugObjPalWindow()
{
   if(hwndDlgOBJPal != NULL)
       ::DestroyWindow(hwndDlgOBJPal);
   hwndDlgOBJPal = NULL;
   hCursorPicker = NULL;
}
//---------------------------------------------------------------------------
void UpdateDebugObjPalWindow()
{
   if(hwndDlgOBJPal != NULL){
       InvalidateRect(hwndDlgOBJPal,NULL,FALSE);
       UpdateWindow(hwndDlgOBJPal);
   }
}
//---------------------------------------------------------------------------
static BOOL CALLBACK DlgProcObjPal(HWND hwndDlg,UINT uMsg,WPARAM,LPARAM lParam)
{
   PAINTSTRUCT ps;
   BOOL res;
   POINT pt;
   LONG baseUnits;
   u8 x,y;
   int i;
   RECT rc;

   res = FALSE;
   switch(uMsg){
       case WM_INITDIALOG:
           SetWindowText(hwndDlg,"Sprite Palette");
           dbgpalobj.colorPicker = -1;
           baseUnits = GetDialogBaseUnits();
           x = (u8)((LOWORD(baseUnits) >> 2) << 3);
           y = (u8)((HIWORD(baseUnits) >> 3) << 3);
           GetClientRect(hwndDlg,&rc);
           i = (rc.right - (x << 4)) >> 1;
           ::SetRect(&dbgpalobj.rcColorDraw,i,10,i+(x << 4),10 + (y << 4));
           pt.x = (rc.right - 60) >> 1;
           pt.y = dbgpalobj.rcColorDraw.bottom + 10;
           ::SetRect(&dbgpalobj.rcColorPicker,pt.x,pt.y,pt.x+60,pt.y+40);
           dbgpalobj.hwnd = hwndDlg;
           dbgpalobj.lpPalette = &translated_palette[256];
           if(hCursorPicker == NULL)
               hCursorPicker = LoadCursor(hInstance,MAKEINTRESOURCE(IDI_PICKER));
       break;
       case WM_MOUSEMOVE:
           pt.x = LOWORD(lParam);
           pt.y = HIWORD(lParam);
           if(PtInRect(&dbgpalobj.rcColorDraw,pt))
               SetCursor(hCursorPicker);
           res = TRUE;
       break;
       case WM_LBUTTONDOWN:
           pt.x = LOWORD(lParam);
           pt.y = HIWORD(lParam);
           if(PtInRect(&dbgpalobj.rcColorDraw,pt)){
               pt.x -= dbgpalobj.rcColorDraw.left;
               pt.y -= dbgpalobj.rcColorDraw.top;
               pt.x >>= 4;
               pt.y = (pt.y >> 4) << 4;
               dbgpalobj.colorPicker = translated_palette[256 + pt.y + pt.x];
               dbgpalobj.hdc = NULL;
               DrawColorPicker(&dbgpalobj);
               SetCursor(hCursorPicker);
               res = TRUE;
           }
       break;
       case WM_LBUTTONUP:
           pt.x = LOWORD(lParam);
           pt.y = HIWORD(lParam);
           if(PtInRect(&dbgpalobj.rcColorDraw,pt)){
               SetCursor(hCursorPicker);
               res = TRUE;
           }
       break;
       case WM_PAINT:
       case UM_UPDATE:
           ::BeginPaint(hwndDlg,&ps);
           dbgpalobj.hdc = ps.hdc;
           DrawPalette(&dbgpalobj);
           DrawColorPicker(&dbgpalobj);
           ::EndPaint(hwndDlg,&ps);
       break;
       case WM_CLOSE:
           if(hwndDlgOBJPal != NULL)
               ::DestroyWindow(hwndDlgOBJPal);
       break;
       case WM_DESTROY:
           hwndDlgOBJPal = NULL;
       break;
   }
   return res;
}
#endif