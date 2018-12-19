//---------------------------------------------------------------------------
#include <windows.h>
#pragma hdrstop

#include "mode3.h"

//---------------------------------------------------------------------------
u16 FASTCALL drawPixelMode3(LPROTMATRIX rot)
{
   u16 value,*p;
   s32 x,y;

   *lcd.pCurrentZBuffer++ = rot->iLayer;
   p = (u16 *)rot->pBuffer;
   if(!rot->bRot)
       value = bgrtorgb(p[(rot->bMosaic ? rot->x - (rot->x % lcd.xMosaic) : rot->x) + rot->bg[1]]);
   else{
       x = rot->bg[0] >> 8;
       y = rot->bg[1] >> 8;
       if(x < 0 || y < 0 || x > 239 || y > 159)
           value = 0;
       else
           value = bgrtorgb(p[x + y * 240]);
       rot->bg[0] += rot->rotMatrix[0];
       rot->bg[1] += rot->rotMatrix[2];
   }
   return value;
}
//---------------------------------------------------------------------------
void drawLineMode3(LPROTMATRIX rot)
{
   u8 n;
   u16 *p1;

   p1 = (u16 *)lcd.SourceBuffer;
   rot->pBuffer = vram_u8;
   if(lcd.layers[2].Enable)
       rot->iLayer = (u32)0xFFFF02FF;
   else
       rot->iLayer = (u32)NULLZPIXEL;
   for(n=0;n<240;n++){
       rot->x = n;
       *p1++ = drawPixelMode3(rot);
   }
   if(lcd.iBlend != 1 || !lcd.layers[2].Enable)
       return;
   for(p1 = (u16 *)lcd.TargetBuffer,n=0;n<120;n++)
       *((u32 *)p1)++ = (u32)-1;
}
//---------------------------------------------------------------------------
void drawLineMode3Window(LPROTMATRIX rot)
{
   LPWINCONTROL winc;
   u8 CountPoint,iBlend,iBlendCopy,i3,i2;
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
       rot->pBuffer = (u8 *)(vram_u16 + x);
       rot->iLayer = (u16)((lcd.layers[2].Priority | (2 << 8)));
       switch((lcd.iBlend = iBlend)){
           case 0:
               lcd.GetPixelSprite = GetPixelSprite;
               for(i3=0;i>0;i3++,i--,x++){
                   rot->x = (u8)x;
                   *p++ = drawPixelMode3(rot);
               }
           break;
           case 1:
               lcd.GetPixelSprite = GetPixelSpriteAlpha;
               sw = (u16 *)lcd.SourceBuffer;
               dw = (u16 *)lcd.TargetBuffer;
               for(i3=0;i>0;i3++,i--,x++){
                   rot->x = (u8)x;
                   *sw++ = drawPixelMode3(rot);
                   *dw++ = 0;
               }
           break;
           case 2:
           case 3:
               lcd.GetPixelSprite = GetPixelSpriteBrightness;
               sw = (u16 *)lcd.SourceBuffer;
               for(i3=0;i>0;i3++,i--,x++){
                   rot->x = (u8)x;
                   *sw++ = drawPixelMode3(rot);
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

