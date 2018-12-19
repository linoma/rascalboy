//---------------------------------------------------------------------------
#include <windows.h>
#pragma hdrstop

#include "mode5.h"

//---------------------------------------------------------------------------
u16 drawPixelMode5(LPROTMATRIX rot)
{
   u16 value;
   s32 x,y;

   *lcd.pCurrentZBuffer++ = (u32)0xFFFF02FF;
   if(!rot->bRot){
       if(VCOUNT > 127)
           value = 0;
       else
           value = bgrtorgb(((u16 *)rot->pBuffer)[(rot->bMosaic ? rot->x - (rot->x % lcd.xMosaic) : rot->x) + rot->bg[1]]);
   }
   else{
       x = rot->bg[0] >> 8;
       y = rot->bg[1] >> 8;
       if(x >= 0 && x < 160 && y >= 0 && y < 127)
           value = bgrtorgb(((u16 *)rot->pBuffer)[x + y * 160]);
       else
           value = 0;
       rot->bg[0] += rot->rotMatrix[0];
       rot->bg[1] += rot->rotMatrix[2];
   }
   return value;
}
//---------------------------------------------------------------------------
void drawLineMode5(LPROTMATRIX rot)
{
   u8 n;
   u16 *p1,*p;

   p = vram_u16;
   if(lcd.FrameBuffer != 0)
       p += 0x5000;
   p1 = (u16 *)lcd.SourceBuffer;
   n = (u8)(rot->bRot ? 240 : 160);
   rot->pBuffer = (u8 *)p;
   for(;n != 0;n--,rot->x++)
       *p1++ = drawPixelMode5(rot);
   for(;rot->x < 240;rot->x++)
       *p1++ = 0;
   if(lcd.iBlend != 1 || !lcd.layers[2].Enable)
       return;
   for(n = (u8)(rot->bRot ? 240 : 160);n != 0;n--)
       ((u16 *)lcd.TargetBuffer)[n] = -1;
}