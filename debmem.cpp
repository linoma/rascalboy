//---------------------------------------------------------------------------
#include <windows.h>
#pragma hdrstop

#include "debmem.h"
#include "gba.h"
#include "resource.h"
#include "debug.h"
#include "memory.h"
#include "trad.h"
#include "bios.h"
#include "inputtext.h"
#include "zipfile.h"
#include "lregkey.h"
#include <math.h>

//---------------------------------------------------------------------------
#ifdef _DEBUG
//---------------------------------------------------------------------------
#define MEMFILTERFILE "Memory dump files (*.md)\0*.md\0Tutti i files (*.*)\0*.*\0\0\0\0\0"
//---------------------------------------------------------------------------
HWND hWndMem;
static HWND hwndDebugMemoryStatusBar,hWndIO;
static char *lpszText;
static int yScroll,iCB2DropWidth,bInfoMode = 0;
static WNDPROC oldEditMemoryWndProc;
static LList *pInfoMemList = NULL;
static struct {
   u32 dwSelStart;
   u32 dwSelEnd;
   u16 SelStart;
   u16 SelEnd;
} SEL;
//---------------------------------------------------------------------------
typedef struct _tagInfoMem{
   char adr[9];
   char descr[20];
   char descrex[100];
   char descrbit[456];
} INFOMEM,*LPINFOMEM;
//---------------------------------------------------------------------------
static LPINFOMEM currentInfoMem;
static DWORD dwCurrentAddress,dwCurrentValue,bUpdateIOReg;
//---------------------------------------------------------------------------
static BOOL CALLBACK DlgProcMem(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
static LRESULT CALLBACK EditMemoryWindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
static void OnSelOkComboBox2();
static void OnEditMemory(HWND hwnd,LPPOINT pt);
static void SaveMemoryAddress();
static void LoadMemoryAddress();
static void GotoAddress(char *s);
static BOOL AddressFromPoint(LPPOINT pt,u32 *dwAddress);
static void DisplayInfoMemory(int);
static void UpdateStatusBar(char *string,int index);
extern "C" void FillPalette(u16 index);
//---------------------------------------------------------------------------
#ifdef _DEBPRO
static void OnSelectAllMemory()
{
   int i;

   i = SendDlgItemMessage(hWndMem,IDC_COMBOBOX2,CB_GETCURSEL,0,0);
   SEL.dwSelEnd = SEL.dwSelStart = MemoryAddress[i].Address;
   SEL.dwSelEnd += MemoryAddress[i].Size;
   InvalidateRect(hWndMem,NULL,FALSE);
   UpdateWindow(hWndMem);
}
//---------------------------------------------------------------------------
static void OnPasteMemory()
{
   int i,i2;
   DWORD dw,dw1,dw2;
   HANDLE h;
   LPBYTE p;

   if(SEL.dwSelStart == (u32)-1 || SEL.dwSelEnd == (u32)-1){
       i2 = SendDlgItemMessage(hWndMem,IDC_COMBOBOX3,CB_GETCURSEL,0,0);
       i = SendDlgItemMessage(hWndMem,IDC_COMBOBOX2,CB_GETCURSEL,0,0);
       dw = MemoryAddress[i].Address + yScroll;
       if(i2 != 0)
           dw &= ~((i2 << 1) - 1);
       i = MemoryAddressToIndex(dw);
       if(!(i > -1 && MemoryAddress[i].Size > 1))
           return;
       dw1 = MemoryAddress[i].Address + MemoryAddress[i].Size - 1;
       dw1 -= dw;
   }
   else{
       dw = SEL.dwSelStart;
       dw1 = SEL.dwSelEnd - dw;
   }
   OpenClipboard(hWndMem);
   h = GetClipboardData(CF_GDIOBJFIRST);
   if(h != NULL && (dw2 = GlobalSize(h)) != 0){
       p = (LPBYTE)GlobalLock(h);
       for(;dw1 > 0 && dw2 > 0;dw1--,dw2--)
           WriteMem(dw++,AMM_BYTE,*p++);
       GlobalUnlock(h);
   }
   CloseClipboard();
   InvalidateRect(hWndMem,NULL,FALSE);
   UpdateWindow(hWndMem);
}
//---------------------------------------------------------------------------
static void OnCopyMemory()
{
   int i,i2;
   DWORD dw,dw1;
   HANDLE h;
   LPBYTE p;
   u8 value;
   
   if(SEL.dwSelStart == (u32)-1 || SEL.dwSelEnd == (u32)-1){
       i2 = SendDlgItemMessage(hWndMem,IDC_COMBOBOX3,CB_GETCURSEL,0,0);
       i = SendDlgItemMessage(hWndMem,IDC_COMBOBOX2,CB_GETCURSEL,0,0);
       dw = MemoryAddress[i].Address + yScroll;
       if(i2 != 0)
           dw &= ~((i2 << 1) - 1);
       i = MemoryAddressToIndex(dw);
       if(!(i > -1 && MemoryAddress[i].Size > 1))
           return;
       dw1 = MemoryAddress[i].Address + MemoryAddress[i].Size - 1;
       dw1 -= dw;
   }
   else{
       dw = SEL.dwSelStart;
       dw1 = SEL.dwSelEnd - dw;
   }
   h = GlobalAlloc(GMEM_SHARE|GMEM_MOVEABLE,dw1);
   p = (LPBYTE)GlobalLock(h);
   for(;dw1 > 0;dw1--,dw++){
       value = (u8)ReadMem(dw,AMM_BYTE);
       *p++ = value;
   }
   GlobalUnlock(h);
   OpenClipboard(hWndMem);
   EmptyClipboard();
   SetClipboardData(CF_GDIOBJFIRST,h);
   CloseClipboard();
   GlobalFree(h);
}
//---------------------------------------------------------------------------
static void OnFillMemory()
{
   int i,i2;
   DWORD dw,dw1;
   LString c;

   if(SEL.dwSelStart == (u32)-1 || SEL.dwSelEnd == (u32)-1){
       i2 = SendDlgItemMessage(hWndMem,IDC_COMBOBOX3,CB_GETCURSEL,0,0);
       i = SendDlgItemMessage(hWndMem,IDC_COMBOBOX2,CB_GETCURSEL,0,0);
       dw = MemoryAddress[i].Address + yScroll;
       if(i2 != 0)
           dw &= ~((i2 << 1) - 1);
       i = MemoryAddressToIndex(dw);
       if(!(i > -1 && MemoryAddress[i].Size > 1))
           return;
       dw1 = MemoryAddress[i].Address + MemoryAddress[i].Size - 1;
       dw1 -= dw;
   }
   else{
       dw = SEL.dwSelStart;
       dw1 = SEL.dwSelEnd - dw;
   }
   for(;dw1 > 0;dw1--,dw++)
       WriteMem(dw,AMM_BYTE,0);
   InvalidateRect(hWndMem,NULL,FALSE);
   UpdateWindow(hWndMem);
}
#endif
//---------------------------------------------------------------------------
static void OnLoadMemory()
{
   int i,i2;
   DWORD dw,dw1;
   LFile *file;
   u8 value;
   char szFile[MAX_PATH];

   if(SEL.dwSelStart == (u32)-1 || SEL.dwSelEnd == (u32)-1){
       i2 = SendDlgItemMessage(hWndMem,IDC_COMBOBOX3,CB_GETCURSEL,0,0);
       i = SendDlgItemMessage(hWndMem,IDC_COMBOBOX2,CB_GETCURSEL,0,0);
       dw = MemoryAddress[i].Address + yScroll;
       if(i2 != 0)
           dw &= ~((i2 << 1) - 1);
       i = MemoryAddressToIndex(dw);
       if(!(i > -1 && MemoryAddress[i].Size > 1))
           return;
       dw1 = MemoryAddress[i].Address + MemoryAddress[i].Size - 1;
       dw1 -= dw;
   }
   else{
       dw = SEL.dwSelStart;
       dw1 = SEL.dwSelEnd - dw;
   }
   *((LPDWORD)szFile) = 0;
   if(!ShowOpenDialog(NULL,szFile,MEMFILTERFILE,hWndMem))
       return;
   file = new LFile(szFile);
   if(file == NULL || !file->Open()){
       if(file != NULL)
           delete file;
       return;
   }
   for(;dw1 > 0;dw1--,dw++){
       if(file->Read(&value,1) == 0)
           break;
       WriteMem(dw,AMM_BYTE,(u32)value);
   }
   delete file;
}
//---------------------------------------------------------------------------
static void OnSaveMemory(void)
{
   int i,i2;
   DWORD dw,dw1;
   LFile *file;
   u8 value;
   char szFile[MAX_PATH];
   LString c;

   if(SEL.dwSelStart == (u32)-1 || SEL.dwSelEnd == (u32)-1){
       i2 = SendDlgItemMessage(hWndMem,IDC_COMBOBOX3,CB_GETCURSEL,0,0);
       i = SendDlgItemMessage(hWndMem,IDC_COMBOBOX2,CB_GETCURSEL,0,0);
       dw = MemoryAddress[i].Address + yScroll;
       if(i2 != 0)
           dw &= ~((i2 << 1) - 1);
       i = MemoryAddressToIndex(dw);
       if(!(i > -1 && MemoryAddress[i].Size > 1))
           return;
       dw1 = MemoryAddress[i].Address + MemoryAddress[i].Size - 1;
       dw1 -= dw;
   }
   else{
       dw = SEL.dwSelStart;
       dw1 = SEL.dwSelEnd - dw;
   }
   lstrcpy(szFile,"memdump.dp");
   if(!ShowSaveDialog(NULL,szFile,MEMFILTERFILE,hWndMem,NULL))
       return;
   c = szFile;
   c.AddEXT(".md");
   file = new LFile(c.c_str());
   if(file == NULL || !file->Open(GENERIC_WRITE,CREATE_ALWAYS)){
       if(file != NULL)
           delete file;
       return;
   }
   for(;dw1 > 0;dw1--,dw++){
       value = (u8)ReadMem(dw,AMM_BYTE);
       file->Write(&value,1);
   }
   delete file;
   InvalidateRect(hWndMem,NULL,FALSE);
   UpdateWindow(hWndMem);
}
//---------------------------------------------------------------------------
static BOOL AddressFromPoint(LPPOINT pt,u32 *dwAddress)
{
   int line,pos,i;
   RECT rc;
   HDC hdc;
   SIZE sz;
   HWND hwnd;

   i = SendDlgItemMessage(hWndMem,IDC_COMBOBOX2,CB_GETCURSEL,0,0);
   *dwAddress = MemoryAddress[i].Address + yScroll;
   hdc = GetDC(hwnd = GetDlgItem(hWndMem,IDC_EDIT1));
   if(hdc == NULL)
       return FALSE;
   SetRect(&rc,0,0,0,0);
   DrawText(hdc,lpszText,-1,&rc,DT_CALCRECT|DT_LEFT|DT_EXTERNALLEADING|DT_EDITCONTROL);
   GetTextExtentPoint32(hdc,"0",1,&sz);
   ::ReleaseDC(hwnd,hdc);
   sz.cy = (LONG)(((float)rc.bottom / 15.0) + .5);
   line = pt->y / sz.cy;
   for(i=pos=0;i < line;pos++){
       if(lpszText[pos] == 10)
           i++;
   }
   pt->x = (pt->x / sz.cx) * sz.cx;
   for(i=0;i <= pt->x;i += sz.cx){
       if(lpszText[pos] == 13 || lpszText[pos+1] == 13)
           break;
       pos++;
   }
   pos -= (line * 13) + 12;
   i = SendDlgItemMessage(hWndMem,IDC_COMBOBOX3,CB_GETCURSEL,0,0);
   switch(i){
       case 0:
           *dwAddress += (pos / 3);
       break;
       case 1:
           *dwAddress += (pos - (pos / 5)) >> 1;
       break;
       case 2:
           *dwAddress += (pos - (pos / 9)) >> 1;
       break;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
static void OnClickMemory(LPPOINT pt)
{
   DWORD dwAddress;
   char string[20];

   if(!AddressFromPoint(pt,&dwAddress))
       return;
   wsprintf(string,"0x%08X",dwAddress);
   UpdateStatusBar(string,1);
}
//---------------------------------------------------------------------------
static u8 *GetMemoryFromAddress(u32 *adress)
{
   u8 *p;

   p = NULL;
	switch ((*adress >> 24) & 0xF){
		case 0:
//           if(IsBiosLoad()){
               p = zero_page_u8;
			    *adress &= 0x3FFF;
//           }
       break;
		case 2:
			p = wram_ext_u8;
           *adress &= 0x3FFFF;
       break;
		case 3:
			p = wram_int_u8;
           *adress &= 0x7FFF;
       break;
		case 4:
			p = io_ram_u8;
           *adress &= 0x3FF;
       break;
		case 5:
			p = pal_ram_u8;
           *adress &= 0x3FF;
       break;
		case 6:
			p = vram_u8;
           *adress &= 0x1FFFF;
       break;
		case 7:
			p = oam_u8;
           *adress &= 0x3FF;
       break;
       case 8:
       case 9:
			p = (u8*)rom_pages_u8[(*adress>>16)&0x1FF];
           *adress &= 0xFFFF;
       break;
       case 10:
       case 11:
//			p = rom_pack1_u8[(adress >> 16) & 0x1FF];
//           adress &= 0xFFFF;
       break;
       case 12:
       case 13:
//			p = rom_pack2_u8[(adress >> 16) & 0x1FF];
//           adress &= 0xFFFF;
       break;
       case 14:
       case 15:
           if(cram_u8 != NULL){
               p = ((LPSRAM)cram_u8)->buffer;
               *adress &= ((LPSRAM)cram_u8)->mask;
           }
       break;
	}
   return p;
}
//---------------------------------------------------------------------------
void WriteMem(u32 adress,u8 mode,u32 data)
{
   u8 *p;
   u32 i;

   i = adress;
   p = GetMemoryFromAddress(&i);
   if(p == NULL)
       return;
   switch(mode){
       case AMM_HWORD:
           *((u16 *)(p + i)) = (u16)data;
       break;
       case AMM_WORD:
           *((u32 *)(p + i)) = data;
       break;
       case AMM_BYTE:
           *(p + i) = (u8)data;
       break;
   }
   switch(((adress >> 24) & 0xF)){
       case 5:
           FillPalette((u16)(i >>= 1));
           if(mode != AMM_BYTE)
               FillPalette((u16)i++);
           if(mode != AMM_HWORD)
               FillPalette((u16)i);
       break;
       case 4:
           io_write_handles[i]((u16)i,AMM_HWORD);
       break;
   }
}
//---------------------------------------------------------------------------
u32 ReadMem(u32 adress,u8 mode)
{
   u8 *p;
   u32 i;

   i = adress;
   p = GetMemoryFromAddress(&i);
   if(p == NULL)
       return 0;
   switch(mode){
       case AMM_HWORD:
           return *((u16 *)(p + i));
       case AMM_WORD:
           return *((u32 *)(p + i));
       case AMM_BYTE:
           return *(p + i);
       default:
           return 0;
   }
}
//---------------------------------------------------------------------------
static void UpdateMemoryDump(LPDWORD p,BOOL bErase)
{
   DWORD dw,dw1;
   int i,i1,i2,i3,x;
   char s[31],bRead,cr[]={13,10,0};

   i2 = SendDlgItemMessage(hWndMem,IDC_COMBOBOX3,CB_GETCURSEL,0,0);
   i3 = 1 << (4 - i2);
   if(p == NULL){
       i = SendDlgItemMessage(hWndMem,IDC_COMBOBOX2,CB_GETCURSEL,0,0);
       dw = MemoryAddress[i].Address + yScroll;
   }
   else
       dw = *p;
   if(i2 != 0)
       dw &= ~((i2 << 1) - 1);
   i = MemoryAddressToIndex(dw);
   if(i > -1 && MemoryAddress[i].Size > 1)
       bRead = 1;
   else
       bRead = 0;
   SEL.SelStart = SEL.SelEnd = -1;
   ZeroMemory(lpszText,1000);
   for(x=i=0;i<14;i++){
       x += 11;
       wsprintf(s,"%08X : ",dw);
       if(dw > 0xF0000000)
           dw = dw;
       lstrcat(lpszText,s);
       for(i1=0;i1<i3;i1++){
           if(SEL.SelStart == (u16)-1 && dw >= SEL.dwSelStart && dw <= SEL.dwSelEnd)
               SEL.SelStart = (u16)x;
           switch(i2){
               case 0:
                   if(bRead != 0)
                       dw1 = ReadMem(dw,AMM_BYTE);
                   else
                       dw1 = 0;
                   wsprintf(s,"%02X ",dw1);
                   x += 3;
                   dw++;
               break;
               case 1:
                   if(bRead != 0)
                       dw1 = ReadMem(dw,AMM_HWORD);
                   else
                       dw1 = 0;
                   dw += 2;
                   wsprintf(s,"%04X ",dw1);
                   x += 5;
               break;
               case 2:
                   if(bRead != 0)
                       dw1 = ReadMem(dw,AMM_WORD);
                   else
                       dw1 = 0;
                   dw += 4;
                   wsprintf(s,"%08X ",dw1);
                   x += 9;
               break;
           }
           if(SEL.SelStart != (u16)-1 && SEL.dwSelEnd != (u16)-1){
               if(dw <= SEL.dwSelEnd)
                   SEL.SelEnd = (u16)(x - SEL.SelStart);
           }
           lstrcat(lpszText,s);
       }
       lstrcat(lpszText,cr);
   }
   InvalidateRect(GetDlgItem(hWndMem,IDC_EDIT1),NULL,bErase);
   UpdateWindow(GetDlgItem(hWndMem,IDC_EDIT1));
}
//---------------------------------------------------------------------------
int MemoryAddressToIndex(DWORD dwAddress)
{
   int i,bUp;

   bUp = 0;
   for(i=0;i < (int)SizeMemoryAddressStruct;i++){
       if(!bUp && dwAddress >= MemoryAddress[i].Address)
           bUp = 1;
       else if(bUp && dwAddress < MemoryAddress[i].Address)
           break;
   }
   if(!bUp || i >= (int)SizeMemoryAddressStruct)
       return -1;
   return i - 1;
}
//---------------------------------------------------------------------------
int MemoryStringToIndex(char *string)
{
   return MemoryAddressToIndex(StrToHex(string));
}
//---------------------------------------------------------------------------
void UpdateVertScrollBarMemoryDump(DWORD address,int index)
{
   char s[31];
   SCROLLINFO si;
   HWND hwnd;

   if(hWndMem == NULL)
       return;
   if(index == -1){
       if((int)address == -1){
           ::GetWindowText(GetDlgItem(hWndMem,IDC_EDIT2),s,20);
           *((long *)&s[10]) = 0;
           address = StrToHex(s);
       }
       wsprintf(s,"%d",address);
       index = MemoryStringToIndex(s);
   }
   hwnd = GetDlgItem(hWndMem,IDC_VSBMEM);
   ZeroMemory(&si,sizeof(SCROLLINFO));
   si.cbSize = sizeof(SCROLLINFO);
   si.fMask = SIF_ALL;
   si.nMax = MemoryAddress[index].Size-1;
   if(si.nMax > 0)
       si.nPage = 14*16;
   else
       si.nPage = 1;
   SetScrollInfo(hwnd,SB_CTL,&si,FALSE);
   yScroll = ((address - MemoryAddress[index].Address) >> 4) << 4;
   SetScrollPos(hwnd,SB_CTL,yScroll,TRUE);
}
//---------------------------------------------------------------------------
void InitDebugMemoryWindow()
{
   hWndMem = hwndDebugMemoryStatusBar = hWndIO = NULL;
   bUpdateIOReg = FALSE;
   lpszText = NULL;
   pInfoMemList = NULL;
}
//---------------------------------------------------------------------------
void DestroyDebugMemoryWindow()
{
   if(hwndDebugMemoryStatusBar != NULL)
       ::DestroyWindow(hwndDebugMemoryStatusBar);
   hwndDebugMemoryStatusBar = NULL;
   if(hWndIO != NULL)
       ::DestroyWindow(hWndIO);
   hWndIO = NULL;
   if(hWndMem != NULL){
       SaveMemoryAddress();
       ::DestroyWindow(hWndMem);
   }
   hWndMem = NULL;
   if(lpszText != NULL)
       GlobalFree((HGLOBAL)lpszText);
   lpszText = NULL;
   if(pInfoMemList != NULL)
       delete pInfoMemList;
   pInfoMemList = NULL;
}
//---------------------------------------------------------------------------
void UpdateDebugMemoryWindow()
{
   if(hWndMem == NULL)
       return;
   UpdateMemoryDump(NULL,TRUE);
   UpdateEditMemoryValue(0,TRUE,TRUE);
}
//---------------------------------------------------------------------------
static void GotoAddress(char *s)
{
   int x,i;
   DWORD dw;
   char string [100];
   LPINFOMEM p;

   x = MemoryAddressToIndex((dw = StrToHex(s)));
   if(x < 0)
       return;
   ZeroMemory(string,100);
   wsprintf(s,"0x%08X",dw);
   i = SendDlgItemMessage(hWndMem,IDC_COMBOBOX2,CB_GETCURSEL,0,0);
   if(i != x){
       SendDlgItemMessage(hWndMem,IDC_COMBOBOX2,CB_SETCURSEL,(WPARAM)x,0);
       DisplayInfoMemory(x);
   }
   if(bInfoMode)
       SEL.dwSelStart = dw;
   else if(SendDlgItemMessage(hWndMem,IDC_EDIT2,CB_FINDSTRING,-1,(LPARAM)s) == -1){
       i = SendDlgItemMessage(hWndMem,IDC_EDIT2,CB_GETCOUNT,0,0);
       if(i > 100)
           SendDlgItemMessage(hWndMem,IDC_EDIT2,CB_DELETESTRING,0,0);
       SendDlgItemMessage(hWndMem,IDC_EDIT2,CB_ADDSTRING,0,(LPARAM)s);
       EnableMenuItem(GetMenu(hWndMem),ID_MEMORY_RESET,MF_BYCOMMAND|MF_ENABLED);
   }
   i = SendDlgItemMessage(hWndMem,IDC_COMBOBOX3,CB_GETCURSEL,0,0);
   dw = (dw >> 4) << 4;
   if(((1 << i) & MemoryAddress[x].AccessMode) == 0){
       i = MemoryAddress[x].AccessMode;
       if((i & AMM_HWORD) != 0)
           i = 1;
       else if((i & AMM_WORD) != 0)
           i = 2;
       else
           i = 0;
       SendDlgItemMessage(hWndMem,IDC_COMBOBOX3,CB_SETCURSEL,(WPARAM)i,0);
   }
   if(bInfoMode){
       switch(i){
           case 1:
               SEL.dwSelStart &= ~1;
           break;
           case 2:
               SEL.dwSelStart &= ~3;
           break;
       }
       SEL.dwSelEnd = SEL.dwSelStart + (i == 0 ? 1 : (i << 1));
       i = SendDlgItemMessage(hWndMem,IDC_EDIT2,CB_GETCURSEL,0,0);
       if(i != CB_ERR && pInfoMemList != NULL){
           p = (LPINFOMEM)pInfoMemList->GetItem(i+1);
           if(p!= NULL)
               lstrcpy(string,p->descrex);
       }
   }
   UpdateStatusBar(string,0);
   UpdateVertScrollBarMemoryDump(dw,x);
   UpdateMemoryDump(&dw,TRUE);
}
//---------------------------------------------------------------------------
static void UpdateStatusBar(char *string,int index)
{
   if(hwndDebugMemoryStatusBar == NULL)
       return;
   if(index >=0)
       SendMessage(hwndDebugMemoryStatusBar,SB_SETTEXT,(WPARAM)index,(LPARAM)string);
   else{
       SendMessage(hwndDebugMemoryStatusBar,SB_SETTEXT,(WPARAM)0,(LPARAM)string);
       SendMessage(hwndDebugMemoryStatusBar,SB_SETTEXT,(WPARAM)1,(LPARAM)string);
   }
}
//---------------------------------------------------------------------------
static BOOL OnInitDialog(HWND hwndDlg)
{
   int item[3],i;
   RECT rc;
   char s[61];

   hWndMem = hwndDlg;
   SendDlgItemMessage(hwndDlg,IDC_EDIT1,WM_SETFONT,(WPARAM)GetStockObject(SYSTEM_FIXED_FONT),0);
   for(i=0;i < (int)SizeMemoryAddressStruct;i++){
       wsprintf(s,"0x%08X - %s",MemoryAddress[i].Address,MemoryAddress[i].Name);
       SendDlgItemMessage(hwndDlg,IDC_COMBOBOX2,CB_ADDSTRING,0,(LPARAM)s);
   }
   SendDlgItemMessage(hwndDlg,IDC_COMBOBOX3,CB_ADDSTRING,0,(LPARAM)"Byte");
   SendDlgItemMessage(hwndDlg,IDC_COMBOBOX3,CB_ADDSTRING,0,(LPARAM)"Half Word");
   SendDlgItemMessage(hwndDlg,IDC_COMBOBOX3,CB_ADDSTRING,0,(LPARAM)"Word");
   SendDlgItemMessage(hwndDlg,IDC_COMBOBOX3,CB_SETCURSEL,0,0);
   SendDlgItemMessage(hwndDlg,IDC_COMBOBOX2,CB_SETCURSEL,0,0);
   iCB2DropWidth = (int)SendDlgItemMessage(hwndDlg,IDC_EDIT2,CB_GETDROPPEDWIDTH,0,0);
   wsprintf(s,"0x%08X",MemoryAddress[0].Address);
   SetWindowText(GetDlgItem(hwndDlg,IDC_EDIT2),s);
   LoadMemoryAddress();
   UpdateVertScrollBarMemoryDump(0,0);
   Translation(hwndDlg,IDR_MEMORY,IDD_DIALOG3);
   SEL.dwSelStart = SEL.dwSelEnd = -1;
   oldEditMemoryWndProc = (WNDPROC)SetWindowLong(GetDlgItem(hwndDlg,IDC_EDIT1),GWL_WNDPROC,(LONG)EditMemoryWindowProc);

   hwndDebugMemoryStatusBar = NULL;
   if(hWndMem == NULL)
       return FALSE;
   hwndDebugMemoryStatusBar = CreateStatusWindow(WS_CHILD|WS_VISIBLE|WS_BORDER|CCS_BOTTOM,"",hWndMem,IDM_STATUSBARDEBUG);
   if(hwndDebugMemoryStatusBar == NULL)
       return FALSE;
   ::GetClientRect(hwndDebugMemoryStatusBar,&rc);
   item[0] = rc.right - 100;
   item[1] = rc.right;
   ::SendMessage(hwndDebugMemoryStatusBar,SB_SETPARTS,2,(LPARAM)(LPINT)item);

   return TRUE;
}
//---------------------------------------------------------------------------
static BOOL CALLBACK DlgProcMem(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   int wID,x;
   char s[61];
   HWND hwnd;
   SCROLLINFO si;
   DWORD dw;
   POINT pt;
   RECT rc;

   switch(uMsg){
       case WM_SIZE:
           if(hwndDebugMemoryStatusBar != NULL)
               SendMessage(hwndDebugMemoryStatusBar,WM_SIZE,wParam,lParam);
       break;
       case WM_INITDIALOG:
           OnInitDialog(hwndDlg);
       break;
       case WM_MOUSEWHEEL:
           x = (int)(short)HIWORD(wParam) / -7.5;
           SetScrollPos(GetDlgItem(hwndDlg,IDC_VSBMEM),SB_CTL,yScroll+x,TRUE);
           SendMessage(hwndDlg,WM_VSCROLL,MAKEWPARAM(SB_THUMBPOSITION,0),(LPARAM)GetDlgItem(hwndDlg,IDC_VSBMEM));
       break;
       case WM_VSCROLL:
           hwnd = (HWND)lParam;
           ZeroMemory(&si,sizeof(SCROLLINFO));
           si.cbSize = sizeof(SCROLLINFO);
           si.fMask = SIF_ALL;
           if(GetScrollInfo(hwnd,SB_CTL,&si) != 0){
               switch(LOWORD(wParam)){
                   case SB_PAGEUP:
                       if((x = yScroll - si.nPage) < 0)
                           x = 0;
                   break;
                   case SB_TOP:
                       x = 0;
                   break;
                   case SB_BOTTOM:
                       x = (si.nMax - si.nPage) + 1;
                   break;
                   case SB_PAGEDOWN:
                       x = yScroll + si.nPage;
                       if(x > (int)((si.nMax - si.nPage)+1))
                           x = (si.nMax - si.nPage) + 1;
                   break;
                   case SB_LINEUP:
                       x = yScroll - 16;
                       if(x < 0)
                           x = 0;
                   break;
                   case SB_LINEDOWN:
                       x = yScroll + 16;
                       if(x > (int)((si.nMax - si.nPage)+1))
                           x = (si.nMax - si.nPage) + 1;
                   break;
                   case SB_THUMBPOSITION:
                       x = si.nPos;
                   break;
                   case SB_THUMBTRACK:
                       x = si.nTrackPos;
                   break;
                   default:
                       x = yScroll;
                   break;
               }
               x = (x >> 4) << 4;
               if(x == yScroll)
                   return 0;
               yScroll = x;
               ::SetScrollPos(hwnd,SB_CTL,x,TRUE);
               dw = SendDlgItemMessage(hwndDlg,IDC_COMBOBOX2,CB_GETCURSEL,0,0);
               dw = MemoryAddress[dw].Address + x;
               UpdateMemoryDump(&dw,FALSE);
           }
       break;
       case WM_CTLCOLORSTATIC:
           if(GetDlgCtrlID((HWND)lParam) == IDC_EDIT1)
               return (BOOL)GetStockObject(WHITE_BRUSH);
           else
               return (BOOL)GetClassLong((HWND)lParam,GCL_HBRBACKGROUND);
       case WM_COMMAND:
           wID = LOWORD(wParam);
           if(HIWORD(wParam) == 0 && lParam == 0){
               switch(wID){
#ifdef _DEBPRO
                   case ID_MEMORY_FILLZERO:
                       OnFillMemory();
                   break;
                   case ID_MEMORY_SELECTALL:
                       OnSelectAllMemory();
                   break;
                   case ID_MEMORY_COPY:
                       OnCopyMemory();
                   break;
                   case ID_MEMORY_PASTE:
                       OnPasteMemory();
                   break;
#endif
                   case ID_MEMORY_SAVE:
                       OnSaveMemory();
                   break;
                   case ID_MEMORY_LOAD:
                       OnLoadMemory();
                   break;
                   case ID_MEMORY_RESET:
                       if(!bInfoMode){
                           SendDlgItemMessage(hWndMem,IDC_EDIT2,CB_RESETCONTENT,0,0);
                           EnableMenuItem(GetMenu(hWndMem),ID_MEMORY_RESET,MF_BYCOMMAND|MF_GRAYED);
                       }
                   break;
               }
               return FALSE;
           }
           switch(HIWORD(wParam)){
               case STN_DBLCLK:
                   switch(wID){
                       case IDC_EDIT1:
                           GetCursorPos(&pt);
                           ScreenToClient((HWND)LOWORD(lParam),&pt);
                           OnEditMemory((HWND)LOWORD(lParam),&pt);
                       break;
                   }
               break;
               case BN_CLICKED:
                   switch(wID){
                       case IDC_EDIT1:
                           if(GetFocus() != GetDlgItem(hwndDlg,IDC_VSBMEM))
                               SetFocus(GetDlgItem(hwndDlg,IDC_VSBMEM));
                           else{
                               GetCursorPos(&pt);
                               ScreenToClient((HWND)LOWORD(lParam),&pt);
                               GetClientRect((HWND)LOWORD(lParam),&rc);
                               if(PtInRect(&rc,pt))
                                   OnClickMemory(&pt);
                           }
                       break;
                       case IDC_BUTTON1:
                           ::GetWindowText(GetDlgItem(hwndDlg,IDC_EDIT2),s,20);
                           GotoAddress(s);
                       break;
                   }
               break;
               case CBN_SELENDOK:
                   switch(wID){
                       case IDC_EDIT2:
                           x = SendDlgItemMessage(hwndDlg,IDC_EDIT2,CB_GETCURSEL,0,0);
                           if(x != CB_ERR){
                               SendDlgItemMessage(hwndDlg,IDC_EDIT2,CB_GETLBTEXT,x,(LPARAM)s);
                               if(bInfoMode)
                                   s[10] = 0;
                               GotoAddress(s);
                           }
                       break;
                       case IDC_COMBOBOX2:
                           OnSelOkComboBox2();
                       break;
                       case IDC_COMBOBOX3:
                       	wID = SendDlgItemMessage(hwndDlg,IDC_COMBOBOX3,CB_GETCURSEL,0,0);
                       	x = SendDlgItemMessage(hwndDlg,IDC_COMBOBOX2,CB_GETCURSEL,0,0);
                           if(((1 << wID) & MemoryAddress[x].AccessMode) == 0){
                               wID = MemoryAddress[x].AccessMode;
                               if((wID & AMM_HWORD) != 0)
                                   wID = 1;
                               else if((wID & AMM_WORD) != 0)
                                   wID = 2;
                               else
                                   wID = 0;
                               SendDlgItemMessage(hwndDlg,IDC_COMBOBOX3,CB_SETCURSEL,(WPARAM)wID,0);
                           }
                           wsprintf(s,"0x%08X",MemoryAddress[x].Address + yScroll);
                           x = MemoryAddressToIndex((dw = StrToHex(s)));
                           if(x >= 0)
                              	UpdateMemoryDump(&dw,TRUE);
                       break;
                   }
               break;
           }
       break;
       case WM_PAINT:
       case UM_UPDATE:
           UpdateMemoryDump(NULL,FALSE);
       break;
       case WM_CLOSE:
           DestroyDebugMemoryWindow();
       break;
   }
   return FALSE;
}
//---------------------------------------------------------------------------
static void SaveMemoryAddress()
{
   LRegKey reg;
   int i,i1;
   LString s1;
   DWORD dw;
   LPBYTE p1;
   HGLOBAL p;

   if(bInfoMode)
       return;
   p = GlobalAlloc(GHND,200*sizeof(DWORD));
   if(p == NULL)
       return;
   p1 = (LPBYTE)GlobalLock(p);
   reg.Open("Software\\RascalBoy\\Debug");
   for(i=0;i<SendDlgItemMessage(hWndMem,IDC_EDIT2,CB_GETCOUNT,0,0);i++){
       if((i1 = SendDlgItemMessage(hWndMem,IDC_EDIT2,CB_GETLBTEXTLEN,i,0)) == CB_ERR || i1 < 1)
           continue;
       s1.Capacity(i1+1);
       SendDlgItemMessage(hWndMem,IDC_EDIT2,CB_GETLBTEXT,i,(LPARAM)s1.c_str());
       sscanf(s1.c_str(),"0x%08X",&dw);
       memcpy(p1,&dw,sizeof(DWORD));
       p1 += sizeof(DWORD);
   }
   GlobalUnlock(p);
   p1 = (LPBYTE)GlobalLock(p);
   reg.WriteBinaryData("MemoryAddress",(char *)p1,i*4);
   GlobalUnlock(p);
   GlobalFree((HGLOBAL)p);
}
//---------------------------------------------------------------------------
static void LoadMemoryAddress()
{
   LRegKey reg;
   int i,i1;
   LString s;
   DWORD dw;
   char s1[20];

   reg.Open("Software\\RascalBoy\\Debug");
   s.Capacity(800);
   i1 = reg.ReadBinaryData("MemoryAddress",s.c_str(),800);
   SendDlgItemMessage(hWndMem,IDC_EDIT2,CB_RESETCONTENT,0,0);
   for(i=0;i < i1;i+=4){
       dw = *((LPDWORD)(s.c_str() + i));
       wsprintf(s1,"0x%08X",dw);
       SendDlgItemMessage(hWndMem,IDC_EDIT2,CB_ADDSTRING,0,(LPARAM)s1);
   }
   if(i > 0)
       EnableMenuItem(GetMenu(hWndMem),ID_MEMORY_RESET,MF_BYCOMMAND|MF_ENABLED);
}
//---------------------------------------------------------------------------
static void DrawEditMemory(HDC hdc,LPRECT prcClip)
{
   char *p;
   int y,x,x1;
   SIZE sz;
   BOOL flag;
   char s[30];

   SelectObject(hdc,(HFONT)GetStockObject(SYSTEM_FIXED_FONT));
   SetTextColor(hdc,GetSysColor(COLOR_WINDOWTEXT));
   SetBkMode(hdc,OPAQUE);
   SetTextAlign(hdc,TA_UPDATECP|TA_LEFT|TA_BOTTOM);
   p = lpszText;
   GetTextExtentPoint32(hdc,p,10,&sz);
   y = x = 0;
   while(*p != 0){
       y += sz.cy;
       MoveToEx(hdc,0,y,NULL);
       x1 = 0;
       while(*p != 13){
           flag = FALSE;
           if(x1 > 10 && SEL.SelStart != (u16)-1 && SEL.SelEnd != (u16)-1){
               if(x >= SEL.SelStart && x < (SEL.SelStart + SEL.SelEnd))
                   flag = TRUE;
           }
           if(flag){
               SetBkColor(hdc,GetSysColor(COLOR_HIGHLIGHT));
               SetTextColor(hdc,GetSysColor(COLOR_HIGHLIGHTTEXT));
           }else{
               SetBkColor(hdc,GetSysColor(COLOR_WINDOW));
               SetTextColor(hdc,GetSysColor(COLOR_WINDOWTEXT));
           }
           ExtTextOut(hdc,0,0,ETO_CLIPPED,prcClip,p++,1,NULL);
           x1++;
           x++;
       }
       p+=2;
   }
   wsprintf(s,"%d",SEL.dwSelEnd - SEL.dwSelStart);
   UpdateStatusBar(s,1);
}
//---------------------------------------------------------------------------
static LRESULT CALLBACK EditMemoryWindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   POINT pt;
   DWORD dw;
   PAINTSTRUCT ps;
   BOOL bFlag;
   HRESULT res;
   RECT rc;
   HMENU menu;

   bFlag = FALSE;
   switch(uMsg){
       case WM_ERASEBKGND:
           GetClientRect(GetDlgItem(hWndMem,IDC_EDIT1),&rc);
           FillRect((HDC)wParam,&rc,(HBRUSH)GetStockObject(WHITE_BRUSH));
           res = 1;
           bFlag = TRUE;
       break;
       case WM_PAINT:
           BeginPaint(hwnd,&ps);
           DrawEditMemory(ps.hdc,&ps.rcPaint);
           EndPaint(hwnd,&ps);
           bFlag = TRUE;
           res = 1;
       break;
       case WM_RBUTTONDOWN:
           menu = GetSubMenu(LoadMenu(hInstance,MAKEINTRESOURCE(IDR_MEMORY)),0);
           TranslateMenu(menu,0xC000 + ((IDR_MEMORY -IDR_MAINFRAME) << 10));
           if(SendDlgItemMessage(hWndMem,IDC_EDIT2,CB_GETCOUNT,0,0) > 0)
               EnableMenuItem(menu,ID_MEMORY_RESET,MF_BYCOMMAND|MF_ENABLED);
           GetCursorPos(&pt);
           TrackPopupMenu(menu,TPM_LEFTALIGN,pt.x,pt.y,0,hWndMem,NULL);
           DestroyMenu(menu);
           bFlag = 1;
           res = 0;
       break;
       case WM_LBUTTONUP:
           ReleaseCapture();
       break;
       case WM_GETFONT:
           bFlag = TRUE;
           res = (HRESULT)SendMessage(GetParent(hwnd),WM_GETFONT,0,0);
       break;
       case WM_LBUTTONDOWN:
           FillMemory(&SEL,sizeof(SEL),0xFF);
           InvalidateRect(GetDlgItem(hWndMem,IDC_EDIT1),NULL,TRUE);
           UpdateWindow(GetDlgItem(hWndMem,IDC_EDIT1));
       break;
       case WM_MOUSEMOVE:
           if((wParam & MK_LBUTTON)){
               GetCursorPos(&pt);
               ScreenToClient(hwnd,&pt);
               GetClientRect(hwnd,&rc);
               if(pt.y < 0){
                   SendMessage(hWndMem,WM_VSCROLL,MAKEWPARAM(SB_LINEUP,0),(LPARAM)GetDlgItem(hWndMem,IDC_VSBMEM));
                   SetCapture(hwnd);
                   pt.y = 0;
               }
               else if(pt.y > rc.bottom){
                   SendMessage(hWndMem,WM_VSCROLL,MAKEWPARAM(SB_LINEDOWN,0),(LPARAM)GetDlgItem(hWndMem,IDC_VSBMEM));
                   SetCapture(hwnd);
                   pt.y = rc.bottom+1;
               }
               if(AddressFromPoint(&pt,&dw)){
                   if(SEL.dwSelStart == (DWORD)-1){
                       SEL.dwSelStart = dw;
                       SEL.dwSelEnd = dw;
                   }
                   else{
                       dw++;
                       if(dw <= SEL.dwSelStart)
                           SEL.dwSelStart = dw;
                       else
                           SEL.dwSelEnd = dw;
                       if(SEL.dwSelStart > SEL.dwSelEnd){
                           dw = SEL.dwSelStart;
                           SEL.dwSelStart = SEL.dwSelEnd;
                           SEL.dwSelEnd = dw;
                       }
                   }
                   UpdateMemoryDump(NULL,FALSE);
               }
           }
       break;
   }
   if(!bFlag)
       res = CallWindowProc(oldEditMemoryWndProc,hwnd,uMsg,wParam,lParam);
   return res;
}
//---------------------------------------------------------------------------
static void DisplayInfoMemory(int iMemoryBank)
{
   LMemoryFile File;
   LZipFile zipFile;
   HRSRC hRes;
   HGLOBAL hgRes;
   LPBYTE pRes;
   DWORD dwWrite;
   BYTE c;
   LString s;
   int i,len;
   LPINFOMEM im;
   char *p;

   if(pInfoMemList != NULL)
       delete pInfoMemList;
   pInfoMemList = NULL;
   if(MemoryAddress[iMemoryBank].idMemoryInfo == 0){
       if(bInfoMode){
           LoadMemoryAddress();
           SendDlgItemMessage(hWndMem,IDC_EDIT2,CB_SETDROPPEDWIDTH,iCB2DropWidth,0);
           SEL.dwSelStart = SEL.dwSelEnd = (DWORD)-1;
       }
       bInfoMode = 0;
       return;
   }
   hRes = FindResource(hInstance,MAKEINTRESOURCE(MemoryAddress[iMemoryBank].idMemoryInfo),RT_RCDATA);
   if(hRes == NULL)
       return;
   if((dwWrite = SizeofResource(hInstance,hRes)) == 0)
       return;
   hgRes = LoadResource(hInstance,hRes);
   if(hgRes == NULL)
       return;
   pRes = (LPBYTE)LockResource(hgRes);
   if(pRes == NULL)
       goto ex_DisplayInfoMemory;
   if(!File.Open())
       goto ex_DisplayInfoMemory;
   File.Write(pRes,dwWrite);
   zipFile.SetFileStream(&File);
   if(!zipFile.Open(NULL))
       goto ex_DisplayInfoMemory;
   if(!zipFile.OpenZipFile(1))
       goto ex_DisplayInfoMemory;
   if((pInfoMemList = new LList()) == NULL)
       goto ex_DisplayInfoMemory;
   SaveMemoryAddress();
   SendDlgItemMessage(hWndMem,IDC_EDIT2,CB_RESETCONTENT,0,0);
   while(zipFile.ReadZipFile(&c,1)){
       if(c == 10){
           len = s.Length();
           if(len == 0)
               continue;
           if((im = new INFOMEM) != NULL){
               lstrcpy(im->adr,"04000");
               lstrcpyn(&im->adr[5],s.c_str(),4);
               p = im->descr;
               for(i=4;i<=len && s[i] != 9;i++)
                   *p++ = s[i];
               *p = 0;
               for(;i <= len && s[i] == 9;i++);
               p = im->descrex;
               for(;i<=len && s[i] != 9;i++)
                   *p++ = s[i];
               *p = 0;
               for(;i <= len && s[i] == 9;i++);
               p = im->descrbit;
               for(;i<=len;i++)
                   *p++ = s[i];
               *p = 0;
               s = "0x";
               s += im->adr;
               s += " REG_";
               s += im->descr;
               pInfoMemList->Add((LPVOID)im);
               SendDlgItemMessage(hWndMem,IDC_EDIT2,CB_ADDSTRING,0,(LPARAM)s.c_str());
           }
           s = "";
           continue;
       }
       s += (char)c;
   }
   SendDlgItemMessage(hWndMem,IDC_EDIT2,CB_SETDROPPEDWIDTH,200,0);
   bInfoMode = 1;
ex_DisplayInfoMemory:
   FreeResource(hgRes);
}
//---------------------------------------------------------------------------
static void OnSelOkComboBox2()
{
   char s[20];
   int iMemoryBank,iMemoryReadMode;

   iMemoryBank = SendDlgItemMessage(hWndMem,IDC_COMBOBOX2,CB_GETCURSEL,0,0);
   iMemoryReadMode = SendDlgItemMessage(hWndMem,IDC_COMBOBOX3,CB_GETCURSEL,0,0);
   if(((1 << iMemoryReadMode) & MemoryAddress[iMemoryBank].AccessMode) == 0){
       iMemoryReadMode = MemoryAddress[iMemoryBank].AccessMode;
       if((iMemoryReadMode & AMM_HWORD) != 0)
           iMemoryReadMode = 1;
       else if((iMemoryReadMode & AMM_WORD) != 0)
           iMemoryReadMode = 2;
       else
           iMemoryReadMode = 0;
       SendDlgItemMessage(hWndMem,IDC_COMBOBOX3,CB_SETCURSEL,(WPARAM)iMemoryReadMode,0);
   }
   if(((1 << iMemoryReadMode) & MemoryAddress[iMemoryBank].defAccessMode) == 0){
       iMemoryReadMode = MemoryAddress[iMemoryBank].defAccessMode;
       if((iMemoryReadMode & AMM_HWORD) != 0)
           iMemoryReadMode = 1;
       else if((iMemoryReadMode & AMM_WORD) != 0)
           iMemoryReadMode = 2;
       else
           iMemoryReadMode = 0;
       SendDlgItemMessage(hWndMem,IDC_COMBOBOX3,CB_SETCURSEL,(WPARAM)iMemoryReadMode,0);
   }
   *((LPDWORD)s) = 0;
   UpdateStatusBar(s,0);
   wsprintf(s,"0x%08X",MemoryAddress[iMemoryBank].Address);
   ::SetWindowText(GetDlgItem(hWndMem,IDC_EDIT2),s);
   UpdateVertScrollBarMemoryDump(MemoryAddress[iMemoryBank].Address,iMemoryBank);
   DisplayInfoMemory(iMemoryBank);
   UpdateMemoryDump(NULL,TRUE);
}
//---------------------------------------------------------------------------
u8 CreateDebugMemory()
{
   if(lpszText == NULL)
       lpszText = (char *)GlobalAlloc(GMEM_FIXED,1001);
   if(hWndMem == NULL){
       hWndMem = ::CreateDialog(hInstance,MAKEINTRESOURCE(IDD_DIALOG3),NULL,(DLGPROC)DlgProcMem);
       BringWindowToTop(hWin);
   }
   else
       ::BringWindowToTop(hWndMem);
   return 1;
}
//---------------------------------------------------------------------------
void BringDebugMemoryToTop(HWND win)
{
   if(hWndMem != NULL)
       ::SetWindowPos(hWndMem,win,0,0,0,0,SWP_NOMOVE|SWP_NOSENDCHANGING|SWP_NOACTIVATE|SWP_NOSIZE);
   if(hWndIO != NULL)
       ::SetWindowPos(hWndIO,win,0,0,0,0,SWP_NOMOVE|SWP_NOSENDCHANGING|SWP_NOACTIVATE|SWP_NOSIZE);
}
//---------------------------------------------------------------------------
void UpdateEditMemoryValue(DWORD dwValue,BOOL bValue,BOOL bReset)
{
   int i,i1;
   char s[10];

   if((!bUpdateIOReg && !bReset) || hWndIO == NULL)
       return;
   if(bValue)
       dwValue = ReadMem(dwCurrentAddress,AMM_HWORD);
   if(dwValue == dwCurrentValue && !bReset)
       return;
   dwCurrentValue = dwValue;
   wsprintf(s,"0x%04X",dwCurrentValue);
   SendDlgItemMessage(hWndIO,26,WM_SETTEXT,0,(LPARAM)s);
   for(i=10,i1=1;i < 26;i++,i1 <<= 1){
       SendDlgItemMessage(hWndIO,i,BM_SETCHECK,(WPARAM)((i1 & dwCurrentValue) != 0 ? BST_CHECKED : BST_UNCHECKED),0);
       ProcessaMessaggi();
   }
}
//---------------------------------------------------------------------------
static void UpdateEditmemoryInfo()
{
   LPINFOMEM im;
   char s[20],s1[200];
   char *p,*p1;
   int i,len,bit,i1,i2,i3,bLoad,bInsert;

   im = currentInfoMem;
   wsprintf(s1,"0x%s - %s",im->adr,im->descrex);
   SetWindowText(hWndIO,s1);
   p = im->descrbit;
   len = lstrlen(im->descrbit);
   i = 0;
   for(bLoad = 1,i2=10,i3=1;i2<26;i2++,i3<<=1){
       if(bLoad){
           if(i >= len)
               break;
           for(;i<len && p[i] == 9;i++);
           p1 = s;
           *p1++ = '0';
           *p1++ = 'x';
           for(i1=2;i1<6 && i <len;i1++,i++)
               *p1++ = p[i];
           *p1 = 0;
           bit = (int)StrToHex(s);
           p1 = s1;
           for(;i<len && p[i] != 9;i++)
               *p1++ = p[i];
           *p1 = 0;
           bInsert = 1;
           bLoad = 0;
       }
       if((i3 & bit) != 0){
           EnableWindow(GetDlgItem(hWndIO,i2),TRUE);
           if(!bInsert){
               SendDlgItemMessage(hWndIO,i2,WM_SETTEXT,0,(LPARAM)"");
               continue;
           }
           bInsert = 0;
           SendDlgItemMessage(hWndIO,i2,WM_SETTEXT,0,(LPARAM)s1);
       }
       else if(i3 > bit){
           bLoad = 1;
           i2--;
           i3 >>= 1;
       }
       else{
           SendDlgItemMessage(hWndIO,i2,WM_SETTEXT,0,(LPARAM)"");
           EnableWindow(GetDlgItem(hWndIO,i2),FALSE);
       }
   }
   for(;i2 < 26;i2++){
       SendDlgItemMessage(hWndIO,i2,WM_SETTEXT,0,(LPARAM)"");
       EnableWindow(GetDlgItem(hWndIO,i2),FALSE);
   }
   dwCurrentValue = 0;
   UpdateEditMemoryValue(0,TRUE,TRUE);
}
//---------------------------------------------------------------------------
static BOOL CALLBACK DlgProcEditmemoryInfo(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   int i;
   DWORD dw;
   char s[20];
   RECT rc;

   switch(uMsg){
       case WM_INITDIALOG:
           Translation(hwndDlg,0,IDD_DIALOG17);
       break;
       case WM_CLOSE:
           DestroyWindow(hwndDlg);
           hWndIO = NULL;
       break;
       case WM_COMMAND:
           if(HIWORD(wParam) == BN_CLICKED){
               if(LOWORD(wParam) == IDC_UPDIOREG)
                   bUpdateIOReg = SendMessage((HWND)lParam,BM_GETCHECK,0,0) == BST_CHECKED ? TRUE : FALSE;
               else if(LOWORD(wParam) == 26){
                   GetWindowRect((HWND)lParam,&rc);
                   MapWindowPoints(NULL,hwndDlg,(LPPOINT)&rc,2);
                   GetWindowText((HWND)lParam,s,19);
                   if(InputText(hwndDlg,&rc,0,s,19)){
                       dw = StrToHex(s);
                       wsprintf(s,"0x%04X",dw);
                       SetWindowText((HWND)lParam,s);
                       WriteMem(dwCurrentAddress,AMM_HWORD,dw);
                       UpdateEditMemoryValue(dw,FALSE,TRUE);
                       dw = dwCurrentAddress & 0x3FF;
                       io_write_handles[dw]((u16)dw,AMM_HWORD);
                   }
               }
               else{
                   dw = 0;
                   for(i=10;i<=26;i++){
                       if(SendDlgItemMessage(hwndDlg,i,BM_GETCHECK,0,0) == BST_CHECKED)
                           dw |= (1 << (i-10));
                   }
                   WriteMem(dwCurrentAddress,AMM_HWORD,dw);
                   wsprintf(s,"0x%04X",dw);
                   SendDlgItemMessage(hwndDlg,26,WM_SETTEXT,0,(LPARAM)s);
                   dw = dwCurrentAddress & 0x3FF;
                   io_write_handles[dw]((u16)dw,AMM_HWORD);
               }
           }
       break;
   }
   return FALSE;
}
//---------------------------------------------------------------------------
static int EditMemoryWithInfo(u32 dwAddress,u8 iMode)
{
   DWORD dwPos,dw;
   LPINFOMEM im;
   char s[30];

   if(pInfoMemList == NULL || bInfoMode == 0 || iMode != 1)
       return 0;
   dwAddress &= ~1;
   im = (LPINFOMEM)pInfoMemList->GetFirstItem(&dwPos);
   while(im != NULL){
       wsprintf(s,"0x%s",im->adr);
       dw = (DWORD)StrToHex(s);
       if(dwAddress == dw)
           break;
       im = (LPINFOMEM)pInfoMemList->GetNextItem(&dwPos);
   }
   if(im == NULL || im->descrbit[0] == 0){
       if(hWndIO != NULL)
           DestroyWindow(hWndIO);
       hWndIO = NULL;
       return 0;
   }
   currentInfoMem = im;
   dwCurrentAddress = dwAddress;
   if(hWndIO == NULL){
       hWndIO = CreateDialog(FindResourceInternal(IDD_DIALOG17,RT_DIALOG),MAKEINTRESOURCE(IDD_DIALOG17),
           hWndMem,(DLGPROC)DlgProcEditmemoryInfo);
       if(hWndIO == NULL)
           return 0;
   }
   else
       BringWindowToTop(hWndIO);
   UpdateEditmemoryInfo();
   return 1;
}
//---------------------------------------------------------------------------
static void OnEditMemory(HWND hwnd,LPPOINT pt)
{
   int line,pos,i,len;
   DWORD dw,dwAddress;
   RECT rc;
   char string[20];
   HDC hdc;
   SIZE sz;

   i = SendDlgItemMessage(hWndMem,IDC_COMBOBOX2,CB_GETCURSEL,0,0);
   dwAddress = MemoryAddress[i].Address + yScroll;
   if(dwAddress < 0x4000 && !UseBiosFile(TRUE) || MemoryAddress[i].Size < 2)
       return;
   hdc = GetDC(hwnd);
   if(hdc == NULL)
       return;
   SetRect(&rc,0,0,0,0);
   DrawText(hdc,lpszText,-1,&rc,DT_CALCRECT|DT_LEFT|DT_EXTERNALLEADING|DT_EDITCONTROL);
   GetTextExtentPoint32(hdc,"0",1,&sz);
   ::ReleaseDC(hwnd,hdc);
   sz.cy = (LONG)(((float)rc.bottom / 15.0) + .5);

   line = pt->y / sz.cy;
   for(i=pos=0;i < line;pos++){
       if(lpszText[pos] == 10)
           i++;
   }
   pt->x = (pt->x / sz.cx) * sz.cx;
   for(i=0;i <= pt->x;i += sz.cx){
       if(lpszText[pos] == 10 || lpszText[pos] == 13)
           return;
       pos++;
   }
   if(lpszText[pos] == 10 || lpszText[pos] == 13)
       return;
   pos -= (line * 13) + 11;
   i = SendDlgItemMessage(hWndMem,IDC_COMBOBOX3,CB_GETCURSEL,0,0);
   pt->x = 11 * sz.cx;
   switch(i){
       case 0:
           pos = (pos / 3);
           pt->x += (pos % 16) * 3 * sz.cx;
           dwAddress += pos;
           dw = ReadMem(dwAddress,AMM_BYTE);
       break;
       case 1:
           pos = (pos / 5);
           pt->x += (pos % 8) * 5 * sz.cx;
           dwAddress += pos << 1;
           dw = ReadMem(dwAddress,AMM_HWORD);
       break;
       case 2:
           pos = (pos / 9);
           pt->x += ((pos % 4)) * 9 * sz.cx;
           dwAddress += pos << 2;
           dw = ReadMem(dwAddress,AMM_WORD);
       break;
   }
   if(EditMemoryWithInfo(dwAddress,(u8)i))
       goto ex_OnEditMemory;
   len = (1 << (i+1)) + 1;
   pt->y = line * sz.cy;
   SetRect(&rc,pt->x,pt->y,pt->x + len * sz.cx,pt->y+sz.cy);
   wsprintf(string,"0x%0X",dw);
   if(!InputText(hwnd,&rc,0,string,len+3))
       return;
   dw = StrToHex(string);
   switch(i){
       case 0:
           WriteMem(dwAddress,AMM_BYTE,dw);
       break;
       case 1:
           WriteMem(dwAddress,AMM_HWORD,dw);
       break;
       case 2:
           WriteMem(dwAddress,AMM_WORD,dw);
       break;
   }
ex_OnEditMemory:
   UpdateMemoryDump(NULL,TRUE);
}
#endif
