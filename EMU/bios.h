#include "gbaemu.h"

#ifndef biosH
#define biosH

typedef struct
{
   u16 adress;
   u8  mode;
} BIOSFUNC;


#ifdef __cplusplus
extern "C" {
#endif
void bios(u16 func);
u8 biosEmulation(u8 func);
BOOL ResetBios(char *lpNameFile);
int GetBiosSubCodeError();
BOOL UseBiosFile(int flag);
int GetBiosFileName(char *dst,int len);
BOOL SkipBiosIntro(int flag);
void SetUseBios(BOOL flag);
void SetSkipBiosIntro(BOOL flag);
void ResetBiosParam();
void SetBiosFileName(char *lpFileName);
BOOL CheckIntrWait();

#ifdef __cplusplus
}
#endif

#endif
