#include "graphics.h"
#include "sprite.h"
#include "modeTile.h"
#include "mode3.h"
#include "mode4.h"
#include "mode5.h"

//---------------------------------------------------------------------------
u16 rgbFadePixel(u16 Color)
{
   u8 r,g,b;

   if(!(Color & 0x8000)){
       b = (u8)(Color & 0x1F);
       g = (u8)((Color >> 5) & 0x1F);
       r = (u8)((Color >> 10) & 0x1F);
       if(lcd.iBlend == 3){
           r -= lcd.pevy[r];
           g -= lcd.pevy[g];
           b -= lcd.pevy[b];
       }
       else{
           r += lcd.pevy1[r];
           g += lcd.pevy1[g];
           b += lcd.pevy1[b];
           if(r > 31) r = 31;
           if(g > 31) g = 31;
           if(b > 31) b = 31;
       }
       Color = (u16)((r << 10) | (g << 5) | b);
   }
   else
       Color &= (u16)~0x8000;
   return Color;
}
//---------------------------------------------------------------------------
u16 rgbAlphaLayerPixel(u8 x,u8 enableObj,u8 enableSrc,u8 enableTar)
{
   u8 r,g,b;
   u16 col1,col2,cols,col;

   col2 = (u16)(enableTar != 0 ? ((u16 *)lcd.TargetBuffer)[x] : 0xFFFF);
   col1 = (u16)(enableSrc != 0 ? col1 = ((u16 *)lcd.SourceBuffer)[x] : 0xFFFF);
   cols = (u16)(enableObj != 0 ? ((u16 *)lcd.SpriteBuffer)[x] : 0xFFFF);
   if(col1 < 0x8000){
       if(col2 >= 0x8000)
           col2 = 0xFFFF;
       else if((cols & 0x400))
           cols = 0xFFFF;
   }
   else if(col2 < 0x8000){
       col1 = 0xFFFF;    
       col2 = col2;
       if((cols & 0x400))
           cols = 0xFFFF;
   }
   else{
       col1 = (u16)(lcd.Source[6] ? translated_palette[0] : 0xFFFF);
       col2 = (u16)(lcd.Target[6] ? translated_palette[0] : 0xFFFF);
   }
   if(cols != 0xFFFF && !(cols & 0x100)){
       if((cols & 0x8000)){
           if(lcd.Source[5] != NULL)
               col1 = paletteSprite[(u8)cols];
           else
               col2 = paletteSprite[(u8)cols];
       }
       else{
           col1 = paletteSprite[(u8)cols];
           col2 = 0xFFFF;
       }
       cols = 0xFFFF;
   }
   if(col1 == 0xFFFF)
       col = col2;
   else if(col2 == 0xFFFF)
       col = col1;
   else{
       r = (u8)(lcd.peva[(col1 >> 10) & 0x1F] + lcd.pevb[(col2 >> 10) & 0x1F]);
       if(r > 31) r = 31;
       g = (u8)(lcd.peva[(col1 >> 5) & 0x1F] + lcd.pevb[(col2 >> 5) & 0x1F]);
       if(g > 31) g = 31;
       b = (u8)(lcd.peva[col1 & 0x1F] + lcd.pevb[col2 & 0x1F]);
       if(b > 31) b = 31;
       col = (u16)((r << 10)| (g << 5) | b);
   }
   if(cols != 0xFFFF){
       if(col1 == 0xFFFF && col2 == 0xFFFF)
           return paletteSprite[(u8)cols];
       col1 = paletteSprite[(u8)cols];
       col2 = col;
       r = (u8)(lcd.peva[(col1 >> 10) & 0x1F] + lcd.pevb[(col2 >> 10) & 0x1F]);
       if(r > 31) r = 31;
       g = (u8)(lcd.peva[(col1 >> 5) & 0x1F] + lcd.pevb[(col2 >> 5) & 0x1F]);
       if(g > 31) g = 31;
       b = (u8)(lcd.peva[col1 & 0x1F] + lcd.pevb[col2 & 0x1F]);
       if(b > 31) b = 31;
       col = (u16)((r << 10)| (g << 5) | b);
   }
   return col;
}
//---------------------------------------------------------------------------
static BOOL PtInWindow(LPWINGBA p,u8 x)
{
   if(VCOUNT < p->top || VCOUNT > p->bottom)
       return FALSE;
   if(x < p->left || x > p->right > 240)
       return FALSE;
   return TRUE;
}
//---------------------------------------------------------------------------
void DrawLineObjWindow(u8 xStart,u8 xEnd)
{
   u16 Color,col1,col2,cols;
   u8 x,idxSrc,idxTar,r,g,b;
   LPWINCONTROL win;
   u32 zValue;

   for(x= xStart;x < xEnd;x++){
       if(lcd.WinObjSprite[x] == 1){
           lcd.WinObjSprite[x] = (u8)-1;
           win = (LPWINCONTROL)&lcd.winObj;
       }
       else{
           if(lcd.win[0].Enable && lcd.VisibleLayers[5] != 0 && PtInWindow(&lcd.win[0],x))
               win = &lcd.win[0].Control;
           else if(lcd.win[1].Enable && lcd.VisibleLayers[6] != 0 && PtInWindow(&lcd.win[1],x))
               win = &lcd.win[1].Control;
           else
               win = &lcd.winOut;
       }
       zValue = lcd.pZBuffer[x];
       idxSrc = (u8)(zValue >> 8);
       idxTar = (u8)(zValue >> 24);
       switch((win->EnableBlend ? lcd.iBlend : 0)){
           case 0:
               col2 = (u16)(lcd.iBlend == 1 ? ((u16 *)lcd.TargetBuffer)[x] : 0xFFFF);
               col1 = ((u16 *)lcd.SourceBuffer)[x];
               if(col1 < 0x8000){
                   if(col2 != 0xFFFF){
                       col1 = 0xFFFF;
                       col2 &= (u16)~0x8000;
                   }
               }
               else if(col2 < 0x8000)
                   col1 = 0xFFFF;
               else{
                   if(col1 == 0xFFFF)
                       col1 = translated_palette[0];
                   col2 = 0xFFFF;
               }
               cols = ((u16 *)lcd.SpriteBuffer)[x];
               if(win->EnableObj && !(cols & 0x400) && cols != 0xFFFF){
                   Color = paletteSprite[(u8)cols];
                   if((cols & 0x100) && col1 != 0xFFFF){
                       r = (u8)(lcd.peva[(Color >> 10) & 0x1F] + lcd.pevb[(col1 >> 10) & 0x1F]);
                       if(r > 31) r = 31;
                       g = (u8)(lcd.peva[(Color >> 5) & 0x1F] + lcd.pevb[(col1 >> 5) & 0x1F]);
                       if(g > 31) g = 31;
                       b = (u8)(lcd.peva[Color & 0x1F] + lcd.pevb[col1 & 0x1F]);
                       if(b > 31) b = 31;
                       Color = (u16)((r << 10)| (g << 5) | b);
                   }
               }
               else{
                   if(idxSrc != 0xFF && col1 != 0xFFFF){
                       if(win->EnableBg[idxSrc] != NULL)
                           Color = col1;
                       else if(idxTar != 0xFF && win->EnableBg[idxTar] != NULL && col2 != 0xFFFF)
                           Color = col2;
                       else{
                           if(col2 != 0xFFFF)
                               Color = col2;
                           else if(col1 != 0xFFFF)
                               Color = col1;
                           else
                               Color = translated_palette[0];
                       }
                   }
                   else if(idxTar != 0xFF && win->EnableBg[idxTar] != NULL && col2 != 0xFFFF)
                       Color = col2;
                   else
                       Color = translated_palette[0];
               }
           break;
           case 1:
               if(idxSrc == 0xFF)
                   idxSrc = 0;
               else if(win->EnableBg[idxSrc] != NULL)
                   idxSrc = 1;
               else
                   idxSrc = 0;
               if(idxTar == 0xFF)
                   idxTar = 0;
               else if(win->EnableBg[idxTar] != NULL)
                   idxTar = 1;
               else
                   idxTar = 0;
               Color = rgbAlphaLayerPixel(x,win->EnableObj,idxSrc,idxTar);
           break;
           case 2:
           case 3:
               col2 = (u16)(lcd.iBlend == 1 ? ((u16 *)lcd.TargetBuffer)[x] : 0xFFFF);
               col1 = ((u16 *)lcd.SourceBuffer)[x];
               if(col1 < 0x8000){
                   if(col2 >= 0x8000)
                       col2 = 0xFFFF;
               }
               else if(col2 < 0x8000)
                   col1 = 0xFFFF;
               else{
                   if(col1 == 0xFFFF)
                       col1 = translated_palette[0];
                   col2 = 0xFFFF;
               }
               cols = ((u16 *)lcd.SpriteBuffer)[x];
               if(win->EnableObj && cols != 0xFFFF)
                   Color = (u16)((cols & 0x8000) | paletteSprite[(u8)cols]);
               else{
                   if(idxSrc != 0xFF && col1 != 0xFFFF){
                       if(win->EnableBg[idxSrc] != NULL)
                           Color = col1;
                       else if(idxTar != 0xFF && win->EnableBg[idxTar])
                           Color = col2;
                       else
                           Color = translated_palette[0];
                   }
                   else if(idxTar != 0xFF && win->EnableBg[idxTar] != NULL && col2 != 0xFFFF)
                       Color = col2;
                   else
                       Color = (u16)((((u8)lcd.Source[6] << 11) ^ 0x8000) | translated_palette[0]);
               }
               Color = rgbFadePixel(Color);
           break;
       }
       ((u16 *)lcd.SpriteBuffer)[x] = (u16)-1;
       *lcd.pBuffer++ = Color;
   }
}
//---------------------------------------------------------------------------
int GetPointsWindow(LPPOINTWINDOW pt)
{
   LPWINGBA Win[2],win;
   u8 CountPoint,check;
   int i,x,i1,CountWindow;

   for(CountWindow = -1,x = 0,i1 = 1;x < 2;x++,i1 <<= 1){
       if(!(lcd.WinEnable & i1) || !lcd.VisibleLayers[5+x])
           continue;
       win = &lcd.win[x];
       if(win->left > 240 || win->right > 240 || win->left == win->right)
           continue;
       if(win->Enable && VCOUNT >= win->top && VCOUNT < win->bottom)
           Win[++CountWindow] = win;
   }
   if(CountWindow > 0 && Win[1]->left <= Win[0]->left){
       win = Win[0];
       Win[0] = Win[1];
       Win[1] = win;
   }
   CountPoint = 0;
   x = i = 0;
   for(i1 = 0;i1 <= CountWindow;i1++){
       check = 0;
       win = Win[i1];
       if(win->left > x){
           if(win->left > win->right && win->right >= i){
               i += pt[CountPoint].width = (u8)((x = win->right) - i);
               pt[CountPoint++].winc = &win->Control;
               win->right = 240;
           }
           i += pt[CountPoint].width = (u8)(win->left - x);
           pt[CountPoint++].winc = &lcd.winOut;
       }
       if(win->left < win->right && x < win->right){
           if(i1 != CountWindow && Win[i1+1]->left < win->right){
               x = Win[i1+1]->left - x;
               check = 1;
           }
           else
               x = win->right;
           i += pt[CountPoint].width = (u8)(x - i);
           if(i > 240)
               pt[CountPoint].width = (u8)(240 - (i - 240));
           pt[CountPoint++].winc = &win->Control;
           if(!check)
               continue;
           x = Win[i1+1]->right;
           i += pt[CountPoint].width = (u8)(x - i);
           if(i > 240)
               pt[CountPoint].width = (u8)(240 - (i - 240));
           pt[CountPoint++].winc = &Win[i1+1]->Control;
           if(i >= win->right)
               continue;
           x = win->right;
           i += pt[CountPoint].width = (u8)(x - i);
           if(i > 240)
               pt[CountPoint].width = (u8)(240 - (i - 240));
           pt[CountPoint++].winc = &win->Control;
       }
   }
   if(i < 240){
       pt[CountPoint].width = (u8)(240 - i);
       pt[CountPoint++].winc = &lcd.winOut;
   }
   return CountPoint;
}
//---------------------------------------------------------------------------
void ResetLayer(LPLAYER layer)
{
   layer->Enable = 0;
   layer->Priority = 0xFF;
   layer->rotMatrix[0] = layer->rotMatrix[3] = 0x100;
   layer->rotMatrix[1] = layer->rotMatrix[2] = 0;
   layer->bg[0] = layer->bg[1] = 0;
   layer->bgrot[0] = layer->bgrot[1] = 0;
   layer->bMosaic = 0;
   layer->bPalette = 0;
   layer->ScreenBaseBlock = NULL;
   layer->CharBaseBlock = NULL;
   layer->scrbb = NULL;
   layer->Width = 0;
   layer->Height = 0;
   layer->log2 = 0;
   layer->tileY = 0;
   layer->offsetToAdd1 = 0;
   layer->vFlip = 0;
   layer->bWrap = 0;
   layer->CurrentX = 0;
   layer->CurrentY = 0;
   layer->CurrentScrollX = 0;
   layer->CurrentScrollY = 0;
   layer->Visible = lcd.VisibleLayers[layer->Index];
}
//---------------------------------------------------------------------------
/*void InitRunFrame()
{
   lcd.pBuffer = screen;
   if(lcd.layers[2].Enable != 0){
       lcd.layers[2].CurrentX = lcd.layers[2].bgrot[0];
       lcd.layers[2].CurrentY = lcd.layers[2].bgrot[1];
   }
   if(lcd.layers[3].Enable != 0){
       lcd.layers[3].CurrentX = lcd.layers[3].bgrot[0];
       lcd.layers[3].CurrentY = lcd.layers[3].bgrot[1];
   }
}*/
//---------------------------------------------------------------------------
void render_modeb_frame(void)
{
   int i;

   for(i=0;i<240;i++)
       *lcd.pBuffer++ = (u16)((0x1F)| (0x1F << 5) | (0x1F << 10));
}
//---------------------------------------------------------------------------
void render_mode0_frame()
{
   if(lcd.WinEnable != 0)
       drawLineModeTileWindow();
   else{
       drawLineModeTile();
       DrawSprite(0,240);
   }
}
//---------------------------------------------------------------------------
void render_mode1_frame(void)
{
   if(lcd.WinEnable != 0)
       drawLineModeTileWindow();
   else{
       drawLineModeTile();
       DrawSprite(0,240);
   }
   if(lcd.layers[2].Enable != 0){
       lcd.layers[2].CurrentX += lcd.layers[2].rotMatrix[1];
       lcd.layers[2].CurrentY += lcd.layers[2].rotMatrix[3];
   }
   if(lcd.layers[3].Enable != 0){
       lcd.layers[3].CurrentX += lcd.layers[3].rotMatrix[1];
       lcd.layers[3].CurrentY += lcd.layers[3].rotMatrix[3];
   }
}
//---------------------------------------------------------------------------
void render_mode2_frame (void)
{
   if(lcd.WinEnable != 0)
       drawLineModeTileWindow();
   else{
       drawLineModeTile();
       DrawSprite(0,240);
   }
   if(lcd.layers[2].Enable != 0){
       lcd.layers[2].CurrentX += lcd.layers[2].rotMatrix[1];
       lcd.layers[2].CurrentY += lcd.layers[2].rotMatrix[3];
   }
   if(lcd.layers[3].Enable != 0){
       lcd.layers[3].CurrentX += lcd.layers[3].rotMatrix[1];
       lcd.layers[3].CurrentY += lcd.layers[3].rotMatrix[3];
   }
}
//---------------------------------------------------------------------------
void render_mode3_frame (void)
{
   ROTMATRIX rot;
   u8 yScr;

   ZeroMemory(&rot,sizeof(ROTMATRIX));
   yScr = VCOUNT;
   if(lcd.layers[2].Enable != 0){
       if(lcd.layers[2].bMosaic != 0 && (lcd.xMosaic != 0 || lcd.yMosaic != 0))
           rot.bMosaic = 1;
       else{
           if(lcd.layers[2].rotMatrix[0] != 256 || lcd.layers[2].rotMatrix[3] != 256 || lcd.layers[2].rotMatrix[1] != 0 || lcd.layers[2].rotMatrix[2] != 0){
               rot.rotMatrix[0] = lcd.layers[2].rotMatrix[0];
               rot.rotMatrix[1] = lcd.layers[2].rotMatrix[1];
               rot.rotMatrix[2] = lcd.layers[2].rotMatrix[2];
               rot.rotMatrix[3] = lcd.layers[2].rotMatrix[3];
               rot.bg[0] = lcd.layers[2].CurrentX;
               rot.bg[1] = lcd.layers[2].CurrentY;
               rot.bRot = 1;
           }
           else
               rot.bRot = 0;
       }
   }
   if(!rot.bRot)
       rot.bg[1] = (rot.bMosaic ? (yScr - (yScr % lcd.xMosaic)) : yScr) * 240;
   else{
       lcd.layers[2].CurrentX += rot.rotMatrix[1];
       lcd.layers[2].CurrentY += rot.rotMatrix[3];
   }
   if(!lcd.WinEnable){
       drawLineMode3(&rot);
       DrawSprite(0,240);
   }
   else
       drawLineMode3Window(&rot);
}
//---------------------------------------------------------------------------
void render_mode4_frame()
{
   ROTMATRIX rot;
   u8 yScr;

   ZeroMemory(&rot,sizeof(ROTMATRIX));
   yScr = VCOUNT;
   if(lcd.layers[2].Enable != 0){
       if(lcd.layers[2].bMosaic != 0 && (lcd.xMosaic != 0 || lcd.yMosaic != 0))
           rot.bMosaic = 1;
       else{
           if(lcd.layers[2].rotMatrix[0] != 256 || lcd.layers[2].rotMatrix[3] != 256 || lcd.layers[2].rotMatrix[1] != 0 || lcd.layers[2].rotMatrix[2] != 0){
               rot.rotMatrix[0] = lcd.layers[2].rotMatrix[0];
               rot.rotMatrix[1] = lcd.layers[2].rotMatrix[1];
               rot.rotMatrix[2] = lcd.layers[2].rotMatrix[2];
               rot.rotMatrix[3] = lcd.layers[2].rotMatrix[3];
               rot.bg[0] = lcd.layers[2].CurrentX;
               rot.bg[1] = lcd.layers[2].CurrentY;
               rot.bRot = 1;
           }
           else
               rot.bRot = 0;
       }
   }
   if(!rot.bRot){
       if(rot.bMosaic && lcd.yMosaic)
           rot.bg[1] = (yScr - (yScr % lcd.yMosaic)) * 240;
       else
           rot.bg[1] = yScr * 240;
   }
   else{
       lcd.layers[2].CurrentX += lcd.layers[2].rotMatrix[1];
       lcd.layers[2].CurrentY += lcd.layers[2].rotMatrix[3];
   }
   if(!lcd.WinEnable){
       drawLineMode4(&rot);
       DrawSprite(0,240);
       return;
   }
   drawLineMode4Window(&rot);
}
//---------------------------------------------------------------------------
void render_mode5_frame()
{
   ROTMATRIX rot;
   u8 yScr;

   ZeroMemory(&rot,sizeof(ROTMATRIX));
   yScr = VCOUNT;
   if(lcd.layers[2].Enable != 0){
       if(lcd.layers[2].bMosaic != 0 && (lcd.xMosaic != 0 || lcd.yMosaic != 0))
           rot.bMosaic = 1;
       else{
           if(lcd.layers[2].rotMatrix[0] != 256 || lcd.layers[2].rotMatrix[3] != 256 || lcd.layers[2].rotMatrix[1] != 0 || lcd.layers[2].rotMatrix[2] != 0){
               rot.rotMatrix[0] = lcd.layers[2].rotMatrix[0];
               rot.rotMatrix[1] = lcd.layers[2].rotMatrix[1];
               rot.rotMatrix[2] = lcd.layers[2].rotMatrix[2];
               rot.rotMatrix[3] = lcd.layers[2].rotMatrix[3];
               rot.bg[0] = lcd.layers[2].CurrentX;
               rot.bg[1] = lcd.layers[2].CurrentY;
               rot.bRot = 1;
           }
           else
               rot.bRot = 0;
       }
   }
   if(!rot.bRot)
       rot.bg[1] = (rot.bMosaic ? (yScr - (yScr % lcd.xMosaic)) : yScr) * 160;
   else{
       lcd.layers[2].CurrentX += rot.rotMatrix[1];
       lcd.layers[2].CurrentY += rot.rotMatrix[3];
   }
   if(!lcd.WinEnable){
       drawLineMode5(&rot);
       DrawSprite(0,240);
   }
}
//---------------------------------------------------------------------------
void FillPalette(u16 index)
{
   int r,g,b,i;
   u16 color;

   color = bgrtorgb(pal_ram_u16[index]);
   r = (color >> 10) & 0x1F;
   g = (color >> 5) & 0x1F;
   b = color & 0x1F;
   if((i = lcd.Luminosita) == 0)
       i = 100;
   if((r = r * i / 100) > 31)
       r = 31;
   if((g = g * i / 100) > 31)
       g = 31;
   if((b = b * i / 100) > 31)
       b = 31;
	translated_palette[index] = (u16)((r <<10) | (g << 5) | b);
}
//---------------------------------------------------------------------------
u16 FASTCALL bgrtorgb(u16 color)
{
   return (u16)(((color & 0x1F) << 10) | (color & 0x3e0) | ((color >> 10) & 0x1F));
}
//---------------------------------------------------------------------------
void rgbNormalLine(u8 xStart,u8 xEnd)
{
   u16 r,g,b,a;

   for(xEnd -= xStart;xEnd != 0;lcd.pCurOB++,lcd.pCurSB++,xEnd--){
       r = *lcd.pCurOB;
       if(r != 0xFFFF){
           *lcd.pCurOB = (u16)-1;
           if((r & 0x8100) != 0){
               a = paletteSprite[(u8)r];
               b = *lcd.pCurSB;
               if((r = (u16)(lcd.peva[(a >> 10) & 0x1F] + lcd.pevb[(b >> 10) & 0x1F])) > 31) r = 31;
               if((g = (u16)(lcd.peva[(a >> 5) & 0x1F] + lcd.pevb[(b >> 5) & 0x1F])) > 31) g = 31;
               if((b = (u16)(lcd.peva[a & 0x1F] + lcd.pevb[b & 0x1F])) > 31) b = 31;
               *lcd.pBuffer++ = (u16)((r << 10)| (g << 5) | b);
           }
           else
               *lcd.pBuffer++ = paletteSprite[(u8)r];
       }
       else
           *lcd.pBuffer++ = *lcd.pCurSB;
   }
}
//---------------------------------------------------------------------------
void rgbFadeLine(u8 xStart,u8 xEnd)
{
   u16 col1,col,cols;
   u8 r,g,b;

   for(xEnd -= xStart;xEnd != 0;xEnd--,lcd.pCurSB++){
       cols = col = *lcd.pCurOB;
       *lcd.pCurOB++ = (u16)-1;
       if(col == 0xFFFF){
           col = *lcd.pCurSB;
           col1 = col;
       }
       else
           col1 = paletteSprite[(u8)col];
       if(!(col & 0x8000)){
           b = (u8)(col1 & 0x1F);
           g = (u8)((col1 >> 5) & 0x1F);
           r = (u8)((col1 >> 10) & 0x1F);
           if(lcd.iBlend == 3){
               r -= lcd.pevy[r];
               g -= lcd.pevy[g];
               b -= lcd.pevy[b];
           }
           else{
               if((r += lcd.pevy1[r]) > 31) r = 31;
               if((g += lcd.pevy1[g]) > 31) g = 31;
               if((b += lcd.pevy1[b]) > 31) b = 31;
           }
           col1 = (u16)((r << 10)| (g << 5) | b);
       }
       else if(cols != 0xFFFF && (cols & 0x100)){
           col1 = paletteSprite[(u8)cols];
           col = *lcd.pCurSB;
           if((r = (u8)(lcd.peva[(col1 >> 10) & 0x1F] + lcd.pevb[(col >> 10) & 0x1F])) > 31) r = 31;
           if((g = (u8)(lcd.peva[(col1 >> 5) & 0x1F] + lcd.pevb[(col >> 5) & 0x1F])) > 31) g = 31;
           if((b = (u8)(lcd.peva[col1 & 0x1F] + lcd.pevb[col & 0x1F])) > 31) b = 31;
           col1 = (u16)((r << 10)| (g << 5) | b);
       }
       else
           col1 &= (u16)~0x8000;
       *lcd.pBuffer++ = col1;
   }
}
//---------------------------------------------------------------------------
void rgbAlphaLayerLine(u8 xStart,u8 xEnd)
{
   u8 r,g,b;
   u16 col1,col2,cols,*pTarget,col,colBk1,colBk2;

   pTarget = &((u16 *)lcd.TargetBuffer)[xStart];
   colBk1 = (u16)(lcd.Source[6] ? translated_palette[0] : 0xFFFF);
   colBk2 = (u16)(lcd.Target[6] ? translated_palette[0] : 0xFFFF);
   if(colBk1 == 0xFFFF && colBk2 == 0xFFFF)
       colBk1 = translated_palette[0];
   for(xEnd -= xStart;xEnd != 0;xEnd--){
       col2 = *pTarget++;
       cols = *lcd.pCurOB;
       *lcd.pCurOB++ = 0xFFFF;
       if((col1 = *lcd.pCurSB++) < 0x8000){
           if(col2 >= 0x8000)
               col2 = 0xFFFF;
           else if((cols & 0x400))
               cols = 0xFFFF;
       }
       else if(col2 < 0x8000){
           col1 = 0xFFFF;
           if((cols & 0x400))
               cols = 0xFFFF;
       }
       else{
           col1 = colBk1;
           col2 = colBk2;
       }
       if(cols != 0xFFFF && !(cols & 0x100)){
           if((cols & 0x8000)){
               if(lcd.Source[5] != NULL){
                   if(lcd.DrawMode > 2)
                   	col2 = col1;
                   col1 = paletteSprite[(u8)cols];
               }
               else
                   col2 = paletteSprite[(u8)cols];
           }
           else{
               col1 = paletteSprite[(u8)cols];
               col2 = 0xFFFF;
           }
           cols = 0xFFFF;
       }
       if(col1 == 0xFFFF)
           col = col2;
       else if(col2 == 0xFFFF)
           col = col1;
       else{
           if((r = (u8)(lcd.peva[(col1 >> 10) & 0x1F] + lcd.pevb[(col2 >> 10) & 0x1F])) > 31) r = 31;
           if((g = (u8)(lcd.peva[(col1 >> 5) & 0x1F] + lcd.pevb[(col2 >> 5) & 0x1F])) > 31) g = 31;
           if((b = (u8)(lcd.peva[col1 & 0x1F] + lcd.pevb[col2 & 0x1F])) > 31) b = 31;
           col = (u16)((r << 10)| (g << 5) | b);
       }
       if(cols != 0xFFFF){
           col1 = paletteSprite[(u8)cols];
           col2 = col;
           if((r = (u8)(lcd.peva[(col1 >> 10) & 0x1F] + lcd.pevb[(col2 >> 10) & 0x1F])) > 31) r = 31;
           if((g = (u8)(lcd.peva[(col1 >> 5) & 0x1F] + lcd.pevb[(col2 >> 5) & 0x1F])) > 31) g = 31;
           if((b = (u8)(lcd.peva[col1 & 0x1F] + lcd.pevb[col2 & 0x1F])) > 31) b = 31;
           col = (u16)((r << 10)| (g << 5) | b);
       }
       *lcd.pBuffer++ = col;
   }
}

