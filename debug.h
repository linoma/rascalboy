#include <windows.h>

#ifdef __cplusplus

#include "lstring.h"
#include "list.h"

#endif

#ifndef _DEBUGGBAH_
#define _DEBUGGBAH_

#define UM_UPDATE                  WM_USER + 5000

#define TM_BREAKPOINT              2000
#define TM_STATUSBAR               2001

#define BREAKPOINT_VERSION         "BREAK120"
#define BREAKPOINT_VERSION_LENGHT  sizeof(BREAKPOINT_VERSION)

#define BT_PROGRAM     0
#define BT_MEMORY      1

#define DBV_RUN    0
#define DBV_VIEW   1
#define DBV_NULL   0xFF

#define MAX_BREAK          40
#define MAX_MESSAGE        20
#define MAX_BREAKON        40

#define MESSAGE_IRQ        0
#define MESSAGE_DMA        1
#define MESSAGE_SWI        2
#define MESSAGE_SWI5       3
#define MESSAGE_POWER      4
#define MESSAGE_TIMER      5
#define MESSAGE_CPU        8
#define MESSAGE_DMA0       9
#define MESSAGE_DMA1       10
#define MESSAGE_DMA2       11
#define MESSAGE_DMA3       12
#define MESSAGE_MEMORY     13


#define BREAK_START_DMA    1
#define BREAK_ENTER_IRQ    2
#define BREAK_EXIT_IRQ     3
#define BREAK_ENTER_IRQ1   4
#define BREAK_ENTER_IRQ2   5
#define BREAK_ENTER_IRQ3   6
#define BREAK_ENTER_IRQ4   7
#define BREAK_ENTER_IRQ5   8
#define BREAK_ENTER_IRQ6   9
#define BREAK_ENTER_IRQ7   10
#define BREAK_ENTER_IRQ8   11
#define BREAK_ENTER_IRQ9   12
#define BREAK_ENTER_IRQ10  13
#define BREAK_ENTER_IRQ11  14
#define BREAK_ENTER_IRQ12  15
#define BREAK_ENTER_IRQ13  16
#define BREAK_ENTER_IRQ14  17
#define BREAK_EXIT_IRQ1    18
#define BREAK_EXIT_IRQ2    19
#define BREAK_EXIT_IRQ3    20
#define BREAK_EXIT_IRQ4    21
#define BREAK_EXIT_IRQ5    22
#define BREAK_EXIT_IRQ6    23
#define BREAK_EXIT_IRQ7    24
#define BREAK_EXIT_IRQ8    25
#define BREAK_EXIT_IRQ9    26
#define BREAK_EXIT_IRQ10   27
#define BREAK_EXIT_IRQ11   28
#define BREAK_EXIT_IRQ12   29
#define BREAK_EXIT_IRQ13   30
#define BREAK_EXIT_IRQ14   31
#define BREAK_ENTER_SWI    32

#define CC_EQ      1
#define CC_NE      2
#define CC_GT      3
#define CC_GE      4
#define CC_LT      5
#define CC_LE      6

typedef void (*DECODEARM)(u32 op, u32 adress, char *dest);
typedef void (*DECODETHUMB)(u16 op, u32 adress, char *dest);

typedef struct
{
   char Name[30];
   DWORD Address;
   DWORD Size;
   BYTE  AccessMode;
   WORD  idMemoryInfo;
   BYTE  defAccessMode;
} MEMORYINFO;

typedef struct breakpoint
{
   u32 adress;
   u8 bEnable;
   u32 PassCount,PassCountCondition;
   char Condition[30];
   u8 Type;
} BREAKPOINT,* LPBREAKPOINT;

typedef struct _dispointer{
   u32 StartAddress;
   u32 EndAddress;
} DISVIEW,*LPDISVIEW;

typedef struct {
   u32 yScroll;
   DISVIEW views[2];
   u8 iMaxItem,bTrackMode,iCurrentView,bCurrentTrackMode;
} DEBUGGER;

BOOL CALLBACK DialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK DlgProcBRK(HWND hwndDlg,UINT uMsg,WPARAM,LPARAM lParam);
LRESULT CALLBACK ListBoxMessageWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
LRESULT CALLBACK ListBoxAssemblerWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

#ifdef __cplusplus

typedef struct main_message
{
   char bEnable;
   char bValue[MAX_MESSAGE];
   char bBreak[MAX_BREAKON];
   BREAKPOINT breakpoint[MAX_BREAK];
   u8 nBreak,BreakMode,bPass,bBreakChanged;
   u32 dwBreak;
   LString FileNameBreakList;
   u8 bStack,bExceptions;
} main_message;

extern LRecentFile *pDebugRecentFile;
extern "C" {

#endif

void ControlMemoryBreakPoint(u32 dwCurrentAddress,u8 mode,u32 data);
u8 GetCurrentIncPipe(u8 indexView);
void ExitDebugMode();
void UpdateAddressBar(u32 dwAddress,u8 forceUpdate);
void EnterDebugMode();
DWORD StrToHex(char *string);
u8 IsMessageEnable(u8 message);
u8 DestroyDebug();
u8 InitDebug();
void BringDebugToTop(HWND win);
u8 CreateDebugWindow(u8 mode);
void ResetDebug();
void WriteMessage(u8 message,char *mes,...);
void UpdateRegister(void);
void WaitDebugger(BOOL bThumb);
void ControlBreakPoint(void);
s8 InsertBreakPoint(u32 adress);
u8 SaveBreakPointList();
u8 LoadBreakPointList();
u8 InsertBreapointFromListBox();
WORD GetBreakPointFromPos(int y);
void RefillBreakPointList();
void ControlBreakOn(u8 nBreak);
void EnableBreak(HMENU hMenu,u8 nBreak,WORD wID);
u8 LoadBreakPointListFromName(char *pNome);
u8 CheckStopBreakPoint(u8 Index);
void WriteDebugFile();
void UpdateVertScrollBarDisDump(DWORD address,int index);
void SetGamePakRomSize(u8 index,u32 size);
void EnterDebugMode();
void OnDebugVScroll(WPARAM wParam,LPARAM lParam);
u8 ToBreakPointString(u32 adress,char *string,int maxLen);
int FillListDiss(u32 *p,u8 indexView,u8 nItem);
u8 ConditionToValue(u8 Index,int *rd,u8 *cond,u32 *value);
void InsertStackAddress(u32 address,char *mes,...);
void InsertException(char *mes,...);

#ifdef __cplusplus

LString ConditionToString(u8 Index);
}
extern main_message MAIN_MESSAGE;
#endif

extern BOOL bPass,bDebug,bWriteDebug,bInLoop;
extern HWND hwndDebug,hWndMem;
extern DWORD dwKey;
extern DECODEARM *debug_handles;
extern DECODETHUMB *debug_handles_t;
extern char **opcode_strings_t;
extern char **opcode_strings;
extern char *irq_strings[16];
extern MEMORYINFO MemoryAddress[];
extern UINT SizeMemoryAddressStruct,dwBreakPointItem;
extern DEBUGGER dbg;
extern LPBREAKPOINT listBreakPoint;

extern char *condition_strings[];
extern char *register_strings[];
extern char *shift_strings[];

#endif