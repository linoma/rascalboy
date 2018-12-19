//---------------------------------------------------------------------------
#include <windows.h>
#include <commctrl.h>
#include "gbaemu.h"
#include "gba.h"
#include "debbkg.h"
#include "fstream.h"
#include "lcd.h"

#ifdef _DEBPRO
//---------------------------------------------------------------------------
static BOOL CALLBACK DlgProcBkg(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
static void CloseDialog(int result);
static LRESULT CALLBACK WndProcControlBkg(HWND hwndControl,UINT uMsg,WPARAM wParam,LPARAM lParam);
static void Aggiorna(BOOL bInit);
static BOOL DrawBackground(HDC hDC);
static BOOL FillBackground();
static BOOL DrawMode1(LPLAYER layer);
static BOOL DrawMode4(LPLAYER l);
//---------------------------------------------------------------------------
static HWND hwndDlgBkg;
static WNDPROC oldWndProcControl;
static HBITMAP hBit = NULL,hBitmap = NULL;
static BITMAPINFO BmpInfo;
static u16 *pBuffer;
static HDC hdcBitmap;
static int layer,xScroll,yScroll,drawMode,charBaseBlock;
//---------------------------------------------------------------------------
void InitDebugBkgWindow()
{
   hBitmap = hBit = NULL;

   hwndDlgBkg = NULL;
}
//---------------------------------------------------------------------------
void DestroyDebugBkgWindow()
{
   CloseDialog(0);
}
//---------------------------------------------------------------------------
u8 CreateDebugBkgWindow(HWND win)
{
   if(hwndDlgBkg == NULL)
       if((hwndDlgBkg = CreateDialog(hInstance,MAKEINTRESOURCE(IDD_DIALOG9),win,(DLGPROC)DlgProcBkg)) == NULL)
           return 0;
   else{
       BringWindowToTop(hwndDlgBkg);
       InvalidateRect(hwndDlgBkg,NULL,TRUE);
       UpdateWindow(hwndDlgBkg);
   }
   return 1;
}
//---------------------------------------------------------------------------
static BOOL DrawTileMode()
{
   int i,i1,i2,i3;
   u8 *src;
   u16 *p,*p1;

   p = pBuffer;
   src = vram_u8 + (charBaseBlock << 14);
   for(i = 0;i < 16;i++){
       for(i3=0;i3<16;i3++){
           p1 = p;
           for(i1=0;i1<8;i1++){
               for(i2=0;i2<8;i2++)
                   *p1++ = translated_palette[*src++];
               p1 += 120;
           }
           p += 8;
       }
       p += 896;
   }
}
//---------------------------------------------------------------------------
static BOOL DrawMode5(LPLAYER l)
{
   int x,y;
   u16 *p,*src;

   p = pBuffer;
   src = vram_u16;
   if(layer != 0)
       src += 0x5000;
   for(y=0;y<120;y++){
       for(x=0;x<160;x++){
           p[y*160+x] = bgrtorgb(src[y*160+x]);
       }
   }
}
//---------------------------------------------------------------------------
static BOOL DrawMode3(LPLAYER l)
{
   int i;
   u16 *p,*src;

   p = pBuffer;
   src = vram_u16;
   for(i=0;i<38400;i++)
       *p++ = bgrtorgb(*src++);
}
//---------------------------------------------------------------------------
static BOOL DrawMode4(LPLAYER l)
{
   int i;
   u16 *p;
   u8 *src;

   p = pBuffer;
   src = vram_u8;
   if(layer != 0)
       src += 0xA000;
   for(i=0;i<38400;i++)
       *p++ = translated_palette[*src++];
}
//---------------------------------------------------------------------------
static BOOL DrawMode1(LPLAYER l)
{
   int i,y2,n,iy;
   u8 y,y1,col;
   u16 *p,tile_data;
   u32 tile_pointer,map_pointer;

   p = pBuffer;
	for(y2 = i = 0; i < l->Width;i++,y2 += l->Height << 6){
       map_pointer = i << l->log2;
		for (n=0; n < l->Width << 3;n += 8) {
           iy = y2 + n;
           tile_data = l->ScreenBaseBlock[map_pointer++];
			tile_pointer = (tile_data & 0x3FF) << 6;
           for(y=0;y<8;y++,iy += l->Width << 3){
               for(y1 = 0;y1 < 8;y1++,tile_pointer++){
//                   if(!l->bPalette)
/*                       col = l->CharBaseBlock[tile_pointer++];
                   else{
                       col = l->CharBaseBlock[tile_pointer >> 1];
                       col = ((col << ((y1 & 1) << 2)) & 0xF) + ((tile_data & 0xF000) >> 8);
                   }*/
                   p[iy+y1] = translated_palette[col];
               }
           }
		}
	}
   return TRUE;
}
//---------------------------------------------------------------------------
static BOOL FillBackground()
{
   LAYER l;
   SCROLLINFO si;
   HWND hwnd;
   int width,height;

   if(layer == -1)
       return TRUE;
   if(hBit != NULL)
       ::DeleteObject(hBit);
   hBit = NULL;
   switch(drawMode){
       case 0:
       case 1:
       case 2:
           CopyMemory(&l,&lcd.layers[layer - 2],sizeof(LAYER));
           width = l.Width << 3;
           height = (l.Height << 3);
       break;
       case 3:
           CopyMemory(&l,&lcd.layers[2],sizeof(LAYER));
           width = 240;
           height = 160;
       break;
       case 5:
           CopyMemory(&l,&lcd.layers[2],sizeof(LAYER));
           width = 160;
           height = 128;
       break;
       case 4:
           width = 240;
           height = 160;
           CopyMemory(&l,&lcd.layers[2],sizeof(LAYER));
       break;
       default:
           width = 128;
           height = 128;
       break;
   }
   ZeroMemory(&BmpInfo,sizeof(BITMAPINFO));
	BmpInfo.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	BmpInfo.bmiHeader.biBitCount    = 16;
	BmpInfo.bmiHeader.biWidth       = width;
	BmpInfo.bmiHeader.biHeight      = -height;
	BmpInfo.bmiHeader.biPlanes      = 1;
	BmpInfo.bmiHeader.biCompression = BI_RGB;
   hBit = ::CreateDIBSection(NULL,&BmpInfo,DIB_RGB_COLORS,(VOID**)&pBuffer,NULL,0);
   if(hBit == NULL)
       return FALSE;
   hwnd = GetDlgItem(hwndDlgBkg,IDC_HSBBKG);
   if(width > 240){
       ZeroMemory(&si,sizeof(SCROLLINFO));
       si.cbSize = sizeof(SCROLLINFO);
       si.fMask = SIF_ALL;
       si.nMax = width - 1;
       si.nPage = 240;
       ShowScrollBar(hwnd,SB_CTL,TRUE);
       SetScrollInfo(hwnd,SB_CTL,&si,TRUE);
   }
   else
       ShowScrollBar(hwnd,SB_CTL,FALSE);
   hwnd = GetDlgItem(hwndDlgBkg,IDC_VSBBKG);
   if(height > 160){
       ZeroMemory(&si,sizeof(SCROLLINFO));
       si.cbSize = sizeof(SCROLLINFO);
       si.fMask = SIF_ALL;
       si.nMax = height - 1;
       si.nPage = 160;
       ShowScrollBar(hwnd,SB_CTL,TRUE);
       SetScrollInfo(hwnd,SB_CTL,&si,TRUE);
   }
   else
       ShowScrollBar(hwnd,SB_CTL,FALSE);
   xScroll = yScroll = 0;

   switch(drawMode){
//       case 0:
       case 1:
       case 2:
           DrawMode1(&l);
       break;
       case 3:
           DrawMode3(&l);
       break;
       case 4:
           DrawMode4(&l);
       break;
       case 5:
           DrawMode5(&l);
       break;
       default:
           DrawTileMode();
       break;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
static BOOL DrawBackground(HDC hDC)
{
   BOOL bInternal;
   HWND hwnd;
   RECT rc;

   if(hDC == NULL){
       hwnd = GetDlgItem(hwndDlgBkg,IDC_SPRITE);
       if((hDC = GetDC(hwnd)) == NULL)
           return FALSE;
       bInternal = TRUE;
   }
   else
       bInternal = FALSE;
   if(hdcBitmap != NULL){
       if(hBitmap != NULL)
           DeleteObject(hBitmap);
       hBitmap = NULL;
       ::DeleteDC(hdcBitmap);
       hdcBitmap = NULL;
   }
   if((hdcBitmap = CreateCompatibleDC(hDC)) != NULL){
       if((hBitmap = CreateCompatibleBitmap(hDC,240,160)) != NULL){
           ::SelectObject(hdcBitmap,hBitmap);
           ::SetRect(&rc,0,0,240,160);
           FillRect(hdcBitmap,&rc,(HBRUSH)GetStockObject(BLACK_BRUSH));
           ::StretchDIBits(hdcBitmap,0,0,240,160,xScroll,yScroll,240,160,pBuffer,&BmpInfo,DIB_RGB_COLORS,SRCCOPY);
           BitBlt(hDC,0,0,240,160,hdcBitmap,0,0,SRCCOPY);
       }
   }
   if(bInternal)
       ::ReleaseDC(hwnd,hDC);
   return TRUE;
}
//---------------------------------------------------------------------------
static void Aggiorna(BOOL bInit)
{
   BOOL item[6];
   int i,sel;
   char s[20];

   sel = -1;
   if(!lcd.Enable){
       item[0] = item[1] = item[2] = item[3] = item[4] = item[5] = FALSE;
       lstrcpy(s,"None");
   }
   else{
       if(lcd.DrawMode < 3){
           item[0] = item[1] = FALSE;
           for(i=0;i<4;i++){
               if((item[2 + i] = lcd.layers[i].Enable) != 0 && sel == -1)
                   sel = 2 + i;
           }
       }
       else if(lcd.DrawMode == 3){
           item[1] = item[2] = item[3] = item[4] = item[5] = FALSE;
           item[0] = TRUE;
           sel = 0;
       }
       else{
           item[2] = item[3] = item[4] = item[5] = FALSE;
           item[0] = item[1] = TRUE;
           sel = lcd.FrameBuffer;
       }
       wsprintf(s,"%d",lcd.DrawMode);
   }
   EnableWindow(GetDlgItem(hwndDlgBkg,IDC_RADIO1),item[0]);
   EnableWindow(GetDlgItem(hwndDlgBkg,IDC_RADIO2),item[1]);
   EnableWindow(GetDlgItem(hwndDlgBkg,IDC_RADIO3),item[2]);
   EnableWindow(GetDlgItem(hwndDlgBkg,IDC_RADIO4),item[3]);
   EnableWindow(GetDlgItem(hwndDlgBkg,IDC_RADIO5),item[4]);
   EnableWindow(GetDlgItem(hwndDlgBkg,IDC_RADIO6),item[5]);
   if(bInit){
       if((layer = sel) != -1)
           SendMessage(GetDlgItem(hwndDlgBkg,IDC_RADIO1 + layer),BM_SETCHECK,(WPARAM)BST_CHECKED,0);
   }
   if(layer != -1){
       FillBackground();
       DrawBackground(NULL);
   }
   SetWindowText(GetDlgItem(hwndDlgBkg,IDC_DRAWMODE),s);
}
//---------------------------------------------------------------------------
static void CloseDialog(int result)
{
   if(oldWndProcControl != NULL && hwndDlgBkg != NULL)
       SetWindowLong(GetDlgItem(hwndDlgBkg,IDC_SPRITE),GWL_WNDPROC,(LONG)oldWndProcControl);
   if(hBitmap != NULL)
       ::DeleteObject(hBitmap);
   hBitmap = NULL;
   if(hdcBitmap != NULL)
       ::DeleteDC(hdcBitmap);
   hdcBitmap = NULL;
   if(hBit != NULL)
       ::DeleteObject(hBit);
   hBit =  NULL;
   hwndDlgBkg = NULL;
}
//---------------------------------------------------------------------------
static BOOL CALLBACK DlgProcBkg(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   BOOL res;
   RECT rc,rc1;
   HWND hwnd;
   WORD wID;
   int i;

   res = FALSE;
   switch(uMsg){
       case WM_INITDIALOG:
           hwnd = GetDlgItem(hwndDlg,IDC_SPRITE);
           oldWndProcControl = (WNDPROC)SetWindowLong(hwnd,GWL_WNDPROC,(LONG)WndProcControlBkg);
           hwndDlgBkg = hwndDlg;

           GetWindowRect(hwnd,&rc);
           GetClientRect(hwnd,&rc1);
           i = rc.right - rc.left - rc1.right;
           SetWindowPos(hwnd,NULL,0,0,240 + i,160 + i,SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSENDCHANGING);
           GetWindowRect(hwnd,&rc1);
           MapWindowPoints(NULL,hwndDlg,(LPPOINT)&rc1,2);

           hwnd = GetDlgItem(hwndDlg,IDC_VSBBKG);
           GetWindowRect(hwnd,&rc);
           i = rc.right - rc.left;
           SetWindowPos(hwnd,NULL,rc1.right,rc1.top,i,rc1.bottom - rc1.top,SWP_NOREPOSITION|SWP_NOSENDCHANGING);

           hwnd = GetDlgItem(hwndDlg,IDC_HSBBKG);
           GetWindowRect(hwnd,&rc);
           SetWindowPos(hwnd,NULL,rc1.left,rc1.bottom,rc1.right - rc1.left,i,SWP_NOREPOSITION|SWP_NOSENDCHANGING);
           drawMode = lcd.DrawMode;
           Aggiorna(TRUE);
       break;
       case WM_VSCROLL:
           wID = (WORD)GetDlgCtrlID((HWND)lParam);
           switch(wID){
               case IDC_VSBBKG:
                   switch(LOWORD(wParam)){
                       case SB_LINEUP:
                           i = yScroll - 1;
                       break;
                       case SB_LINEDOWN:
                           i = yScroll + 1;
                       break;
                       case SB_THUMBPOSITION:
                       case SB_THUMBTRACK:
                           i = HIWORD(wParam);
                       break;
                       default:
                           i = yScroll;
                       break;
                   }
                   if(i != yScroll){
                       yScroll = i;
                       SetScrollPos((HWND)lParam,SB_CTL,yScroll,TRUE);
                       DrawBackground(NULL);
                   }
               break;
           }
       break;
       case WM_HSCROLL:
           wID = (WORD)GetDlgCtrlID((HWND)lParam);
           switch(wID){
               case IDC_HSBBKG:
                   switch(LOWORD(wParam)){
                       case SB_LINEUP:
                           i = xScroll - 1;
                       break;
                       case SB_LINEDOWN:
                           i = xScroll + 1;
                       break;
                       case SB_THUMBPOSITION:
                       case SB_THUMBTRACK:
                           i = HIWORD(wParam);
                       break;
                       default:
                           i = xScroll;
                       break;
                   }
                   if(i != xScroll){
                       xScroll = i;
                       SetScrollPos((HWND)lParam,SB_CTL,xScroll,TRUE);
                       DrawBackground(NULL);
                   }
               break;
           }
       break;
       case WM_COMMAND:
           wID = LOWORD(wParam);
           switch(HIWORD(wParam)){
               case BN_CLICKED:
                   switch(wID){
                   	case IDOK:
                       	DestroyWindow(hwndDlg);
                       break;
                       case IDC_RADIO1:
                       case IDC_RADIO2:
                       case IDC_RADIO3:
                       case IDC_RADIO4:
                       case IDC_RADIO5:
                       case IDC_RADIO6:
                           drawMode = lcd.DrawMode;
                           layer = wID - IDC_RADIO1;
                           Aggiorna(FALSE);
                       break;
                       case IDC_RADIO7:
                       case IDC_RADIO8:
                       case IDC_RADIO9:
                       case IDC_RADIO10:
                           drawMode = 0xFF;
                           charBaseBlock = wID - IDC_RADIO7;
                           Aggiorna(FALSE);
                       break;
                       case IDC_BUTTON2:
                           Aggiorna(FALSE);
                       break;
                   }
               break;
           }
       break;
       case WM_CLOSE:
           DestroyWindow(hwndDlg);
       break;
       case WM_DESTROY:
           CloseDialog(0);
           res = TRUE;
       break;
   }
   return res;
}
//---------------------------------------------------------------------------
static LRESULT CALLBACK WndProcControlBkg(HWND hwndControl,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   LRESULT res;
   BOOL bFlag;
   RECT rc;
   PAINTSTRUCT ps;

   bFlag = FALSE;
   switch(uMsg){
       case WM_PAINT:
           res = 1;
           bFlag = TRUE;
           ::BeginPaint(hwndControl,&ps);
           GetClientRect(hwndControl,&rc);
//           FillRect(ps.hdc,&rc,GetStockObject(BLACK_BRUSH));
           ::EndPaint(hwndControl,&ps);
       break;
   }
   if(!bFlag)
       res = CallWindowProc(oldWndProcControl,hwndControl,uMsg,wParam,lParam);
   return res;
}
#endif
