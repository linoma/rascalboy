//---------------------------------------------------------------------------
#include <windows.h>
#pragma hdrstop

#include "service.h"
#include "guidService.h"
#include "fstream.h"
#include "zipfile.h"
#include "savestate.h"
//---------------------------------------------------------------------------
LServiceInterface::LServiceInterface()
{
}
//---------------------------------------------------------------------------
LServiceInterface::~LServiceInterface()
{
}
//---------------------------------------------------------------------------
HRESULT LServiceInterface::CreateService(REFIID iid,LPVOID *pUnk)
{
   HRESULT hr;

	if(pUnk == NULL)
   	return E_FAIL;
	hr = E_FAIL;
   *pUnk = NULL;
   if(iid == IID_IMemoryFile){
   	if((*pUnk = (LPVOID)new LMemoryFile()) != NULL)
       	hr = S_OK;
   }
   else if(iid == IID_IZipFile){
   	if((*pUnk = (LPVOID)new LZipFile()) != NULL)
       	hr = S_OK;
   }
	else if(iid == IID_ISaveState){
   	if((*pUnk = (LPVOID)new LSaveState()) != NULL)
       	hr = S_OK;
   }
	return hr;
}
