#include <windows.h>

//---------------------------------------------------------------------------
#ifndef Unit1H
#define Unit1H
//---------------------------------------------------------------------------
#define PM_KEYPRESSED WM_USER + 3333
//---------------------------------------------------------------------------
void RunDragDrop(int x,int y);
int ParseCommandLine(char *pCommandLine);
BOOL LoadConfig();
BOOL SaveConfig();
BOOL ResizeWindow(u8 mode);
void ChangeFrameBuffer(u8 mode);
int SetAutoStart(int value);
BOOL RegisterRBAClass(HINSTANCE this_inst);
void UnregisterRBAClass(HINSTANCE this_inst);
BOOL OnRButtonDown();
void ResetMainPlugInMenu(HMENU hMenu);
void OnApplyIPSPatch();
void UpdateStatusBar(char *string,int index);
void OnPreResizeWindow(LPRECT rc2);
void OnResizeWindow();
BOOL LoadBios();
void GetRBAWindowRect(LPRECT p);
BOOL OnRButtonDownStatusBar();

#endif
 