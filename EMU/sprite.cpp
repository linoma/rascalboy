#include "sprite.h"
#include "memory.h"
#include "graphics.h"
#if !defined(__BORLANDC__)
#include <math.h>
#endif

static u8 sprite_sizes_x[0x10] = {8,16,32,64,16,32,32,64, 8, 8,16,32,0,0,0,0};
static u8 sprite_sizes_y[0x10] = {8,16,32,64, 8, 8,16,32,16,32,32,64,0,0,0,0};
static LPSPRITE sprite_buffer;
static u8 *sprite_priority[4];
static PSROTMATRIX rotMatrix;
//---------------------------------------------------------------------------
static void CalcSpriteYIncr(LPSPRITE pSprite);
//---------------------------------------------------------------------------
u8 InitSprite()
{
   u8 i,*p;

   sprite_buffer = (LPSPRITE)GlobalAlloc(GPTR,128*sizeof(SPRITE) + 530 + 32*sizeof(SROTMATRIX));
   if(sprite_buffer == NULL)
       return FALSE;
   p = (u8 *)((u8 *)sprite_buffer + 128*sizeof(SPRITE));
   for(i=0;i<4;i++,p+=132)
       sprite_priority[i] = (u8 *)p;
   rotMatrix = (PSROTMATRIX)p;
   ResetSprite();
   return 1;
}
//---------------------------------------------------------------------------
void DestroySprite()
{
   u8 i;

   rotMatrix = NULL;
   for(i=0;i<4;i++)
       sprite_priority[i] = NULL;
   if(sprite_buffer != NULL)
       GlobalFree((HGLOBAL)sprite_buffer);
   sprite_buffer = NULL;
}
//---------------------------------------------------------------------------
void ResetSprite()
{
   u8 i;

   for(i=0;i<128;i++){
       ZeroMemory(&sprite_buffer[i],sizeof(SPRITE));
       sprite_buffer[i].Index = sprite_buffer[i].Priority = 0xFF;
   }
   for(i=0;i<4;i++){
       FillMemory(sprite_priority[i],130,0xFF);
       sprite_priority[i][129] = 0;
   }
   ZeroMemory(rotMatrix,32*sizeof(SROTMATRIX));
}
//---------------------------------------------------------------------------
void DrawSprite(u8 xStart,u8 xEnd)
{
   u8 *b;
   LPSPRITE pSprite;
   s8 i;

   if(!lcd.SpriteEnable || !lcd.VisibleLayers[4])
       goto ex_DrawSprite;
   i = 3;
   do{
       b = sprite_priority[i];                        
       while(*b != 0xFF){
           pSprite = &sprite_buffer[*b++];
           if(pSprite->Enable != 0 && ((lcd.DrawMode > 2 && pSprite->tileBaseNumber > 511) || lcd.DrawMode < 3 || (pSprite->VisibleMode == 2 && lcd.winObj.Enable)))
               pSprite->pFunc(pSprite,xStart,xEnd);
       }
   }while(--i > -1);
ex_DrawSprite:
   if(lcd.winObj.Enable)
       DrawLineObjWindow(xStart,xEnd);
   else
       lcd.swapBuffer[lcd.iBlend](xStart,xEnd);
}
#ifdef _DEBPRO
//---------------------------------------------------------------------------
LPSPRITE GetSprite(u8 nSprite)
{
   if(nSprite < 128)
       return &sprite_buffer[nSprite];
   else
       return NULL;
}
//---------------------------------------------------------------------------
short GetPaletteColor(u8 nSprite,LPPALETTEENTRY lppe)
{
   LPSPRITE pSprite;
   int iStart,iEnd,i;
   u16 color;

   if((pSprite = GetSprite(nSprite)) == NULL)
       return -1;
   if(pSprite->bPalette){
       iStart = pSprite->iPalette;
       iEnd = 16;
   }
   else{
       iStart = 0;
       iEnd = 256;
   }
   for(i=0;iEnd > 0;iEnd--,i++){
       color = paletteSprite[iStart++];
       lppe[i].peBlue = (color & 0x1F) << 3;
       lppe[i].peGreen = ((color >> 5) & 0x1F) << 3;
       lppe[i].peRed = ((color >> 10) & 0x1F) << 3;
   }
   return i;
}
//---------------------------------------------------------------------------
u8 DrawDebugSprite(u8 nSprite,u16 *pBuffer,int inc)
{
   LPSPRITE pSprite;
   int x2,xc,yc,y2,y3,x3,y,xc2,y2PB,y2PD,yTile,x2PA,x2PC;
   u8 sx,sy,x;
   s16 PB,PD;
   u32 tileBaseNumber,tileDataAddress,tileNumber,PA,PC;
   u8 xTile,xSubTile,ySubTile,color8,color4,xNbTile,yNbTile;

   if((pSprite = GetSprite(nSprite)) == NULL)
       return 0;
   if(!pSprite->bRot && pSprite->bDouble)
       return 0;
   sy = pSprite->SizeY;
   sx = pSprite->SizeX;
   if(pSprite->bRot)
       goto DrawDebugSprite_Rot;
   for(y2=0;y2<sy;y2++){
       tileBaseNumber = pSprite->tileBaseNumber + ((y2 >> 3) << pSprite->tileNumberYIncr);
       x2 = (y2 & 0x7) << 2;
       if(!pSprite->bPalette)
           x2 <<= 1;
       tileDataAddress = 0x10000 + x2;
       for(x=0;x<sx;x++,pBuffer++){
           xSubTile = (u8)((x2 = (pSprite->hFlip ? sx - 1 - x : x)) & 0x7);
           if (!pSprite->bPalette){
               tileNumber = (tileBaseNumber + ((x2 >> 3) << 1)) << 5;
               if((color8 = vram_u8[tileDataAddress + tileNumber + xSubTile]) != 0)
                   *pBuffer = paletteSprite[color8];
               else
                   *pBuffer = 0;
           }
           else{
               tileNumber = (tileBaseNumber + (x2 >> 3)) << 5;
               color8 = vram_u8[tileDataAddress + tileNumber + (xSubTile >> 1)];
               if((color4 = (color8 >> ((xSubTile & 0x1) << 2)) & 0xf) != 0)
                   *pBuffer = paletteSprite[color4+pSprite->iPalette];
               else
                   *pBuffer = 0;
           }
       }
   }
   return 1;
DrawDebugSprite_Rot:
   xNbTile = sx;
   yNbTile = sy;
   xc = sx >> 1;
   yc = sy >> 1;
   if(pSprite->bDouble != 0){
       sx <<= 1;
       sy <<= 1;
   }
   tileBaseNumber = pSprite->tileBaseNumber;
   tileDataAddress = 0x10000;

   PA = pSprite->rotMatrix->PA << 8;
   PB = pSprite->rotMatrix->PB;
   PC = pSprite->rotMatrix->PC << 8;
   PD = pSprite->rotMatrix->PD;

   for(y=0;y<sy;y++){
       xc2 = (0 - (sx >> 1));
       y2 = (y - (sy >> 1)) << 8;
       y2PB = y2 * PB;
       y2PD = y2 * PD;
       x2PA = xc2 * PA;
       x2PC = xc2 * PC;
       for(x = 0; x < sx;x++,x2PA += PA,x2PC += PC,pBuffer++){
           x3 = ((x2PA + y2PB) >> 16) + xc;
		    y3 = ((x2PC + y2PD) >> 16) + yc;
           if(x3 >= 0 && x3 < xNbTile && y3 >= 0 && y3 < yNbTile){
               xTile = x3 >> 3;
		        yTile = (y3 >> 3) << pSprite->tileNumberYIncr;
		        xSubTile = x3 & 0x07;
		        ySubTile = y3 & 0x07;
               if(!pSprite->bPalette){
                   tileNumber = (tileBaseNumber + (xTile << 1) + yTile);
                   if((color8 = vram_u8[tileDataAddress + tileNumber + xSubTile]) != 0)
                       *pBuffer = paletteSprite[color8];
                   else
                       *pBuffer = 0;
               }
               else{
                   tileNumber = (tileBaseNumber + xTile + yTile);
                   color8 = vram_u8[tileDataAddress + (tileNumber << 5) + (xSubTile >> 1) + (ySubTile << 2)];
                   if((color4 = (color8 >> ((xSubTile & 0x1) << 2)) & 0xf) != 0)
                       *pBuffer = paletteSprite[color4+pSprite->iPalette];
                   else
                       *pBuffer = 0;
               }
           }
           else
               *pBuffer = 0;
       }
   }
   return 1;
}
#endif
//---------------------------------------------------------------------------
u16 GetPixelSprite(LPSPRITE pSprite,u8 xPos,u16 iColor)
{
   if(pSprite->Priority > (u8)lcd.pZBuffer[xPos])
       return 0xFFFF;
   switch(pSprite->VisibleMode){
       case 0:
           return iColor;
       case 1:
           if(lcd.Target[7] != 0)
               return (u16)(0x8100 | iColor);
           else
               return iColor;
       case 2:
           lcd.WinObjSprite[xPos] = 1;
           return ((u16 *)lcd.SpriteBuffer)[xPos];
       default:
           return 0xFFFF;
   }
}
//---------------------------------------------------------------------------
u16 GetPixelSpriteBrightness(LPSPRITE pSprite,u8 xPos,u16 iColor)
{
   if(pSprite->Priority > (u8)lcd.pZBuffer[xPos])
       return 0xFFFF;
   switch(pSprite->VisibleMode){
       case 0:
           if(lcd.Source[5] == NULL)
               return (u16)(0x8000 | iColor);
           else
               return iColor;
       case 1:
           if(lcd.Target[7] != 0)
               return (u16)(0x8100 | iColor);
           else
               return iColor;
       case 2:
           lcd.WinObjSprite[xPos] = 1;
           return ((u16 *)lcd.SpriteBuffer)[xPos];
       default:
           return 0xFFFF;
   }
}
//---------------------------------------------------------------------------
u16 GetPixelSpriteAlpha(LPSPRITE pSprite,u8 xPos,u16 iColor)
{
   u8 prtSrc,idxSrc,prtTar,idxTar;
   u32 value;

   switch(pSprite->VisibleMode){
       case 0:
           prtSrc = GETZPRTSRC((value = lcd.pZBuffer[xPos]));
           prtTar = GETZPRTTAR(value);
           idxTar = GETZIDXTAR(value);
//           idxSrc = GETZIDXSRC(value);
           if(lcd.Source[5] != NULL){
               if(prtSrc != 0xFF){
                   if(pSprite->Priority <= prtSrc)
                       return (u16)(0x8000 | iColor);
                   return (u16)0xFFFF;
               }
               else if(idxTar != 0xFF){
                   if(lcd.layers[idxTar].Tipo == 'T'){
                       if(pSprite->Priority > prtTar)
                           return (u16)(0x8000 | iColor);
                       else
                           return iColor;
                   }
                   else{
                       if(pSprite->Priority <= prtTar)
                           return iColor;
                       else
                           return 0xFFFF;
                   }
               }
           }
           else if(lcd.Target[5] != NULL){
               if(prtSrc != 0xFF){
                   if(prtTar != 0xFF){
                       if(pSprite->Priority < prtTar){
                           if(pSprite->Priority <= prtSrc)
                               return iColor;
                           else
                               return (u16)(0x8000 | iColor);
                       }
                       else if(pSprite->Priority == prtTar){
                           if(prtTar > prtSrc)
                               return (u16)(0x8000 | iColor);
                           else
                               return iColor;
                       }
                       else{
                           if(lcd.layers[idxTar].Tipo == 'T')
                               return (u16)(0x8400 | iColor);
                           return (u16)0xFFFF;
                       }
                   }
                   else if(pSprite->Priority <= prtSrc)
                       return (u16)iColor;
                   return (u16)(0x400 | iColor);
               }
               else if(prtTar != 0xFF){
                   if(pSprite->Priority > prtTar)
                       return (u16)(0x400 | iColor);
                   else if(pSprite->Priority == prtTar){
                       if(lcd.layers[idxTar].Tipo == 'T')
                           return (u16)(0x8000 | iColor);
                       else
                           return iColor;
                   }
                   return iColor;
               }
           }
           else{
               if(prtSrc < pSprite->Priority)
                   return (u16)(0x8000|iColor);
               if(prtTar < pSprite->Priority)
                   return 0xFFFF;
           }
       break;
       case 1:
           if(pSprite->Priority > (prtTar = GETZPRTTAR((value = lcd.pZBuffer[xPos]))))
               return 0xFFFF;
           idxSrc = GETZIDXSRC(value);
           if(idxSrc != 0xFF && lcd.layers[idxSrc].Tipo == ' '){
               if(lcd.Target[7] == 0)
                   return iColor;
               return 0xFFFF;
           }
           if(idxSrc == 0xFF && prtTar == 0xFF)
               return iColor;
           iColor |= 0x8100;
       break;
       case 2:
           lcd.WinObjSprite[xPos] = 1;
           iColor = ((u16 *)lcd.SpriteBuffer)[xPos];
       break;
       case 3:
           iColor = 0xFFFF;
       break;
   }
   return iColor;
}
//---------------------------------------------------------------------------
void drawPixelSprite(LPSPRITE pSprite,u8 xStart,u8 xEnd)
{
   s16 xPos,sx,sy;
   u8 x8,y8,sxEnd,x,*p1;
   u16 *p;

   if(VCOUNT < (xPos = pSprite->yPos) || VCOUNT >= xPos + (sy = pSprite->SizeY))
       return;
   y8 = (u8)(VCOUNT - xPos);
   if((xPos = pSprite->xPos) > xEnd || (xPos + (sxEnd = sx = pSprite->SizeX)) < xStart)
       return;
   p = (u16 *)lcd.SpriteBuffer + xPos;
   if(pSprite->bMosaic != 0 && lcd.spryMosaic)
       y8 = (u8)(y8 - (y8 % lcd.spryMosaic));
   y8 = (u8)(pSprite->vFlip ? sy - 1 - y8 : y8);
   if(!pSprite->bPalette)
       p1 = &vram_u8[(y8 & 0x7) << 3];
   else
       p1 = &vram_u8[(y8 & 0x7) << 2];
   p1 += 0x10000 + ((pSprite->tileBaseNumber + ((y8 >> 3) << pSprite->tileNumberYIncr)) << 5);
   if((sy = (s16)(xEnd - xPos)) < sx)
       sxEnd = (u8)sy;
   for(x=0;x < sxEnd;x++,p++,xPos++){
       if(xPos < xStart)
           continue;
       if(pSprite->bMosaic != 0 && lcd.sprxMosaic)
           x8 = (u8)(x - (x % lcd.sprxMosaic));
       else
           x8 = x;
       x8 = (u8)((y8 = (u8)(pSprite->hFlip ? sx - 1 - x8 : x8)) & 0x7);
       if(!pSprite->bPalette){
           if((y8 = p1[((y8 >> 3) << 6) + x8]) == 0)
               continue;
       }
       else{
           y8 = p1[((y8 >> 3) << 5) + (x8 >> 1)];
           if((y8 = (u8)((y8 >> ((x8 & 0x1) << 2)) & 0xf)) == 0)
               continue;
           y8 += (u8)pSprite->iPalette;
       }
       *p = lcd.GetPixelSprite(pSprite,xPos,y8);
   }
}
//---------------------------------------------------------------------------
void drawPixelSpriteRot(LPSPRITE pSprite,u8 xStart,u8 xEnd)
{
	u16 *p,iColor;
   int xc,yc,y2,y3,x3,y,xc2,yTile,x2PA,x2PC;
   u8 sx,sy,x;
   s16 yPos,xPos;
   u32 tileBaseNumber,PA,PC;
   u8 xTile,xSubTile,ySubTile,yScr,xNbTile,yNbTile;

   xNbTile = sx = pSprite->SizeX;
   yNbTile = sy = pSprite->SizeY;
   xc = sx >> 1;
   yc = sy >> 1;
   if(pSprite->bDouble != 0){
       sx <<= 1;
       sy <<= 1;
   }
   if((yScr = VCOUNT) < (yPos = pSprite->yPos) || yScr >= yPos + sy)
       return;
   y = yScr - yPos;
   p = (u16 *)lcd.SpriteBuffer + (xPos = pSprite->xPos);
   tileBaseNumber = pSprite->tileBaseNumber;
   xc2 = (0 - (sx >> 1));
   y2 = (y - (sy >> 1)) << 8;
   if(pSprite->rotMatrix == NULL)
       pSprite->rotMatrix = rotMatrix;
   x2PA = xc2 * (PA = pSprite->rotMatrix->PA << 8) + (y2 * pSprite->rotMatrix->PB);
   x2PC = xc2 * (PC = pSprite->rotMatrix->PC << 8) + (y2 * pSprite->rotMatrix->PD);
   if((x3 = xEnd - xPos) < sx)
       sx = (u8)x3;
   for(x = 0; x < sx;x++,p++,x2PA += PA,x2PC += PC,xPos++){
       if(xPos < xStart)
           continue;
       x3 = (x2PA >> 16) + xc;
		y3 = (x2PC >> 16) + yc;
       if(!(x3 >= 0 && x3 < xNbTile && y3 >= 0 && y3 < yNbTile))
           continue;
       xTile = (u8)(x3 >> 3);
		yTile = (y3 >> 3) << pSprite->tileNumberYIncr;
       xSubTile = (u8)(x3 & 0x07);
		ySubTile = (u8)(y3 & 0x07);
       if (!pSprite->bPalette){
           if((iColor = vram_u8[0x10000 + ((tileBaseNumber + (xTile << 1) + yTile) << 5) + xSubTile + (ySubTile << 3)]) == 0)
               continue;
       }
       else{
           iColor = vram_u8[0x10000 + ((tileBaseNumber + xTile + yTile) << 5) + (xSubTile >> 1) + (ySubTile << 2)];
           if((iColor = (u16)((iColor >> ((xSubTile & 0x1) << 2)) & 0xf)) == 0)
               continue;
           iColor += pSprite->iPalette;
       }
       *p = lcd.GetPixelSprite(pSprite,xPos,iColor);
   }
}
//---------------------------------------------------------------------------
static void SetrotMatrix(u16 *p,LPSPRITE pSprite)
{
   u16 i;

   if((pSprite->bRot = (u8)((*p >> 8) & 1)) != 0){
       i = (u16)((*(p+1) >> 9) & 0x1f);
       if(rotMatrix[i].bWrite != pSprite->Index){
           rotMatrix[i].bWrite = pSprite->Index;
           pSprite->rotMatrix = &rotMatrix[i];
       }
       pSprite->pFunc = drawPixelSpriteRot;
   }
   else
       pSprite->pFunc = drawPixelSprite;
}
//---------------------------------------------------------------------------
static void CalcAttr0(LPSPRITE pSprite,u16 *p)
{
   s16 s;
   u16 i;

   SetrotMatrix(p,pSprite);
   pSprite->bDouble = (u8)((*p >> 9) & 1);
   if((s = (s16)(*p & 255)) > 159)
       s = (s16)(0 - (255 - s));
   pSprite->yPos = s;
   pSprite->bPalette = (u8)(!((*p >> 13) & 1));
   pSprite->VisibleMode = (u8)((*p >> 10) & 3);
   if(!pSprite->bRot)
       pSprite->bMosaic = (u8)((*p >> 12) & 1);
   else
       pSprite->bMosaic = 0;
   pSprite->a0 = *p;
   i = (u16)((*p++ >> 14) << 2);
   i += (u16)(*p >> 14);
   pSprite->SizeX = sprite_sizes_x[i];
   pSprite->SizeY = sprite_sizes_y[i];
   CalcSpriteYIncr(pSprite);
}
//---------------------------------------------------------------------------
void RemapAllSprite()
{
   u8 i;

   for(i=0;i<128;i++){
       if(sprite_buffer[i].Priority != 0xFF)
           CalcSpriteYIncr(&sprite_buffer[i]);
   }
}
//---------------------------------------------------------------------------
static void CalcSpriteYIncr(LPSPRITE pSprite)
{
   u16 i;

   i = (u16)(lcd.bDoubleSize != 0 ? (!pSprite->bPalette ? pSprite->SizeX >> 2 : pSprite->SizeX >> 3) : 32);
   if(i > 0)
       pSprite->tileNumberYIncr = log2((u16)i);
   else
       pSprite->tileNumberYIncr = 0;
}
//---------------------------------------------------------------------------
static void CalcAttr1(LPSPRITE pSprite,u16 *p)
{
   s16 s;
   u16 i;

   SetrotMatrix(p,pSprite);
   i = (u16)((*p++ >> 14) << 2);
   i += (u16)(*p >> 14);
   pSprite->SizeX = sprite_sizes_x[i];
   pSprite->SizeY = sprite_sizes_y[i];
   if((s = (s16)(*p & 0x1FF)) > 255)
       s = (s16)(0 - (512 - s));
   pSprite->xPos = s;
   pSprite->a1 = *p;
   pSprite->hFlip = (u8)((*p >> 12) & 1);
   pSprite->vFlip = (u8)((*p >> 13) & 1);
   CalcSpriteYIncr(pSprite);
}
//---------------------------------------------------------------------------
static void SpriteSort(u8 *tab,int left,int right)
{
   u8 *p,*p1,*p2,value,value1;
   int i;

   if(left >= right)
       return;
   value = *(p = p2 = &tab[left]);
   *p = *(p1 = &tab[(left + right) >> 1]);
   *p1 = value;
   for(p1 = p,value = *p++,i = left + 1;i<=right;i++,p++){
       if(*p <= value)
           continue;
       value1 = *(++p1);
       *p1 = *p;
       *p = value1;
   }
   value = *p2;
   *p2 = *p1;
   *p1 = value;
   SpriteSort(tab,left,(i = p1 - tab) - 1);
   SpriteSort(tab,i + 1,right);
}
//---------------------------------------------------------------------------
static void CalcAttr2(LPSPRITE pSprite,u16 *p,u8 Index)
{
   u8 i1,i2,*p1,*p2,i,i3,i4,*p4;

   p += 2;
   i4 = pSprite->Priority;
   if((i3 = pSprite->Priority = (u8)((*p >> 10) & 3)) != i4){
       p4 = sprite_priority[i3];
       p4[i2 = p4[129]] = Index;
       if(pSprite->Index != 0xFF){
           p1 = p2 = sprite_priority[i4];
           p1[pSprite->Index] = 0xFF;
           p1[129]--;
           for(i1=0,i=0;i<128;i++,p1++){
               if(*p1 != 0xFF)
                   sprite_buffer[*p2++ = *p1].Index = i1++;
           }
           for(;i1<128;i1++)
               *p2++ = 0xFF;
       }
       pSprite->Index = i2;
       SpriteSort(p4,0,i1 = p4[129]++);
       for(i=0;i<=i1;i++)
           sprite_buffer[*p4++].Index = i;
   }
   pSprite->iPalette = (u16)((*p >> 12) << 4);
   pSprite->tileBaseNumber = (*p & 0x3ff);
   i1 = (u8)((DISPCNT & 0x40) != 0 ? TRUE : FALSE);
   if(!pSprite->bPalette && !i1)
       pSprite->tileBaseNumber &= 0xfffe;
   pSprite->a2 = *p;
}
//---------------------------------------------------------------------------
static void CalcRotMatrix(u32 adress)
{
   u8 Index;
   u16 i;
   PSROTMATRIX p;

   Index = (u8)(adress >> 5);
   i = (u16)(Index << 4);
   p = &rotMatrix[Index];
   switch(((adress - (Index << 5)) >> 3)){
       case 0:
           p->PA = oam_u16[i + 3];
       break;
       case 1:
           p->PB = oam_u16[i + 7];
       break;
       case 2:
           p->PC = oam_u16[i + 11];
       break;
       case 3:
           p->PD = oam_u16[i + 15];
       break;
   }
   p->bWrite = 0xFF;
}
//---------------------------------------------------------------------------
void WriteSprite(u16 adress,u8 mode)
{
   u8 Index;
   LPSPRITE pSprite;
   u16 *p;
   int x,y;

   pSprite = &sprite_buffer[(Index = (u8)(adress >> 3))];
   p = &oam_u16[Index << 2];                              //8003f0c
   switch((adress & 7)){
       case 0:
       case 1:
           CalcAttr0(pSprite,p);
           if(mode == AMM_WORD)
               CalcAttr1(pSprite,p);
       break;
       case 2:
       case 3:
           CalcAttr1(pSprite,p);
           if(mode == AMM_WORD)
               CalcAttr2(pSprite,p,Index);
       break;
       case 4:
       case 5:
           CalcAttr2(pSprite,p,Index);
           if(mode == AMM_WORD)
               CalcRotMatrix(adress+2);
       break;
       case 6:
       case 7:
           CalcRotMatrix(adress);
       break;
   }
   pSprite->Enable = (u8)((!pSprite->a0 && !pSprite->a1) ? 0 : 1);
   x = pSprite->SizeX + pSprite->xPos;
   y = pSprite->SizeY + pSprite->yPos;
   if(!pSprite->bRot && pSprite->bDouble || y < 0 || pSprite->yPos > 159 || x < 0 || pSprite->xPos > 240)
       pSprite->Enable = 0;
}

