#include "gbaemu.h"

#ifndef INPUTGBAH
#define INPUTGBAH

typedef struct MYDATA {
    LONG  lX;
    LONG  lY;
    BYTE  rgbButtons[8];
} MYDATA;

#ifdef __cplusplus
extern "C" {
#endif

u8 InitInput();
void DestroyInput();
void Acquire();
void InputActivateWindow(WPARAM wParam,LPARAM lParam);
int GetInputSubCodeError();
void OnKeyConfig();
char *GetKeyConfig();
void SetKeyConfig(char *src);
extern short KeyField;
int OnKeyPressed(WPARAM wParam,LPARAM lParam);

#ifdef __cplusplus
}
#endif

#endif











