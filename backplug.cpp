//---------------------------------------------------------------------------
#include <windows.h>
#pragma hdrstop

#include "backplug.h"
#include "gbaemu.h"
//---------------------------------------------------------------------------
extern BOOL WriteFileSRAM(LStream *pFile);
extern BOOL WriteFileEEPROM(LStream *pFile);
extern BOOL ReadFileSRAM(LStream *pFile);
extern DWORD ReadFileEEPROM(LStream *pFile);
//---------------------------------------------------------------------------
LBackupPlugin::LBackupPlugin() : PlugIn()
{
}
//---------------------------------------------------------------------------
LBackupPlugin::~LBackupPlugin()
{
}
//---------------------------------------------------------------------------
BOOL LBackupPlugin::Run(LStream *stream)
{
   if(!bEnable)
       return TRUE;
   if(pRunFunc == NULL)
       return FALSE;
   return ((LPBACKUPPLUGRUN)pRunFunc)((WORD)index,stream);
}
//---------------------------------------------------------------------------
BOOL LBackupPlugin::Enable(BOOL bFlag)
{
   LMemoryFile *pFile;

	if(!PlugIn::Enable(bFlag))
   	return FALSE;
   if(bFlag && (dwFlags & PIT_ENABLERUN)){
   	if((pFile = new LMemoryFile()) != NULL){
           if(bin.bLoadRam & 1)
				WriteFileSRAM(pFile);
           else if(bin.bLoadRam & 6)
           	WriteFileEEPROM(pFile);
       }
   	if(Run(pFile) && pFile != NULL){
           if(bin.bLoadRam & 1)
				ReadFileSRAM(pFile);
           else if(bin.bLoadRam & 6)
           	ReadFileEEPROM(pFile);
       }
   	if(pFile != NULL)
       	delete pFile;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
LBackupPlugList::LBackupPlugList() : PlugInList("BackupPlugIn")
{
}
//---------------------------------------------------------------------------
int LBackupPlugList::Run(LStream *stream,int type)
{
   int i;
   elem_list *p;

   if(nCount == 0)
       return -1;
   i = 0;
   p = First;
   do{
       if(((PlugIn *)p->Ele)->IsEnable() &&
       	!((PlugIn *)p->Ele)->IsAttribute(PIT_ENABLERUN) &&
           !((PlugIn *)p->Ele)->IsAttribute(type)){
           if(!((LBackupPlugin *)p->Ele)->Run(stream))
               break;
           else
               i++;
       }
   }while((p = p->Next) != NULL);
   return i;
}
