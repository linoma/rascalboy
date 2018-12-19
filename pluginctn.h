#include "plugin.h"
#include "audioplug.h"
#include "videoplug.h"
#include "backplug.h"
#include "sio.h"
#include "cpu.h"
#include "service.h"

#ifndef pluginctnH
#define pluginctnH

#define PIL_SIO        1
#define PIL_AUDIO      2
#define PIL_VIDEO      4
#define PIL_BACKUP		8

//---------------------------------------------------------------------------
class PlugInContainer
{
public:
   PlugInContainer();
   ~PlugInContainer();
   BOOL Init();
   void Destroy();
   void NotifyState(DWORD item,DWORD dwState,DWORD dwStateMask = 0);
   inline SioPlugList* GetSioPlugInList(){return pSioPlugList;};
   inline VideoPlugList* GetVideoPlugInList(){return pVideoPlugList;};
   inline AudioPlugList* GetAudioPlugInList(){return pAudioPlugList;};
   inline LServiceInterface* GetServiceInterface(){return pServiceInterface;};
   inline LBackupPlugList* GetBackupPlugInList(){return pBackupPlugList;};
   int GetLastError(){return iLastError;};
   BOOL EnableCallback(PlugIn *p);
   void IncCycles(int cycles);
protected:
   SioPlugList *pSioPlugList;
   VideoPlugList *pVideoPlugList;
   AudioPlugList *pAudioPlugList;
   LBackupPlugList *pBackupPlugList;
   LServiceInterface *pServiceInterface;
   int iLastError;
};

extern PlugInContainer *pPlugInContainer;

#endif
