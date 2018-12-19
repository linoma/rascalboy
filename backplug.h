#include <windows.h>
#include "plugin.h"
#include "fstream.h"

//---------------------------------------------------------------------------
#ifndef backplugH
#define backplugH
//---------------------------------------------------------------------------
class LBackupPlugin : public PlugIn
{
public:
	LBackupPlugin();
   ~LBackupPlugin();
   BOOL Run(LStream *stream);
   BOOL Enable(BOOL bFlag);
};
//---------------------------------------------------------------------------
class LBackupPlugList : public PlugInList
{
public:
   LBackupPlugList();
  	int Run(LStream *stream,int type);
};
#endif
