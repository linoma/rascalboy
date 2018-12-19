#include <windows.h>
#include "list.h"
#include "plugin.h"

//---------------------------------------------------------------------------
#ifndef videoplugH
#define videoplugH
//---------------------------------------------------------------------------
class VideoPlug : public PlugIn
{
public:
   VideoPlug();
   BOOL InitPlugInInfo(LPPLUGININFO p,DWORD dwState,DWORD dwStateMask);
   virtual BOOL Run(unsigned short *InBuffer,unsigned short *OutBuffer);
   BOOL Enable(BOOL bFlag);
};
//---------------------------------------------------------------------------
class Blur : public VideoPlug
{
public:
   Blur();
   BOOL Run(unsigned short *InBuffer,unsigned short *OutBuffer);
};
//---------------------------------------------------------------------------
class Bilinear : public VideoPlug
{
public:
   Bilinear();
   BOOL Run(unsigned short *InBuffer,unsigned short *OutBuffer);
};
//---------------------------------------------------------------------------
class Trilinear : public VideoPlug
{
public:
   Trilinear();
   BOOL Run(unsigned short *InBuffer,unsigned short *OutBuffer);
};
//---------------------------------------------------------------------------
class MotionBlur : public VideoPlug
{
public:
   MotionBlur();
   BOOL Run(unsigned short *InBuffer,unsigned short *OutBuffer);
};
//---------------------------------------------------------------------------
class ModoTV : public VideoPlug
{
public:
   ModoTV();
   BOOL Run(unsigned short *InBuffer,unsigned short *OutBuffer);
};
//---------------------------------------------------------------------------
class TFT : public VideoPlug
{
public:
   TFT();
   BOOL Run(unsigned short *InBuffer,unsigned short *OutBuffer);
};
//---------------------------------------------------------------------------
class VideoPlugList : public PlugInList
{
public:
   VideoPlugList();
   BOOL PreLoad(WORD *wID);
   VideoPlug *BuildPlugIn(char *path);
   BOOL Add(PlugIn *ele);
   int Run(unsigned short *InBuffer,unsigned short *OutBuffer);
protected:
   VIDEO2 vs;
};


#endif
