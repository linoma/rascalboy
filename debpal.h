#include <windows.h>
#include "cpu.h"

//---------------------------------------------------------------------------
#ifndef debpalH
#define debpalH
//---------------------------------------------------------------------------
typedef struct {
   HWND hwnd;
   HDC hdc;
   int colorPicker;
   RECT rcColorDraw,rcColorPicker;
   u16 *lpPalette;
} DEBPALETTE,*LPDEBPALETTE;

#ifdef __cplusplus
extern "C" {
#endif

u8 CreateDebugBgPalWindow(HWND win);
u8 CreateDebugDMAWindow(HWND win);
void InitDebugDMAWindow();
void InitDebugBgPalWindow();
void DestroyDebugDMAWindow();
void DestroyDebugBgPalWindow();
void UpdateDebugDMAWindow();
void UpdateDebugBgPalWindow();
void DrawColorPicker(LPDEBPALETTE pPalette);
void DrawPalette(LPDEBPALETTE pPalette);

#ifdef __cplusplus
}
#endif

#endif
