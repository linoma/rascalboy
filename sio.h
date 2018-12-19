#include <windows.h>
#include "list.h"
#include "plugin.h"

//---------------------------------------------------------------------------
#ifndef sioH
#define sioH
//---------------------------------------------------------------------------
#define SIO_NOTIFY     WM_USER + 3333
//---------------------------------------------------------------------------
class SioPlug : public PlugIn
{
public:
   SioPlug();
   virtual BOOL Run(LPSERIALIO p);
   int GetInfo(LPPLUGININFO p,BOOL bReloadAll = FALSE);
   BOOL IsSync(){return (isASync == 0);};
   BYTE GetASyncMethod(){return isASync;};
   BOOL InitPlugInInfo(LPPLUGININFO p,DWORD dwState = 0,DWORD dwStateMask = 0);
   inline BOOL IsRunnable(){
       if(!bEnable || pRunFunc == NULL)
           return FALSE;
       return TRUE;
   };
protected:
   BYTE isASync;
};
//---------------------------------------------------------------------------
class SioPlugList : public PlugInList
{                                                
public:
   SioPlugList();
   ~SioPlugList();
   int Run(DWORD mask);
   SioPlug *BuildPlugIn(char *path);
   BOOL Enable(LPGUID p,BOOL bEnable);
   void DestroyASyncMethod();
   BOOL EnableASyncMethod(int mode);
   HANDLE GetEventHandle(int e = 0){if(!e) return hEvent; return hEventRun;};
   HANDLE GetThreadHandle(int t = 0){if(!t) return hThreadRun; return hThread;};
   inline void GetCallback(LPDWORD p1){
       if(p1 != NULL){
           p1[0] = dwCallback[0];
           p1[1] = dwCallback[1];
       }
   }
protected:
   DWORD dwCallback[2],dwThreadId[2];
   HANDLE hEvent,hThread,hEventRun,hThreadRun;
};
//---------------------------------------------------------------------------
BOOL InitSio();
void DestroySio();
int ResetSio();
BOOL OnInitSioPlugMenu(HMENU menu);
BOOL EnableSioPlug(WORD wID);

#endif
 