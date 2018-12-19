#include "gba.h"
#include "gbaemu.h"
#include "debug.h"
#include "debug1.h"
#include "fstream.h"
#include "memory.h"
#include "resource.h"
#include "lcd.h"
#include "debsprite.h"
#include "debmem.h"
#include "debpal.h"
#include "debpalobj.h"
#include "debug1.h"
#include "trad.h"
#include "lregkey.h"
#include "sound.h"
#include "debbkg.h"
#include "source.h"
#include "brkmem.h"
#include "exec.h"
#include "sio.h"

//---------------------------------------------------------------------------
#ifdef _DEBUG
char rigaDebug[255];
DECODEARM *debug_handles;
char **opcode_strings;
DECODETHUMB *debug_handles_t;
char **opcode_strings_t;
char *irq_strings[16];

main_message MAIN_MESSAGE;
LPBREAKPOINT listBreakPoint;
LRecentFile *pDebugRecentFile;

BOOL bPass,bDebug,bWriteDebug,bInLoop;
HWND hwndDebug;
DWORD dwKey;
LFile *fpDebug;
DEBUGGER dbg;

MEMORYINFO MemoryAddress[] = {
   {"BIOS ROM",0x00000000,0x4000,AMM_ALL,0,AMM_ALL},
   {"EXTERN WRAM",0x02000000,0x40000,AMM_ALL,0,AMM_ALL},
   {"INTERN WRAM",0x03000000,0x8000,AMM_ALL,0,AMM_ALL},
   {"I/O",0x04000000,0x0400,AMM_ALL,IDI_INFO_REG,AMM_HWORD},
   {"PALETTE",0x05000000,0x0400,AMM_ALL,0,AMM_ALL},
   {"VIDEO",0x06000000,0x18000,AMM_ALL,0,AMM_ALL},
   {"OAM",0x07000000,0x0400,AMM_ALL,0,AMM_ALL},
   {"GAME PACK 0",0x08000000,1,AMM_ALL,0,AMM_ALL},
   {"GAME PACK 1",0x0A000000,1,AMM_HWORD,0,AMM_HWORD},
   {"GAME PACK 2",0x0C000000,1,AMM_HWORD,0,AMM_HWORD},
   {"CART RAM",0x0E000000,0x10000,AMM_BYTE,0,AMM_BYTE}
};

char *condition_strings[] = {"eq","ne","cs","cc","mi","pl","vs","vc","hi","ls","ge","lt","gt","le","",""};
char *register_strings[]  = {"r0","r1","r2","r3","r4","r5","r6","r7","r8","r9","r10","r11","r12","sp","lr","pc"};
char *shift_strings[]     = {"lsl","lsr","asr","ror"};

UINT dwBreakPointItem,SizeMemoryAddressStruct = sizeof(MemoryAddress) / sizeof(MEMORYINFO);
//---------------------------------------------------------------------------
static u8 GetCurrentAddressPipe(u32 *Address,u8 withindex,u8 remap);
u8 GetCurrentIncPipe(u8 indexView);
static u8 SaveBreakPointListFromName(char *szFile);
static void IsChangedBreakPointList();
//---------------------------------------------------------------------------
void SetGamePakRomSize(u8 index,u32 size)
{
   if(index > 2)
       return;
   index += (u8)7;
   MemoryAddress[index].Size = size;
   if(hwndDebug != NULL)
       UpdateVertScrollBarDisDump(MemoryAddress[index].Address,index);
   UpdateVertScrollBarMemoryDump((DWORD)-1,-1);
}
//---------------------------------------------------------------------------
u8 IsMessageEnable(u8 message)
{
   return MAIN_MESSAGE.bEnable && MAIN_MESSAGE.bValue[message];
}
//---------------------------------------------------------------------------
u8 InitDebug()
{
   debug_handles = NULL;
   debug_handles_t = NULL;
   opcode_strings_t = NULL;
   opcode_strings = NULL;

   if((debug_handles = new DECODEARM[0x1002]) == NULL)
       return FALSE;
   if((debug_handles_t = new DECODETHUMB[0x402]) == NULL)
       return FALSE;
   if((opcode_strings = new char*[0x1002]) == NULL)
       return FALSE;
   if((opcode_strings_t = new char*[0x402]) == NULL)
       return FALSE;
   InitDebugMemoryWindow();
#ifdef _DEBPRO
   InitDebugSpriteWindow();
   InitDebugBkgWindow();
   InitDebugDMAWindow();
   InitDebugSourceWindow();
#endif
   InitDebugObjPalWindow();
   InitDebugBgPalWindow();

   hwndDebugToolBar = hwndDebug = NULL;
   OldListBoxBreakPointWndProc = NULL;
   ResetDebug();
   ZeroMemory(&MAIN_MESSAGE,sizeof(main_message));
   listBreakPoint = MAIN_MESSAGE.breakpoint;
   pDebugRecentFile = NULL;
   fpDebug = NULL;
   bDebug = 0;

   return TRUE;
}
//---------------------------------------------------------------------------
u8 DestroyDebug()
{
   if(debug_handles != NULL)
       delete debug_handles;
   if(debug_handles_t != NULL)
       delete debug_handles_t;
   if(opcode_strings_t != NULL)
       delete opcode_strings_t;
   if(opcode_strings != NULL)
       delete opcode_strings;
   debug_handles = NULL;
   debug_handles_t = NULL;
   opcode_strings_t = NULL;
   opcode_strings = NULL;

   if(fpDebug != NULL)
       fpDebug->Close();
   fpDebug = NULL;
   if(pDebugRecentFile != NULL){
       pDebugRecentFile->Save("Software\\RascalBoy\\Debug\\RecentFile");
       delete pDebugRecentFile;
   }
   pDebugRecentFile = NULL;
   IsChangedBreakPointList();
   if(hDebugPopupMenu != NULL)
       ::DestroyMenu(hDebugPopupMenu);
   hDebugPopupMenu = NULL;
   if(hwndDebugStatusBar != NULL)
       ::DestroyWindow(hwndDebugStatusBar);
   hwndDebugStatusBar = NULL;
   if(hwndDebugToolBar != NULL)
       ::DestroyWindow(hwndDebugToolBar);
   hwndDebugToolBar = NULL;
#ifdef _DEBPRO
   DestroyDebugSpriteWindow();
   DestroyDebugDMAWindow();
   DestroyDebugBkgWindow();
#endif
   DestroyDebugMemoryWindow();
   DestroyDebugObjPalWindow();
   DestroyDebugBgPalWindow();
   if(hwndDebug != NULL)
       ::DestroyWindow(hwndDebug);
   hwndDebug = NULL;
   return TRUE;
}
//---------------------------------------------------------------------------
u8 CreateDebugWindow(u8 mode)
{
    switch(mode){
       case 0:
           if(hwndDebug == NULL){
               hwndDebug = ::CreateDialog(hInstance,MAKEINTRESOURCE(IDD_DIALOG1),NULL,(DLGPROC)DialogProc);
               pDebugRecentFile = new LRecentFile();
               pDebugRecentFile->Read("Software\\RascalBoy\\Debug\\RecentFile");
               ::BringWindowToTop(hWin);
           }
           else
               ::BringWindowToTop(hwndDebug);
           EnterDebugMode();
       break;
       case 1:
          CreateDebugMemory();
       break;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
void BringDebugToTop(HWND win)
{
   if(hwndDebug != NULL)
       ::SetWindowPos(hwndDebug,win,0,0,0,0,SWP_NOMOVE|SWP_NOSENDCHANGING|SWP_NOACTIVATE|SWP_NOSIZE);
   BringDebugMemoryToTop(win);
}
//---------------------------------------------------------------------------
void ResetDebug()
{
   u8 i;

   bPass = TRUE;
   bDebug = 0;
   if(hwndDebug == NULL)
       return;
   ::SendDlgItemMessage(hwndDebug,IDC_LIST1,LB_RESETCONTENT,0,0);
   ::SendDlgItemMessage(hwndDebug,IDC_LIST2,LB_RESETCONTENT,0,0);
#ifdef _DEBPRO
   ::SendDlgItemMessage(hwndDebug,IDC_LIST3,LB_RESETCONTENT,0,0);
   ::SendDlgItemMessage(hwndDebug,IDC_LIST5,LB_RESETCONTENT,0,0);
   ::SendDlgItemMessage(hwndDebug,IDC_LIST6,LB_RESETCONTENT,0,0);
   ::SendDlgItemMessage(hwndDebug,IDC_LIST4,LB_SETCURSEL,(WPARAM)-1,0);
   if(GetMenuState(GetMenu(hwndDebug),ID_DEBUG_PROCEDURE,MF_BYCOMMAND) == MF_CHECKED)
       SendMessage(hwndDebug,WM_COMMAND,MAKEWPARAM(ID_DEBUG_PROCEDURE,0),0);
   if(MAIN_MESSAGE.bEnable)
       ::SendMessage(hwndDebug,WM_COMMAND,MAKEWPARAM(ID_DEBUG_MESSAGE,0),NULL);
#endif
   ::SendDlgItemMessage(hwndDebug,IDC_RASTERLINE,WM_SETTEXT,0,(LPARAM)"");
   ::SendDlgItemMessage(hwndDebug,IDC_STATUS,WM_SETTEXT,0,(LPARAM)"");
   ::SendDlgItemMessage(hwndDebug,IDC_IRQRET,WM_SETTEXT,0,(LPARAM)"");
   ::SendDlgItemMessage(hwndDebug,IDC_MODE,WM_SETTEXT,0,(LPARAM)"");
   ::SendDlgItemMessage(hwndDebug,IDC_COMBOBOX3,CB_SETCURSEL,(WPARAM)2,0);

   for(i=0;i<MAX_BREAK;i++)
       listBreakPoint[i].PassCount = 0;
   ZeroMemory(&dbg.views[DBV_RUN],sizeof(DISVIEW));
   ZeroMemory(&dbg.views[DBV_VIEW],sizeof(DISVIEW));
   dbg.yScroll = 0;
   dbg.bTrackMode = 2;
   EnterDebugMode();
   bPass = FALSE;
   bDebug = 1;
}
//---------------------------------------------------------------------------
static u8 GetCurrentAddressPipe(u32 *Address,u8 withindex,u8 remap)
{
   char s[20];
   u32 d,i,mask;

   if(bThreadRun){
       d = arm.ra;
       if((CPSR & T_BIT) != 0){
           if((REG_PC - arm.ra) > 4)
               d = REG_PC - 2;
           d &= ~1;
       }
       else{
           if((REG_PC - arm.ra) > 8)
               d = REG_PC - 4;
       }
   }
   else
       d = MemoryAddress[7].Address;
   if(remap && (i = MemoryAddressToIndex(d)) != -1){
       mask = 1 << (log2(MemoryAddress[i].Size - 1) + 1);
       d &= MemoryAddress[i].Address | (mask - 1);
   }
   *Address = d;
   if(!withindex)
       return 0;
   wsprintf(s,"%d",d);
   return (u8)MemoryStringToIndex(s);
}
//---------------------------------------------------------------------------
void EnterDebugMode()
{
   char s[20];
   DWORD dw;
   u8 index;

   if(!bDebug){
       if(bThreadRun)
           index = GetCurrentAddressPipe(&dw,TRUE,TRUE);
       else{
           dw = MemoryAddress[7].Address;
           index = 7;
       }
       wsprintf(s,"0x%08X",dw);
       SendDlgItemMessage(hwndDebug,IDC_COMBOBOX2,CB_SETCURSEL,index,0);
       SetWindowText(GetDlgItem(hwndDebug,IDC_EDIT2),s);
       dbg.iCurrentView = DBV_RUN;
       bDebug = 1;
   }
   else
       index = GetCurrentAddressPipe(&dw,TRUE,TRUE);
   UpdateVertScrollBarDisDump(dw,index);
   bPass = FALSE;
   UpdateDebugToolBar();
   dbg.bCurrentTrackMode = 0;
   if(bThreadRun)
       UpdateDebugStatusBar("STOP",1,0);
   PauseRestartSound(0x10);
   SetExecFunction((u8)((CPSR & T_BIT) >> 5));
}
//---------------------------------------------------------------------------
void ExitDebugMode()
{
   if(bThreadRun)
       UpdateDebugStatusBar("RUN",1,0);
   bPass = TRUE;
   UpdateDebugToolBar();
   BringWindowToTop(hWin);
   ZeroMemory(&dbg.views[DBV_RUN],sizeof(DISVIEW));
   ZeroMemory(&dbg.views[DBV_VIEW],sizeof(DISVIEW));
   PauseRestartSound(0x1);
}
//---------------------------------------------------------------------------
void WriteDebugFile()
{
   char *s;
   int i,i1;

   if(bWriteDebug && fpDebug != NULL && fpDebug->IsOpen()){
       i = SendDlgItemMessage(hwndDebug,IDC_LIST1,LB_GETCURSEL,0,0);
       if(i == LB_ERR)
           return;
       i1 = SendDlgItemMessage(hwndDebug,IDC_LIST1,LB_GETTEXTLEN,(WPARAM)i,0);
       if(i1 == LB_ERR)
           return;
       if((s = new char[i1+1]) == NULL)
           return;
       SendDlgItemMessage(hwndDebug,IDC_LIST1,LB_GETTEXT,(WPARAM)i,(LPARAM)s);
       fpDebug->WriteF("%s\r\n",s);
       delete s;
   }
}
//---------------------------------------------------------------------------
DWORD StrToHex(char *string)
{
   DWORD iValue;
   int i,i1,i2;
   char c[3],*p,car;
   LString s;

   s = string;
   s.LowerCase();
   if(s[1] != 'r' && (s[1] == '0' || strpbrk(s.c_str(),"abcdefx") !=  NULL)){
       lstrcpyn(c,string,3);
       strlwr(c);
       if((p = strrchr(c,'x')) != NULL)
           s = string + (p - c) + 1;
       else
           s = string;
       s.UpperCase();
       for(iValue = 0,i2 = s.Length(),i = 1;i<=i2;i++){
           car = s[i];
           if(car > 64 && car < 'G')
               i1 = (s[i] - 55);
           else if(car > 47 && car < 58)
               i1 = s[i] - 48;
           else
               i1 = 0;
           iValue = (iValue << 4) + i1;
       }
   }
#ifdef _DEBPRO
   else if(s[1] == 'r'){
       sscanf(s.c_str()+1,"%d",&i);
       if(i < 0 || i > 15)
           i = 15;
       iValue = GP_REG[i];
       i = i > 9 ? 2 : 1;
       s = s.SubString(i+2,s.Length() - i - 1).AllTrim();
       if(s.Length() > 1){
           s = s.SubString(2,s.Length() - 1);
           iValue += StrToHex(s.c_str());
       }
   }
#endif
   else
       iValue = atoi(string);
   return iValue;
}
//---------------------------------------------------------------------------
u8 ToBreakPointString(u32 adress,char *string,int maxLen)
{
   char s1[30];
   u8 i;

   wsprintf(s1,"0x%08X",adress);
   i = (u8)lstrlen(s1);
   if(maxLen > i)
       lstrcpy(string,s1);
   return i;
}
//---------------------------------------------------------------------------
void RefillBreakPointList()
{
   BREAKPOINT pa[MAX_BREAK];
   u8 i,i1;

   for(i=i1=0;i<MAIN_MESSAGE.nBreak && i < MAX_BREAK;i++){
       if(listBreakPoint[i].adress == 0 && listBreakPoint[i].bEnable == 0)
           continue;
       CopyMemory(&pa[i1++],&listBreakPoint[i],sizeof(BREAKPOINT));
   }
   if(i1 != MAIN_MESSAGE.nBreak)
       MAIN_MESSAGE.bBreakChanged = TRUE;
   ZeroMemory(listBreakPoint,sizeof(BREAKPOINT) * MAX_BREAK);
   CopyMemory(listBreakPoint,pa,i1 * sizeof(BREAKPOINT));
   MAIN_MESSAGE.nBreak = i1;
}
//---------------------------------------------------------------------------
u32 BreapointFromListBox(char *p)
{
   int wID;
   char s1[130],s[12];
   HWND hwnd;

   if(p != NULL)
       *p = 0;
   hwnd = GetDlgItem(hwndDebug,IDC_LIST1);
   if((wID = SendMessage(hwnd,LB_GETCURSEL,0,0)) == LB_ERR)
       return 0;
   SendMessage(hwnd,LB_GETTEXT,wID,(LPARAM)s1);
   ZeroMemory(s,12);
   CopyMemory(s,s1,8);
   if(p != NULL)
       lstrcpy(p,s);
   return StrToHex(s);
}
//---------------------------------------------------------------------------
u8 InsertBreapointFromListBox()
{
   DWORD dw;

   dw = BreapointFromListBox(NULL);
   if(InsertBreakPoint(dw) > 0)
       return 1;
   return 0;
}
//---------------------------------------------------------------------------
u8 GetCurrentIncPipe(u8 indexView)
{
   u8 inc;

   if(dbg.bTrackMode != 2 && indexView != DBV_RUN)
       inc = (u8)(1 << (2 - dbg.bTrackMode));
   else
       inc = (u8)((CPSR & T_BIT) ? 2 : 4);
   return inc;
}
//---------------------------------------------------------------------------
int FillListDiss(u32 *p,u8 indexView,u8 nItem)
{
   DWORD dwAddress;
   u32 opcode;
   u8 inc;
   int i;
   HWND hwnd;
   char riga[200];
   LPDISVIEW p1;

   if(!bThreadRun)
       return 0;
   if(p != NULL)
       dwAddress = *p;
   else
       GetCurrentAddressPipe(&dwAddress,FALSE,TRUE);
   p1 = &dbg.views[(dbg.iCurrentView = indexView)];
   hwnd = GetDlgItem(hwndDebug,IDC_LIST1);
   inc = GetCurrentIncPipe(indexView);
   if(nItem != 0){
       if((dwAddress - p1->EndAddress) > inc || !dwAddress){//Se super il contenuto della view
           p1->StartAddress = dwAddress;
           i = dbg.iMaxItem;
           SendMessage(hwnd,LB_RESETCONTENT,0,0);
       }
       else{
           i = 1;
           SendMessage(hwnd,LB_DELETESTRING,0,0);
           if(indexView == DBV_RUN)
               p1->StartAddress += inc;
       }
   }
   else{
       p1->StartAddress = dwAddress;
       i = dbg.iMaxItem;
       SendMessage(hwnd,LB_RESETCONTENT,0,0);
   }
   for(;i > 0;i--){
       if(inc == 2){
           opcode = ReadMem(dwAddress,AMM_HWORD);
           wsprintf(riga,"%08X %08X ",dwAddress,opcode);
           debug_handles_t[opcode >> 6]((u16)opcode,dwAddress,rigaDebug);
       }
       else{
           opcode = ReadMem(dwAddress,AMM_WORD);
           wsprintf(riga,"%08X %08X ",dwAddress,opcode);
           debug_handles[((opcode&0xFF00000)>>16)|((opcode&0xF0)>>4)](opcode, dwAddress, rigaDebug);
       }
       lstrcat(riga,rigaDebug);
       SendMessage(hwnd,LB_INSERTSTRING,(WPARAM)-1,(LPARAM)riga);
       dwAddress += inc;
   }
   p1->EndAddress = dwAddress - inc;
   if(indexView == DBV_RUN)
       CopyMemory(&dbg.views[DBV_VIEW],p1,sizeof(DISVIEW));
   return 1;
}
//---------------------------------------------------------------------------
void UpdateVertScrollBarDisDump(DWORD address,int index)
{
   char s[31];
   SCROLLINFO si;
   HWND hwnd;
   u8 i;

   if(index == -1){
       wsprintf(s,"%d",address);
       index = MemoryStringToIndex(s);
   }
   hwnd = GetDlgItem(hwndDebug,IDC_VSBDIS);
   ZeroMemory(&si,sizeof(SCROLLINFO));
   si.cbSize = sizeof(SCROLLINFO);
   si.fMask = SIF_ALL;
   si.nMax = MemoryAddress[index].Size-1;
   if(si.nMax != 0){
       i = (u8)GetCurrentIncPipe(dbg.iCurrentView);
       si.nPage = (dbg.iMaxItem * i);
       dbg.yScroll = ((address - MemoryAddress[index].Address) / i) * i;
   }
   else{
       si.nPage = 1;
       dbg.yScroll = 0;
   }
   SetScrollInfo(hwnd,SB_CTL,&si,FALSE);
   SetScrollPos(hwnd,SB_CTL,dbg.yScroll,TRUE);
}
//---------------------------------------------------------------------------
void UpdateAddressBar(u32 dwAddress,u8 forceUpdate)
{
   u8 index;
   char s[20];

   wsprintf(s,"0x%08X",dwAddress);
   index = (u8)MemoryStringToIndex(s);

   if(forceUpdate != 0 || SendDlgItemMessage(hwndDebug,IDC_COMBOBOX2,CB_GETCURSEL,0,0) != index){
       SendDlgItemMessage(hwndDebug,IDC_COMBOBOX2,CB_SETCURSEL,(WPARAM)index,0);
       UpdateVertScrollBarDisDump(dwAddress,index);
   }
   else{
       dwAddress -= MemoryAddress[index].Address;
       dbg.yScroll = dwAddress;
       SetScrollPos(GetDlgItem(hwndDebug,IDC_VSBDIS),SB_CTL,dbg.yScroll,TRUE);
   }
   SendDlgItemMessage(hwndDebug,IDC_COMBOBOX3,CB_SETCURSEL,(WPARAM)dbg.bTrackMode,0);
}
//---------------------------------------------------------------------------
void WaitDebugger(BOOL bThumb)
{
   BOOL bFlag;
   int i,item;
   DWORD dw;
   char s[15];
   LPDISVIEW p;
   u8 inc;

   if(!bDebug || bPass)
       return;
   bFlag = FALSE;                                                    //80292d6      3001991
   p = &dbg.views[DBV_RUN];
   GetCurrentAddressPipe(&dw,FALSE,TRUE);
   inc = GetCurrentIncPipe(DBV_RUN);
   if(dbg.bTrackMode != 2 || inc != dbg.bCurrentTrackMode){
       dbg.bTrackMode = 2;
       dbg.bCurrentTrackMode = inc;
       bFlag = TRUE;
   }
   else if(p->StartAddress == 0)
       bFlag = TRUE;
   else if(dbg.iCurrentView != DBV_RUN){
       i = ((int)p->StartAddress - (int)dbg.views[DBV_VIEW].StartAddress);
       if(abs(i) < (dbg.iMaxItem >> 1)){
           if(i < 0){
               CopyMemory(p,&dbg.views[DBV_VIEW],sizeof(DISVIEW));
               dbg.iCurrentView = DBV_RUN;
           }
       }
       else
           bFlag = TRUE;
   }
   else if(dw < p->StartAddress || dw > p->EndAddress)
       bFlag = TRUE;
   if(bFlag)
       FillListDiss(NULL,DBV_RUN,1);
   else
       wsprintf(s,"%08X",dw);
   item = (dw - p->StartAddress) / inc;
   SetFocus(hwndDebug);
   SendDlgItemMessage(hwndDebug,IDC_LIST1,LB_SETCURSEL,item,0);
   bFlag = TRUE;
   UpdateAddressBar(p->StartAddress,0);
   bInLoop = TRUE;
   WriteDebugFile();
   while(!bPass && bFlag && hwndDebug != NULL && !bQuit){
       ProcessaMessaggi();
       switch(dwKey){
           case VK_F8:
               bFlag = FALSE;
               bPass = FALSE;
           break;
           case VK_F6:
               MAIN_MESSAGE.dwBreak = BreapointFromListBox(s);
               MAIN_MESSAGE.BreakMode = 1;
               ExitDebugMode();
           break;
           case VK_F11:
               if(bThumb)
                   MAIN_MESSAGE.dwBreak = REG_PC;
               else
                   MAIN_MESSAGE.dwBreak = REG_PC - 4;
               MAIN_MESSAGE.BreakMode = 1;
               ExitDebugMode();
           break;
           case VK_F9:
               InsertBreapointFromListBox();
           break;
           case VK_F5:
               MAIN_MESSAGE.BreakMode = 0;
               ExitDebugMode();
           break;
           default:
               SleepEx(5,FALSE);
           break;
       }
       dwKey = 0;
   }
   bInLoop = FALSE;
}
#ifdef _DEBPRO
//---------------------------------------------------------------------------
void InsertLineIntoListBox(WORD wID,char *string)
{
   u32 i1;
   char line[300];
   HWND hwndControl;
   
   if(!bDebug) return;
   hwndControl = GetDlgItem(hwndDebug,wID);
   i1 = SendMessage(hwndControl,LB_GETCOUNT,0,0);
   if(i1 > 10000){
       SendMessage(hwndControl,LB_DELETESTRING,0,0);
       i1--;
   }
   wsprintf(line,"0x%08X ",arm.ra);
   lstrcat(line,string);
   SendMessage(hwndControl,LB_ADDSTRING,0,(LPARAM)line);
   SendMessage(hwndControl,LB_SETCURSEL,i1,0);
}
//---------------------------------------------------------------------------
void InsertException(char *mes,...)
{
   char line[150];
   va_list ap;

   if(!MAIN_MESSAGE.bExceptions)
       return;
   va_start(ap,mes);
   vsprintf(line,mes,ap);
   va_end(ap);
   InsertLineIntoListBox(IDC_LIST6,line);
}
//---------------------------------------------------------------------------
void InsertStackAddress(u32 address,char *mes,...)
{
   char string[30],line[100];
   va_list ap;

   if(!MAIN_MESSAGE.bStack)
       return;
   va_start(ap,mes);
   vsprintf(line,mes,ap);
   wsprintf(string," 0x%08X ",address);
   lstrcat(line,string);
   va_end(ap);
   InsertLineIntoListBox(IDC_LIST5,line);
}
//---------------------------------------------------------------------------
u8 ConditionToValue(u8 Index,int *rd,u8 *cond,u32 *value)
{
   LPBREAKPOINT p;
   char s[50];
   int rs;

   p = &listBreakPoint[Index];
   if((lstrlen(p->Condition)) != 0){
       lstrcpy(s,p->Condition);
       strlwr(s);
       sscanf(s,"r%02d%02d",rd,cond);
       if(s[5] == 'r')
           sscanf(&s[5],"r%02d",value);
       else{
           rs = -1;
           if(s[6] == 'x')
               sscanf(&s[7],"%8X",value);
           else
               sscanf(&s[5],"%d",value);
       }
   }
   else
       return 0;
   if(s[5] == 'r')
       return 2;
   else
       return 1;
}
//---------------------------------------------------------------------------
LString ConditionToString(u8 Index)
{
   LString condition;
   u8 cond,res;
   char s[50];
   int rd;
   u32 value;

   condition = "";
   if((res = ConditionToValue(Index,&rd,&cond,&value)) != 0){
       condition = "r";
       condition += (int)rd;
       switch(cond){
           case CC_EQ:
               condition += " == ";
           break;
           case CC_NE:
               condition += " != ";
           break;
           case CC_GT:
               condition += " >  ";
           break;
           case CC_GE:
               condition += " >= ";
           break;
           case CC_LT:
               condition += " <  ";
           break;
           case CC_LE:
               condition += " <= ";
           break;
       }
       if(res == 1)
       	wsprintf(s," 0x%08X",value);
       else
			wsprintf(s," r%d",value);
       condition += s;
   }
   return condition;
}
//---------------------------------------------------------------------------
void ControlBreakOn(u8 nBreak)
{
   if(!bDebug || !bPass)
       return;
   if(nBreak < BREAK_ENTER_IRQ14 && MAIN_MESSAGE.bBreak[BREAK_ENTER_IRQ] != 0)
       EnterDebugMode();
   else if(nBreak < BREAK_EXIT_IRQ14 && MAIN_MESSAGE.bBreak[BREAK_EXIT_IRQ] != 0)
       EnterDebugMode();
   else if(MAIN_MESSAGE.bBreak[nBreak] == 0)
       return;
   EnterDebugMode();
}
//---------------------------------------------------------------------------
u8 CheckStopBreakPoint(u8 Index)
{
   LPBREAKPOINT p;
   u8 res;
   char s[50];
   int rd,condition,rs;
   u32 value;

   res = 0;
   p = &listBreakPoint[Index];
   if((lstrlen(p->Condition)) != 0){
       lstrcpy(s,p->Condition);
       strlwr(s);
       sscanf(&s[1],"%02d%02d",&rd,&condition);
       if(s[5] == 'r'){
           sscanf(&s[5],"r%02d",&rs);
           value = GP_REG[rs];
       }
       else{
           if(s[6] == 'x')
               sscanf(&s[7],"%8X",&value);
           else
               sscanf(&s[5],"%d",&value);
       }
       switch(condition){
           case CC_EQ:
               if(GP_REG[rd] == value)
                   res = 1;
           break;
           case CC_NE:
               if(GP_REG[rd] != value)
                   res = 1;
           break;
           case CC_GT:
               if((int)GP_REG[rd] > (int)value)
                   res = 1;
           break;
           case CC_GE:
               if((int)GP_REG[rd] >= (int)value)
                   res = 1;
           break;
           case CC_LT:
               if((int)GP_REG[rd] < (int)value)
                   res = 1;
           break;
           case CC_LE:
               if((int)GP_REG[rd] <= (int)value)
                   res = 1;
           break;
       }
   }
   else{
       if(p->PassCountCondition != 0){
           p->PassCount++;
           if(p->PassCount == p->PassCountCondition){
               res = 1;
               p->PassCount = 0;
           }
       }
       else
           res = 1;
   }
   return res;
}
//---------------------------------------------------------------------------
void ControlMemoryBreakPoint(u32 dwCurrentAddress,u8 mode,u32 data)
{
   s8 i;
   LString mes;
   PMEMORYBRK p;
   BOOL bFlag;
   u8 accessMode;

   if(!bDebug || MAIN_MESSAGE.nBreak < 1)
       return;
   dwCurrentAddress = (dwCurrentAddress & 0x0F000000) | (dwCurrentAddress & 0xFFFF);
   accessMode = (u8)(mode >> 4);
   mode &= 0xF;
   for(i = (s8)(MAIN_MESSAGE.nBreak - 1);i >= 0;i--){
       if(listBreakPoint[i].Type != BT_MEMORY || listBreakPoint[i].bEnable == 0 || dwCurrentAddress != listBreakPoint[i].adress)
           continue;
       p = (PMEMORYBRK)listBreakPoint[i].Condition;
       bFlag = FALSE;
       if(mode){//scrittura
           if(p->bModify){
               if(ReadMem(dwCurrentAddress,accessMode) != data)
                   bFlag = TRUE;
           }
           if(p->bWrite)
               bFlag = TRUE;

       }
       else{
           if(p->bRead){
               bFlag = TRUE;
               data = ReadMem(dwCurrentAddress,accessMode);
           }
       }
       if(!bFlag || (p->mAccess && !(p->mAccess & accessMode)))
           continue;
       switch(accessMode){
           case AMM_BYTE:
               mes = "byte";
           break;
           case AMM_HWORD:
               mes = "halfword";
           break;
           case AMM_WORD:
               mes = "word";
           break;
       }
       WriteMessage(0xFF,"Memory breakpoint 0x%08X %s 0x%08X %s",dwCurrentAddress,mode ? "Write" : "Read",data,mes.c_str());
       if(!p->bBreak)
           continue;
       EnterDebugMode();
       SetActiveWindow(hwndDebug);
       wTimerBreakPointID = SetTimer(hwndDebug,TM_BREAKPOINT,100,NULL);
       mes = TranslateGetMessage(0xF000);
       UpdateDebugStatusBar(mes.c_str(),0,1);
       break;
   }
}
#endif
//---------------------------------------------------------------------------
void ControlBreakPoint()
{
   s8 i;
   char s[30];
   u32 dwCurrentAddress;
   LString mes;

   UpdateEditMemoryValue(0,TRUE,FALSE);
   if(dwKey == VK_F4){
       dwKey = 0;
       EnterDebugMode();
       return;
   }
   if(!bPass || (MAIN_MESSAGE.BreakMode == 0 && MAIN_MESSAGE.nBreak < 1))
       return;
   GetCurrentAddressPipe(&dwCurrentAddress,FALSE,FALSE);
   if(MAIN_MESSAGE.BreakMode == 1){
       if(dwCurrentAddress == MAIN_MESSAGE.dwBreak){
           MAIN_MESSAGE.dwBreak = 0;
           MAIN_MESSAGE.BreakMode = 0;
           EnterDebugMode();
           return;
       }
   }
   for(i = (s8)(MAIN_MESSAGE.nBreak - 1);i >= 0;i--){
       if(listBreakPoint[i].Type == BT_PROGRAM && listBreakPoint[i].bEnable != 0 && dwCurrentAddress == listBreakPoint[i].adress){
#ifdef _DEBPRO
           if(!CheckStopBreakPoint(i))
               continue;
#endif
           EnterDebugMode();
           ToBreakPointString(dwCurrentAddress,s,29);
           SetWindowText(hwndDebugComboBox1,s);
           SetActiveWindow(hwndDebug);
           wTimerBreakPointID = SetTimer(hwndDebug,TM_BREAKPOINT,100,NULL);
           if(SendDlgItemMessage(hwndDebug,IDC_LIST4,LB_SELECTSTRING,(WPARAM)-1,(LPARAM)(s+2)) != LB_ERR){
           	TabCtrl_SetCurSel(GetDlgItem(hwndDebug,IDC_TAB1),1);
               OnChangePageTab1();
           }
           mes = TranslateGetMessage(0xF000);
           UpdateDebugStatusBar(mes.c_str(),0,1);
           break;
       }
   }
}
//---------------------------------------------------------------------------
void OnDebugVScroll(WPARAM wParam,LPARAM lParam)
{
   SCROLLINFO si;
   HWND hwnd;
   int i;
   u8 inc;
   DWORD dw;

   hwnd = (HWND)lParam;
   ZeroMemory(&si,sizeof(SCROLLINFO));
   si.cbSize = sizeof(SCROLLINFO);
   si.fMask = SIF_ALL;
   if(!GetScrollInfo(hwnd,SB_CTL,&si))
       return;
   inc = GetCurrentIncPipe(DBV_VIEW);
   switch(LOWORD(wParam)){
       case SB_TOP:
           i = 0;
       break;
       case SB_BOTTOM:
           i = (si.nMax - si.nPage) + 1;
       break;
       case SB_PAGEDOWN:
           i = dbg.yScroll + inc * dbg.iMaxItem;
           if(i > (int)((si.nMax - si.nPage)+1))
               i = (si.nMax - si.nPage) + 1;
       break;
       case SB_PAGEUP:
           if((i = dbg.yScroll - inc * dbg.iMaxItem) < 0)
               i = 0;
       break;
       case SB_LINEUP:
           if((i = dbg.yScroll - inc) < 0)
               i = 0;
       break;
       case SB_LINEDOWN:
           i = dbg.yScroll + inc;
           if(i > (int)((si.nMax - si.nPage)+1))
               i = (si.nMax - si.nPage) + 1;
       break;
       case SB_THUMBPOSITION:
           i = si.nPos;
       break;
       case SB_THUMBTRACK:
           i = si.nTrackPos;
       break;
       default:
           i = dbg.yScroll;
       break;
   }
   i = (i / inc) * inc;
   if(i == (int)dbg.yScroll)
       return;
   dbg.yScroll = i;
   ::SetScrollPos(hwnd,SB_CTL,i,TRUE);
   dw = SendDlgItemMessage(hwndDebug,IDC_COMBOBOX2,CB_GETCURSEL,0,0);
   dw = MemoryAddress[dw].Address + i;
   FillListDiss(&dw,DBV_VIEW,1);
}
//---------------------------------------------------------------------------
void WriteMessage(u8 message,char *mes,...)
{
#ifdef _DEBPRO
   va_list ap;
   char string[500];

   if(!bDebug || (message != 0xFF && (!MAIN_MESSAGE.bEnable || !MAIN_MESSAGE.bValue[message])))
       return;

   va_start(ap,mes);
   vsprintf(string,mes,ap);
   va_end(ap);
   InsertLineIntoListBox(IDC_LIST3,string);
#endif
}
//---------------------------------------------------------------------------
void UpdateRegister()
{
   HWND hwndControl;
   char riga[100],c[4];

   hwndControl = GetDlgItem(hwndDebug,ID_DEBUG_DIS);
   SendMessage(hwndControl,LB_RESETCONTENT,0,0);
   wsprintf(riga,"R0   %08X",GP_REG[0]);
   SendMessage(hwndControl,LB_ADDSTRING,0,(LPARAM)riga);
   wsprintf(riga,"R1   %08X",GP_REG[1]);
   SendMessage(hwndControl,LB_ADDSTRING,0,(LPARAM)riga);
   wsprintf(riga,"R2   %08X",GP_REG[2]);
   SendMessage(hwndControl,LB_ADDSTRING,2,(LPARAM)riga);
   wsprintf(riga,"R3   %08X",GP_REG[3]);
   SendMessage(hwndControl,LB_ADDSTRING,3,(LPARAM)riga);
   wsprintf(riga,"R4   %08X",GP_REG[4]);
   SendMessage(hwndControl,LB_ADDSTRING,4,(LPARAM)riga);
   wsprintf(riga,"R5   %08X",GP_REG[5]);
   SendMessage(hwndControl,LB_ADDSTRING,5,(LPARAM)riga);
   wsprintf(riga,"R6   %08X",GP_REG[6]);
   SendMessage(hwndControl,LB_ADDSTRING,6,(LPARAM)riga);
   wsprintf(riga,"R7   %08X",GP_REG[7]);
   SendMessage(hwndControl,LB_ADDSTRING,7,(LPARAM)riga);
   wsprintf(riga,"R8   %08X",GP_REG[8]);
   SendMessage(hwndControl,LB_ADDSTRING,8,(LPARAM)riga);
   wsprintf(riga,"R9   %08X",GP_REG[9]);
   SendMessage(hwndControl,LB_ADDSTRING,9,(LPARAM)riga);
   wsprintf(riga,"R10 %08X",GP_REG[10]);
   SendMessage(hwndControl,LB_ADDSTRING,10,(LPARAM)riga);
   wsprintf(riga,"R11 %08X",GP_REG[11]);
   SendMessage(hwndControl,LB_ADDSTRING,11,(LPARAM)riga);
   wsprintf(riga,"R12 %08X",GP_REG[12]);
   SendMessage(hwndControl,LB_ADDSTRING,12,(LPARAM)riga);
   wsprintf(riga,"R13 %08X",GP_REG[13]);
   SendMessage(hwndControl,LB_ADDSTRING,13,(LPARAM)riga);
   wsprintf(riga,"R14 %08X",GP_REG[14]);
   SendMessage(hwndControl,LB_ADDSTRING,14,(LPARAM)riga);

   SendDlgItemMessage(hwndDebug,IDC_CHECK1,BM_SETCHECK,NFLAG != 0 ? BST_CHECKED : BST_UNCHECKED,0);
   SendDlgItemMessage(hwndDebug,IDC_CHECK2,BM_SETCHECK,CFLAG != 0 ? BST_CHECKED : BST_UNCHECKED,0);
   SendDlgItemMessage(hwndDebug,IDC_CHECK3,BM_SETCHECK,VFLAG != 0 ? BST_CHECKED : BST_UNCHECKED,0);
   SendDlgItemMessage(hwndDebug,IDC_CHECK4,BM_SETCHECK,ZFLAG != 0 ? BST_CHECKED : BST_UNCHECKED,0);
   SendDlgItemMessage(hwndDebug,IDC_CHECK5,BM_SETCHECK,(CPSR & IRQ_BIT) ? BST_CHECKED : BST_UNCHECKED,0);
   SendDlgItemMessage(hwndDebug,IDC_CHECK6,BM_SETCHECK,(CPSR & T_BIT) ? BST_CHECKED : BST_UNCHECKED,0);

   SendDlgItemMessage(hwndDebug,IDC_MODE,WM_SETTEXT,0,(LPARAM)cpu_mode_strings[CPSR & 0x1F]);
   wsprintf(c,"%02X",VCOUNT);
   SendDlgItemMessage(hwndDebug,IDC_RASTERLINE,WM_SETTEXT,0,(LPARAM)c);
   UpdateDebugMemoryWindow();
#ifdef _DEBPRO
   wsprintf(c,"%04d",cpu_cycles - ((VCOUNT * LINE_CYCLES) >> CYCLES_SHIFT));
   SendDlgItemMessage(hwndDebug,IDC_CYCLE,WM_SETTEXT,0,(LPARAM)c);
   UpdateDebugDMAWindow();
#endif
   UpdateDebugObjPalWindow();
   UpdateDebugBgPalWindow();
}
//---------------------------------------------------------------------------
u8 LoadBreakPointListFromName(char *pNome)
{
   LFile *fp;
   u8 i;
   char VER[BREAKPOINT_VERSION_LENGHT+1],s[MAX_PATH];
   typedef struct{
       u32 adress;
       u8 bEnable;
   } OLDBREAKPOINT;
   typedef struct{
       u32 adress;
       u8 bEnable;
       u32 PassCount,PassCountCondition;
   } BREAKPOINT100;
   typedef struct{
       u32 adress;
       u8 bEnable;
       u32 PassCount,PassCountCondition;
       char Condition[30];
   } BREAKPOINT110;
   HWND hwnd;
   int i1;
   BOOL bFlag;

   IsChangedBreakPointList();
   fp = new LFile(pNome);
   if(fp == NULL || !fp->Open()){
       if(fp != NULL)
           delete fp;
       return 0;
   }
   ::SendDlgItemMessage(hwndDebug,IDC_LIST4,LB_RESETCONTENT,0,0);
   ZeroMemory(listBreakPoint,sizeof(BREAKPOINT)*MAX_BREAK);
   ZeroMemory(VER,BREAKPOINT_VERSION_LENGHT+1);
   if(fp->Read(VER,BREAKPOINT_VERSION_LENGHT) != BREAKPOINT_VERSION_LENGHT){
       delete fp;
       return 0;
   }
   bFlag = FALSE;
   if(lstrcmp(VER,BREAKPOINT_VERSION) != 0){
       if(lstrcmp(VER,"BREAK110") == 0){
           if(fp->Read(listBreakPoint,sizeof(BREAKPOINT110)*20))
               bFlag = TRUE;
       }
       else if(lstrcmp(VER,"BREAK100") == 0){
           if(fp->Read(listBreakPoint,sizeof(BREAKPOINT100)*20))
               bFlag = TRUE;
       }
       else{
           fp->SeekToBegin();
           i = 0;
           while(1){
               if(!fp->Read(&listBreakPoint[i++],sizeof(OLDBREAKPOINT)));
                   break;
           }
       }
   }
   else{
       if(fp->Read(listBreakPoint,sizeof(BREAKPOINT)*MAX_BREAK))
           bFlag = TRUE;
   }
   if(bFlag){
       hwnd = GetDlgItem(hwndDebug,IDC_LIST4);
       while(1){
           if(fp->Read(&i1,sizeof(int))){
               fp->Read(s,i1);
               SendMessage(hwnd,LB_ADDSTRING,0,(LPARAM)s);
           }
           else
               break;
       }
   }
   delete fp;
   MAIN_MESSAGE.nBreak = 0;
   MAIN_MESSAGE.bBreakChanged = FALSE;
   MAIN_MESSAGE.FileNameBreakList = pNome;
   for(i=0;i<MAX_BREAK;i++){
       if(listBreakPoint[i].adress != 0)
           MAIN_MESSAGE.nBreak++;
   }
   RefillBreakPointList();
   FillComboBreakPoint();
   pDebugRecentFile->Add(pNome);
   UpdateDebugToolBar();

   return 1;
}
//---------------------------------------------------------------------------
u8 LoadBreakPointList()
{
   char szFile[MAX_PATH];
   LString c;
   MENUITEMINFO mi;

   IsChangedBreakPointList();
   ZeroMemory(&mi,sizeof(MENUITEMINFO));
   mi.cbSize = sizeof(MENUITEMINFO);
   mi.fMask = MIIM_TYPE;
   c.Capacity(80);
   mi.dwTypeData = c.c_str();
   mi.cch = 79;
   GetMenuItemInfo(GetMenu(hwndDebug),ID_DEBUG_LOAD,FALSE,&mi);
   MenuStringToString(c.c_str());
   c += "...";
   *((LPDWORD)szFile) = 0;
   if(!ShowOpenDialog(c.c_str(),szFile,"Breakpoint List files(*.lst)\0*.lst\0\0\0\0\0",hwndDebug))
       return 0;
   return LoadBreakPointListFromName(szFile);
}
//---------------------------------------------------------------------------
static void IsChangedBreakPointList()
{
   if(MAIN_MESSAGE.bBreakChanged && MAIN_MESSAGE.nBreak > 0){
       if(MessageBox(hwndDebug,TranslateGetMessage(0xF001).c_str(),"RascalBoy Advance",MB_YESNO) == IDYES){
           if(MAIN_MESSAGE.FileNameBreakList.IsEmpty())
               SaveBreakPointList();
           else
               SaveBreakPointListFromName(MAIN_MESSAGE.FileNameBreakList.c_str());
       }
       MAIN_MESSAGE.bBreakChanged = FALSE;
   }
}
//---------------------------------------------------------------------------
static u8 SaveBreakPointListFromName(char *szFile)
{
   LFile *fp;
   int i,i1,i2;
   char s[MAX_PATH];
   HWND hwnd;

   fp = new LFile(szFile);
   if(fp == NULL || !fp->Open(GENERIC_WRITE,CREATE_ALWAYS)){
       if(fp != NULL)
           delete fp;
       return 0;
   }
   fp->Write((char *)BREAKPOINT_VERSION,BREAKPOINT_VERSION_LENGHT);
   fp->Write(listBreakPoint,sizeof(BREAKPOINT)*MAX_BREAK);
   hwnd = GetDlgItem(hwndDebug,IDC_LIST4);
   i1 = SendMessage(hwnd,LB_GETCOUNT,0,0);
   for(i=0;i<i1;i++){
       SendMessage(hwnd,LB_GETTEXT,(WPARAM)i,(LPARAM)s);
       i2 = lstrlen(s) + 1;
       fp->Write(&i2,sizeof(int));
       fp->Write(s,i2);
   }
   delete fp;
   MAIN_MESSAGE.bBreakChanged = FALSE;
   MAIN_MESSAGE.FileNameBreakList = szFile;
   UpdateDebugToolBar();
   return 1;
}
//---------------------------------------------------------------------------
u8 SaveBreakPointList()
{
   char szFile[MAX_PATH];
   LString c;
   MENUITEMINFO mi;

   if(MAIN_MESSAGE.nBreak == 0)
       return 2;
   ZeroMemory(&mi,sizeof(MENUITEMINFO));
   mi.cbSize = sizeof(MENUITEMINFO);
   mi.fMask = MIIM_TYPE;
   c.Capacity(80);
   mi.dwTypeData = c.c_str();
   mi.cch = 79;
   GetMenuItemInfo(GetMenu(hwndDebug),ID_DEBUG_SAVE,FALSE,&mi);
   MenuStringToString(c.c_str());
   c += "...";
   if(MAIN_MESSAGE.FileNameBreakList.IsEmpty())
       *((LPDWORD)szFile) = 0;
   else
       lstrcpy(szFile,MAIN_MESSAGE.FileNameBreakList.c_str());
   if(!ShowSaveDialog(c.c_str(),szFile,"Breakpoint List files(*.lst)\0*.lst\0\0\0\0\0",hwndDebug,NULL))
       return 0;
   c = szFile;
   c.AddEXT(".lst");
   if(SaveBreakPointListFromName(c.c_str())){
       MAIN_MESSAGE.bBreakChanged = FALSE;
       return 1;
   }
   else
       return 0;
}
//---------------------------------------------------------------------------
s8 InsertBreakPoint(u32 adress)
{
   u8 i,bRedraw;
   char s[30];
   s8 res;
   HWND hwnd;

   for(i=0;i<MAX_BREAK;i++){
       if(listBreakPoint[i].adress == adress && listBreakPoint[i].Type == BT_PROGRAM)
           break;
   }
   if(i < MAX_BREAK){
       res = (s8)(i * -1);
       bRedraw = 1;
#ifdef _DEBPRO
       listBreakPoint[i].bEnable = !listBreakPoint[i].bEnable;
#endif
   }
   else if(MAIN_MESSAGE.nBreak < MAX_BREAK - 1){
       listBreakPoint[MAIN_MESSAGE.nBreak].adress = adress;
       listBreakPoint[MAIN_MESSAGE.nBreak].Type = BT_PROGRAM;
       listBreakPoint[MAIN_MESSAGE.nBreak++].bEnable = 1;
       SendMessage(hwndDebugComboBox1,CB_ADDSTRING,0,(LPARAM)MAIN_MESSAGE.nBreak);
       ToBreakPointString(adress,s,29);
       SetWindowText(hwndDebugComboBox1,s);
       res = i;
       bRedraw = 1;
   }
   else{
       res = 0;
       bRedraw = 0;
   }
   if(bRedraw != 0){
       hwnd = GetDlgItem(hwndDebug,IDC_LIST1);
       InvalidateRect(hwnd,NULL,FALSE);
       UpdateWindow(hwnd);
   }
   if(res)
       MAIN_MESSAGE.bBreakChanged = TRUE;
   UpdateDebugToolBar();
   return res;
}
//---------------------------------------------------------------------------
#endif





