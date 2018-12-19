#include "debug.h"
#include "resource.h"
#include "graphics.h"
//---------------------------------------------------------------------------
#ifndef debbkgH
#define debbkgH
//---------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

void InitDebugBkgWindow();
void DestroyDebugBkgWindow();
u8 CreateDebugBkgWindow(HWND win);

#ifdef __cplusplus
}
#endif

#endif
