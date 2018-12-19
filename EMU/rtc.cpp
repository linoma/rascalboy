//---------------------------------------------------------------------------
#include <windows.h>
#pragma hdrstop

#include "rtc.h"

//---------------------------------------------------------------------------
RTC rtcData;
static BOOL bEnableRTC = TRUE;
//---------------------------------------------------------------------------
static u8 toBCD(int value)
{
  value = value % 100;
  int l = value % 10;
  int h = value / 10;
  return (u8)((h << 4) + l);
}
//---------------------------------------------------------------------------
void InitRTC()
{
   ZeroMemory(&rtcData,sizeof(RTC));
}
//---------------------------------------------------------------------------
static void writeRegData(u32 data)
{
   SYSTEMTIME st;

   switch(rtcData.state){
       case 0:
           if((data & 5) == 5){
               rtcData.state = 1;
               rtcData.command = 0;
           }
       break;
       case 1:
           if(!(data & 1))
               return;
           rtcData.command |= (u8)(((data & 2) >> 1) << (7 - rtcData.bits));
           rtcData.bits++;
           if(rtcData.bits >= 8) {
               rtcData.bits = 0;
               switch(rtcData.command){
                   case 0x60:
                       rtcData.state = 0;
                       rtcData.bits = 0;
                   break;
                   case 0x62:
                       rtcData.state = 3;
                       rtcData.dataLen = 1;
                   break;
                   case 0x63:
                       rtcData.dataLen = 1;
                       rtcData.data[0] = 0x40;
                       rtcData.state = 2;
                   break;
                   case 0x65:
                       GetLocalTime(&st);

                       rtcData.dataLen = 7;
                       rtcData.data[0] = toBCD(st.wYear);
                       rtcData.data[1] = toBCD(st.wMonth);
                       rtcData.data[2] = toBCD(st.wDay);
                       rtcData.data[3] = toBCD((st.wDayOfWeek == 0 ? 6 : st.wDayOfWeek - 1));
                       rtcData.data[4] = toBCD(st.wHour);
                       rtcData.data[5] = toBCD(st.wMinute);
                       rtcData.data[6] = toBCD(st.wSecond);
                       rtcData.state = 2;//Data ready
                   break;
                   case 0x67:
                       GetLocalTime(&st);
                       rtcData.dataLen = 3;
                       rtcData.data[0] = toBCD(st.wHour);
                       rtcData.data[1] = toBCD(st.wMinute);
                       rtcData.data[2] = toBCD(st.wSecond);
                       rtcData.state = 2;
                   break;
               }
           }
       break;
   }
}
//---------------------------------------------------------------------------
static void readRegData(u32 data)
{
   switch(rtcData.state){
       default:
           rtcData.REG_DATA = (u8)((rtcData.REG_DATA & ~2) |
                           ((rtcData.data[rtcData.bits >> 3] >>
                           (rtcData.bits & 7)) & 1)*2);
           if(!(data & 1) || rtcData.state != 2){
               if((data & 5) == 5){
                   rtcData.state = 1;
                   rtcData.command = 0;
               }
               return;
           }
           rtcData.bits++;
           if(rtcData.bits != 8*rtcData.dataLen)
               return;
           rtcData.bits = 0;
           rtcData.state = 0;
           rtcData.command = 0;
       break;
       case 3:
/*           rtcData.data1[rtcData.bits >> 3] = (u8)(
               (rtcData.data1[rtcData.bits >> 3] >> 1) |
               ((data << 6) & 128));*/
           rtcData.bits++;
           if(rtcData.bits == 8*rtcData.dataLen) {
               rtcData.bits = 0;
               rtcData.state = 0;
           }
       break;
   }
}
//---------------------------------------------------------------------------
void rtcWrite(u32 adress,u32 data)
{
   if(!bEnableRTC)
       return;
   if(adress == 0x80000c8)
       rtcData.REG_ENABLE = (u8)data;
   else if(adress == 0x80000c6)
       rtcData.REG_RW = (u8)data;
   else if(adress == 0x80000c4){
       if(!rtcData.REG_ENABLE)
           return;
       switch(rtcData.REG_RW){
           case 5:  //Mode read
               readRegData(data);
           break;
           default: // Mode write
               writeRegData(data);
           break;
       }
   }
}
//---------------------------------------------------------------------------
void EnableRTC(BOOL flag)
{
   bEnableRTC = flag;
}
