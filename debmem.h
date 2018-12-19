#include <windows.h>
#include "cpu.h"

//---------------------------------------------------------------------------
#ifndef debmemH
#define debmemH
//---------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

u8 CreateDebugMemory();
void BringDebugMemoryToTop(HWND win);
void UpdateDebugMemoryWindow();
void InitDebugMemoryWindow();
void DestroyDebugMemoryWindow();
int MemoryStringToIndex(char *string);
int MemoryAddressToIndex(DWORD dwAddress);
void UpdateVertScrollBarMemoryDump(DWORD address,int index);
u32 ReadMem(u32 address,u8 mode);
void WriteMem(u32 adress,u8 mode,u32 data);
void UpdateEditMemoryValue(DWORD dwCurrentValue,BOOL bValue,BOOL bReset);

#ifdef __cplusplus
}
#endif

#endif
