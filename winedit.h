#include <windows.h>
#include "list.h"
#include "fstream.h"
//---------------------------------------------------------------------------
#if !defined(wineditH) && defined(_DEBPRO)
#define wineditH
//---------------------------------------------------------------------------
typedef struct {
   LString Name;
   BYTE tipo;
} KEYWORDSTYLE,*PKEYWORDSTYLE,*LPKEYWORDSTYLE;
//---------------------------------------------------------------------------
class LSyntaxList : public LList
{
public:
   LSyntaxList();
   ~LSyntaxList();
   BOOL Add(char *lpszName,BYTE tipo);
   BOOL FindKeyword(char *p,DWORD dwLen,LPDWORD pPosKeyword,LPDWORD pLenKeyword,COLORREF *pColorKeyword);
protected:
   void DeleteElem(LPVOID ele);
   COLORREF color[10];
};
//---------------------------------------------------------------------------
class WinEdit
{
public:
   WinEdit();
   ~WinEdit();
   BOOL CreateWnd(char *lpszName,HWND parent);
   HWND Handle(){return hwnd;};
   void SetSyntaxList(LSyntaxList *p){pSyntaxList = p;};
   void OnDraw(HDC hDC,LPRECT rcPaint);
   void SetFont(HFONT f);
   BOOL GetTextLine(DWORD dwStart,LPDWORD dwLength,LPBYTE ccr,LPDWORD pdwTabs);
   void OnVScroll(int nScrollCode,int nPos,HWND hwndScrollBar);
   void OnHScroll(int nScrollCode,int nPos,HWND hwndScrollBar);
   void OnSize(int nWidth,int nHeight);
   void OnEraseBackground(HDC hDC);
   void FintText(const char *lpszText);
protected:
   BOOL UpdateScrollBar(int width,int height);
   void DrawGutter(HDC hdc,int y,int height);
   BOOL CreateTextBackground(int width);
//---------------------------------------------------------------------------
   HWND hwnd;
   HGLOBAL pBuffer;
   LFile *hFile;
   LSyntaxList *pSyntaxList;
   DWORD dwLine,dwSize,dwRowSize,dwYscroll,dwXscroll,dwTabs;
   int xStart;
   HFONT hFont;
   SIZE szFont,sztextBM;
   HDC textDC;
   HBITMAP textBM;
};
//---------------------------------------------------------------------------
class LWinEditList : public LList
{
public:
   LWinEditList(HWND parent);
   ~LWinEditList();
   BOOL Add(char *lpszName);
   void RepositionWindows();
   void SetSyntaxList(LSyntaxList *p){pSyntaxList = p;};
protected:
   void DeleteElem(LPVOID ele);
   HWND hwndParent;
   LSyntaxList *pSyntaxList;
   HFONT hFont;
};
//---------------------------------------------------------------------------
#endif
