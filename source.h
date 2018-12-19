#include "debug.h"
#include "resource.h"
#include "list.h"
#include "lstring.h"
#include "fstream.h"

//---------------------------------------------------------------------------
#ifndef sourceH
#define sourceH
//---------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

void InitDebugSourceWindow();
void DestroyDebugSourceWindow();
u8 CreateDebugSourceWindow(HWND win);

#ifdef __cplusplus
}
#endif

#endif
 