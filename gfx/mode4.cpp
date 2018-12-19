//---------------------------------------------------------------------------
#include <windows.h>
#pragma hdrstop

#include "mode4.h"

//---------------------------------------------------------------------------
u8 FASTCALL drawPixelMode4(LPROTMATRIX rot)
{
   s32 x,y;
   u8 res;

   if(!rot->bRot){
       if(rot->bMosaic && lcd.xMosaic)
           res = rot->pBuffer[(rot->x - (rot->x % lcd.xMosaic)) + rot->bg[1]];
       else
           res = rot->pBuffer[rot->x + rot->bg[1]];
   }
   else{
       x = rot->bg[0] >> 8;
       y = rot->bg[1] >> 8;
       rot->bg[0] += rot->rotMatrix[0];
       rot->bg[1] += rot->rotMatrix[2];
       if(x < 0 || y < 0 || x > 239 || y > 159)
           res = 0;
       else
           res = rot->pBuffer[x + y * 240];
   }
   *lcd.pCurrentZBuffer++ = res ? rot->iLayer : 0xFFFFFFFF;
   return res;
}
//---------------------------------------------------------------------------
void drawLineMode4Window(LPROTMATRIX rot)
{
   LPWINCONTROL winc;
   u8 CountPoint,iBlend,iBlendCopy,i3,i2,*p1;
   u16 *p,*sw,*dw;
   int i,x;
   POINTWINDOW ptLine[5];
   GETSPRITEPIXEL oldGetPixelSprite;

   oldGetPixelSprite = lcd.GetPixelSprite;
   iBlendCopy = lcd.iBlend;
   CountPoint = (u8)GetPointsWindow(ptLine);
   for(x = i2 = 0;i2<CountPoint;i2++){
       winc = ptLine[i2].winc;
       iBlend = (u8)(winc->EnableBlend != 0 ? iBlendCopy : 0);
       i = ptLine[i2].width;
       if(lcd.layers[2].Enable && winc->EnableBg[2] != NULL)
           rot->iLayer = (u16)(lcd.layers[2].Priority | (2 << 8));
       else
           rot->iLayer = (u16)NULLZPIXEL;
       p = (u16 *)lcd.SourceBuffer + x;
       p1 = vram_u8 + x;
       if(lcd.FrameBuffer != 0)
           p1 += 0xA000;
       rot->pBuffer = p1;
       switch((lcd.iBlend = iBlend)){
           case 0:
               lcd.GetPixelSprite = GetPixelSprite;
               for(i3=0;i>0;i3++,i--,x++){
                   rot->x = (u8)x;
                   *p++ = translated_palette[drawPixelMode4(rot)];
               }
           break;
           case 1:
               lcd.GetPixelSprite = GetPixelSpriteAlpha;
               sw = (u16 *)lcd.SourceBuffer;
               dw = (u16 *)lcd.TargetBuffer;
               for(i3=0;i>0;i3++,i--,x++){
                   rot->x = (u8)x;
                   *sw++ = translated_palette[drawPixelMode4(rot)];
                   *dw++ = (u16)-1;
               }
           break;
           case 2:
           case 3:
               lcd.GetPixelSprite = GetPixelSpriteBrightness;
               sw = (u16 *)lcd.SourceBuffer;
               for(i3=0;i>0;i3++,i--,x++){
                   rot->x =(u8) x;
                   *sw++ = translated_palette[drawPixelMode4(rot)];
               }
           break;
       }
       if(winc->EnableObj)
           DrawSprite((u8)(x-i3),(u8)x);
       else
           SwapBuffer((u8)(x-i3),(u8)x);
   }
   lcd.GetPixelSprite = oldGetPixelSprite;
   lcd.iBlend = iBlendCopy;
}
//---------------------------------------------------------------------------
void drawLineMode4(LPROTMATRIX rot)
{
   u8 *p,n;
   u16 *p1,value;

   p = vram_u8;
   if(lcd.FrameBuffer != 0)
       p += 0xA000;
   p1 = (u16 *)lcd.SourceBuffer;
   value = 0;
   if(lcd.layers[2].Enable){
       rot->iLayer = (u32)0xFFFF0200|lcd.layers[2].Priority;
       if(lcd.layers[2].Tipo == 'T' || lcd.Target[7] == 0)
           value = 0xFFFF;
   }
   else
       rot->iLayer = (u32)NULLZPIXEL;
   switch(lcd.iBlend){
       case 0:
           rot->pBuffer = p;
           for(rot->x=0;rot->x<240;rot->x++)
               *p1++ = translated_palette[drawPixelMode4(rot)];
       break;
       case 1:
           rot->pBuffer = p;
           for(n=0;n<240;n++){
               rot->x = n;
               *p1++ = translated_palette[drawPixelMode4(rot)];
               ((u16 *)lcd.TargetBuffer)[n] = value;
           }
       break;
       case 2:
       case 3:
           rot->pBuffer = p;
           for(n=0;n<240;n++){
               rot->x = n;
               *p1++ = translated_palette[drawPixelMode4(rot)];
           }
       break;
   }
}
