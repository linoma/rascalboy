#include "cpu.h"
#include "resource.h"
#include "fstream.h"
#include "zipfile.h"

//---------------------------------------------------------------------------
#ifndef savestateH
#define savestateH
typedef void WINAPI (*LPSAVESTATECALLBACK)(int);
//---------------------------------------------------------------------------
struct LISaveState
{
	virtual void Release() PURE;
   virtual LPVOID Save(LPSAVESTATECALLBACK pCallBack,int iLevel = 2,int index = -1) PURE;
   virtual BOOL Load(LPSAVESTATECALLBACK pCallBack,UINT index) PURE;
   virtual void set_IndexMax(UINT u) PURE;
   virtual void set_CurrentIndex(UINT u) PURE;
	virtual UINT get_CurrentIndex() PURE;
   virtual void Reset() PURE;
   virtual UINT Count() PURE;
};
//---------------------------------------------------------------------------
class LSaveState : public LISaveState
{
public:
	LSaveState();
   ~LSaveState();
   void Release(){delete this;};
   LPVOID Save(LPSAVESTATECALLBACK pCallBack = NULL,int iLevel = 2,int index = -1);
   BOOL Load(LPSAVESTATECALLBACK pCallBack,UINT index);
   void set_IndexMax(UINT u){IndexMax = u;};
   UINT get_IndexMax(){return IndexMax;};
   void set_CurrentIndex(UINT u){Index = u;};
	UINT get_CurrentIndex(){return Index;};
   void Reset();
   UINT Count(){return zipFile.Count();};
   BOOL set_File(const char *fileName);
protected:
//---------------------------------------------------------------------------
	BOOL Init(BOOL bOpenAlways = FALSE);
//---------------------------------------------------------------------------
	LZipFile zipFile;
   LMemoryFile *pMemoryFile;
   UINT Index,IndexMax;
   BOOL bUseFile;
   char cFileName[MAX_PATH];
};
//---------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

void ResetSaveState();
BOOL OnInitMenuSaveState(HMENU menu,int sub);
BOOL SaveState(int index);
BOOL LoadState(int index);
void ReadSaveStateList();
BOOL LoadMostRecentSaveState();
BOOL RecordGameState();
BOOL PlayGameState();

#ifdef __cplusplus
}
#endif

#endif
 