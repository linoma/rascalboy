//---------------------------------------------------------------------------
#include <windows.h>
#include <commctrl.h>

#pragma hdrstop

#include "winedit.h"
#include "fstream.h"
#include "resource.h"
//---------------------------------------------------------------------------
extern HINSTANCE hInstance;
//---------------------------------------------------------------------------
#ifdef _DEBPRO
//---------------------------------------------------------------------------
WinEdit::WinEdit()
{
   pBuffer = NULL;
   hwnd = NULL;
   hFile = NULL;
   dwYscroll = dwXscroll = dwLine = dwSize = 0;
   xStart = 0;
   hFont = NULL;
   textDC = NULL;
   textBM = NULL;
   sztextBM.cx = sztextBM.cy = 0;
}
//---------------------------------------------------------------------------
WinEdit::~WinEdit()
{
   if(textDC != NULL)
       ::DeleteDC(textDC);
   if(textBM != NULL)
       ::DeleteObject(textBM);
   if(hFile != NULL)
       delete hFile;
   if(hwnd != NULL)
       ::DestroyWindow(hwnd);
   if(pBuffer != NULL)
       ::GlobalFree((HGLOBAL)pBuffer);
}
//---------------------------------------------------------------------------
BOOL WinEdit::GetTextLine(DWORD dwStart,LPDWORD dwLength,LPBYTE ccr,LPDWORD pdwTabs)
{
   char *p;
   DWORD dw,dw1,dwTabs;

   *dwLength = 0;
   *ccr = 0;
   if(dwStart > dwSize)
       return FALSE;
   p = (char *)GlobalLock(pBuffer);
   if(p == NULL)
       return FALSE;
   p += dwStart;
   dw = dw1 = dwTabs = 0;
   while(dwStart < dwSize){
       dw++;
       if(*p == 0x9)
           dwTabs++;
       if(*p == 0xD)
           dw1++;
       if(*p == 0xA){
           dw1++;
           break;
       }
       p++;
   }
   if(pdwTabs)
       *pdwTabs = dwTabs;
   *dwLength = dw;
   *ccr = dw1;
   GlobalUnlock(pBuffer);
   return TRUE;
}
//---------------------------------------------------------------------------
void WinEdit::OnVScroll(int nScrollCode,int nPos,HWND hwndScrollBar)
{
   SCROLLINFO si;
   int i,iscroll;
   RECT rc;

   ZeroMemory(&si,sizeof(SCROLLINFO));
   si.cbSize = sizeof(SCROLLINFO);
   si.fMask = SIF_ALL;
   if(!GetScrollInfo(hwnd,SB_VERT,&si))
       return;
   switch(nScrollCode){
       case SB_LINEUP:
           i = dwYscroll - 1;
       break;
       case SB_LINEDOWN:
           i = dwYscroll + 1;
       break;
       case SB_THUMBPOSITION:
       case SB_THUMBTRACK:
           i = si.nTrackPos;
       break;
       default:
           i = dwYscroll;
       break;
   }
   if(i == dwYscroll || i < 0 || i > si.nMax)
       return;
   iscroll = dwYscroll - i;
   dwYscroll = i;
   SetScrollPos(hwnd,SB_VERT,dwYscroll,TRUE);
   GetClientRect(hwnd,&rc);
   OffsetRect(&rc,xStart,0);
   ScrollWindowEx(hwnd,0,iscroll * szFont.cy,NULL,NULL,NULL,&rc,0);
   rc.top = (rc.top / szFont.cy) * szFont.cy;
   InvalidateRect(hwnd,&rc,TRUE);
   ::UpdateWindow(hwnd);
}
//---------------------------------------------------------------------------
void WinEdit::OnSize(int nWidth,int nHeight)
{
   CreateTextBackground(nWidth);
   UpdateScrollBar(nWidth,nHeight);
}
//---------------------------------------------------------------------------
void WinEdit::OnHScroll(int nScrollCode,int nPos,HWND hwndScrollBar)
{
   SCROLLINFO si;
   int i,iscroll;
   RECT rc;

   ZeroMemory(&si,sizeof(SCROLLINFO));
   si.cbSize = sizeof(SCROLLINFO);
   si.fMask = SIF_ALL;
   if(!GetScrollInfo(hwnd,SB_HORZ,&si))
       return;
   switch(nScrollCode){
       case SB_LINEUP:
           i = dwXscroll - 1;
       break;
       case SB_LINEDOWN:
           i = dwXscroll + 1;
       break;
       case SB_THUMBPOSITION:
       case SB_THUMBTRACK:
           i = si.nTrackPos;
       break;
       default:
           i = dwXscroll;
       break;
   }
   if(i == dwXscroll || i < 0 || i > (si.nMax - si.nPage + 1))
       return;
   iscroll = dwXscroll - i;
   dwXscroll = i;
   SetScrollPos(hwnd,SB_HORZ,dwXscroll,TRUE);
   ::GetClientRect(hwnd,&rc);
   rc.left += xStart;
   ScrollWindowEx(hwnd,iscroll * szFont.cx,0,NULL,&rc,NULL,&rc,0);
   rc.left -= xStart;
   rc.right += xStart;
   InvalidateRect(hwnd,&rc,TRUE);
   ::UpdateWindow(hwnd);
}
//---------------------------------------------------------------------------
void WinEdit::OnEraseBackground(HDC hDC)
{
}
//---------------------------------------------------------------------------
void WinEdit::OnDraw(HDC hDC,LPRECT rcPaint)
{
   RECT rc,rc1,rc2,rc3;
   char *p,*p1,cr,line[400];
   int len,i,i1,ipos;
   DWORD dwPos,dwLength,dwTabs,dwPosKeyword,dwLenKeyword;
   COLORREF color;
   BOOL bKeyword;

   if(textDC == NULL || textBM == NULL)
       return;
   SelectObject(textDC,textBM);
   if(hFont != NULL){
       SelectObject(textDC,hFont);
       SelectObject(hDC,hFont);
   }
   p = (char *)GlobalLock(pBuffer);
   if(p == NULL)
       return;
   CopyRect(&rc,rcPaint);
   GetClientRect(hwnd,&rc1);
   if(!EqualRect(&rc,&rc1)){
       if(rc.left < xStart)
           rc.left = 0;
       else
           rc.left -= xStart;
   }
   SetTextAlign(textDC,TA_LEFT|TA_NOUPDATECP);
   dwPos = 0;
   CopyRect(&rc1,&rc);
   rc1.bottom = rc.top + szFont.cy;
   //Ci posizionamo all riga di vscroll
   for(i=dwYscroll + (rc.top / szFont.cy);i>0;i--){
       if(!GetTextLine(dwPos,&dwLength,&cr,&dwTabs))
           break;
       dwPos += dwLength;
       p += dwLength;
   }
   while(rc1.top < rc.bottom){
       if(!GetTextLine(dwPos,&dwLength,&cr,&dwTabs))
           break;
       len = dwLength - cr;
       SetRect(&rc3,0,0,sztextBM.cx,sztextBM.cy);
       FillRect(textDC,&rc3,GetSysColorBrush(COLOR_WINDOW));
       if(len > 0){
           for(p1 = p,i=i1=0;i<len;i++,p1++){
               if(*p1 == 0x9){
                   *((LPDWORD)&line[i1]) = 0x20202020;
                   i1 += 4;
               }
               else
                   line[i1++] = *p1;
           }
           line[i1] = 0;
           p1 = line;
           if(pSyntaxList)
               bKeyword = pSyntaxList->FindKeyword(p1,i1,&dwPosKeyword,&dwLenKeyword,&color);
           else
               bKeyword = FALSE;
           if(!bKeyword){
               SetTextColor(textDC,RGB(0,0,0));
               DrawText(textDC,p1,-1,&rc3,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
           }
           else{
               ipos = 0;
               do{
                   if(dwPosKeyword){
                       SetTextColor(textDC,RGB(0,0,0));
                       SetRect(&rc2,rc3.left,0,rc3.right,szFont.cy);
                       DrawText(textDC,p1,dwPosKeyword,&rc2,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_CALCRECT);
                       DrawText(textDC,p1,dwPosKeyword,&rc3,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
                       rc3.left = rc2.right;
                       ipos += dwPosKeyword;
                       p1 += dwPosKeyword;
                   }
                   if(dwLenKeyword){
                       SetTextColor(textDC,color);
                       SetRect(&rc2,rc3.left,0,rc3.right,szFont.cy);
                       DrawText(textDC,p1,dwLenKeyword,&rc2,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_CALCRECT);
                       DrawText(textDC,p1,dwLenKeyword,&rc3,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
                       rc3.left = rc2.right;
                       ipos += dwLenKeyword;
                       p1 += dwLenKeyword;
                   }
                   if(ipos < i1 && !(bKeyword = pSyntaxList->FindKeyword(p1,i1 - ipos,&dwPosKeyword,&dwLenKeyword,&color))){
                       SetTextColor(textDC,RGB(0,0,0));
                       DrawText(textDC,p1,i1 - ipos,&rc3,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
                       ipos = i1;
                   }
               }while(ipos < i1);
           }
       }
       ::BitBlt(hDC,rc1.left+xStart,rc1.top,rc1.right-rc1.left,rc1.bottom - rc1.top,textDC,rc1.left + dwXscroll * szFont.cx,0,SRCCOPY);
       DrawGutter(hDC,rc1.top,rc1.bottom - rc1.top);
       p += dwLength;
       dwPos += dwLength;

       rc1.top += szFont.cy;
       rc1.bottom += szFont.cy;
   }
   DrawGutter(hDC,rc1.top,rc.bottom - rc1.bottom);
   GlobalUnlock(pBuffer);
}
//---------------------------------------------------------------------------
void WinEdit::DrawGutter(HDC hdc,int y,int height)
{
   RECT rc;

   if(height < 1 || xStart < 2)
       return;
   SetRect(&rc,0,y,xStart,y+height);
   ::FillRect(hdc,&rc,GetSysColorBrush(COLOR_BTNFACE));
   ::SetRect(&rc,xStart - 2,y,xStart - 1,y+height);
   ::FillRect(hdc,&rc,GetSysColorBrush(COLOR_WINDOW));
   ::SetRect(&rc,xStart-1,y,xStart,y+height);
   ::FillRect(hdc,&rc,GetSysColorBrush(COLOR_BTNSHADOW));
}
//---------------------------------------------------------------------------
BOOL WinEdit::CreateWnd(char *lpszName,HWND parent)
{
   RECT rc;
   char *p,*p1;
   DWORD dw;
   int i,i1;

   ::GetClientRect(parent,&rc);
   TabCtrl_AdjustRect(parent,FALSE,&rc);

   hwnd = ::CreateWindowEx(WS_EX_CLIENTEDGE,"RASCALBOYSOURCEWINDOW",NULL,WS_CHILD|WS_HSCROLL|WS_VSCROLL|WS_CLIPCHILDREN,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,
       parent,(HMENU)NULL,hInstance,NULL);
   if(hwnd == NULL)
       return FALSE;
   hFile = new LFile(lpszName);
   if(hFile == NULL || !hFile->Open())
      return FALSE;
   dwSize = hFile->Size(&dw);
   if(dwSize == 0)
       return FALSE;
   pBuffer = GlobalAlloc(GMEM_MOVEABLE,dwSize + 1);
   if(pBuffer == NULL)
       return FALSE;
   p = (char *)GlobalLock(pBuffer);
   if(p == NULL)
       return FALSE;
   dw = hFile->Read(p,dwSize);
   if(dw != dwSize){
       GlobalUnlock(pBuffer);
       return FALSE;
   }
   p1 = p;
   dwLine = dwRowSize = 0;
   for(i=i1=0;i<dwSize;i++){
       if(*p1 == 0x9)
           i1 += 4;
       else
           i1++;
       if(*p1++ == 0xA){
           dwLine++;
           if(i1 > dwRowSize)
               dwRowSize = (DWORD)i1;
           i1 = 0;
       }
   }
   dwLine++;
   if(i1 > dwRowSize)
       dwRowSize = (DWORD)i1;
   GlobalUnlock(pBuffer);
   GetWindowRect(hwnd,&rc);
   UpdateScrollBar(rc.right,rc.bottom);
   SetWindowLong(hwnd,GWL_USERDATA,(LONG)this);
   ShowWindow(hwnd,SW_SHOW);
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL WinEdit::CreateTextBackground(int width)
{
   HDC hdc;
   BOOL res;

   if(sztextBM.cx >= width)
       return TRUE;
   res = FALSE;
   hdc = GetDC(hwnd);
   if(hdc == NULL)
       goto ex_CreateTextBackground;
   if(textDC != NULL)
       ::DeleteDC(textDC);
   textDC = NULL;
   if(textBM != NULL)
       ::DeleteObject(textBM);
   textBM = NULL;
   textDC = CreateCompatibleDC(hdc);
   if(textDC == NULL)
       goto ex_CreateTextBackground;
   textBM = CreateCompatibleBitmap(hdc,sztextBM.cx = width,sztextBM.cy = szFont.cy);
   if(textBM != NULL)
       res = TRUE;
ex_CreateTextBackground:
   ::ReleaseDC(hwnd,hdc);
   return res;
}
//---------------------------------------------------------------------------
void WinEdit::FintText(const char *lpszText)
{
}
//---------------------------------------------------------------------------
void WinEdit::SetFont(HFONT f)
{
   HDC hdc;
   RECT rc;

   hdc = GetDC(hwnd);
   if(hdc == NULL)
       return;
   hFont = f;
   if(hFont != NULL)
       SelectObject(hdc,hFont);
   GetTextExtentPoint32(hdc,"X",1,&szFont);
   xStart = szFont.cx << 2;
   ::ReleaseDC(hwnd,hdc);
   CreateTextBackground(dwRowSize * szFont.cx);
   GetClientRect(hwnd,&rc);
   UpdateScrollBar(rc.right,rc.bottom);
}
//---------------------------------------------------------------------------
BOOL WinEdit::UpdateScrollBar(int width,int height)
{
   SCROLLINFO si;
   DWORD dw;

   dw = dwLine * szFont.cy;
   if(dw < height)
       ShowScrollBar(hwnd,SB_VERT,FALSE);
   else{
       ZeroMemory(&si,sizeof(SCROLLINFO));
       si.cbSize = sizeof(SCROLLINFO);
       si.fMask = SIF_ALL;
       si.nPage = height / szFont.cy;
       si.nMax = dwLine - 1;
       SetScrollInfo(hwnd,SB_VERT,&si,TRUE);
       ShowScrollBar(hwnd,SB_VERT,TRUE);
   }
   dw = dwRowSize * szFont.cx;
   if(dw < width)
       ShowScrollBar(hwnd,SB_HORZ,FALSE);
   else{
       ZeroMemory(&si,sizeof(SCROLLINFO));
       si.cbSize = sizeof(SCROLLINFO);
       si.fMask = SIF_ALL;
       si.nPage = width / szFont.cx;
       si.nMax = (dwRowSize - 1);
       SetScrollInfo(hwnd,SB_HORZ,&si,TRUE);
       ShowScrollBar(hwnd,SB_HORZ,TRUE);
   }
   dwXscroll = dwYscroll = 0;
   return TRUE;
}
//---------------------------------------------------------------------------
LWinEditList::LWinEditList(HWND parent) : LList()
{
   HDC hDC;
   LOGFONT lf;

   hwndParent = parent;
   hDC = CreateCompatibleDC(NULL);
   ZeroMemory(&lf,sizeof(LOGFONT));
   lf.lfHeight = -(10 * GetDeviceCaps(hDC, LOGPIXELSY) / 72);
   lstrcpy(lf.lfFaceName,"Courier New");
   lf.lfWeight = FW_NORMAL;
   lf.lfCharSet = DEFAULT_CHARSET;
   lf.lfOutPrecision = OUT_TT_PRECIS;
   lf.lfQuality = PROOF_QUALITY|FF_DECORATIVE;
   hFont = CreateFontIndirect(&lf);
   ::DeleteDC(hDC);
}
//---------------------------------------------------------------------------
LWinEditList::~LWinEditList()
{
   if(hFont)
       ::DeleteObject(hFont);
   Clear();
}
//---------------------------------------------------------------------------
void LWinEditList::RepositionWindows()
{
   DWORD dw;
   WinEdit *pWin;
   RECT rc;

   GetClientRect(hwndParent,&rc);
   TabCtrl_AdjustRect(hwndParent,FALSE,&rc);

   pWin = (WinEdit *)GetFirstItem(&dw);
   while(pWin != NULL){
       ::MoveWindow(pWin->Handle(),rc.left,rc.top,rc.right-rc.left,rc.bottom - rc.top,TRUE);
       pWin = (WinEdit *)GetNextItem(&dw);
   }
}
//---------------------------------------------------------------------------
BOOL LWinEditList::Add(char *lpszName)
{
   class WinEdit *pWin;

   pWin = new WinEdit();
   if(pWin == NULL)
       return FALSE;
   if(!pWin->CreateWnd(lpszName,hwndParent)){
       delete pWin;
       return FALSE;
   }
   if(!LList::Add((LPVOID)pWin)){
       delete pWin;
       return FALSE;
   }
   pWin->SetSyntaxList(pSyntaxList);
   if(hFont != NULL)
       pWin->SetFont(hFont);
   return TRUE;
}
//---------------------------------------------------------------------------
void LWinEditList::DeleteElem(LPVOID ele)
{
   if(ele != NULL)
       delete (WinEdit *)ele;
}
//---------------------------------------------------------------------------
LSyntaxList::LSyntaxList() : LList()
{
   Add("#define",2);
   Add("#include",2);
   Add("#if",2);
   Add("#endif",2);
   Add("#elif",2);
   Add("#else",2);

   Add("return",1);
   Add("if",1);
   Add("for",1);
   Add("while",1);
   Add("do",1);
   Add("break",1);
   Add("switch",1);
   Add("class",1);
   Add("protected",1);
   Add("public",1);
   Add("private",1);
   Add("struct",1);
   Add("typedef",1);
   Add("goto",1);
   Add("case",1);
   Add("continue",1);
   Add("new",1);
   Add("delete",1);
   Add("void",1);
   Add("char",1);
   Add("short",1);
   Add("long",1);
   Add("int",1);
   Add("signed",1);
   Add("unsigned",1);
   Add("double",1);
   Add("float",1);
   Add("static",1);
   Add("extern",1);

   color[0] = RGB(0,0,128);
   color[1] = RGB(0,0,255);
}
//---------------------------------------------------------------------------
LSyntaxList::~LSyntaxList()
{
   Clear();
}
//---------------------------------------------------------------------------
BOOL LSyntaxList::FindKeyword(char *p,DWORD dwLen,LPDWORD pPosKeyword,LPDWORD pLenKeyword,COLORREF *pColorKeyword)
{
   int i,iLen;
   DWORD dwPos;
   LPKEYWORDSTYLE ele;
   char delimiter[]={'(',';',' '};
   BOOL bFlag,res;

   if(pPosKeyword)                                           
       *pPosKeyword = 0;
   if(pLenKeyword)                                              
       *pLenKeyword = 0;

   if(nCount < 1)
       return FALSE;
   bFlag = FALSE;
   for(i=0;i<dwLen;i++){
       ele = (LPKEYWORDSTYLE)GetFirstItem(&dwPos);
       do{
           if(*p == ele->Name[1]){
               if(!strncmpi(ele->Name.c_str(),p,(iLen = ele->Name.Length()))){
                   if(strchr(delimiter,*(p + iLen)))
                       bFlag = TRUE;
               }
           }
       }while(!bFlag && (ele = (LPKEYWORDSTYLE)GetNextItem(&dwPos)) != NULL);
       if(bFlag)
           break;
       p++;
   }
   if(bFlag){
       if(pPosKeyword)
           *pPosKeyword = i;
       if(pLenKeyword)
           *pLenKeyword = ele->Name.Length();
       if(pColorKeyword)
           *pColorKeyword = color[ele->tipo-1];
   }
   return bFlag;
}
//---------------------------------------------------------------------------
void LSyntaxList::DeleteElem(LPVOID ele)
{
   if(ele != NULL)
       delete (LPKEYWORDSTYLE)ele;
}
//---------------------------------------------------------------------------
BOOL LSyntaxList::Add(char *lpszName,BYTE tipo)
{
   LPKEYWORDSTYLE ele;

   ele = new KEYWORDSTYLE;
   ele->Name = lpszName;
   ele->tipo = tipo;
   return LList::Add((LPVOID)ele);
}
#endif
