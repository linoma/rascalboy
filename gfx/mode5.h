#include <windows.h>
#include "cpu.h"
#include "gbaemu.h"
#include "memory.h"
#include "lcd.h"
#include "graphics.h"

#ifndef mode5H
#define mode5H

#ifdef __cplusplus
extern "C" {
#endif
//---------------------------------------------------------------------------

u16 drawPixelMode5(LPROTMATRIX rot);
void drawLineMode5(LPROTMATRIX rot);

#ifdef __cplusplus
}
#endif

#endif
