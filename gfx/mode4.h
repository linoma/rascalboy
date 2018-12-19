#include <windows.h>
#include "cpu.h"
#include "gbaemu.h"
#include "memory.h"
#include "lcd.h"
#include "graphics.h"
#include "sprite.h"

#ifndef mode4H
#define mode4H

#ifdef __cplusplus
extern "C" {
#endif
//---------------------------------------------------------------------------

u8 FASTCALL drawPixelMode4(LPROTMATRIX rot);
void drawLineMode4(LPROTMATRIX rot);
void drawLineMode4Window(LPROTMATRIX rot);

#ifdef __cplusplus
}
#endif

#endif
