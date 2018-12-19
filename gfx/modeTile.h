#include <windows.h>
#include "cpu.h"
#include "gbaemu.h"
#include "memory.h"
#include "lcd.h"
#include "graphics.h"
#include "sprite.h"

//---------------------------------------------------------------------------
#ifndef modeTileH
#define modeTileH
//---------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

void FASTCALL PostDrawMode1(LPLAYER layer);
void FASTCALL InitLayerMode1(LPLAYER layer);
u8 FASTCALL drawPixelMode1(LPLAYER layer);
void FASTCALL InitLayerMode0(LPLAYER layer);
u8 FASTCALL drawPixelMode0(LPLAYER layer);
void drawLineModeTile(void);
void drawLineModeTileWindow(void);

#ifdef __cplusplus
}
#endif

#endif
 