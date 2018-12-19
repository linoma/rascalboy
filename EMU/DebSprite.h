#include "debug.h"
#include "resource.h"

#ifndef _DEBSPRITEH_
#define _DEBSPRITEH_

#ifdef __cplusplus
extern "C" {
#endif

void InitDebugSpriteWindow();
u8 CreateDebugSpriteWindow(HWND win);
void DestroyDebugSpriteWindow();
BOOL SaveBitmap(HDC hdc,HBITMAP bit,char *fileName);

#ifdef __cplusplus
}
#endif

#endif

