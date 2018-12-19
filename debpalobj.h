#include <windows.h>
#include "cpu.h"

//---------------------------------------------------------------------------
#ifndef debpalobjH
#define debpalobjH
//---------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

u8 CreateDebugObjPalWindow(HWND win);
void InitDebugObjPalWindow();
void DestroyDebugObjPalWindow();
void UpdateDebugObjPalWindow();

#ifdef __cplusplus
}
#endif

#endif
