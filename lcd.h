#include <windows.h>
#include <f:\borland\include\ddraw.h>
#include "cpu.h"
#include "memory.h"
#include "gbaemu.h"

//---------------------------------------------------------------------------
#ifndef lcdH
#define lcdH
//---------------------------------------------------------------------------

#define BM_DIRECTDRAW              0
#define BM_GDI                     1
#define BM_DIRECTDRAWFULLSCREEN    2

#define MAKEZPIXEL(prtSrc,indexSrc,ptrTar,indexTar) (u32)((u8)prtSrc | ((u8)indexSrc << 8) | ((u8)ptrTar << 16) | ((u8)indexTar << 24))
#define MAKEZPIXEL16(ptrSrc,indexSrc) (u16)(prtSrc | (indexSrc << 8))
#define NULLZPIXEL (u32)0xFFFFFFFF
#define NULLPIZEL NULLZPIXEL
#define GETZPRTSRC(value) (u8)value
#define GETZPRTTAR(value) (u8)(value >> 16)
#define GETZIDXSRC(value) (u8)(value >> 8)
#define GETZIDXTAR(value) (u8)(value >> 24)

//---------------------------------------------------------------------------
struct _layer;
struct _sprite;
typedef u8 FASTCALL (*DRAWPIXEL)(struct _layer *l);
typedef void FASTCALL (*INITDRAWLINE)(struct _layer *l);
typedef void FASTCALL (*POSTDRAWPIXEL)(struct _layer *l);
typedef void (*RENDERMODE)();
typedef u16 (*GETSPRITEPIXEL)(struct _sprite *,u8,u16);
typedef void (*SWAPBUFFER)(u8,u8);

//---------------------------------------------------------------------------
typedef struct _layer
{
   DRAWPIXEL drawPixel;
   INITDRAWLINE initDrawLine;
   POSTDRAWPIXEL postDraw;
   u8 Enable,Priority,Visible;
   u8 Index;
   u8 Tipo;
   s16 rotMatrix[4];
   s32 bg[2];
   s32 bgrot[2];
   u8 bMosaic;
   u8 bPalette;
   u8 *ScreenBaseBlock,*CharBaseBlock,*scrbb;
   u16 Width,Height;
   u8 log2;
   u16 tileY,offsetToAdd1;
   u8 vFlip,bWrap;
   s32 CurrentX,CurrentY,CurrentScrollX,CurrentScrollY;
} LAYER,*PLAYER,FAR *LPLAYER;
//---------------------------------------------------------------------------
typedef struct {
   LPLAYER EnableBg[4];
   u8 EnableObj;
   u8 EnableBlend;
} WINCONTROL,*PWINCONTROL,*LPWINCONTROL;
//---------------------------------------------------------------------------
typedef struct {
   LPLAYER EnableBg[4];
   u8 EnableObj;
   u8 EnableBlend;
   u8 Enable;
} WINCONTROLEX,*PWINCONTROLEX,*LPWINCONTROLEX;
//---------------------------------------------------------------------------
typedef struct _tagPOINTWINDOW{
   u8 width;
   LPWINCONTROL winc;
} POINTWINDOW,*LPPOINTWINDOW;
//---------------------------------------------------------------------------
typedef struct{
   u8 Enable;
   u8 left,top;
   u8 right,bottom;
   WINCONTROL Control;
} WINGBA,*PWINGBA,*LPWINGBA;
//---------------------------------------------------------------------------
typedef struct _lcd
{
   LPLAYER Source[8],Target[8];
   LAYER layers[4];
   u8 VisibleLayers[9];
   WINGBA win[2];
   WINCONTROL winOut;
   WINCONTROLEX winObj;
   RECT rc;
   u8 Enable,SpriteEnable,WinEnable,DrawMode,BlitMode,x;
   u16 Width,Height;
   u8 iBlend,FrameBuffer;
   u8 eva,evb,evy;
   u8 CountSource,CountTarget;
   u8 xMosaic,yMosaic,sprxMosaic,spryMosaic;
   s32 Luminosita,LumStart;
   u8 nFilter;
   u8 bDoubleSize,blankBit;
   u8 bFullScreen,iFps;
   RENDERMODE Render;
   GETSPRITEPIXEL GetPixelSprite;
   SWAPBUFFER swapBuffer[4];
   u16 *pBuffer;
   u32 *pZBuffer,*pCurrentZBuffer;
   u8 *SourceBuffer,*TargetBuffer,*SpriteBuffer,*WinObjSprite;
   u16 *pCurSB,*pCurOB;
   u16 *pFilterBuffer;
   u8 *tabColor,*tabColor1,*peva,*pevb,*pevy,*pevy1;
   HBITMAP hBitmap;
   LPDIRECTDRAW7 lpDD;
   LPDIRECTDRAWSURFACE7 lpDDSPrimary,lpDDBack,lpDDSurface;
   HDC hdcWin;
} LCD,*PLCD,*LPLCD;
//---------------------------------------------------------------------------
typedef struct
{
   u8 Enable,bMosaic,bRot,x;
   u32 iLayer;
   s16 rotMatrix[4];
   s32 bg[2];
   u8 *pBuffer;
} ROTMATRIX,*PROTMATRIX,*LPROTMATRIX;
//---------------------------------------------------------------------------
extern LAYER *layers[4];
extern LCD lcd;
extern u16 *screen;
extern RENDERMODE render_mode[8];
extern u16 *translated_palette;
extern u16 *paletteSprite;


#ifdef __cplusplus
extern "C" {
#endif

int DrawLcdBitmap(HDC hdc);
BOOL EnableVideoPlug(WORD wID);
BOOL OnInitVideoPlugMenu(HMENU menu);
int GetLcdSubCodeError();
void RecalcLayout();
void SetLcd(u16 adress,u8 accessMode);
u8 InitLcd();
void DestroyLcd();
u8 ResetLcd(void);
void BlitFrame(HDC hDC);
void BlackFrame(HDC hDC);
void SetRectLcd(int x,int y);
void GetBackgroundRect(u8 num);
void FillListBackground();
u16 *GetSurfacePointer(void);
u8 EnableDirectDraw(u8 bFullScreen);
void FillBaseLayer(LPLAYER layer,u16 Control);
void CopyBit(HDC hDC,LPRECT rcSrc,LPRECT rcDst);
void ReleaseDirectDraw();
void EnterWindowMode();
void ExitWindowMode();
void GetBrightnessIndex();
void GetAlphaIndex();

#ifdef __cplusplus
}
#endif

#endif
