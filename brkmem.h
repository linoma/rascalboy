#include <windows.h>
#include "cpu.h"

//---------------------------------------------------------------------------
#ifndef brkmemH
#define brkmemH
//---------------------------------------------------------------------------
typedef struct{
   char bWrite;
   char bRead;
   char bModify;
   char bBreak;
   char mAccess;
}MEMORYBRK,*PMEMORYBRK;

#ifdef __cplusplus
extern "C" {
#endif

int ShowDialogBRKMEM(HWND parent);

#ifdef __cplusplus
}
#endif

#endif
