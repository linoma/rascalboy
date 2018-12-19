#define INITGUID
//---------------------------------------------------------------------------
#include <windows.h>
#pragma hdrstop

#include "videoplug.h"
#include "audioplug.h"
#include "pluginctn.h"
#include "resource.h"
#include "lcd.h"

DEFINE_GUID(RBABLUR, 0xD0FF6594L, 0xBB49, 0x42DD, 0xB8, 0xEF, 0xEF, 0x8A, 0xE8, 0x25, 0x75, 0x93);
DEFINE_GUID(RBABILINEAR, 0x66B5C145L, 0x9AE6, 0x4DBD, 0x96, 0x85, 0x34, 0xB4, 0xEB, 0x0B, 0xDE, 0xAD);
DEFINE_GUID(RBAMOTIONBLUR, 0x2C4A5ACAL, 0x15F0, 0x41B5, 0x98, 0x31, 0xBF, 0xBD, 0xDB, 0x32, 0xEA, 0xDE);
DEFINE_GUID(RBAMODOTV, 0xEB77B813L, 0xDF7D, 0x474D, 0x8B, 0x3D, 0x83, 0x9F, 0x98, 0xA7, 0x40, 0x73);
DEFINE_GUID(RBATRILINEAR,0xF9B52B69,0xBE9A,0x4804,0xA9,0x6F,0xB2,0x6C,0x9C,0x0B,0x58,0xB9);
DEFINE_GUID(RBATFT,      0x047EBBCE,0xEB72,0x48BD,0xAA,0xA1,0xB2,0xF0,0xB8,0xB1,0x9A,0x64);

//---------------------------------------------------------------------------
static VIDEO2 vs;
extern u8 nSkipMaster;
//---------------------------------------------------------------------------
VideoPlug::VideoPlug() : PlugIn()
{
   wType = PIT_VIDEO;
}
//---------------------------------------------------------------------------
BOOL VideoPlug::Run(unsigned short *InBuffer,unsigned short *OutBuffer)
{
   VIDEOAUDIO s;

   if(!bEnable)
       return TRUE;
   if(pRunFunc == NULL)
       return FALSE;
   if(!IsAttribute(PIT_VIDEOAUDIO))
       return ((LPVIDEOPLUGRUN)pRunFunc)((WORD)index,InBuffer,OutBuffer);
   s.video.InBuffer = InBuffer;
   s.video.OutBuffer = OutBuffer;
   s.audio.mem2 = (LPVOID)-1;
   s.audio.dwSize2 = (DWORD)-1;
   return ((LPVIDEOAUDIOPLUGRUN)pRunFunc)((WORD)index,&s);
}
//---------------------------------------------------------------------------
BOOL VideoPlug::InitPlugInInfo(LPPLUGININFO p,DWORD dwState,DWORD dwStateMask)
{
   if(!PlugIn::InitPlugInInfo(p,dwState,dwStateMask))
       return FALSE;
   vs.mode = lcd.BlitMode;
   vs.pCanvas = vs.mode == BM_GDI ? (LPVOID)lcd.hdcWin : (LPVOID)lcd.lpDDSPrimary;
   vs.x = lcd.rc.left;
   vs.y = lcd.rc.top;
   vs.cx = lcd.rc.right - vs.x;
   vs.cy = lcd.rc.bottom - vs.y;
   vs.frameSkip = nSkipMaster;
   p->lParam = (LPARAM)&vs;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL VideoPlug::Enable(BOOL bFlag)
{
   PlugIn *p;
   BOOL res;

   if((res = PlugIn::Enable(bFlag)) && (dwFlags & PIT_ENABLERUN) && bFlag)
   	Run(NULL,NULL);
   if(!IsAttribute(PIT_VIDEOAUDIO))
       return res;
   p = pPlugInContainer->GetAudioPlugInList()->GetItemFromGUID(&guid);
   if(p != NULL)
       res = p->Enable(bFlag);
   return res;
}
//---------------------------------------------------------------------------
Blur::Blur() : VideoPlug()
{
   guid = RBABLUR;
}
//---------------------------------------------------------------------------
BOOL Blur::Run(unsigned short *InBuffer,unsigned short *OutBuffer)
{
   unsigned short i;
   int r,g,b,n;

   if(!bEnable)
       return TRUE;
   if(InBuffer == NULL || OutBuffer == NULL)
       return FALSE;
   for(i=0;i<38400;i++){
       n = *InBuffer++;
       r = (n >> 10) & 0x1F;
       g = (n >> 5) & 0x1F;
       b = n & 0x1F;
       if(i > 239){
           n = *(InBuffer-2);
           r += (n >> 10) & 0x1F;
           g += (n >> 5) & 0x1F;
           b += n & 0x1F;
           n = *InBuffer;
           r += (n >> 10) & 0x1F;
           g += (n >> 5) & 0x1F;
           b += n & 0x1F;
           n = *(InBuffer-241);
           r = (r + ((n >> 10) & 0x1F)) >> 2;
           g = (g + ((n >> 5) & 0x1F)) >> 2;
           b = (b + (n & 0x1F)) >> 2;
       }
       *OutBuffer++ = (USHORT)((r << 10) | (g << 5) | b);
   }
   return TRUE;
}
//---------------------------------------------------------------------------
Bilinear::Bilinear() : VideoPlug()
{
   guid = RBABILINEAR;
}
//---------------------------------------------------------------------------
BOOL Bilinear::Run(unsigned short *InBuffer,unsigned short *OutBuffer)
{
   USHORT i,n;
   int r,g,b;

   if(!bEnable)
       return TRUE;
   for(i=0;i<38400;i++){
       if(i > 239 && i < 38160){
           n = *(InBuffer-1);
           r = (n >> 10) & 0x1F;
           g = (n >> 5) & 0x1F;
           b = n & 0x1F;
           n = *(InBuffer+1);
           r += (n >> 10) & 0x1F;
           g += (n >> 5) & 0x1F;
           b += n & 0x1F;
           n = *(InBuffer+240);
           r += (n >> 10) & 0x1F;
           g += (n >> 5) & 0x1F;
           b += n & 0x1F;
           n = *(InBuffer-240);
           r = (r + ((n >> 10) & 0x1F)) >> 2;
           g = (g + ((n >> 5) & 0x1F)) >> 2;
           b = (b + (n & 0x1F)) >> 2;
       }
       else{
           n = *InBuffer;
           r = (n >> 10) & 0x1F;
           g = (n >> 5) & 0x1F;
           b = n & 0x1F;
       }
       n = *InBuffer++;
       r = (r + ((n >> 10) & 0x1F)) >> 1;
       g = (g + ((n >> 5) & 0x1F)) >> 1;
       b = (b + (n & 0x1F)) >> 1;
       *OutBuffer++ = (USHORT)((r << 10) | (g << 5) | b);
   }
   return TRUE;
}
//---------------------------------------------------------------------------
Trilinear::Trilinear() : VideoPlug()
{
   guid = RBATRILINEAR;
}
//---------------------------------------------------------------------------
BOOL Trilinear::Run(unsigned short *InBuffer,unsigned short *OutBuffer)
{
   USHORT i,n;
   int r,g,b;

   if(!bEnable)
       return TRUE;
   for(i=0;i<38400;i++){
       if(i > 241 && i < 38159){
           n = *(InBuffer-1);
           r = (n >> 10) & 0x1F;
           g = (n >> 5) & 0x1F;
           b = n & 0x1F;
           n = *(InBuffer+1);
           r += (n >> 10) & 0x1F;
           g += (n >> 5) & 0x1F;
           b += n & 0x1F;
           n = *(InBuffer+240);
           r += (n >> 10) & 0x1F;
           g += (n >> 5) & 0x1F;
           b += n & 0x1F;
           n = *(InBuffer-240);
           r += (n >> 10) & 0x1F;
           g += (n >> 5) & 0x1F;
           b += n & 0x1F;
           n = *(InBuffer-241);
           r += (n >> 10) & 0x1F;
           g += (n >> 5) & 0x1F;
           b += n & 0x1F;
           n = *(InBuffer+241);
           r += (n >> 10) & 0x1F;
           g += (n >> 5) & 0x1F;
           b += n & 0x1F;
           n = *(InBuffer+239);
           r += (n >> 10) & 0x1F;
           g += (n >> 5) & 0x1F;
           b += n & 0x1F;
           n = *(InBuffer-239);
           r = (r + ((n >> 10) & 0x1F)) >> 3;
           g = (g + ((n >> 5) & 0x1F)) >> 3;
           b = (b + (n & 0x1F)) >> 3;
       }
       else{
           n = *InBuffer;
           r = (n >> 10) & 0x1F;
           g = (n >> 5) & 0x1F;
           b = n & 0x1F;
       }
       n = *InBuffer++;
       r = (r + ((n >> 10) & 0x1F)) >> 1;
       g = (g + ((n >> 5) & 0x1F)) >> 1;
       b = (b + (n & 0x1F)) >> 1;
       *OutBuffer++ = (USHORT)((r << 10) | (g << 5) | b);
   }
   return TRUE;
}
//---------------------------------------------------------------------------
MotionBlur::MotionBlur() : VideoPlug()
{
   guid = RBAMOTIONBLUR;
}
//---------------------------------------------------------------------------
BOOL MotionBlur::Run(unsigned short *InBuffer,unsigned short *OutBuffer)
{
   USHORT y,n;
   BYTE x,i;
   int xx,r,g,b;

   if(!bEnable)
       return TRUE;
   if(InBuffer == NULL || OutBuffer == NULL)
       return FALSE;
   for(y=0;y<160*240;y += (USHORT)240){
       for(x=0;x<240;x++){
           xx = x;
           r = g = b = 0;
           for(i=0;i<2;){
               n = InBuffer[y+xx];
               r += (n >> 10) & 0x1F;
               g += (n >> 5) & 0x1F;
               b += n & 0x1F;
               i++;
               xx++;
               if(xx < 0 || xx >= 240)
                   break;
           }
           switch(i){
               case 0:
                   n = InBuffer[y+xx];
               break;
               case 1:
                   n = (USHORT)((r << 10) | (g << 5) | b);
               break;
               case 2:
                   r >>= 1;
                   g >>= 1;
                   b >>= 1;
                   n = (USHORT)((r << 10) | (g << 5) | b);
               break;
           }
           *OutBuffer++ = (USHORT)n;
       }
   }
   return TRUE;
}
//---------------------------------------------------------------------------
ModoTV::ModoTV() : VideoPlug()
{
   guid = RBAMODOTV;
}
//---------------------------------------------------------------------------
BOOL ModoTV::Run(unsigned short *InBuffer,unsigned short *OutBuffer)
{
   int r,g,b;
   BYTE y,x;
   USHORT n;

   if(!bEnable)
       return TRUE;
   if(InBuffer == NULL || OutBuffer == NULL)
       return FALSE;
   for(y = 0;y<160;y++){
       if((y & 1)){
           for(x=0;x<240;x++,InBuffer++){
               n = *InBuffer;
               if((r = ((n >> 10) & 0x1F) - 2) < 0)
                   r = 0;
               if((g = ((n >> 5) & 0x1F) - 2) < 0)
                   g = 0;
               if((b = (n & 0x1F) - 2) < 0)
                   b = 0;
               *OutBuffer++ = (USHORT)((r << 10) | (g << 5) | b);
           }
       }
       else{
           for(x=0;x<120;x++)
               *((LPDWORD)OutBuffer)++ = *((LPDWORD)InBuffer)++;
       }
   }
   return TRUE;
}
//---------------------------------------------------------------------------
TFT::TFT() : VideoPlug()
{
   guid = RBATFT;
}
//---------------------------------------------------------------------------
BOOL TFT::Run(unsigned short *InBuffer,unsigned short *OutBuffer)
{
   int r,g,b;
   BYTE y,x;
   USHORT n;

   if(!bEnable)
       return TRUE;
   if(InBuffer == NULL || OutBuffer == NULL)
       return FALSE;
   for(y = 0;y<160;y++){
       for(x=0;x<240;x++,InBuffer++){
           n = *InBuffer;
           if((r = ((n >> 10) & 0x1F) - 6) < 0)
               r = 0;
           if((g = ((n >> 5) & 0x1F) - 6) < 0)
               g = 0;
           if((b = (n & 0x1F) - 6) < 0)
               b = 0;
           *OutBuffer++ = (USHORT)((r << 10) | (g << 5) | b);
       }
   }
   return TRUE;
}
//---------------------------------------------------------------------------
VideoPlugList::VideoPlugList() : PlugInList("VideoFilter")
{
}
//---------------------------------------------------------------------------
VideoPlug *VideoPlugList::BuildPlugIn(char *path)
{
   VideoPlug *p;

   if(path == NULL || (p = new VideoPlug()) == NULL)
       return NULL;
   p->SetLibraryPath(path);
   return p;
}
//---------------------------------------------------------------------------
BOOL VideoPlugList::PreLoad(WORD *wID)
{
   VideoPlug *pVideoPlug;
   PLUGININFO pi;

   *wID = ID_VIDEO_PLUG_START;
   pVideoPlug = new Blur();
   if(pVideoPlug != NULL){
       ZeroMemory(&pi,sizeof(PLUGININFO));
       pi.cbSize = sizeof(PLUGININFO);
       pi.wID = (*wID)++;
       if(!pVideoPlug->SetInfo(&pi)){
           (*wID)--;
           delete pVideoPlug;
       }
       else if(!Add(pVideoPlug)){
           (*wID)--;
           delete pVideoPlug;
       }
   }
   pVideoPlug = new Bilinear();
   if(pVideoPlug != NULL){
       ZeroMemory(&pi,sizeof(PLUGININFO));
       pi.cbSize = sizeof(PLUGININFO);
       pi.wID = (*wID)++;
       if(!pVideoPlug->SetInfo(&pi)){
           (*wID)--;
           delete pVideoPlug;
       }
       else if(!Add(pVideoPlug)){
           (*wID)--;
           delete pVideoPlug;
       }
   }
   pVideoPlug = new MotionBlur();
   if(pVideoPlug != NULL){
       ZeroMemory(&pi,sizeof(PLUGININFO));
       pi.cbSize = sizeof(PLUGININFO);
       pi.wID = (*wID)++;
       if(!pVideoPlug->SetInfo(&pi)){
           (*wID)--;
           delete pVideoPlug;
       }
       else if(!Add(pVideoPlug)){
           (*wID)--;
           delete pVideoPlug;
       }
   }
   pVideoPlug = new ModoTV();
   if(pVideoPlug != NULL){
       ZeroMemory(&pi,sizeof(PLUGININFO));
       pi.cbSize = sizeof(PLUGININFO);
       pi.wID = (*wID)++;
       if(!pVideoPlug->SetInfo(&pi))
           delete pVideoPlug;
       else if(!Add(pVideoPlug))
           delete pVideoPlug;
   }
   pVideoPlug = new Trilinear();
   if(pVideoPlug != NULL){
       ZeroMemory(&pi,sizeof(PLUGININFO));
       pi.cbSize = sizeof(PLUGININFO);
       pi.wID = (*wID)++;
       if(!pVideoPlug->SetInfo(&pi)){
           (*wID)--;
           delete pVideoPlug;
       }
       else if(!Add(pVideoPlug)){
           (*wID)--;
           delete pVideoPlug;
       }
   }
   pVideoPlug = new TFT();
   if(pVideoPlug != NULL){
       ZeroMemory(&pi,sizeof(PLUGININFO));
       pi.cbSize = sizeof(PLUGININFO);
       pi.wID = (*wID)++;
       if(!pVideoPlug->SetInfo(&pi)){
           (*wID)--;
           delete pVideoPlug;
       }
       else if(!Add(pVideoPlug)){
           (*wID)--;
           delete pVideoPlug;
       }
   }
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL VideoPlugList::Add(PlugIn *ele)
{
   AudioPlug *p;
   GUID guid;

   p = NULL;
   if(ele->IsAttribute(PIT_VIDEOAUDIO)){
       if((p = new AudioPlug(pcm.hz,pcm.bitsPerSample)) == NULL)
           return FALSE;
       ele->GetGuid(&guid);
       p->OrAttribute(0x8000|PIT_VIDEOAUDIO);
       p->SetGuid(&guid);
       p->SetRunFunc(ele->GetRunFunc());
       if(pPlugInContainer == NULL || pPlugInContainer->GetAudioPlugInList() == NULL ||
           !pPlugInContainer->GetAudioPlugInList()->Add(p)){
               delete p;
               return FALSE;
       }
   }
   if(!PlugInList::Add(ele)){
       if(p != NULL){
           pPlugInContainer->GetAudioPlugInList()->Delete(IndexFromEle(p));
           delete p;
       }
       return FALSE;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
int VideoPlugList::Run(unsigned short *InBuffer,unsigned short *OutBuffer)
{
   int i;
   elem_list *p;

   if(nCount == 0)
       return -1;
   i = 0;
   p = First;
   do{
       if(((PlugIn *)p->Ele)->IsEnable() && !((PlugIn *)p->Ele)->IsAttribute(PIT_ENABLERUN)){
           if(!((VideoPlug *)p->Ele)->Run(InBuffer,OutBuffer))
               continue;
           else{
               if(!((PlugIn *)p->Ele)->IsAttribute(PIT_NOMODIFY))
                   i++;
               if(((PlugIn *)p->Ele)->IsAttribute(PIT_BITBLT))
                   i |= 0x10000000;
           }
       }
   }while((p = p->Next) != NULL);
   return i;
}


