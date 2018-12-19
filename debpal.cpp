//---------------------------------------------------------------------------
#include <windows.h>
#pragma hdrstop

#include "debpal.h"
#include "gba.h"
#include "resource.h"
#include "debug.h"
#include "memory.h"
#include "gbaemu.h"
#include "lcd.h"

#ifdef _DEBUG
//---------------------------------------------------------------------------
static BOOL CALLBACK DlgProcBGPal(HWND hwndDlg,UINT uMsg,WPARAM,LPARAM);
static BOOL CALLBACK DlgProcDMA(HWND hwndDlg,UINT uMsg,WPARAM,LPARAM lParam);
//---------------------------------------------------------------------------
static HWND hwndDlgBGPal,hwndDlgDMA;
static HCURSOR hCursorPicker = NULL;
static DEBPALETTE dbgpalbk;
//---------------------------------------------------------------------------
u8 CreateDebugBgPalWindow(HWND win)
{
   if(hwndDlgBGPal == NULL)
       if((hwndDlgBGPal = CreateDialog(hInstance,MAKEINTRESOURCE(IDD_DIALOG4),win,(DLGPROC)DlgProcBGPal)) == NULL)
           return 0;
   else{
       BringWindowToTop(hwndDlgBGPal);
       InvalidateRect(hwndDlgBGPal,NULL,TRUE);
       UpdateWindow(hwndDlgBGPal);
   }
   return 1;
}
//---------------------------------------------------------------------------
void InitDebugBgPalWindow()
{
   hwndDlgBGPal = NULL;
}
//---------------------------------------------------------------------------
void DestroyDebugBgPalWindow()
{
   if(hwndDlgBGPal != NULL)
       ::DestroyWindow(hwndDlgBGPal);
   hwndDlgBGPal = NULL;
   hCursorPicker = NULL;
}
//---------------------------------------------------------------------------
void UpdateDebugBgPalWindow()
{
   if(hwndDlgBGPal == NULL)
       return;
   InvalidateRect(hwndDlgBGPal,NULL,FALSE);
   UpdateWindow(hwndDlgBGPal);
}
//---------------------------------------------------------------------------
void DrawColorPicker(LPDEBPALETTE pPalette)
{
   RECT rc;
   HBRUSH hBrush;
   COLORREF col;
   char s[50];
   HFONT hFont;
   SIZE sz;
   int y,x,colorPicker;
   u8 flag;
   HDC hdc;
   HWND hwnd;

   hwnd = pPalette->hwnd;
   hdc = pPalette->hdc;
   if(hdc == NULL){
       hdc = GetDC(hwnd);
       flag = 1;
   }
   else
       flag = 0;
   CopyRect(&rc,&pPalette->rcColorPicker);
   InflateRect(&rc,GetSystemMetrics(SM_CXBORDER)<<1,GetSystemMetrics(SM_CYBORDER)<<1);
   DrawEdge(hdc,&rc,EDGE_SUNKEN,BF_RECT);
   OffsetRect(&rc,rc.right-rc.left+5,0);
   FillRect(hdc,&rc,(HBRUSH)GetSysColorBrush(COLOR_3DFACE));
   if((colorPicker = pPalette->colorPicker) == -1)
       col = 0;
   else
       col = RGB(((colorPicker >> 10) & 0x1F)<<3,((colorPicker >> 5) & 0x1F)<<3,(colorPicker & 0x1F)<<3);
   CopyRect(&rc,&pPalette->rcColorPicker);
   if((hBrush = CreateSolidBrush(col)) != NULL){
       FillRect(hdc,&rc,hBrush);
       ::DeleteObject(hBrush);
   }
   if(colorPicker == -1)
       goto Ex_DrawColorPicker;
   hFont = (HFONT)SendMessage(hwnd,WM_GETFONT,0,0);
   SelectObject(hdc,hFont);
   ::SetBkMode(hdc,TRANSPARENT);
   GetTextExtentPoint32(hdc,"X",1,&sz);
   x = rc.right + 10;
   y = rc.top;
   wsprintf(s,"R : 0x%02X",GetRValue(col));
   TextOut(hdc,x,y,s,lstrlen(s));
   y += sz.cy;
   wsprintf(s,"G : 0x%02X",GetGValue(col));
   TextOut(hdc,x,y,s,lstrlen(s));
   y += sz.cy;
   wsprintf(s,"B : 0x%02X",GetBValue(col));
   TextOut(hdc,x,y,s,lstrlen(s));
Ex_DrawColorPicker:
   if(flag != 0)
       ::ReleaseDC(hwnd,hdc);
}
//---------------------------------------------------------------------------
void DrawPalette(LPDEBPALETTE pPalette)
{
   u8 i,i1,x,y;
   u16 x1,y1,x2;
   HBRUSH hBrush;
   COLORREF rgb;
   u16 color;
   RECT rc;
   LONG baseUnits;
   u16 *palette;

   baseUnits = GetDialogBaseUnits();
   x = (u8)((LOWORD(baseUnits) >> 2) << 3);
   y = (u8)((HIWORD(baseUnits) >> 3) << 3);
   ::CopyRect(&rc,&pPalette->rcColorDraw);
   y1 = (u16)rc.top;
   x2 = (u16)rc.left;
   InflateRect(&rc,GetSystemMetrics(SM_CXBORDER)<<1,GetSystemMetrics(SM_CYBORDER)<<1);
   DrawEdge(pPalette->hdc,&rc,EDGE_SUNKEN,BF_RECT);
   palette = pPalette->lpPalette;
   for(i=0;i<16;i++){
       x1 = x2;
       for(i1=0;i1<16;i1++){
           color = (u16)(*palette++);
           rgb = RGB(((color >> 10) & 0x1F)<<3,((color >> 5) & 0x1F)<<3,(color & 0x1F)<<3);
           hBrush = ::CreateSolidBrush(rgb);
           ::SetRect(&rc,x1,y1,x1 + x,y1+y);
           ::FillRect(pPalette->hdc,&rc,hBrush);
           ::DeleteObject(hBrush);
           x1 += x;
       }
       y1 += y;
   }
}
//---------------------------------------------------------------------------
static BOOL CALLBACK DlgProcBGPal(HWND hwndDlg,UINT uMsg,WPARAM,LPARAM lParam)
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
           SetWindowText(hwndDlg,"Background Palette");
           dbgpalbk.colorPicker = -1;
           baseUnits = GetDialogBaseUnits();
           x = (u8)((LOWORD(baseUnits) >> 2) << 3);
           y = (u8)((HIWORD(baseUnits) >> 3) << 3);
           GetClientRect(hwndDlg,&rc);
           i = (rc.right - (x << 4)) >> 1;
           ::SetRect(&dbgpalbk.rcColorDraw,i,10,i+(x << 4),10 + (y << 4));
           pt.x = (rc.right - 60) >> 1;
           pt.y = dbgpalbk.rcColorDraw.bottom + 10;
           ::SetRect(&dbgpalbk.rcColorPicker,pt.x,pt.y,pt.x+60,pt.y+40);
           dbgpalbk.lpPalette = translated_palette;
           dbgpalbk.hwnd = hwndDlg;
           if(hCursorPicker == NULL)
               hCursorPicker = LoadCursor(hInstance,MAKEINTRESOURCE(IDI_PICKER));
       break;
       case WM_MOUSEMOVE:
           pt.x = LOWORD(lParam);
           pt.y = HIWORD(lParam);
           if(PtInRect(&dbgpalbk.rcColorDraw,pt))
               SetCursor(hCursorPicker);
           res = TRUE;
       break;
       case WM_LBUTTONDOWN:
           pt.x = LOWORD(lParam);
           pt.y = HIWORD(lParam);
           if(PtInRect(&dbgpalbk.rcColorDraw,pt)){
               pt.x -= dbgpalbk.rcColorDraw.left;
               pt.y -= dbgpalbk.rcColorDraw.top;
               pt.x >>= 4;
               pt.y = (pt.y >> 4) << 4;
               dbgpalbk.hdc = NULL;
               dbgpalbk.colorPicker = translated_palette[pt.y + pt.x];
               DrawColorPicker(&dbgpalbk);
               SetCursor(hCursorPicker);
               res = TRUE;
           }
       break;
       case WM_LBUTTONUP:
           pt.x = LOWORD(lParam);
           pt.y = HIWORD(lParam);
           if(PtInRect(&dbgpalbk.rcColorDraw,pt)){
               SetCursor(hCursorPicker);
               res = TRUE;
           }
       break;
       case WM_PAINT:
       case UM_UPDATE:
           ::BeginPaint(hwndDlg,&ps);
           dbgpalbk.hdc = ps.hdc;
           DrawPalette(&dbgpalbk);
           DrawColorPicker(&dbgpalbk);
           ::EndPaint(hwndDlg,&ps);
           res = TRUE;
       break;
       case WM_CLOSE:
           if(hwndDlgBGPal != NULL)
               ::DestroyWindow(hwndDlgBGPal);
       break;
       case WM_DESTROY:
           hwndDlgBGPal = NULL;
       break;
   }
   return res;
}
#ifdef _DEBPRO
//---------------------------------------------------------------------------
u8 CreateDebugDMAWindow(HWND win)
{
   if(hwndDlgDMA == NULL)
       if((hwndDlgDMA = CreateDialog(hInstance,MAKEINTRESOURCE(IDD_DIALOG5),win,(DLGPROC)DlgProcDMA)) == NULL)
           return 0;
   else
       BringWindowToTop(hwndDlgDMA);
   return 1;
}
//---------------------------------------------------------------------------
void InitDebugDMAWindow()
{
   hwndDlgDMA = NULL;
}
//---------------------------------------------------------------------------
void DestroyDebugDMAWindow()
{
   if(hwndDlgDMA != NULL)
       ::DestroyWindow(hwndDlgDMA);
   hwndDlgDMA = NULL;
}
//---------------------------------------------------------------------------
void UpdateDebugDMAWindow()
{
   if(hwndDlgDMA != NULL)
       SendMessage(hwndDlgDMA,UM_UPDATE,0,0);
}
//---------------------------------------------------------------------------
static void UpdateDMAWindow(HWND hWnd)
{
   char s[30];
   u16 cnt;

   wsprintf(s,"0x%08X",dma[0].Dst);
   SetWindowText(GetDlgItem(hWnd,IDC_DMA0DAD),s);
   wsprintf(s,"0x%08X",dma[0].Src);
   SetWindowText(GetDlgItem(hWnd,IDC_DMA0SAD),s);
   wsprintf(s,"0x%08X",dma[1].Dst);
   SetWindowText(GetDlgItem(hWnd,IDC_DMA1DAD),s);
   wsprintf(s,"0x%08X",dma[1].Src);
   SetWindowText(GetDlgItem(hWnd,IDC_DMA1SAD),s);
   wsprintf(s,"0x%08X",dma[2].Dst);
   SetWindowText(GetDlgItem(hWnd,IDC_DMA2DAD),s);
   wsprintf(s,"0x%08X",dma[2].Src);
   SetWindowText(GetDlgItem(hWnd,IDC_DMA2SAD),s);
   wsprintf(s,"0x%08X",dma[3].Dst);
   SetWindowText(GetDlgItem(hWnd,IDC_DMA3DAD),s);
   wsprintf(s,"0x%08X",dma[3].Src);
   SetWindowText(GetDlgItem(hWnd,IDC_DMA3SAD),s);
   cnt = DM0CNT_H;
   SendMessage(GetDlgItem(hWnd,IDC_CHK0RP),BM_SETCHECK,(WPARAM)(cnt & 0x200) != 0 ? BST_CHECKED : BST_UNCHECKED,0);
   SendMessage(GetDlgItem(hWnd,IDC_CHK0RL),BM_SETCHECK,(WPARAM)(cnt & 0x60) == 0x60 ? BST_CHECKED : BST_UNCHECKED,0);
   SendMessage(GetDlgItem(hWnd,IDC_CHK0E),BM_SETCHECK,(WPARAM)(cnt & 0x8000) != 0 ? BST_CHECKED : BST_UNCHECKED,0);
   SendDlgItemMessage(hWnd,IDC_CHK0I,BM_SETCHECK,(WPARAM)((cnt & 0x4000) != 0 ? BST_CHECKED : BST_UNCHECKED),0);
   wsprintf(s,"%02d",(cnt & 0x3000) >> 12);
   SetWindowText(GetDlgItem(hWnd,IDC_DMA0START),s);
   cnt = DM1CNT_H;
   SendMessage(GetDlgItem(hWnd,IDC_CHK1RP),BM_SETCHECK,(WPARAM)(cnt & 0x200) != 0 ? BST_CHECKED : BST_UNCHECKED,0);
   SendMessage(GetDlgItem(hWnd,IDC_CHK1RL),BM_SETCHECK,(WPARAM)(cnt & 0x60) == 0x60 ? BST_CHECKED : BST_UNCHECKED,0);
   SendMessage(GetDlgItem(hWnd,IDC_CHK1E),BM_SETCHECK,(WPARAM)(cnt & 0x8000) != 0 ? BST_CHECKED : BST_UNCHECKED,0);
   SendDlgItemMessage(hWnd,IDC_CHK1I,BM_SETCHECK,(WPARAM)((cnt & 0x4000) != 0 ? BST_CHECKED : BST_UNCHECKED),0);
   wsprintf(s,"%02d",(cnt & 0x3000) >> 12);
   SetWindowText(GetDlgItem(hWnd,IDC_DMA1START),s);
   cnt = DM2CNT_H;
   SendMessage(GetDlgItem(hWnd,IDC_CHK2RP),BM_SETCHECK,(WPARAM)(cnt & 0x200) != 0 ? BST_CHECKED : BST_UNCHECKED,0);
   SendMessage(GetDlgItem(hWnd,IDC_CHK2RL),BM_SETCHECK,(WPARAM)(cnt & 0x60) == 0x60 ? BST_CHECKED : BST_UNCHECKED,0);
   SendMessage(GetDlgItem(hWnd,IDC_CHK2E),BM_SETCHECK,(WPARAM)(cnt & 0x8000) != 0 ? BST_CHECKED : BST_UNCHECKED,0);
   SendDlgItemMessage(hWnd,IDC_CHK2I,BM_SETCHECK,(WPARAM)((cnt & 0x4000) != 0 ? BST_CHECKED : BST_UNCHECKED),0);
   wsprintf(s,"%02d",(cnt & 0x3000) >> 12);
   SetWindowText(GetDlgItem(hWnd,IDC_DMA2START),s);
   cnt = DM3CNT_H;
   SendMessage(GetDlgItem(hWnd,IDC_CHK3RP),BM_SETCHECK,(WPARAM)(cnt & 0x200) != 0 ? BST_CHECKED : BST_UNCHECKED,0);
   SendMessage(GetDlgItem(hWnd,IDC_CHK3RL),BM_SETCHECK,(WPARAM)(cnt & 0x60) == 0x60 ? BST_CHECKED : BST_UNCHECKED,0);
   SendMessage(GetDlgItem(hWnd,IDC_CHK3E),BM_SETCHECK,(WPARAM)(cnt & 0x8000) != 0 ? BST_CHECKED : BST_UNCHECKED,0);
   SendDlgItemMessage(hWnd,IDC_CHK3I,BM_SETCHECK,(WPARAM)((cnt & 0x4000) != 0 ? BST_CHECKED : BST_UNCHECKED),0);
   wsprintf(s,"%02d",(cnt & 0x3000) >> 12);
   SetWindowText(GetDlgItem(hWnd,IDC_DMA3START),s);
}
//---------------------------------------------------------------------------
static BOOL CALLBACK DlgProcDMA(HWND hwndDlg,UINT uMsg,WPARAM,LPARAM lParam)
{
   WORD wID;

   switch(uMsg){
       case WM_CTLCOLORSTATIC:
           wID = (WORD)GetDlgCtrlID((HWND)lParam);
           switch(wID){
               case IDC_DMA0DAD:
               case IDC_DMA1DAD:
               case IDC_DMA2DAD:
               case IDC_DMA3DAD:
               case IDC_DMA0SAD:
               case IDC_DMA1SAD:
               case IDC_DMA2SAD:
               case IDC_DMA3SAD:
                   return (BOOL)GetStockObject(WHITE_BRUSH);
               default:
                   return (BOOL)GetClassLong((HWND)lParam,GCL_HBRBACKGROUND);
           }
       case UM_UPDATE:
       case WM_PAINT:
           UpdateDMAWindow(hwndDlg);
       break;
       case WM_CLOSE:
           if(hwndDlgDMA != NULL)
               ::DestroyWindow(hwndDlgDMA);
       break;
       case WM_DESTROY:
           hwndDlgDMA = NULL;
       break;
   }
   return FALSE;
}
#endif
#endif





