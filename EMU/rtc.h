#include <windows.h>
#include "cpu.h"
//---------------------------------------------------------------------------
#ifndef rtcH
#define rtcH
//---------------------------------------------------------------------------
typedef struct{
   u8 REG_ENABLE,REG_DATA,REG_RW;
   u8 command;
   int dataLen;
   int bits;
   int state;
   u8 data[12];
} RTC,*LPRTC;
//---------------------------------------------------------------------------
extern RTC rtcData;
void EnableRTC(BOOL flag);
void InitRTC();
void rtcWrite(u32 adress,u32 data);
//---------------------------------------------------------------------------
#endif
