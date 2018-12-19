//---------------------------------------------------------------------------
#include <windows.h>
#pragma hdrstop

#include "gba.h"
#include "lcd.h"
#include "graphics.h"
#include "modeTile.h"
#include "pluginctn.h"
#include "trad.h"

//---------------------------------------------------------------------------
LCD lcd;
RENDERMODE render_mode[0x8];
u16 *paletteSprite;
u16 *translated_palette;
u16 *screen;
static BITMAPINFO BmpInfo;
static int iSubCodeError;
//---------------------------------------------------------------------------
static u8 CreateDirectDrawFullScreenSurface();
//---------------------------------------------------------------------------
int DrawLcdBitmap(HDC hdc)
{
   return StretchDIBits(hdc,0,0,240,160,0,0,240,160,screen,&BmpInfo,DIB_RGB_COLORS,SRCCOPY);
}
//---------------------------------------------------------------------------
u8 InitLcd()
{
   int i;

   translated_palette = NULL;
   ZeroMemory(&lcd,sizeof(LCD));
   render_mode[0] = render_mode0_frame;
	render_mode[1] = render_mode1_frame;
	render_mode[2] = render_mode2_frame;
	render_mode[3] = render_mode3_frame;
	render_mode[4] = render_mode4_frame;
	render_mode[5] = render_mode5_frame;
	render_mode[6] = render_mode4_frame;
	render_mode[7] = render_mode4_frame;

   for(i=0;i<9;i++)
       lcd.VisibleLayers[i] = 1;
   iSubCodeError = -8;
   if((lcd.SourceBuffer = (u8 *)GlobalAlloc(GMEM_FIXED,84000)) == NULL)
       return 0;
   lcd.TargetBuffer    = (u8 *)((u8 *)lcd.SourceBuffer + 480);
   lcd.pZBuffer        = (u32 *)((u8 *)lcd.TargetBuffer + 480);
   lcd.SpriteBuffer    = (u8 *)((u8 *)lcd.pZBuffer + 960);
   lcd.WinObjSprite    = (u8 *)((u8 *)lcd.SpriteBuffer + 480);
   lcd.tabColor        = (u8 *)((u8 *)lcd.WinObjSprite + 480);
   lcd.tabColor1       = (u8 *)((u8 *)lcd.tabColor + 600);
   translated_palette  = (u16 *)((u8 *)lcd.tabColor1 + 600);
   lcd.pFilterBuffer   = (u16 *)((u8 *)translated_palette + 1100);

   iSubCodeError = -9;
   paletteSprite = translated_palette + 256;
   SetRectLcd(240,160);
   ZeroMemory(&BmpInfo,sizeof(BITMAPINFO));
	BmpInfo.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	BmpInfo.bmiHeader.biBitCount    = 16;
	BmpInfo.bmiHeader.biWidth       = 240;
	BmpInfo.bmiHeader.biHeight      = -160;
	BmpInfo.bmiHeader.biPlanes      = 1;
	BmpInfo.bmiHeader.biCompression = BI_RGB;
	if((lcd.hBitmap = CreateDIBSection(NULL,&BmpInfo,DIB_RGB_COLORS,(VOID**)&screen,NULL,0)) == NULL)
		return 0;
   iSubCodeError = 0;
   lcd.BlitMode = 0xFF;
   return 1;
}
//---------------------------------------------------------------------------
int GetLcdSubCodeError()
{
   return iSubCodeError;
}
//---------------------------------------------------------------------------
void ReleaseDirectDraw()
{
   lcd.bFullScreen = 0;
   if(lcd.lpDD != NULL)
       lcd.lpDD->RestoreDisplayMode();
   RELEASE(lcd.lpDDSurface)
   RELEASE(lcd.lpDDBack)
   RELEASE(lcd.lpDDSPrimary)
   RELEASE(lcd.lpDD)
}
//---------------------------------------------------------------------------
u8 EnableClipper()
{
   LPDIRECTDRAWCLIPPER lpDDClipper;

   if(lcd.lpDDSPrimary->GetClipper(&lpDDClipper) != DD_OK){
       iSubCodeError = -4;
       if(lcd.lpDD->CreateClipper(0,&lpDDClipper, NULL) != DD_OK)
           return 0;
       iSubCodeError = -5;
       if(lpDDClipper->SetHWnd(0,hWin) != DD_OK)
           return 0;
       iSubCodeError = -6;
       if(lcd.lpDDSPrimary->SetClipper(lpDDClipper) != DD_OK)
           return 0;
   }
   RELEASE(lpDDClipper);
   return 1;
}
//---------------------------------------------------------------------------
u8 EnableDirectDraw(u8 bFullScreen)
{
   DDSURFACEDESC2 ddsd;
   HRESULT res;

   ReleaseDirectDraw();
   iSubCodeError = -1;
   res = ::CoCreateInstance(CLSID_DirectDraw7,NULL,CLSCTX_INPROC_SERVER,IID_IDirectDraw7,(LPVOID *)&lcd.lpDD);
   if(res != DD_OK)
		return 0;
   res = lcd.lpDD->Initialize(NULL);
   if(res != DD_OK)
		return 0;
   iSubCodeError = -2;
   if(!bFullScreen){
       if(lcd.lpDD->SetCooperativeLevel(hWin,DDSCL_NORMAL) != DD_OK)
           return 0;
       ZeroMemory(&ddsd,sizeof(ddsd));
       ddsd.dwSize = sizeof(ddsd);
       ddsd.dwFlags = DDSD_CAPS;
       ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
       iSubCodeError = -3;
	    if((lcd.lpDD->CreateSurface(&ddsd,&lcd.lpDDSPrimary,NULL)) != DD_OK)
           return 0;
       ZeroMemory(&ddsd,sizeof(DDSURFACEDESC));
       ddsd.dwSize = sizeof(ddsd);
       ddsd.dwFlags = DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS;
       ddsd.dwWidth = 240;
       ddsd.dwHeight = 160;
       ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
       iSubCodeError = -7;
	    if((lcd.lpDD->CreateSurface(&ddsd,&lcd.lpDDBack,NULL)) != DD_OK)
           return 0;
       if(!EnableClipper())
           return 0;
   }
   else{
       if(lcd.lpDD->SetCooperativeLevel(hWin,DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN) != DD_OK)
           return 0;
       switch(bFullScreen){
           case 1:
               res = lcd.lpDD->SetDisplayMode(640,480,16,0,0);
           break;
       }
       if(res != DD_OK){
           ReleaseDirectDraw();
           return 0;
       }
       if(!CreateDirectDrawFullScreenSurface())
           return 0;
       if(!EnableClipper()){
           ReleaseDirectDraw();
           return 0;
       }
       lcd.bFullScreen = 1;
   }
   iSubCodeError = 0;
   return 1;
}
//---------------------------------------------------------------------------
void DestroyLcd()
{
   if(lcd.SourceBuffer != NULL)
       GlobalFree(lcd.SourceBuffer);
   lcd.SourceBuffer = lcd.TargetBuffer = lcd.SpriteBuffer = NULL;
   lcd.pZBuffer = NULL;
   lcd.pFilterBuffer = translated_palette = NULL;
   if(lcd.hdcWin != NULL && hWin != NULL)
       ReleaseDC(hWin,lcd.hdcWin);
	if(lcd.hBitmap != NULL)
       ::DeleteObject(lcd.hBitmap);
   lcd.hBitmap = NULL;
   ReleaseDirectDraw();
}
//---------------------------------------------------------------------------
void FillListBackground()
{
   u8 n,n1,i1,i;
   LPLAYER layer;

   n1 = n = 0;
   switch(lcd.iBlend){
       case 0:
       case 2:
       case 3:
           for(i=0;i< 4;i++){
               for(i1=0;i1<4;i1++){
                   layer = &lcd.layers[i1];
                   if(layer->Enable != 0 && layer->Priority == i && layer->drawPixel != NULL && layer->Visible){
                       layer->Tipo = (u8)((BLENDCNT & (1 << i1)) ? 'S' : 32);
                       lcd.Source[n++] = layer;
                   }
               }
           }
       break;
       case 1:
           for(i=0;i< 4;i++){
               for(i1=0;i1<4;i1++){
                   layer = &lcd.layers[i1];
                   if(layer->Enable != 0 && layer->Priority == i && layer->Visible && layer->drawPixel != NULL){
                       if((BLENDCNT & (1 << i1))){
                           layer->Tipo = 'S';
                           lcd.Source[n++] = layer;
                       }
                       else{
                           lcd.Target[n1++] = layer;
                           if( (BLENDCNT & (1 << (i1 + 8))) )
                               layer->Tipo = 'T';
                           else
                               layer->Tipo = ' ';
                       }
                   }
               }
           }
       break;
   }
   lcd.CountSource = n;
   lcd.CountTarget = n1;
   for(;n<5;n++)
       lcd.Source[n] = (LPLAYER)0xFFFFFFFF;
   for(;n1<5;n1++)
       lcd.Target[n1] = (LPLAYER)0xFFFFFFFF;
}
//---------------------------------------------------------------------------
void GetBackgroundRect(u8 num)
{
   LPLAYER layer;
   u16 *p;

   layer = &lcd.layers[num];
   p = io_ram_u16 + 4 + num;
   if(lcd.DrawMode == 0 || (lcd.DrawMode == 1 && num < 2)){
       switch ((*p>>14)&0x3) {
		    case 0:
               layer->Width = 31;
               layer->Height = 31;
           break;
		    case 1:
               layer->Width = 63;
               layer->Height = 31;
           break;
		    case 2:
               layer->Width = 31;
               layer->Height = 63;
           break;
		    case 3:
               layer->Width = 63;
               layer->Height = 63;
           break;
       }
   }
   else if(num > 1){
       switch ((*p>>14)&0x3) {
		    case 0:
               layer->Width = 16;
               layer->Height = 16;
               layer->log2 = 4;
           break;
		    case 1:
               layer->Width = 32;
               layer->Height = 32;
               layer->log2 = 5;
           break;
           case 2:
               layer->Width = 64;
               layer->Height = 64;
               layer->log2 = 6;
           break;
		    case 3:
               layer->Width = 128;
               layer->Height = 128;
               layer->log2 = 7;
           break;
       }
   }
}
//---------------------------------------------------------------------------
u16 *GetSurfacePointer()
{
//   DDSURFACEDESC ddsd;
//   HRESULT hr;

   return screen;

/*   if(lcd.IsLock != 0 || lcd.BlitMode == BM_GDI)
       return screen;
   ZeroMemory(&ddsd,sizeof(DDSURFACEDESC));
   ddsd.dwSize = sizeof(DDSURFACEDESC);
   hr = lcd.lpDDgba->Lock(NULL,&ddsd,DDLOCK_SURFACEMEMORYPTR|DDLOCK_WRITEONLY,0);
   if(hr != DD_OK)
       return NULL;
   lcd.IsLock = 1;
   return (u16 *)ddsd.lpSurface;*/
}
//---------------------------------------------------------------------------
void FillBaseLayer(LPLAYER layer,u16 Control)
{
   layer->Priority = (u8)(Control & 3);
   layer->bPalette = !((Control >> 7) & 1);
   layer->bMosaic = (u8)((Control >> 6) & 1);
   layer->ScreenBaseBlock = vram_u8 + (((Control >> 8) & 0x1F) << 11);
   layer->CharBaseBlock = vram_u8 + (((Control >> 2) & 0x3) << 14);
   layer->bWrap = (u8)((Control >> 13) & 1);
   FillListBackground();
}
//---------------------------------------------------------------------------
void GetAlphaIndex()
{
   u8 i;

   lcd.eva = (u8)((i = (u8)(BLENDV & 31)) > 16 ? 16 : i);
   lcd.evb = (u8)((i = (u8)((BLENDV >> 8) & 31)) > 16 ? 16 : i);
   lcd.peva = lcd.tabColor + (lcd.eva << 5);
   lcd.pevb = lcd.tabColor + (lcd.evb << 5);
}
//---------------------------------------------------------------------------
void GetBrightnessIndex()
{
   u8 i;

   lcd.evy = (u8)((i = (u8)(BLENDY & 31)) > 16 ? 16 : i);
   lcd.pevy = lcd.tabColor + (lcd.evy << 5);
   lcd.pevy1 = lcd.tabColor1 + (lcd.evy << 5);
}
//---------------------------------------------------------------------------
void SetLcd(u16 adress,u8 accessMode)
{
   u8 i;
   LPLAYER layer;
   LPWINGBA win;
   LPWINCONTROL winc;
   s32 s;

   switch(adress){
       case 0:
       case 1:
           if((i = (u8)(DISPCNT & 7)) != lcd.DrawMode){
               lcd.DrawMode = i;
               lcd.Render = render_mode[i];
               switch(i){
                   case 0:
                       for(i=0;i<4;i++){
                           lcd.layers[i].drawPixel     = (DRAWPIXEL)drawPixelMode0;
                           lcd.layers[i].initDrawLine  = (INITDRAWLINE)InitLayerMode0;
                           lcd.layers[i].postDraw      = NULL;//PostdrawPixelMode0;
                       }
                   break;
                   case 1:
                       for(i=0;i<2;i++){
                           lcd.layers[i].drawPixel     = (DRAWPIXEL)drawPixelMode0;
                           lcd.layers[i].initDrawLine  = (INITDRAWLINE)InitLayerMode0;
                           lcd.layers[i].postDraw      = NULL;//PostdrawPixelMode0;
                       }
                       for(;i<3;i++){
                           lcd.layers[i].drawPixel     = (DRAWPIXEL)drawPixelMode1;
                           lcd.layers[i].initDrawLine  = (INITDRAWLINE)InitLayerMode1;
                           lcd.layers[i].postDraw      = (POSTDRAWPIXEL)PostDrawMode1;
                       }
                       lcd.layers[3].drawPixel         = NULL;
                       lcd.layers[3].initDrawLine      = NULL;
                       lcd.layers[3].Enable = 0;
                   break;
                   case 2:
                       for(i=0;i<2;i++){
                           lcd.layers[i].drawPixel     = NULL;
                           lcd.layers[i].initDrawLine  = NULL;
                           lcd.layers[i].postDraw      = NULL;
                       }
                       for(;i<4;i++){
                           lcd.layers[i].drawPixel     = (DRAWPIXEL)drawPixelMode1;
                           lcd.layers[i].initDrawLine  = (INITDRAWLINE)InitLayerMode1;
                           lcd.layers[i].postDraw      = (POSTDRAWPIXEL)PostDrawMode1;
                       }
                   break;
                   default:
                       for(i=0;i<4;i++){
                           lcd.layers[i].Enable = 0;
                           lcd.layers[i].drawPixel = (DRAWPIXEL)1;
                           lcd.layers[i].initDrawLine = NULL;
                           lcd.layers[i].postDraw = NULL;
                       }
                   break;
               }
           }
           lcd.SpriteEnable = (u8)((DISPCNT & 0x1000) >> 12);
           lcd.FrameBuffer = (u8)((DISPCNT & 0x10) >> 4);
           if((lcd.layers[0].Enable = (u8)((DISPCNT & 0x100) >> 8)) != 0)
               FillBaseLayer(&lcd.layers[0],BG0CNT);
           if((lcd.layers[1].Enable = (u8)((DISPCNT & 0x200) >> 9)) != 0)
               FillBaseLayer(&lcd.layers[1],BG1CNT);
           if((lcd.layers[2].Enable = (u8)((DISPCNT & 0x400) >> 10)) != 0)
               FillBaseLayer(&lcd.layers[2],BG2CNT);
           if((lcd.layers[3].Enable = (u8)((DISPCNT & 0x800) >> 11)) != 0)
               FillBaseLayer(&lcd.layers[3],BG3CNT);
           lcd.win[0].Enable = (u8)((DISPCNT & 0x2000) >> 13);
           lcd.win[1].Enable = (u8)((DISPCNT & 0x4000) >> 14);
           lcd.winObj.Enable = (u8)(((DISPCNT & 0x8000) >> 15) & lcd.SpriteEnable);
           lcd.WinEnable = (u8)(lcd.win[0].Enable | (lcd.win[1].Enable << 1));
           FillListBackground();
           lcd.swapBuffer[0] = rgbNormalLine;
           lcd.swapBuffer[1] = rgbAlphaLayerLine;
           lcd.swapBuffer[2] = lcd.swapBuffer[3] = rgbFadeLine;
           if(lcd.DrawMode < 3){
               GetBackgroundRect(0);
               GetBackgroundRect(1);
               GetBackgroundRect(2);
               GetBackgroundRect(3);
           }
           lcd.Enable = 1;
           if(lcd.bDoubleSize != ((DISPCNT & 0x40) >> 6)){
               lcd.bDoubleSize = (u8)((DISPCNT & 0x40) >> 6);
               RemapAllSprite();
           }
       break;
       case 2:
       case 3:
       case 4:
       case 5:
           DISPSTAT = (u16)((DISPSTAT & ~7) | (lcd.blankBit & 7));
           if(accessMode == AMM_WORD)
               VCOUNT = (u16)(cpu_cycles / ((float)LINE_CYCLES / 1024.0));
       break;
       case 6:
       case 7:
           VCOUNT = (u16)(cpu_cycles / ((float)LINE_CYCLES / 1024.0));
       break;
       case 8:
       case 9:
           FillBaseLayer(&lcd.layers[0],BG0CNT);
           GetBackgroundRect(0);
           if(accessMode == 4){
               FillBaseLayer(&lcd.layers[1],BG1CNT);
               GetBackgroundRect(1);
           }
       break;
       case 10:
       case 11:
           FillBaseLayer(&lcd.layers[1],BG1CNT);
           GetBackgroundRect(1);
           if(accessMode == 4){
               FillBaseLayer(&lcd.layers[2],BG2CNT);
               GetBackgroundRect(2);
           }
       break;
       case 12:
       case 13:
           FillBaseLayer(&lcd.layers[2],BG2CNT);
           GetBackgroundRect(2);
           if(accessMode == 4){
               FillBaseLayer(&lcd.layers[3],BG3CNT);
               GetBackgroundRect(3);
           }
       break;
       case 14:
       case 15:
           FillBaseLayer(&lcd.layers[3],BG3CNT);
           GetBackgroundRect(3);
       break;
       case 16:
       case 17:
       case 18:
       case 19:
           layer = &lcd.layers[0];
           layer->bg[0] = io_ram_u16[8] & 0x3FF;
           layer->bg[1] = io_ram_u16[9] & 0x3FF;
       break;
       case 20:
       case 21:
       case 22:
       case 23:
           layer = &lcd.layers[1];
           layer->bg[0] = io_ram_u16[10] & 0x3FF;
           layer->bg[1] = io_ram_u16[11] & 0x3FF;
       break;                                             //803cd16
       case 24:
       case 25:
       case 26:
       case 27:
           layer = &lcd.layers[2];
           layer->bg[0] = io_ram_u16[12] & 0x3FF;
           layer->bg[1] = io_ram_u16[13] & 0x3FF;
       break;
       case 28:
       case 29:
       case 30:
       case 31:
           layer = &lcd.layers[3];
           layer->bg[0] = io_ram_u16[14] & 0x3FF;
           layer->bg[1] = io_ram_u16[15] & 0x3FF;
       break;
       case 32:
       case 33:
       case 34:
       case 35:
           layer = &lcd.layers[2];
           if(layer->Priority == 0xFF)
               FillBaseLayer(layer,BG2CNT);
           layer->rotMatrix[0] = (s16)io_ram_u16[0x10];
           layer->rotMatrix[1] = (s16)io_ram_u16[0x11];
       break;
       case 36:
       case 37:
       case 38:
       case 39:
           layer = &lcd.layers[2];
           if(layer->Priority == 0xFF)
               FillBaseLayer(layer,BG2CNT);
           layer->rotMatrix[2] = (s16)io_ram_u16[0x12];
           layer->rotMatrix[3] = (s16)io_ram_u16[0x13];
       break;
       case 40:
       case 41:
       case 42:
       case 43:
           layer = &lcd.layers[2];
           if(layer->Priority == 0xFF)
               FillBaseLayer(layer,BG2CNT);
           if(((s = io_ram_u32[0xA] & 0x0FFFFFFF) & 0x08000000) != 0)
               s = -(0x10000000 - s);
           layer->CurrentX = layer->bgrot[0] = s;
       break;
       case 44:
       case 45:
       case 46:
       case 47:
           layer = &lcd.layers[2];
           if(layer->Priority == 0xFF)
               FillBaseLayer(layer,BG2CNT);
           if(((s = io_ram_u32[0xB] & 0x0FFFFFFF) & 0x08000000) != 0)
               s = -(0x10000000 - s);
           layer->CurrentY = layer->bgrot[1] = s;
       break;
       case 48:
       case 49:
       case 50:
       case 51:
           layer = &lcd.layers[3];
           if(layer->Priority == 0xFF)
               FillBaseLayer(layer,BG3CNT);
           layer->rotMatrix[0] = (s16)io_ram_u16[0x18];
           layer->rotMatrix[1] = (s16)io_ram_u16[0x19];
       break;
       case 52:
       case 53:
       case 54:
       case 55:
           layer = &lcd.layers[3];
           if(layer->Priority == 0xFF)
               FillBaseLayer(layer,BG3CNT);
           layer->rotMatrix[2] = (s16)io_ram_u16[0x1A];
           layer->rotMatrix[3] = (s16)io_ram_u16[0x1B];
       break;
       case 56:
       case 57:
       case 58:
       case 59:
           layer = &lcd.layers[3];
           if(layer->Priority == 0xFF)
               FillBaseLayer(layer,BG3CNT);
           if(((s = io_ram_u32[0xE] & 0x0FFFFFFF) & 0x08000000) != 0)
               s = -(0x10000000 - s);
           layer->CurrentX = layer->bgrot[0] = s;
       break;
       case 60:
       case 61:
       case 62:
       case 63:
           layer = &lcd.layers[3];
           if(layer->Priority == 0xFF)
               FillBaseLayer(layer,BG3CNT);
           if(((s = io_ram_u32[0xF] & 0x0FFFFFFF) & 0x08000000) != 0)
               s = -(0x10000000 - s);
           layer->CurrentY = layer->bgrot[1] = s;
       break;
       case 64:
       case 65:
       case 66:
       case 67:
           win = &lcd.win[0];
           win->left = (u8)(io_ram_u16[0x20] >> 8);
           if((win->right = (u8)io_ram_u16[0x20]) > 240)
               win->right = 240;
           win = &lcd.win[1];
           win->left = (u8)(io_ram_u16[0x21] >> 8);
           if((win->right = (u8)io_ram_u16[0x21]) > 240)
               win->right = 240;
       break;
       case 68:
       case 69:
       case 70:
       case 71:
           win = &lcd.win[0];
           win->top = (u8)(io_ram_u16[0x22] >> 8);
           win->bottom = (u8)io_ram_u16[0x22];
           win = &lcd.win[1];
           win->top = (u8)(io_ram_u16[0x23] >> 8);
           win->bottom = (u8)io_ram_u16[0x23];
       break;
       case 72:
       case 73:
       case 74:
       case 75:
           winc = &lcd.win[0].Control;
           for(i=0;i<4;i++)
               winc->EnableBg[i] = (WININ & (1 << i)) ? &lcd.layers[i] : NULL;
           winc->EnableObj = (u8)(WININ & 0x10 ? 1 : 0);
           winc->EnableBlend = (u8)(WININ & 0x20 ? 1 : 0);
           winc = &lcd.win[1].Control;
           for(i=0;i<4;i++)
               winc->EnableBg[i] = (WININ & (1 << (i + 8))) ? &lcd.layers[i] : NULL;
           winc->EnableObj = (u8)(WININ & 0x1000 ? 1 : 0);
           winc->EnableBlend = (u8)(WININ & 0x2000 ? 1 : 0);
           winc = &lcd.winOut;
           for(i=0;i<4;i++)
               winc->EnableBg[i] = (WINOUT & (1 << i)) ? &lcd.layers[i] : NULL;
           winc->EnableObj = (u8)((WINOUT & 0x10) >> 4);
           winc->EnableBlend = (u8)((WINOUT & 0x20) >> 5);

           winc = (LPWINCONTROL)&lcd.winObj;
           for(i=0;i<4;i++)
               winc->EnableBg[i] = (WINOUT & (1 << (i + 8))) ? &lcd.layers[i] : NULL;
           winc->EnableObj = (u8)(WINOUT & 0x1000 ? 1 : 0);
           winc->EnableBlend = (u8)(WINOUT & 0x2000 ? 1 : 0);
       break;
       case 76:
       case 77:
       case 78:
       case 79:
           lcd.xMosaic = (u8)(BGFCNT & 0x0f);
           lcd.yMosaic = (u8)((BGFCNT >> 4) & 0x0f);
           lcd.sprxMosaic = (u8)((BGFCNT >> 8) & 0xF);
           lcd.spryMosaic = (u8)((BGFCNT >> 12) & 0xF);
       break;
       case 80:
       case 81:
           lcd.iBlend = (u8)(((BLENDCNT >> 6) & 3));
           switch(lcd.iBlend){
               case 0:
                   lcd.GetPixelSprite = GetPixelSprite;
               break;
               case 1:
                   lcd.GetPixelSprite = GetPixelSpriteAlpha;
               break;
               default:
                   lcd.GetPixelSprite = GetPixelSpriteBrightness;
               break;
           }
           FillListBackground();                                //8004994  8001a40
           if((BLENDCNT & 0x10))
               lcd.Source[5] = (LPLAYER)1;
           else
               lcd.Source[5] = 0;
           if((BLENDCNT & 0x1000))
               lcd.Target[5] = (LPLAYER)1;
           else
               lcd.Target[5] = (LPLAYER)0;
           if((BLENDCNT & 0x20))
               lcd.Source[6] = (LPLAYER)1;
           else
               lcd.Source[6] = 0;
           if((BLENDCNT & 0x2000))
               lcd.Target[6] = (LPLAYER)1;
           else
               lcd.Target[6] = (LPLAYER)0;
           lcd.Source[7] = (LPLAYER)(BLENDCNT & 0x2F);
           lcd.Target[7] = (LPLAYER)(BLENDCNT & 0x2F00);
           if(accessMode == 4)
               GetAlphaIndex();
       break;
       case 82:                                                 //80022cc  //803b44
       case 83:
           GetAlphaIndex();
           if(accessMode == 4)
               GetBrightnessIndex();
       break;
       case 84:
       case 85:
           GetBrightnessIndex();
       break;
   }
}
//---------------------------------------------------------------------------
void RecalcLayout()
{
   RECT rc;

   ::SetRect(&rc,0,0,lcd.Width,lcd.Height);
   MapWindowPoints(hWin,NULL,(LPPOINT)&rc,2);
   CopyRect(&lcd.rc,&rc);
   if(pPlugInContainer != NULL)
       pPlugInContainer->NotifyState(PIL_VIDEO,PIS_SIZEMOVE,PIS_VIDEOMODEMASK);
}
//---------------------------------------------------------------------------
u8 ResetLcd()
{
   u8 i,i1;

   for(i=0;i<4;i++){
       ResetLayer(&lcd.layers[i]);
       lcd.layers[i].Index = i;
   }
   for(i=0;i<7;i++)
       lcd.Source[i] = lcd.Target[i] = NULL;
   for(i=0;i<17;i++){
       for(i1=0;i1<32;i1++)
           lcd.tabColor[i*32+i1] = (u8)(i1 * i >> 4);
   }
   for(i=0;i<17;i++){
       for(i1=0;i1<32;i1++)
           lcd.tabColor1[i*32+i1] = (u8)((31 - i1) * i >> 4);
   }
   lcd.Render = render_modeb_frame;
   lcd.GetPixelSprite = GetPixelSprite;
   lcd.Enable = 0;
   lcd.xMosaic = lcd.yMosaic = 0;
   lcd.SpriteEnable = 0;
   lcd.DrawMode = 0xFF;
   lcd.iBlend = 0;
   lcd.evy = lcd.eva = lcd.evb = 0;
   lcd.peva = lcd.pevb = lcd.pevy = lcd.tabColor;
   lcd.pevy1 = lcd.tabColor1;
   lcd.iFps = 0;
   ZeroMemory(translated_palette,0x210 * sizeof(u16));
   ZeroMemory(lcd.pZBuffer,960);
   if(lcd.hdcWin != NULL){
       ReleaseDC(hWin,lcd.hdcWin);
       lcd.hdcWin = NULL;
   }
   if(lcd.hdcWin == NULL)
       lcd.hdcWin = GetDC(hWin);
   if(pPlugInContainer != NULL && pPlugInContainer->GetVideoPlugInList())
       pPlugInContainer->GetVideoPlugInList()->Reset();
   RecalcLayout();
   BlackFrame(NULL);
   return 1;
}
//---------------------------------------------------------------------------
void BlackFrame(HDC hDC)
{
   ZeroMemory(screen,240*160*2);
   BlitFrame(hDC);
}
//---------------------------------------------------------------------------
static u8 ConvertBitmap(u16 *pSrc)
{
   u16 color;
   LPVOID pDst;
   DDSURFACEDESC2 ddsd;
   u8 i,x,y;
   HRESULT hr;

   ZeroMemory(&ddsd,sizeof(DDSURFACEDESC2));
   ddsd.dwSize = sizeof(DDSURFACEDESC2);
   hr = lcd.lpDDBack->Lock(NULL,&ddsd,DDLOCK_WAIT,0);
   if(hr == DDERR_SURFACELOST){
       lcd.lpDDBack->Restore();
       if(lcd.lpDDSurface != NULL)
           lcd.lpDDSurface->Restore();
       lcd.lpDDSPrimary->Restore();
       return 0;
   }
   else if(hr != DD_OK)
       return 0;
   pDst = ddsd.lpSurface;
   if((i = (u8)ddsd.ddpfPixelFormat.dwRGBBitCount) == 32){
       for(y=0;y<160;y++){
           for(x=0;x<240;x++){
               color = *pSrc++;
               *((u32 *)pDst)++ = RGB((color & 0x1F)<<3,((color >> 5) & 0x1F)<<3,((color >> 10) & 0x1F)<<3);
           }
           ((u8 *)pDst) += ddsd.lPitch - 960;
       }
   }
   else if(i == 24){
       for(y=0;y<160;y++){
           for(x=0;x<240;x++){
               color = *pSrc++;
               *((u8 *)pDst)++ = (u8)((color & 0x1F) << 3);
               *((u8 *)pDst)++ = (u8)(((color >> 5) & 0x1F) << 3);
               *((u8 *)pDst)++ = (u8)(((color >> 10) & 0x1F) << 3);
           }
           ((u8 *)pDst) += ddsd.lPitch - 720;
       }
   }
   else if(i == 16){
       for(y=0;y<160;y++){
           for(x=0;x<240;x++){
               color = *pSrc++;
               *((u16 *)pDst)++ = (u16)((color & 0x1F) | (((color >> 5) & 0x1F) << 6) | (((color >> 10) & 0x1F) << 11));
           }
           ((u8 *)pDst) += ddsd.lPitch - 480;
       }
   }
   lcd.lpDDBack->Unlock(NULL);
   return 1;
}
//---------------------------------------------------------------------------
void BlitFrame(HDC hDC)
{
   HRESULT hr;
   u16 *buffer;

   buffer = screen;
   hr = (HRESULT)pPlugInContainer->GetVideoPlugInList()->Run(buffer,lcd.pFilterBuffer);
   if((hr & 0x10000000))
       return;
   else if((int)hr > 0)
       buffer = (u16 *)lcd.pFilterBuffer;
   switch(lcd.BlitMode){
       case BM_DIRECTDRAW:
           ConvertBitmap(buffer);
           hr = lcd.lpDDSPrimary->Blt(&lcd.rc,lcd.lpDDBack,NULL,DDBLT_WAIT,NULL);
           if(hr == DDERR_SURFACELOST)
               lcd.lpDD->RestoreAllSurfaces();
       break;
       case BM_DIRECTDRAWFULLSCREEN:
           ConvertBitmap(buffer);
           hr = lcd.lpDDSurface->Blt(NULL,lcd.lpDDBack,NULL,DDBLT_WAIT,NULL);
           if(hr == DDERR_SURFACELOST){
               lcd.lpDDBack->Restore();
               lcd.lpDDSurface->Restore();
               lcd.lpDDSPrimary->Restore();
           }
           if(lcd.bFullScreen == 1)
               hr = lcd.lpDDSPrimary->Blt(&lcd.rc,lcd.lpDDSurface,NULL,DDBLT_WAIT,NULL);
           else
               hr = lcd.lpDDSPrimary->Flip(NULL,0);
           if(hr == DDERR_SURFACELOST){
               lcd.lpDDBack->Restore();
               lcd.lpDDSurface->Restore();
               lcd.lpDDSPrimary->Restore();
           }
       break;
       default:
           if(hDC == NULL)
               hDC = lcd.hdcWin;
           StretchDIBits(hDC,0,0,lcd.Width,lcd.Height,0,0,240,160,buffer,&BmpInfo,DIB_RGB_COLORS,SRCCOPY);
       break;
   }
}
//---------------------------------------------------------------------------
void SetRectLcd(int x,int y)
{
   lcd.Width = (u16)x;
   lcd.Height = (u16)y;
}
//---------------------------------------------------------------------------
static u8 CreateDirectDrawFullScreenSurface()
{
   DDSURFACEDESC2 ddsd;

   RELEASE(lcd.lpDDBack);
   RELEASE(lcd.lpDDSurface);
   RELEASE(lcd.lpDDSPrimary);

   ZeroMemory(&ddsd,sizeof(DDSURFACEDESC2));
   ddsd.dwSize = sizeof(DDSURFACEDESC2);
   ddsd.dwFlags = DDSD_CAPS|DDSD_BACKBUFFERCOUNT;
   ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE|DDSCAPS_FLIP|DDSCAPS_COMPLEX;
   ddsd.dwBackBufferCount = 1;
   iSubCodeError = -3;
   if((lcd.lpDD->CreateSurface(&ddsd,&lcd.lpDDSPrimary,NULL)) != DD_OK){
       ReleaseDirectDraw();
       return 0;
   }
   ZeroMemory(&ddsd,sizeof(DDSURFACEDESC2));
   ddsd.dwSize = sizeof(DDSURFACEDESC2);
   ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
   iSubCodeError = -7;
   if((lcd.lpDDSPrimary->GetAttachedSurface(&ddsd.ddsCaps,&lcd.lpDDSurface)) != DD_OK){
       ReleaseDirectDraw();
       return 0;
   }
   lcd.lpDDSurface->AddRef();
   ZeroMemory(&ddsd,sizeof(DDSURFACEDESC2));
   ddsd.dwSize = sizeof(DDSURFACEDESC2);
   ddsd.dwFlags = DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS;
   ddsd.dwWidth = 240;
   ddsd.dwHeight = 160;
   ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
   iSubCodeError = -7;
	if((lcd.lpDD->CreateSurface(&ddsd,&lcd.lpDDBack,NULL)) != DD_OK){
       ReleaseDirectDraw();
       return 0;
   }
   iSubCodeError = 0;
   return 1;
}
//---------------------------------------------------------------------------
void EnterWindowMode()
{
   if(lcd.bFullScreen < 2)
       return;
   CreateDirectDrawFullScreenSurface();
   EnableClipper();
   lcd.bFullScreen = 1;
   ::RedrawWindow(hWin,NULL,NULL,RDW_FRAME|RDW_INVALIDATE|RDW_ERASE|RDW_UPDATENOW|RDW_ALLCHILDREN);
}
//---------------------------------------------------------------------------
void ExitWindowMode()
{
   if(!lcd.bFullScreen)
       return;
   CreateDirectDrawFullScreenSurface();
   lcd.bFullScreen = 2;
}
//---------------------------------------------------------------------------
BOOL OnInitVideoPlugMenu(HMENU menu)
{
   return pPlugInContainer->GetVideoPlugInList()->OnInitMenu(menu);
}
//---------------------------------------------------------------------------
BOOL EnableVideoPlug(WORD wID)
{
   return pPlugInContainer->GetVideoPlugInList()->OnEnablePlug(wID);
}

