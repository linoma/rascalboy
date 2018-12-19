#include <windows.h>
#include "memory.h"
#include "list.h"
#include "fstream.h"

//---------------------------------------------------------------------------
#ifndef cheatH
#define cheatH
//---------------------------------------------------------------------------
#define MAX_CHEATNAME  30

#define CLID_READ      2
#define CLID_WRITE     1
#define CLID_LISTVIEW  3
#define CLID_STATIC    4

//---------------------------------------------------------------------------
enum cheatType{CodeBreak,GameShark};

struct _cheatHeader;

typedef void (*LPCHEATEVALUATEFUNC)(struct _cheatHeader *,u32,u8);

typedef struct _cheatHeader{
   char descr[MAX_CHEATNAME + 1];
   char codeString[20];
   u8 Enable,code,skip;
   cheatType type;
   u32 adr,value,adrEnd;
   LPCHEATEVALUATEFUNC pEvaluateFunc;
   LList *pList;
   struct _cheatHeader *pNext;
} CHEATHEADER,*LPCHEATHEADER;

typedef CHEATHEADER CODEBREAK;
typedef LPCHEATHEADER LPCODEBREAK;

typedef CHEATHEADER GAMESHARK;
typedef LPCHEATHEADER LPGAMESHARK;
//---------------------------------------------------------------------------
typedef BOOL (*LPENUMCHEAT)(LPCHEATHEADER,LPARAM);
//---------------------------------------------------------------------------
class LCheatList : public LList
{
public:
   LCheatList(u32 i,BOOL b=FALSE);
   ~LCheatList();
   void __fastcall EvaluateCheat(u32 address,u8 accessMode);
   BOOL Add(LPCHEATHEADER ele);
   void Clear();
   BOOL Save(LStream *pFile);
   BOOL Load(LStream *pFile);
   BOOL EnumCheat(LPENUMCHEAT pEnumCheatProc,LPARAM lParam);
protected:
   void DeleteElem(LPVOID ele);

   u32 maxAdr,minAdr,id;
   BOOL isClone;
};
//---------------------------------------------------------------------------
extern LCheatList *pCheatList;

#ifdef __cplusplus
extern "C" {
#endif

BOOL InitCheatSystem();
void DestroyCheatSystem();
u8 FASTCALL read_byteCheat(u32 address);
u16 FASTCALL read_hwordCheat(u32 address);
u32 FASTCALL read_wordCheat(u32 address);
void FASTCALL write_byteCheat(u32 address,u32 data);
void FASTCALL write_hwordCheat(u32 address,u16 data);
void FASTCALL write_wordCheat(u32 address,u32 data);
DWORD AddGameShark(const char *lpCode,const char *lpDescr);
void ResetCheatSystem();
DWORD AddCodeBreak(const char *lpCode,const char *lpDescr);
BOOL ShowCheatDialog(HWND hwndParent);
void ApplyAllCheatList();
BOOL LoadCheatList(HWND hwndParent);
BOOL SaveCheatList(HWND hwndParent);
void LoadCheatListFromRom(BOOL bAuto);
int GetCheatSystemError();
void ApplyStaticCheatList();

#ifdef __cplusplus
}
#endif

#endif
