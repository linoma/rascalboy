#include <windows.h>
#include <commctrl.h>
#include "gbaemu.h"
#include "gba.h"
#include "debsprite.h"
#include "sprite.h"
#include "fstream.h"

#ifdef _DEBPRO
//---------------------------------------------------------------------------
static RECT rcSprite;
static SIZE szBorder;
static HBITMAP hBit = NULL,hBitmap = NULL;
static BYTE IndexSprite;
static BITMAPINFO BmpInfo;
static u16 *pBuffer;
static RECT rcControl[10];
static HWND hwndDlgSprite;
static int yScroll,xScroll,iScale;
static HDC hdcBitmap;
//---------------------------------------------------------------------------
static BOOL CALLBACK DlgProcSprite(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
#endif
//---------------------------------------------------------------------------
BOOL SaveBitmap(HDC hdc,HBITMAP bit,char *fileName)
{
	LString nameFile;
	LFile *fp;
   BITMAP bm;
   PBITMAPINFO pbmi;
	BITMAPFILEHEADER hdr;
	LPBYTE lpBits;
	PBITMAPINFOHEADER pbih;
	WORD cClrBits;
	int i;
   BOOL res;

   res = FALSE;
	fp = NULL;
   lpBits = NULL;
   nameFile = fileName;
	nameFile.LowerCase();
   if((i = nameFile.Pos(".")) < 1)
		nameFile += ".bmp";
	else if(nameFile.Pos(".bmp") < 1 && i > 0)
   	nameFile = nameFile.SubString(1,i-1) + ".bmp";
   GetObject(bit,sizeof(BITMAP),&bm);
   cClrBits = (WORD)(bm.bmPlanes * bm.bmBitsPixel);
   if (cClrBits == 1)
   	cClrBits = 1;
   else if (cClrBits <= 4)
   	cClrBits = 4;
   else if (cClrBits <= 8)
   	cClrBits = 8;
   else if (cClrBits <= 16)
   	cClrBits = 16;
   else if (cClrBits <= 24)
   	cClrBits = 24;
   else
   	cClrBits = 32;
   pbmi = (PBITMAPINFO)GlobalAlloc(GPTR,sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (2^cClrBits));
   if(pbmi == NULL)
   	goto Ex_SaveBitmap;
   pbih = &pbmi->bmiHeader;
	pbih->biSize = sizeof(BITMAPINFOHEADER);
   pbih->biWidth = bm.bmWidth;
   pbih->biHeight = bm.bmHeight;
   pbih->biPlanes = bm.bmPlanes;
   pbih->biBitCount = bm.bmBitsPixel;
   pbih->biClrUsed = (cClrBits < 9 ? cClrBits : 0);
   pbih->biCompression = BI_RGB;
   pbih->biSizeImage = (pbih->biWidth + 7) / 8 * pbih->biHeight * cClrBits;
   hdr.bfType = 0x4d42;
   hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) + pbih->biSize + pbih->biClrUsed * sizeof(RGBQUAD) + pbih->biSizeImage);
   hdr.bfReserved1 = 0;
   hdr.bfReserved2 = 0;
   hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + pbih->biSize + pbih->biClrUsed * sizeof (RGBQUAD);
   lpBits = (LPBYTE)GlobalAlloc(GMEM_FIXED,pbih->biSizeImage);
	if(lpBits == NULL)
   	goto Ex_SaveBitmap;
   GetDIBits(hdc,bit,0,(WORD)pbih->biHeight,lpBits,pbmi, DIB_RGB_COLORS);
   fp = new LFile(nameFile.c_str());
   if(fp == NULL || !fp->Open(GENERIC_READ|GENERIC_WRITE,CREATE_ALWAYS))
   	goto Ex_SaveBitmap;
	fp->Write(&hdr,sizeof(BITMAPFILEHEADER));
   fp->Write(pbih,sizeof(BITMAPINFOHEADER) + pbih->biClrUsed * sizeof (RGBQUAD));
   fp->Write(lpBits,pbih->biSizeImage);
   res = TRUE;
Ex_SaveBitmap:
   if(fp != NULL)
       delete fp;
   if(lpBits != NULL)
   	GlobalFree((HGLOBAL)lpBits);
   if(pbmi != NULL)
   	GlobalFree((HGLOBAL)pbmi);
   return res;
}
#ifdef _DEBPRO
//---------------------------------------------------------------------------
void InitDebugSpriteWindow()
{
   hBitmap = hBit = NULL;
   hdcBitmap = NULL;
   hwndDlgSprite = NULL;
   xScroll = 0;
   yScroll = 0;
   iScale = 0;
}
//---------------------------------------------------------------------------
void DestroyDebugSpriteWindow()
{
   if(hBitmap != NULL)
       ::DeleteObject(hBitmap);
   if(hdcBitmap != NULL)
       ::DeleteDC(hdcBitmap);
   if(hBit != NULL)
       ::DeleteObject(hBit);
   if(hwndDlgSprite != NULL)
       ::DestroyWindow(hwndDlgSprite);
   hwndDlgSprite = NULL;
}
//---------------------------------------------------------------------------
u8 CreateDebugSpriteWindow(HWND win)
{
   if(hwndDlgSprite == NULL)
       if((hwndDlgSprite = CreateDialog(hInstance,MAKEINTRESOURCE(IDD_DIALOG8),win,(DLGPROC)DlgProcSprite)) == NULL)
           return 0;
   else{
       BringWindowToTop(hwndDlgSprite);
       InvalidateRect(hwndDlgSprite,NULL,TRUE);
       UpdateWindow(hwndDlgSprite);
   }
   return 1;
}
//---------------------------------------------------------------------------
static int DrawSprite(u8 nSprite)
{
   LPSPRITE pSprite;
   int sx,sy;

   if(hBit != NULL)
       ::DeleteObject(hBit);
   hBit = NULL;
   if((pSprite = GetSprite(nSprite)) == NULL)
       return -1;
   if(!pSprite->bRot && pSprite->bDouble || !pSprite->Enable)
       return -1;
   sx = pSprite->SizeX;
   sy = pSprite->SizeY;
   if(pSprite->bDouble){
       sx <<= 1;
       sy <<= 1;
   }
   ZeroMemory(&BmpInfo,sizeof(BITMAPINFO));
	BmpInfo.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	BmpInfo.bmiHeader.biBitCount    = 16;
	BmpInfo.bmiHeader.biWidth       = sx;
	BmpInfo.bmiHeader.biHeight      = -sy;
	BmpInfo.bmiHeader.biPlanes      = 1;
	BmpInfo.bmiHeader.biCompression = BI_RGB;
   hBit = ::CreateDIBSection(NULL,&BmpInfo,DIB_RGB_COLORS,(VOID**)&pBuffer,NULL,0);
   if(hBit == NULL)
       return -1;
   return DrawDebugSprite(nSprite,pBuffer,0);
}
//---------------------------------------------------------------------------
static void CloseDialog(int result)
{
   if(hBitmap != NULL)
       ::DeleteObject(hBitmap);
   hBitmap = NULL;
   if(hdcBitmap != NULL)
       ::DeleteDC(hdcBitmap);
   hdcBitmap = NULL;
   if(hBit != NULL)
       ::DeleteObject(hBit);
   hBit = NULL;
   hwndDlgSprite = NULL;
}
//---------------------------------------------------------------------------
void EraseBackGround(HDC hdc)
{
   RECT rc;

   ::CopyRect(&rc,&rcSprite);
   InflateRect(&rc,szBorder.cx,szBorder.cy);
   DrawEdge(hdc,&rc,EDGE_SUNKEN,BF_RECT);
   InflateRect(&rc,-szBorder.cx,-szBorder.cy);
   FillRect(hdc,&rc,(HBRUSH)GetStockObject(BLACK_BRUSH));
}
//---------------------------------------------------------------------------
static u8 DrawSpriteBitmap(HDC hdc,u8 eraseBk)
{
   RECT rc,rc1,rc2;
   LPSPRITE pSprite;
   int i;
   u8 flag;
   s8 flagScale;
   HWND hwnd;
   SCROLLINFO si;
   u8 res;
   SIZE sz;

   res = 1;
   if(hdc == NULL){
       hdc = GetDC(hwndDlgSprite);
       flag = 1;
   }
   else
       flag = 0;

   pSprite = GetSprite((u8)(IndexSprite-1));
   ::CopyRect(&rc,&rcSprite);
   if(hdcBitmap == NULL || hBitmap == NULL || pSprite == NULL){
       res = 0;
       EraseBackGround(hdc);
       goto Ex_DrawSpriteBitmap;
   }
   ::SetRect(&rc1,0,0,pSprite->SizeX,pSprite->SizeY);
   sz.cx = rc1.right;
   sz.cy = rc1.bottom;
   i = SendDlgItemMessage(hwndDlgSprite,IDC_TRACK1,TBM_GETPOS,0,0);
   if(i != iScale){
       flagScale = (u8)(iScale > i ? -1 : 1);
       iScale = i;
   }
   else
       flagScale = 0;
   if(i > 0){
       rc1.right *= i;
       rc1.bottom *= i;
   }
   CopyRect(&rc2,&rc1);
   hwnd = GetDlgItem(hwndDlgSprite,IDC_VSBSPR);
   if(rc1.bottom > (i = rc.bottom - rc.top)){
       if(flagScale != 0){
           ZeroMemory(&si,sizeof(SCROLLINFO));
           si.cbSize = sizeof(SCROLLINFO);
           si.fMask = SIF_ALL;
           si.nMax = rc2.bottom-1;
           si.nPage = i;
           if(flagScale < 0)
               yScroll = 0;
           si.nPos = yScroll;
           ShowScrollBar(hwnd,SB_CTL,TRUE);
           SetScrollInfo(hwnd,SB_CTL,&si,TRUE);
       }
       rc2.bottom = i;
   }
   else if(flagScale != 0)
       ShowScrollBar(hwnd,SB_CTL,FALSE);
   hwnd = GetDlgItem(hwndDlgSprite,IDC_HSBSPR);
   if(rc1.right > (i = rc.right - rc.left)){
       if(flagScale != 0){
           ZeroMemory(&si,sizeof(SCROLLINFO));
           si.cbSize = sizeof(SCROLLINFO);
           si.fMask = SIF_ALL;
           si.nMax = rc2.right-1;
           si.nPage = i;
           if(flagScale < 0)
               xScroll = 0;
           si.nPos = xScroll;
           ShowScrollBar(hwnd,SB_CTL,TRUE);
           SetScrollInfo(hwnd,SB_CTL,&si,TRUE);
       }
       rc2.right = i;
   }
   else if(flagScale != 0)
       ShowScrollBar(hwnd,SB_CTL,FALSE);
   rc2.left = rc.left + ((rc.right-rc.left - rc2.right) >> 1);
   rc2.top = rc.top + ((rc.bottom - rc.top - rc2.bottom) >> 1);
   if(flagScale != 0 || eraseBk != 0)
       EraseBackGround(hdc);
   ::SelectObject(hdcBitmap,hBitmap);
   ::StretchDIBits(hdcBitmap,0,0,rc1.right,rc1.bottom,0,0,sz.cx,sz.cy,pBuffer,&BmpInfo,DIB_RGB_COLORS,SRCCOPY);
   BitBlt(hdc,rc2.left,rc2.top,rc2.right,rc2.bottom,hdcBitmap,xScroll,yScroll,SRCCOPY);
Ex_DrawSpriteBitmap:
   if(flag != 0)
       ReleaseDC(hwndDlgSprite,hdc);
   return res;
}
//---------------------------------------------------------------------------
static void UpdateSprite(HDC hdc)
{
   u8 flag;
   RECT rc,rc1,rc2;
   HFONT hFont;
   int i;
   char s[30];
   LPSPRITE pSprite;
   HBRUSH hBrush;

   if(hdc == NULL){
       hdc = GetDC(hwndDlgSprite);
       flag = 1;
   }
   else
       flag = 0;
   hFont = (HFONT)SendMessage(GetDlgItem(hwndDlgSprite,IDC_EDIT1),WM_GETFONT,0,0);
   SelectObject(hdc,hFont);
   lstrcpy(s," ");
   for(i = IDC_SPRITE_ENA;i <= IDC_SPRITE_ATTR2;i++){
       hBrush = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
       FillRect(hdc,&rcControl[i - IDC_SPRITE_ENA],hBrush);
       ::DeleteObject(hBrush);
   }
   CopyRect(&rc,&rcSprite);
   EraseBackGround(hdc);
   if(IndexSprite != 0 && hBit != NULL){
       xScroll = yScroll = 0;
       pSprite = GetSprite((u8)(IndexSprite-1));
       ::SetRect(&rc1,0,0,pSprite->SizeX,pSprite->SizeY);
       SendDlgItemMessage(hwndDlgSprite,IDC_TRACK1,TBM_SETPOS,TRUE,0);
       iScale = 10;
       rc2.left = rc.left + ((rc.right-rc.left - rc2.right) >> 1);
       rc2.top = rc.top + ((rc.bottom - rc.top - rc2.bottom) >> 1);
       if(hdcBitmap != NULL){
           if(hBitmap != NULL)
               DeleteObject(hBitmap);
           hBitmap = NULL;
           ::DeleteDC(hdcBitmap);
           hdcBitmap = NULL;
       }
       if((hdcBitmap = CreateCompatibleDC(hdc)) != NULL){
           i = SendDlgItemMessage(hwndDlgSprite,IDC_TRACK1,TBM_GETRANGEMAX,0,0);
           if((hBitmap = CreateCompatibleBitmap(hdc,pSprite->SizeX * i,pSprite->SizeY * i)) != NULL)
               DrawSpriteBitmap(hdc,FALSE);
       }
       SetBkMode(hdc,TRANSPARENT);
       DrawText(hdc,"Abiltato",-1,&rcControl[0],DT_SINGLELINE|DT_LEFT);
       wsprintf(s,"%1d",pSprite->Priority);
       DrawText(hdc,s,-1,&rcControl[1],DT_SINGLELINE|DT_LEFT);
       wsprintf(s,"%03d,%03d",pSprite->xPos,pSprite->yPos);
       DrawText(hdc,s,-1,&rcControl[2],DT_SINGLELINE|DT_LEFT);
       wsprintf(s,"%s",pSprite->bPalette ? "16" : "256");
       DrawText(hdc,s,-1,&rcControl[3],DT_SINGLELINE|DT_LEFT);
       wsprintf(s,"%02d",pSprite->iPalette >> 4);
       DrawText(hdc,s,-1,&rcControl[4],DT_SINGLELINE|DT_LEFT);
       wsprintf(s,"%02d,%02d",pSprite->SizeX,pSprite->SizeY);
       DrawText(hdc,s,-1,&rcControl[5],DT_SINGLELINE|DT_LEFT);
       wsprintf(s,"%s",pSprite->bRot ? "Si" : "No");
       DrawText(hdc,s,-1,&rcControl[6],DT_SINGLELINE|DT_LEFT);
       wsprintf(s,"0x%04X",pSprite->a0);
       DrawText(hdc,s,-1,&rcControl[7],DT_SINGLELINE|DT_LEFT);
       wsprintf(s,"0x%04X",pSprite->a1);
       DrawText(hdc,s,-1,&rcControl[8],DT_SINGLELINE|DT_LEFT);
       wsprintf(s,"0x%04X",pSprite->a2);
       DrawText(hdc,s,-1,&rcControl[9],DT_SINGLELINE|DT_LEFT);
   }
   if(flag != 0)
       ReleaseDC(hwndDlgSprite,hdc);
}
//---------------------------------------------------------------------------
static BOOL CALLBACK DlgProcSprite(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   WORD wID;
   u32 i;
   RECT rc;
   PAINTSTRUCT ps;
   BOOL res;
   POINT pt;
   HWND hwnd;

   res = FALSE;
   switch(uMsg){
       case WM_INITDIALOG:
           ::SetFocus(GetDlgItem(hwndDlg,IDC_EDIT1));
           ::SendDlgItemMessage(hwndDlg,IDC_SPIN1,UDM_SETRANGE,0,MAKELPARAM(128,0));
           ::SendDlgItemMessage(hwndDlg,IDC_SPIN1,UDM_SETPOS,0,0);
           ::GetWindowRect(GetDlgItem(hwndDlg,IDC_SPRITE),&rc);
           ::DestroyWindow(GetDlgItem(hwndDlg,IDC_SPRITE));
           pt.x = rc.left;
           pt.y = rc.top;
           ::ScreenToClient(hwndDlg,&pt);
           ::SetRect(&rcSprite,pt.x,pt.y,pt.x+128,pt.y+128);
           pt.x = ::GetSystemMetrics(SM_CXEDGE);
           pt.y = ::GetSystemMetrics(SM_CYEDGE);
           rcSprite.right += pt.x;
           rcSprite.bottom +=pt.y;
           rcSprite.left -= pt.x;
           rcSprite.top -= pt.y;
           szBorder.cx = pt.x;
           szBorder.cy = pt.y;
           hwnd = GetDlgItem(hwndDlg,IDC_VSBSPR);
           GetWindowRect(hwnd,&rc);
           i = rc.right - rc.left;
           SetWindowPos(hwnd,NULL,rcSprite.right + 2,rcSprite.top - pt.y,i,
               rcSprite.bottom - rcSprite.top + pt.y,SWP_NOREPOSITION|SWP_NOSENDCHANGING);

           hwnd = GetDlgItem(hwndDlg,IDC_HSBSPR);
           GetWindowRect(hwnd,&rc);
           SetWindowPos(hwnd,NULL,rcSprite.left - pt.x,rcSprite.bottom + 2,rcSprite.right - rcSprite.left + pt.x,
               i,SWP_NOREPOSITION|SWP_NOSENDCHANGING);

           i += rcSprite.bottom + 3;
           hwnd = GetDlgItem(hwndDlg,IDC_TRACK1);
           GetWindowRect(hwnd,&rc);
           pt.x = rcSprite.left + (((rcSprite.right - rcSprite.left) - (rc.right - rc.left)) >> 1);
           SetWindowPos(hwnd,NULL,pt.x,i,0,0,SWP_NOREPOSITION|SWP_NOSIZE|SWP_NOSENDCHANGING);
           SendMessage(hwnd,TBM_SETRANGE,0,MAKELPARAM(0,10));
           SendMessage(hwnd,TBM_SETPOS,TRUE,0);
           IndexSprite = 0;
           for(i = IDC_SPRITE_ENA;i <= IDC_SPRITE_ATTR2;i++){
               GetWindowRect(GetDlgItem(hwndDlg,i),&rc);
               pt.x = rc.right;
               pt.y = rc.top;
               ScreenToClient(hwndDlg,&pt);
               pt.x += 5;
               rcControl[i - IDC_SPRITE_ENA].left = pt.x;
               rcControl[i - IDC_SPRITE_ENA].top = pt.y;
               rcControl[i - IDC_SPRITE_ENA].right = pt.x + 40;
               rcControl[i - IDC_SPRITE_ENA].bottom = pt.y + rc.bottom - rc.top;
           }
       break;
       case WM_VSCROLL:
           wID = (WORD)GetDlgCtrlID((HWND)lParam);
           switch(wID){
               case IDC_SPIN1:
                   switch(LOWORD(wParam)){
                       case SB_THUMBPOSITION:
                           IndexSprite = HIWORD(wParam);
                           if(IndexSprite != 0){
                               DrawSprite((u8)(IndexSprite - 1));
                               UpdateSprite(NULL);
                           }
                       break;
                   }
               break;
               case IDC_VSBSPR:
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
                       DrawSpriteBitmap(NULL,FALSE);
                   }
               break;
           }
       break;
       case WM_HSCROLL:
           wID = (WORD)GetDlgCtrlID((HWND)lParam);
           switch(wID){
               case IDC_TRACK1:
                   DrawSpriteBitmap(NULL,FALSE);
               break;
               case IDC_HSBSPR:
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
                       DrawSpriteBitmap(NULL,FALSE);
                   }
               break;
           }
       break;
       case WM_COMMAND:
           wID = LOWORD(wParam);
           switch(HIWORD(wParam)){
               case EN_CHANGE:
                   if(wID == IDC_EDIT1){
                   }
               break;
               case BN_CLICKED:
                   switch(wID){
                   	case IDOK:
                       	DestroyWindow(hwndDlg);
                       break;
                       case IDCANCEL:
                       	UpdateSprite(NULL);
                       break;
                       case IDC_BUTTON2:
                       break;
                   }
               break;
           }
       break;
       case WM_PAINT:
           ::BeginPaint(hwndDlg,&ps);
           DrawSpriteBitmap(ps.hdc,TRUE);
           ::EndPaint(hwndDlg,&ps);
           res = TRUE;
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
#endif
