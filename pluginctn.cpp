//---------------------------------------------------------------------------
#include <windows.h>
#pragma hdrstop

#include "pluginctn.h"
#include "resource.h"
//---------------------------------------------------------------------------
PlugInContainer *pPlugInContainer;
//---------------------------------------------------------------------------
PlugInContainer::PlugInContainer()
{
   iLastError = -1;
   pSioPlugList = NULL;
   pVideoPlugList = NULL;
   pAudioPlugList = NULL;
   pServiceInterface = NULL;
   pBackupPlugList = NULL;
}
//---------------------------------------------------------------------------
PlugInContainer::~PlugInContainer()
{
   Destroy();
}
//---------------------------------------------------------------------------
void PlugInContainer::NotifyState(DWORD item,DWORD dwState,DWORD dwStateMask)
{
   if(pSioPlugList != NULL && (item & PIL_SIO))
       pSioPlugList->NotifyState(dwState,dwStateMask);
   if(pVideoPlugList != NULL && (item & PIL_VIDEO))
       pVideoPlugList->NotifyState(dwState,dwStateMask);
   if(pAudioPlugList != NULL && (item & PIL_AUDIO))
       pAudioPlugList->NotifyState(dwState,dwStateMask);
   if(pBackupPlugList != NULL && (item & PIL_BACKUP))
   	pBackupPlugList->NotifyState(dwState,dwStateMask);
}
//---------------------------------------------------------------------------
void PlugInContainer::IncCycles(int cycles)
{
/*   if(pSioPlugList != NULL)
       pSioPlugList->IncCycles(cycles);
   if(pVideoPlugList != NULL)
       pVideoPlugList->IncCycles(cycles);
   if(pAudioPlugList != NULL)
       pAudioPlugList->IncCycles(cycles);*/
}
//---------------------------------------------------------------------------
BOOL PlugInContainer::Init()
{
   iLastError = -6;
   if((pAudioPlugList = new AudioPlugList()) == NULL)
       return FALSE;
   pAudioPlugList->Load(ID_SOUND_PLUG_START,PIT_AUDIO);
   iLastError = -2;
   if((pVideoPlugList = new VideoPlugList()) == NULL)
       return FALSE;
   pVideoPlugList->Load(ID_VIDEO_PLUG_START,PIT_VIDEO);
   iLastError = -4;
   if((pSioPlugList = new SioPlugList()) == NULL)
       return FALSE;
   pSioPlugList->Load(ID_SIO_PLUG_START,PIT_SIO);
	iLastError = -5;
   if((pServiceInterface = new LServiceInterface()) == NULL)
   	return FALSE;
	iLastError = -7;
   if((pBackupPlugList = new LBackupPlugList()) == NULL)
   	return FALSE;
   pBackupPlugList->Load(ID_BKP_PLUG_START,PIT_BACKUP);
   iLastError = 0;
   return TRUE;
}
//---------------------------------------------------------------------------
void PlugInContainer::Destroy()
{
	if(pServiceInterface != NULL)
		delete pServiceInterface;
 	pServiceInterface = NULL;
   if(pSioPlugList != NULL)
       delete pSioPlugList;
   if(pVideoPlugList != NULL)
       delete pVideoPlugList;
   if(pAudioPlugList != NULL)
       delete pAudioPlugList;
   pSioPlugList = NULL;
   pVideoPlugList = NULL;
   pAudioPlugList = NULL;
}
