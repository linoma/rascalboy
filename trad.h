#include <windows.h>
#include "resource.h"
#include "lstring.h"
#include "list.h"
#include "gba.h"

#ifndef __tradH__
#define __tradH__

#define MAKEERRORGBA(section,code) (((BYTE)(0x80|(section & 0x7F))) << 8) | ((BYTE)(abs(code) - 1))
//---------------------------------------------------------------------------
typedef struct{
   LString Path;
   LString Name;
   WORD wLanguageID;
   WORD wCodePage;
} TRAD,*LPTRAD;
//---------------------------------------------------------------------------
class LTradList : public LList
{
public:
   LTradList();
   ~LTradList();
   WORD GetLanguageID(){return iLanguageID;};
   WORD GetCodePage(){return wCodePage;};
   BOOL SetLanguageID(WORD id,BOOL bReadraw = TRUE);
   BOOL FindLanguage(WORD wID,LPDWORD item);
protected:
   void DeleteElem(LPVOID ele);
   WORD iLanguageID,wCodePage;
};
//---------------------------------------------------------------------------
extern LTradList *pLanguageList;

#ifdef __cplusplus
extern "C" {
#endif

LPTRAD CreateNewTrad(char *path);
BOOL InitTranslation();
void DestroyTranslation();
BOOL Translation(HWND win,UINT idMenu,UINT iID);
int TranslateMenu(HMENU hMenu,WORD wID);
LString TranslateGetMessage(UINT wID);
LString GetStringFromMenu(HWND hwnd,UINT wID,BOOL bPosition=FALSE);
BOOL TranslateLoadString(LString *string,UINT wID);
HINSTANCE FindResourceInternal(WORD wID,LPCTSTR lpType);

#ifdef __cplusplus
}
#endif

#endif
