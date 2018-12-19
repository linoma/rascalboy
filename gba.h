#include <windows.h>
#include <commctrl.h>
#include "cpu.h"

#ifndef gbaH
#define gbaH

extern HWND hWin;
extern HINSTANCE hInstance;
extern BOOL bThreadRun,bQuit,bPause;

#define TE_LCD     0x0
#define TE_MAIN    0x1
#define TE_INPUT   0x2
#define TE_BIOS    0x3
#define TE_SOUND   0x4
#define TE_PLUGIN  0x5
#define TE_CHEAT   0x6

#define POS_SM_SAVESTATE       2
#define POS_SM_SAVESTATE_LOAD  8
#define POS_SM_SAVESTATE_SAVE  9
#define POS_SM_LANGUAGE        8
#define POS_SM_FILESRECENT     9
#define POS_SM_BACKUP          11
#define POS_SM_SIOPLUGIN       3
#define POS_SM_AUDIOPLUGIN     7
#define POS_SM_VIDEOPLUGIN    12

typedef BOOL WINAPI (*LPFNCSAVEDLG)(LPOPENFILENAME);

#ifdef __cplusplus
extern "C" {
#endif

void SetSyncro(u8 value);
BOOL IsSyncroEnabled();
static BOOL InitApp(HINSTANCE this_inst);
void CleanUpAndPostQuit(void);
void ProcessaMessaggi();
BOOL LoadRom(char *pFileName);
void UpdateStatusBar(char *string,int index);
void ShowMessageError(u8 Section,...);
int MenuStringToString(char *string);
void SetSubCodeError(int err);
void DrawProgressBar(DWORD pos);
void VerifyFile();
BOOL ShowOpenDialog(char *lpstrTitle,char *lpstrFileName,char *lpstrFilter,HWND hwnd);
BOOL ShowSaveDialog(char *lpstrTitle,char *lpstrFileName,char *lpstrFilter,HWND hwnd,LPFNCSAVEDLG pFnc);

#ifdef __cplusplus
}
#endif

#endif