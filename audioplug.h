#include <windows.h>
#include "list.h"
#include "plugin.h"
//---------------------------------------------------------------------------
#ifndef audioplugH
#define audioplugH
//---------------------------------------------------------------------------

#define AP_LEFT    0
#define AP_RIGHT   1
//---------------------------------------------------------------------------
class AudioPlug : public PlugIn
{
public:
   AudioPlug(int sr,int bs);
   virtual BOOL Run(LPVOID mem1,DWORD dwSize1,LPVOID mem2,DWORD dwSize2);
   BOOL Enable(BOOL bFlag);
protected:
   int sampleRate,bitsSample;
};
//---------------------------------------------------------------------------
class SuperBass : public AudioPlug
{
public:
   SuperBass(int sr,int bs);
   BOOL Reset();
   BOOL Run(LPVOID mem1,DWORD dwSize1,LPVOID mem2,DWORD dwSize2);
protected:
   float z[5],out[2][3];
   int in[2][3];
};
//---------------------------------------------------------------------------
class Echo : public AudioPlug
{
public:
   Echo(int sr,int bs,int ms);
   BOOL Reset();
   BOOL Run(LPVOID mem1,DWORD dwSize1,LPVOID mem2,DWORD dwSize2);
   BOOL Destroy();
   BOOL PostLoad(){return TRUE;};   
protected:
   LPBYTE pLeftBuffer,pRightBuffer;
   int delay,maxPos,posBuffer,posSBuffer;
};
//---------------------------------------------------------------------------
class AudioPlugList : public PlugInList
{
public:
   AudioPlugList();
   AudioPlug *BuildPlugIn(char *path);
   BOOL PreLoad(WORD *wID);
   int Run(LPVOID mem1,DWORD dwSize1,LPVOID mem2,DWORD dwSize2);
};

extern AudioPlugList *pAudioPlugList;
#endif
