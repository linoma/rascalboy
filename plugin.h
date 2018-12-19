#include <windows.h>
#include "lstring.h"
#include "list.h"
#include "pluginmain.h"

//---------------------------------------------------------------------------
#ifndef pluginH
#define pluginH
//-----------------------------------------------------------------------
class PlugIn
{
public:
   PlugIn();
   ~PlugIn();           
   BOOL Load();
   void Unload(BOOL bForced = FALSE);
   virtual inline BOOL Reset(){if(pResetFunc != NULL) return pResetFunc();return TRUE;};
   virtual BOOL InitPlugInInfo(LPPLUGININFO p,DWORD dwState = 0,DWORD dwStateMask = 0);
   virtual int GetInfo(LPPLUGININFO p,BOOL bReloadAll = FALSE);
   BOOL SetInfo(LPPLUGININFO p);
   BOOL NotifyState(DWORD dwState,DWORD dwStateMask = 0);
   virtual BOOL Destroy(){if(pDeleteFunc != NULL) return pDeleteFunc();return TRUE;};
   virtual BOOL SetProperty(LPSETPROPPLUGIN p){if(pSetPropertyFunc) return pSetPropertyFunc(p); return FALSE;};
   BOOL IsEnable(){return bEnable;};
   BOOL IsLicensed(){return isLicensed;};
   FARPROC GetRunFunc(){return pRunFunc;};
   void SetRunFunc(FARPROC p){pRunFunc = p;};
   virtual int GetFileInfo(char *pOut);
   virtual BOOL Enable(BOOL bFlag);
   void SetLibraryPath(char *path){pathLibrary = path;};
   void GetGuid(LPGUID p){if(p != NULL) CopyMemory(p,&guid,sizeof(GUID));};
   void SetGuid(LPGUID p){if(p != NULL) CopyMemory(&guid,p,sizeof(GUID));};
   BOOL IsExclusive(){return bExclusive;};
   void SetIndex(char val){index = val;};
   WORD GetIndex(){return index;};
   void SetUsed(){if(isLoad == 1) isLoad = 2;};
   WORD GetType(){return (WORD)(wType & 0xF);};
   DWORD IsAttribute(DWORD value){return (dwFlags & value);};
   void OrAttribute(DWORD value){dwFlags |= value;};
protected:
   char bEnable,isLoad,bDynamic,bExclusive,isReload,isLicensed,index;
   WORD wID,wType;
   DWORD dwFlags;
   GUID guid;
   LString pathLibrary,name;
   FARPROC pRunFunc;
   LPPLUGINRESET pResetFunc;
   LPPLUGINDELETE pDeleteFunc;
   LPPLUGINGETINFO pGetInfoFunc;
   LPPLUGINSETINFO pSetInfoFunc;
   LPPLUGINSETPROPERTY pSetPropertyFunc;
   HINSTANCE hLib;
};
//---------------------------------------------------------------------------
typedef struct{
   DWORD cyclesElapsed;
   DWORD cyclesCounter;
   PlugIn *pPlugIn;
} PLUGINCALLBACK,*LPPLUGINCALLBACK;
//---------------------------------------------------------------------------
class PlugInCallBackList : public LList
{
public:
   PlugInCallBackList();
   ~PlugInCallBackList();
   BOOL Add(LPPLUGINCALLBACK p);
   void IncCycles(int cycles);
protected:
};
//---------------------------------------------------------------------------
class PlugInList : public LList
{
public:
   PlugInList(char *p);
   ~PlugInList();
   virtual PlugIn *GetItemFromGUID(LPGUID p);
   virtual int Reset();
   virtual BOOL PreLoad(WORD *wID);
   virtual PlugIn *BuildPlugIn(char *path);
   int Load(WORD wID,WORD wType);
   BOOL OnInitMenu(HMENU menu);
   int NotifyState(DWORD dwState,DWORD dwStateMask = 0);
   virtual BOOL Add(PlugIn *ele);
   BOOL GetSelectedGUID(LPGUID p);
   void LoadSetConfig();
   void SaveSetConfig();
   virtual BOOL Enable(LPGUID p,BOOL bEnable);
   BOOL OnEnablePlug(WORD wID);
   int GetMaxElem(){return maxElem;};
//   virtual BOOL AddCallback(LPPLUGINCALLBACK p);
//   void IncCycles(int cycles);
protected:
   virtual void DeleteElem(LPVOID ele);
//   PlugInCallBackList *pListCallBack;
   LString name;
   int maxElem;
};
#endif
