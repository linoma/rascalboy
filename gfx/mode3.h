#include <windows.h>
#include "cpu.h"
#include "gbaemu.h"
#include "memory.h"
#include "lcd.h"
#include "graphics.h"
#include "sprite.h"

//---------------------------------------------------------------------------
#ifndef mode3H
#define mode3H
//---------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

u16 FASTCALL drawPixelMode3(LPROTMATRIX rot);
void drawLineMode3(LPROTMATRIX rot);
void drawLineMode3Window(LPROTMATRIX rot);

#ifdef __cplusplus
}
#endif

#endif
