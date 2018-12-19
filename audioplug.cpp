#define INITGUID
//---------------------------------------------------------------------------
#include <windows.h>
#pragma hdrstop

#include "audioplug.h"
#include "resource.h"
#include "cpu.h"
#include <math.h>

DEFINE_GUID(RBAECHO, 0x0BA28251L, 0xB235, 0x4B87, 0x83, 0x03, 0x5A, 0x15, 0xFB, 0x2E, 0xBA, 0x9C);
DEFINE_GUID(RBABASS, 0xC2933BDEL, 0xA023, 0x4F2D, 0x85, 0xAF, 0x5E, 0x2B, 0xF6, 0xA0, 0xB8, 0x1D);

//---------------------------------------------------------------------------
AudioPlug::AudioPlug(int sr,int bs) : PlugIn()
{
   wType = PIT_AUDIO;
   sampleRate = sr;
   bitsSample = bs;
}
//---------------------------------------------------------------------------
BOOL AudioPlug::Enable(BOOL bFlag)
{
	if(!PlugIn::Enable(bFlag))
   	return FALSE;
   if(bFlag && (dwFlags & PIT_ENABLERUN))
   	Run(NULL,0,NULL,0);
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL AudioPlug::Run(LPVOID mem1,DWORD dwSize1,LPVOID mem2,DWORD dwSize2)
{
   VIDEOAUDIO s;

   if(!bEnable)
       return TRUE;
   if(pRunFunc == NULL)
       return FALSE;
   if(!IsAttribute(PIT_VIDEOAUDIO))
       return ((LPAUDIOPLUGRUN)pRunFunc)((WORD)index,mem1,dwSize1,mem2,dwSize2);
   s.audio.mem1 = mem1;
   s.audio.dwSize1 = dwSize1;
   s.audio.mem2 = mem2;
   s.audio.dwSize2 = dwSize2;
   return ((LPVIDEOAUDIOPLUGRUN)pRunFunc)((WORD)index,&s);
}
//---------------------------------------------------------------------------
AudioPlugList::AudioPlugList() : PlugInList("AudioFilter")
{
}
//---------------------------------------------------------------------------
BOOL AudioPlugList::PreLoad(WORD *wID)
{
   AudioPlug *p;
   PLUGININFO pi;

   *wID = ID_SOUND_PLUG_START;
   p = new Echo(pcm.hz,pcm.bitsPerSample,70);
   if(p != NULL){
       ZeroMemory(&pi,sizeof(PLUGININFO));
       pi.cbSize = sizeof(PLUGININFO);
       pi.wID = (*wID)++;
       if(!p->SetInfo(&pi)){
           (*wID)--;
           delete p;
       }
       else if(!LList::Add((LPVOID)p)){
           (*wID)--;
           delete p;
       }
   }
   if((p = new SuperBass(pcm.hz,pcm.bitsPerSample)) != NULL){
       ZeroMemory(&pi,sizeof(PLUGININFO));
       pi.cbSize = sizeof(PLUGININFO);
       pi.wID = (*wID)++;
       if(!p->SetInfo(&pi)){
           (*wID)--;
           delete p;
       }
       else if(!LList::Add((LPVOID)p)){
           (*wID)--;
           delete p;
       }
   }
   return TRUE;
}
//---------------------------------------------------------------------------
AudioPlug *AudioPlugList::BuildPlugIn(char *path)
{
   AudioPlug *p;

   if(path == NULL || (p = new AudioPlug(pcm.hz,pcm.bitsPerSample)) == NULL)
       return NULL;
   p->SetLibraryPath(path);
   return p;
}
//---------------------------------------------------------------------------
int AudioPlugList::Run(LPVOID mem1,DWORD dwSize1,LPVOID mem2,DWORD dwSize2)
{
   int i;
   elem_list *p;

   if(nCount == 0)
       return -1;
   i = 0;
   p = First;
   do{
       if(((PlugIn *)p->Ele)->IsEnable() && !((PlugIn *)p->Ele)->IsAttribute(PIT_ENABLERUN)){
           if(!((AudioPlug *)p->Ele)->Run(mem1,dwSize1,mem2,dwSize2))
               break;
           else
               i++;
       }
   }while((p = p->Next) != NULL);
   return i;
}
//---------------------------------------------------------------------------
SuperBass::SuperBass(int sr,int bs) : AudioPlug(sr,bs)
{
   guid = RBABASS;

   z[0] = 1.0453;
   z[1] = -1.9570;
   z[2] = 0.9129;

   z[3] = -1.9570;
   z[4] = 0.9581;
}
//---------------------------------------------------------------------------
BOOL SuperBass::Reset()
{
   int i,i1;

   for(i=0;i<2;i++){
       for(i1=0;i1<3;i1++){
           in[i][i1] = 0;
           out[i][i1] = 0;
       }
   }
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL SuperBass::Run(LPVOID mem1,DWORD dwSize1,LPVOID mem2,DWORD dwSize2)
{
   int value[2],i,i1,*iv,ch,ret;
   LPBYTE l,r;
   float d,*ov;

   if(!bEnable)
       return TRUE;
   l = (LPBYTE)mem1;
   r = l + 1;
   i = (int)dwSize1;
   for(i1=0;i1<2;i1++){
       for(;i > 0 ;i-=2,l += 2,r += 2){
           value[0] = (int)(char)(*l + 128);
           value[1] = (int)(char)(*r + 128);
           for(ch = 0;ch < 2;ch++){
               iv = in[ch];
               ov = out[ch];
               iv[2] = iv[1];
               iv[1] = iv[0];
               iv[0] = value[ch];

               d = iv[0] * z[0] + iv[1] * z[1] + iv[2] * z[2] - (ov[0] * z[3] + ov[1] * z[4]);
               ret = (int)d;
               ov[1] = ov[0];
               ov[0] = d;
               if(ret > 127)
                   ret = 127;
               else if(ret < -128)
                   ret = -128;
               value[ch] = ret;
           }
           *l = (BYTE)((BYTE)value[0] - 128);
           *r = (BYTE)((BYTE)value[1] - 128);
       }
       if((l = (LPBYTE)mem2) == NULL)
           break;
       r = l + 1;
       i = (int)dwSize2;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
Echo::Echo(int sr,int bs,int ms) : AudioPlug(sr,bs)
{
   int i;

   guid = RBAECHO;
   pLeftBuffer = pRightBuffer = NULL;
   delay = ms;
   maxPos = posBuffer = posSBuffer = 0;
   i = (int)floor((sampleRate * ms / 1000.0) * (bitsSample >> 3)) * 2;
   pLeftBuffer = new BYTE[i * 2];
   if(pLeftBuffer == NULL)
       return;
   pRightBuffer = pLeftBuffer + i;
   maxPos = i - 1;
   posBuffer = maxPos >> 1;
   bDynamic = 1;
}
//---------------------------------------------------------------------------
BOOL Echo::Destroy()
{
   if(pLeftBuffer != NULL)
       delete []pLeftBuffer;
   pLeftBuffer = pRightBuffer = NULL;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL Echo::Reset()
{
   if(pLeftBuffer == NULL)
       return FALSE;
   ZeroMemory(pLeftBuffer,maxPos * 2);
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL Echo::Run(LPVOID mem1,DWORD dwSize1,LPVOID mem2,DWORD dwSize2)
{
   int sl,sr,revValue[2],i,i1;
   LPBYTE l,r;

   if(!bEnable)
       return TRUE;
   if(pLeftBuffer == NULL)
       return FALSE;

   l = (LPBYTE)mem1;
   r = l + 1;
   i = (int)dwSize1;
   for(i1=0;i1<2;i1++){
       for(;i > 0 ;i-=2,l += 2,r += 2){
           revValue[0] = (char)pLeftBuffer[posBuffer] * 80 / 100;
           revValue[1] = (char)pRightBuffer[posBuffer++] * 80 / 100;
           if(posBuffer >= maxPos)
               posBuffer = 0;
           pLeftBuffer[posSBuffer] = (BYTE)((sl = (char)(*l + 128)));
           pRightBuffer[posSBuffer++] = (BYTE)((sr = (char)(*r + 128)));
           if(posSBuffer >= maxPos)
               posSBuffer = 0;
           if((sl += revValue[0]) > 127)
               sl = 127;
           else if(sl < -128)
               sl = -128;
           if((sr += revValue[1]) > 127)
               sr = 127;
           else if(sr < -128)
               sr = -128;
           *l = (BYTE)(sl - 128);
           *r = (BYTE)(sr - 128);
       }
       if((l = (LPBYTE)mem2) == NULL)
           break;
       r = l + 1;
       i = (int)dwSize2;
   }
   return TRUE;
}
