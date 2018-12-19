#include <windows.h>
#include "cpu.h"
#include "gbaemu.h"
#include "memory.h"
#include "lcd.h"

#ifndef graphicsH
#define graphicsH

#ifdef __cplusplus
extern "C" {
#endif

void drawBackground();
void render_modeb_frame(void);
void render_mode0_frame();
void render_mode1_frame();
void render_mode2_frame();
void render_mode3_frame();
void render_mode4_frame();
void render_mode5_frame();
void ResetLayer(LPLAYER layer);
void FillPalette(u16 index);
u16 FASTCALL bgrtorgb(u16 color);
void drawBackground();
void InitRunFrame();
int GetPointsWindow(LPPOINTWINDOW pt);
void DrawLineObjWindow(u8 xStart,u8 xEnd);

void rgbAlphaLayerLine(u8 xStart,u8 xEnd);
void rgbFadeLine(u8 xStart,u8 xEnd);
void rgbNormalLine(u8 xStart,u8 xEnd);

#ifdef __cplusplus
}
#endif

#endif



