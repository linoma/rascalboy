//---------------------------------------------------------------------------
#include <windows.h>
#pragma hdrstop

#include "modeTile.h"

//---------------------------------------------------------------------------
void FASTCALL PostDrawMode1(LPLAYER layer)
{
   layer->CurrentScrollX += layer->rotMatrix[0];
   layer->CurrentScrollY += layer->rotMatrix[2];
}
//---------------------------------------------------------------------------
void FASTCALL InitLayerMode1(LPLAYER layer)
{
   layer->CurrentScrollX = layer->CurrentX;
   layer->CurrentScrollY = layer->CurrentY;
}
//---------------------------------------------------------------------------
u8 FASTCALL drawPixelMode1(LPLAYER layer)
{
	s32 y2,y3;
   int xTile,yTile,ySubTile;
   u8 width,width1;

   width1 = (u8)((width = (u8)layer->Width) - 1);
   xTile = (y2 = layer->CurrentScrollX >> 8) >> 3;
   yTile = (y3 = layer->CurrentScrollY >> 8) >> 3;
   if(layer->bWrap || (xTile >= 0 && xTile < width && yTile >= 0 && yTile < width)){
       yTile = ((yTile & width1) << layer->log2) + (xTile & width1);
       ySubTile = (((y3 & 7) << 3) + (y2 & 7));
       return layer->CharBaseBlock[ySubTile + (layer->ScreenBaseBlock[yTile] << 6)];
   }
   else
       return 0;
}
//---------------------------------------------------------------------------
void FASTCALL InitLayerMode0(LPLAYER layer)
{
   u32 y1;

   if(layer->bMosaic && lcd.yMosaic)
       y1 = (u32)((VCOUNT - (VCOUNT % lcd.yMosaic)) + layer->bg[1]);
   else
       y1 = (u32)(VCOUNT + layer->bg[1]);
   layer->tileY = (u16)((y1 >> 3) & layer->Height);
   layer->offsetToAdd1 = 0;
   if(layer->Width != 31 && layer->tileY > 31){
       layer->offsetToAdd1 += (u16)0x800;
       layer->tileY &= 0x1F;
   }
   layer->tileY <<= 5;
   if(layer->ScreenBaseBlock == NULL)
       layer->ScreenBaseBlock = vram_u8;
   layer->scrbb = layer->ScreenBaseBlock + (layer->tileY << 1);
   layer->vFlip = (u8)((y1 & 0x7) << 3);
}
//---------------------------------------------------------------------------
u8 FASTCALL drawPixelMode0(LPLAYER layer)
{
   u32 tileX,x1;
   u16 tileData;

   if(layer->bMosaic && lcd.xMosaic)
       x1 = (lcd.x - (lcd.x % lcd.xMosaic)) + layer->bg[0];
   else
       x1 = lcd.x + layer->bg[0];
   tileX = (x1 >> 3) & layer->Width;
   if(tileX > 31 && layer->Width != 31)
       tileX = (tileX & 0x1F) + layer->offsetToAdd1 + 0x400;
   else
       tileX += layer->offsetToAdd1;
   tileX = ((tileData = *((u16 *)layer->scrbb + tileX)) & 0x3FF) << 6;
   if((tileData & 0x0400) != 0){
       tileX += 7 - (x1 & 7);
       x1 = ~x1;
   }
   else
       tileX += (x1 & 7);
   if((tileData & 0x0800) != 0)
       tileX += 56 - layer->vFlip;
   else
       tileX += layer->vFlip;
   if(!layer->bPalette)
       return *(layer->CharBaseBlock + tileX);
   if((tileX = (u32)((u8)*(layer->CharBaseBlock + (tileX >> 1)))) == 0)
       return 0;
   if((tileX = (u8)((tileX >> ((x1 & 0x1) << 2)) & 0xf)) == 0)
       return 0;
   return (u8)(tileX + (u8)((tileData & 0xF000) >> 8));
}
//---------------------------------------------------------------------------
void drawLineModeTile()
{
   u8 i5,prtSrc,idxSrc;
   u8 i1;
   u16 *p,*dw,i5w,zValueSrc,zValueTar;
   LPLAYER *p1,pLayer;

   p1 = lcd.Source;
   for(i1=0;i1<lcd.CountSource;i1++)
       ((LPLAYER)(*p1))->initDrawLine(*p1++);
   p1 = lcd.Target;
   for(i1=0;i1<lcd.CountTarget;i1++)
       ((LPLAYER)(*p1))->initDrawLine(*p1++);
   p = (u16 *)lcd.SourceBuffer;
   switch(lcd.iBlend){
       case 0:
           for(lcd.x = 0;lcd.x < 240;lcd.x++){
               zValueSrc = (u16)NULLZPIXEL;
               for(i5 = 0,i1=0;i1<lcd.CountSource;i1++){
                   if(!i5 && (i5 = lcd.Source[i1]->drawPixel(lcd.Source[i1])) != 0)
                       zValueSrc = (u16)(lcd.Source[i1]->Priority|(lcd.Source[i1]->Index << 8));
                   if(lcd.Source[i1]->postDraw != NULL)
                       lcd.Source[i1]->postDraw(lcd.Source[i1]);
               }
               *p++ = translated_palette[i5];
               *lcd.pCurrentZBuffer++ = (u32)zValueSrc;
           }
       break;
       case 1:
           dw = (u16 *)lcd.TargetBuffer;
           for(lcd.x=0;lcd.x<240;lcd.x++,p++,dw++){
               *dw = *p = 0xFFFF;
               zValueSrc = (u16)NULLZPIXEL;
               for(i5=0,i1 = 0;i1<lcd.CountSource;){
                   pLayer = lcd.Source[i1++];
                   if(!i5 && (i5 = pLayer->drawPixel(pLayer)) != 0){
                       zValueSrc = (u16)((prtSrc = pLayer->Priority)|((idxSrc = pLayer->Index) << 8));
                       *p = translated_palette[i5];
                   }
                   if(pLayer->postDraw != NULL)
                       pLayer->postDraw(pLayer);
               }
               zValueTar = (u16)NULLZPIXEL;
               for(i5w = 0,i1=0;i1 < lcd.CountTarget;){
                   pLayer = lcd.Target[i1++];
                   if(!i5w && (i5w = pLayer->drawPixel(pLayer)) != 0){
                       if(*p != 0xFFFF){
                           if(pLayer->Priority > prtSrc && pLayer->Tipo != 'T')
                               i5w |= 0x8000;
                           else if(pLayer->Priority < prtSrc || (pLayer->Priority == prtSrc && idxSrc > pLayer->Index)){
                               *p |= 0x8000;
                               zValueSrc = (u16)NULLZPIXEL;
                           }
                       }
                       *dw = (u16)((i5w & 0x8000) | translated_palette[(u8)i5w]);
                       zValueTar = (u16)(pLayer->Priority|(pLayer->Index << 8));
                   }
                   if(pLayer->postDraw != NULL)
                       pLayer->postDraw(pLayer);
               }
               *lcd.pCurrentZBuffer++ = (u32)(zValueSrc | (zValueTar << 16));
           }
       break;
       case 2:
       case 3:
           for(lcd.x=0;lcd.x<240;lcd.x++){
               zValueSrc = NULLZPIXEL;
               for(i5w=0,i1=0;i1<lcd.CountSource;){
                  pLayer = lcd.Source[i1++];
                   if(!i5w && (i5w = pLayer->drawPixel(pLayer)) != 0){
                       zValueSrc = (u16)(pLayer->Priority|(pLayer->Index << 8));
                       if(pLayer->Tipo != 'S')
                           i5w |= 0x8000;
                   }
                   if(pLayer->postDraw != NULL)
                       pLayer->postDraw(pLayer);
               }
               if(!i5w && !lcd.Source[6])
                   i5w = 0x8000;
               *p++ = (u16)((i5w & 0x8000) | translated_palette[(u8)i5w]);
               *lcd.pCurrentZBuffer++ = (u32)zValueSrc;
           }
       break;
   }
}
//---------------------------------------------------------------------------
void drawLineModeTileWindow()
{
   LPWINCONTROL winc;
   LPLAYER Source[6],Target[6],*p1,pLayer;
   u8 nCountSource,nCountTarget,CountPoint,iBlend,iBlendCopy;
   u8 n,i1,i2,i5,prtSrc,idxSrc;
   u16 *p,*dw,i5w,zValueSrc,zValueTar;
   int i,xStart;
   POINTWINDOW ptLine[5];
   GETSPRITEPIXEL oldGetPixelSprite;

   CountPoint = (u8)GetPointsWindow(ptLine);
   p1 = lcd.Source;
   for(n=0;n<lcd.CountSource;n++,p1++)
       ((LPLAYER)(*p1))->initDrawLine(*p1);
   p1 = lcd.Target;
   for(n=0;n<lcd.CountTarget;n++,p1++)
       ((LPLAYER)(*p1))->initDrawLine(*p1);
   oldGetPixelSprite = lcd.GetPixelSprite;
   iBlendCopy = lcd.iBlend;
   for(lcd.x = i2 = 0;i2<CountPoint;i2++){
       winc = ptLine[i2].winc;
       i = ptLine[i2].width;
       nCountSource = nCountTarget = 0;
       xStart = lcd.x;
       if(winc->EnableBlend && iBlendCopy == 1){
           iBlend = 1;
           p1 = lcd.Source;
           for(n=0;n<lcd.CountSource;n++,p1++){
               if(winc->EnableBg[((LPLAYER)(*p1))->Index])
                   Source[nCountSource++] = *p1;
           }
           p1 = lcd.Target;
           for(n=0;n<lcd.CountTarget;n++,p1++){
               if(winc->EnableBg[((LPLAYER)(*p1))->Index])
                   Target[nCountTarget++] = *p1;
           }
       }
       else{
           if(winc->EnableBlend || lcd.winObj.Enable)
               iBlend = iBlendCopy;
           else                            
               iBlend = 0;
           p1 = lcd.Source;
           for(n=0;n<lcd.CountSource;n++,p1++){
               if(winc->EnableBg[((LPLAYER)(*p1))->Index])
                   Source[nCountSource++] = *p1;
           }
           p1 = lcd.Target;
           for(n=0;n<lcd.CountTarget;n++,p1++){
               if(winc->EnableBg[((LPLAYER)(*p1))->Index])
                   Source[nCountSource++] = *p1;
           }
           for(n=0;n<nCountSource;n++){
               pLayer = Source[n];
               for(i5 = (u8)(n + 1);i5 < nCountSource;i5++){
                   if(pLayer->Priority > Source[i5]->Priority ||
                       (pLayer->Priority == Source[i5]->Priority && pLayer->Index > Source[i5]->Index)){
                       Source[n] = Source[i5];
                       Source[i5] = pLayer;
                       pLayer = Source[n];
                   }
               }
           }
       }
       p = (u16 *)lcd.SourceBuffer + lcd.x;
       lcd.iBlend = iBlend;
       switch(iBlend){
           case 0:
               lcd.GetPixelSprite = GetPixelSprite;
               for(;i > 0;i--,lcd.x++){
                   zValueSrc = (u16)NULLZPIXEL;
                   for(p1 = Source,i5=0,i1=nCountSource;i1 > 0;i1--){
                       pLayer = *p1++;
                       if(!i5 && (i5 = pLayer->drawPixel(pLayer)) != 0)
                           zValueSrc = (u16)(pLayer->Priority|(pLayer->Index << 8));
                       if(pLayer->postDraw != NULL)
                           pLayer->postDraw(pLayer);
                   }
                   *p++ = translated_palette[i5];
                   *lcd.pCurrentZBuffer++ = zValueSrc;
               }
           break;
           case 1:
               lcd.GetPixelSprite = GetPixelSpriteAlpha;
               dw = (u16 *)lcd.TargetBuffer + lcd.x;
               for(;i>0;i--,dw++,p++,lcd.x++){
                   *dw = *p = 0xFFFF;
                   zValueSrc = (u16)NULLZPIXEL;
                   for(p1 = Source,i5=0,i1=nCountSource;i1 > 0;i1--){
                       pLayer = *p1++;
                       if(!i5 && (i5 = pLayer->drawPixel(pLayer)) != 0){
                           zValueSrc = (u16)((prtSrc = pLayer->Priority)|((idxSrc = pLayer->Index) << 8));
                           *p = translated_palette[i5];
                       }
                       if(pLayer->postDraw != NULL)
                           pLayer->postDraw(pLayer);
                   }
                   zValueTar = (u16)NULLZPIXEL;
                   for(p1 = Target,i5w = 0,i1=nCountTarget;i1 > 0;i1--){
                       pLayer = *p1++;
                       if(!i5w && (i5w = pLayer->drawPixel(pLayer)) != 0){
                           if(*p != 0xFFFF){
                               if(pLayer->Priority > prtSrc && pLayer->Tipo != 'T')
                                   i5w |= 0x8000;
                               else if(pLayer->Priority < prtSrc || (pLayer->Priority == prtSrc && idxSrc > pLayer->Index)){
                                   *p |= 0x8000;
                                   zValueSrc = (u16)NULLZPIXEL;
                               }
                           }
                           *dw = (u16)((i5w & 0x8000) | translated_palette[(u8)i5w]);
                           zValueTar = (u16)(pLayer->Priority|(pLayer->Index << 8));
                       }
                       if(pLayer->postDraw != NULL)
                           pLayer->postDraw(pLayer);
                   }
                   *lcd.pCurrentZBuffer++ = (u32)(zValueSrc | (zValueTar << 16));
               }
           break;
           case 2:
           case 3:
               lcd.GetPixelSprite = GetPixelSpriteBrightness;
               for(;i > 0;i--,lcd.x++){
                   zValueSrc = (u16)NULLZPIXEL;
                   for(p1 = Source,i5w=0,i1=nCountSource;i1 > 0;i1--){
                       pLayer = *p1++;
                       if(!i5w && (i5w = pLayer->drawPixel(pLayer)) != 0){
                           zValueSrc = (u16)(pLayer->Priority|(pLayer->Index << 8));
                           if(pLayer->Tipo != 'S')
                               i5w |= 0x8000;
                       }
                       if(pLayer->postDraw != NULL)
                           pLayer->postDraw(pLayer);
                   }
                   if(!i5w && !lcd.Source[6])
                       i5w = 0x8000;
                   *p++ = (u16)((i5w & 0x8000) | translated_palette[(u8)i5w]);
                   *lcd.pCurrentZBuffer++ = (u32)zValueSrc;
               }
           break;
       }
       if(winc->EnableObj || lcd.winObj.Enable)
           DrawSprite((u8)xStart,(u8)lcd.x);
       else
           SwapBuffer((u8)xStart,(u8)lcd.x);
   }
   lcd.GetPixelSprite = oldGetPixelSprite;
   lcd.iBlend = iBlendCopy;
}

