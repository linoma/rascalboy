#include <windows.h>

#ifndef _DEBUGGBA1H_
#define _DEBUGGBA1H_


BOOL CreateDebugToolBar();
void UpdateDebugToolBar();
BOOL CreateDebugStatusBar();
void FillComboBreakPoint();
void UpdateDebugStatusBar(char *string,int index,char activeTimer);
void OnChangePageTab1();
u8 ShowEditProcedure();
int GetBreakpointIndexFromIndex(int index);
void CloseDebugWindow();

extern WNDPROC OldListBoxBreakPointWndProc;
extern HMENU hDebugPopupMenu;
extern HWND hwndDebugStatusBar,hwndDebugToolBar,hwndDebugComboBox1;
extern UINT wTimerBreakPointID;

#endif
