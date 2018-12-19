#include <windows.h>
#include "gbaemu.h"

#ifndef spriteH
#define spriteH


struct _sprite;
typedef void (*OAMFUNC)(u16);
typedef void (*OAMDRAWPIXEL)(struct _sprite *,u8 xStart,u8 xEnd);
//---------------------------------------------------------------------------
typedef struct _rotMatrix
{
   s16 PA,PB,PC,PD;
   u8 bWrite;
} SROTMATRIX,*PSROTMATRIX;
//---------------------------------------------------------------------------
typedef struct _sprite
{
   OAMDRAWPIXEL pFunc;
   u16 a0,a1,a2;
   u8 Enable;
   u8 Priority,Index;
   u8 hFlip,vFlip;
   u8 bDouble,bMosaic;
   s16 xPos,yPos;
   u8 SizeX,SizeY;
   u8 bPalette;
   u16 iPalette;
   u8 bRot;
   u8 VisibleMode;
   u8 value;
   PSROTMATRIX rotMatrix;
   u32 tileBaseNumber;
   u32 tileNumberYIncr;
} SPRITE,*PSPRITE,*LPSPRITE;
//---------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

void WriteSprite(u16 adress,u8 mode);
void drawPixelSprite(LPSPRITE pSprite,u8 xStart,u8 xEnd);
void drawPixelSpriteRot(LPSPRITE pSprite,u8 xStart,u8 xEnd);
u8 InitSprite();
void DestroySprite();
void ResetSprite(void);
void DrawSprite(u8 xStart,u8 xEnd);
u8 DrawDebugSprite(u8 nSprite,u16 *pBuffer,int inc);
LPSPRITE GetSprite(u8 nSprite);
short GetPaletteColor(u8 nSprite,LPPALETTEENTRY lppe);
u16 GetPixelSprite(LPSPRITE pSprite,u8 xPos,u16 iColor);
u16 GetPixelSpriteAlpha(LPSPRITE pSprite,u8 xPos,u16 iColor);
u16 GetPixelSpriteBrightness(LPSPRITE pSprite,u8 xPos,u16 iColor);
void RemapAllSprite();

#ifdef __cplusplus
}
#endif


#endif