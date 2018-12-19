#include <windows.h>
#pragma hdrstop
#if defined(__BORLANDC__)
#include <condefs.h>
#endif
#include <commctrl.h>
#include <pbt.h>

#include "resource.h"
#include "emu\gbaemu.h"
#include "emu\memory.h"
#include "gba.h"
#include "sound.h"
#include "debug.h"                                       
#include "debug1.h"
#include "lcd.h"
#include "graphics.h"
#include "input.h"
#include "emu\bios.h"
#include "trad.h"
#include "lregkey.h"
#include "sprite.h"
#include "about.h"
#include "sio.h"
#include "propplug.h"
#include "pluginctn.h"
#include "savestate.h"
#include "fstream.h"
#include "unit1.h"
#include "cheat.h"
//---------------------------------------------------------------------------
//#pragma link "dp.obj"
#pragma link "zlib.lib"
//---------------------------------------------------------------------------

USEUNIT("EMU\opcodes.cpp");
USEUNIT("EMU\topcodes.cpp");
USEUNIT("EMU\memory.cpp");
USEUNIT("emu\gbaemu.cpp");
USEUNIT("EMU\exec.cpp");
USEUNIT("gfx\mode3.cpp");
USEUNIT("gfx\mode4.cpp");
USEUNIT("gfx\mode5.cpp");
USEUNIT("gfx\modeTile.cpp");
USEUNIT("EMU\cpu.cpp");
USEUNIT("EMU\graphics.cpp");
USEUNIT("emu\ttables.c");
USEUNIT("EMU\io.cpp");
USEUNIT("emu\opedec.c");
USEUNIT("emu\tables.c");
USEUNIT("sound.cpp");
USEUNIT("EMU\bios.cpp");
USEUNIT("EMU\sprite.cpp");
USEUNIT("debug.cpp");
USEUNIT("lcd.cpp");
USEUNIT("list.cpp");
USEUNIT("lstring.cpp");
USEUNIT("input.cpp");
USEUNIT("emu\topedec.c");
USEUNIT("emu\DebSprite.cpp");
USEUNIT("debmem.cpp");
USEUNIT("debpal.cpp");
USEUNIT("debug1.cpp");
USEUNIT("debpalobj.cpp");
USEUNIT("trad.cpp");
USEUNIT("lregkey.cpp");
USEUNIT("inputtext.cpp");
USEUNIT("about.cpp");
USEUNIT("fstream.cpp");
USEUNIT("EMU\debbkg.cpp");
USEUNIT("source.cpp");
USEUNIT("winedit.cpp");
USEUNIT("brkmem.cpp");
USEUNIT("audioplug.cpp");
USEUNIT("videoplug.cpp");
USEUNIT("plugin.cpp");
USEUNIT("sio.cpp");
USEUNIT("propplug.cpp");
USEUNIT("pluginctn.cpp");
USEUNIT("zip\zipfile.cpp");
USEUNIT("savestate.cpp");
USEUNIT("dragdrop.cpp");
USEUNIT("Unit1.cpp");
USEUNIT("linkcontrol.cpp");
USEUNIT("cheat.cpp");
USERC("gba.rc");
USERC("message.rc");
USEUNIT("EMU\rtc.cpp");
USEUNIT("service.cpp");
USEUNIT("guidService.cpp");
USEUNIT("backplug.cpp");
//---------------------------------------------------------------------------
HWND hWin,hWndStatusBar,hwndProgress,hwndTrack;
HINSTANCE hInstance;
HACCEL hAccel;
DWORD dwPriorityClass,dwlastTick,dwFrame,bApplyIPSPath,bLoadCheatList;
int RGSInterleave;
BOOL bThreadRun,bQuit,bPause,bLoadRecent,bRecordGame,bEnableCheatList;
static u8 bAuto,nFrame;
u8 bSincro,bAutoRun;
LRecentFile *pRecentFile;
static int iSubCodeError;
static WNDPROC oldSBWndProc,oldTBWndProc;
static UINT idTimer;
static u16 *pBitCopy;
extern LList *pSaveStateList;
extern LSaveState rewindState;
//---------------------------------------------------------------------------
static void ShowMessageBox(char *mes,va_list ap);
static void OnBrightness();
static void OnMsgTrkBrightness(LPNMHDR msg);
static LONG FAR PASCAL SBWndProc(HWND win, UINT msg, WPARAM wparam, LPARAM lparam);
static LONG FAR PASCAL TBWndProc(HWND win, UINT msg, WPARAM wparam, LPARAM lparam);
static void FireMessage();
//---------------------------------------------------------------------------
#pragma argsused
//---------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInst,HINSTANCE, LPSTR pCommandLine, int cmdshow)
{
   DWORD dw,dw2,dw3;
   char s[20];
	u8 section;

   if(OleInitialize(NULL) != S_OK)
       return -1;
   lcd.BlitMode = 0xFF;
   pBitCopy = NULL;
   hInstance = hInst;
   bPause = bThreadRun = FALSE;
   hAccel = NULL;
   pRecentFile = NULL;
   hwndProgress = hWndStatusBar = NULL;
   section = TE_MAIN;
   if(!InitApp(hInst)){
       if(iSubCodeError == 0)
           iSubCodeError = -8;
       goto Exit_WinMain_Errore;
   }
   if(!init_gbaemu()){
       if((iSubCodeError = GetMainSubCodError()) >= 0)
			section = (u8)iSubCodeError;
       goto Exit_WinMain_Errore;
   }
   iSubCodeError = -11;
   if((pRecentFile = new LRecentFile(10)) == NULL)
       goto Exit_WinMain_Errore;
   section = TE_PLUGIN;
   if((pPlugInContainer = new PlugInContainer()) == NULL)
       goto Exit_WinMain_Errore;
   if(!pPlugInContainer->Init())
       goto Exit_WinMain_Errore;
   iSubCodeError = 0;
   pRecentFile->Read("Software\\RascalBoy\\RecentFile");
   dwFrame = 0;
   bAuto = nFrame = 0;
   LoadConfig();
   ShowWindow(hWin,SW_SHOW);
   if(ParseCommandLine(pCommandLine) > 0 && !bAutoRun)
       PostMessage(hWin,WM_COMMAND,MAKEWPARAM(ID_EMU_START,0),0);
   bQuit = FALSE;
   dw3 = 0;
   while(!bQuit){
       if(bThreadRun){
           run_frame();
           dwFrame++;
           dw = GetTickCount();
           if(!dwlastTick)
               dw2 = dwlastTick = dw;
           else if(bSincro && !pcm.LastWriteBytes){
               if((dw - dw2) < 16)
                   ::SleepEx(15 - (dw - dw2),FALSE);
               dw2 = dw = GetTickCount();
           }
           if((dw - dwlastTick) > 1000){
               wsprintf(s,"\7%3d fps",dwFrame);
               UpdateStatusBar(s,1);
               SetFrequencyRate(dwFrame);
               if(bAuto){
               }
               dwFrame = 0;
               dwlastTick = dw;
               if(bRecordGame){
                   if(++dw3 > (DWORD)RGSInterleave){
                       RecordGameState();
                       dw3 = 0;
                   }
               }
           }
           ProcessaMessaggi();
       }
       else
           FireMessage();
   }
   ProcessaMessaggi();
   SaveConfig();
   if(pRecentFile != NULL){
       pRecentFile->Save("Software\\RascalBoy\\RecentFile");
       delete pRecentFile;
       pRecentFile = NULL;
   }
   goto Exit_WinMain;
Exit_WinMain_Errore:
   ShowMessageError(section);
Exit_WinMain:
   CleanUpAndPostQuit();
   OleUninitialize();
   return 1;
}
//---------------------------------------------------------------------------
void SetSyncro(u8 value)
{
   bSincro = value;
   CheckMenuItem(GetMenu(hWin),ID_SKIP_SYNCRO,MF_BYCOMMAND|(bSincro ? MF_CHECKED : MF_UNCHECKED));
}
//---------------------------------------------------------------------------
BOOL IsSyncroEnabled()
{
   return bSincro;
}
//---------------------------------------------------------------------------
static void CheckMenuItemInternal(WORD wID,BOOL flag)
{
   CheckMenuItem(GetMenu(hWin),wID,MF_BYCOMMAND|(flag ? MF_CHECKED : MF_UNCHECKED));
}
//---------------------------------------------------------------------------
LRESULT CALLBACK MainWindowProc(HWND win, UINT msg, WPARAM wparam, LPARAM lparam)
{
   PAINTSTRUCT ps;
   int wID,wNotifyCode,i;
   MENUITEMINFO mii;
   char s[50],bProc;
   HMENU hMenu;
   DWORD dwPos;
   LString *pString,s1;
   LPTRAD lpTrad;
   LONG res;
   LPMINMAXINFO lpmmi;
   RECT rc;
   LPNMMOUSE lpNM;

   bProc = TRUE;
   switch(msg){
       case PM_KEYPRESSED:
           OnKeyPressed(wparam,lparam);
       break;
       case WM_GETMINMAXINFO:
           rc.right = 240;
           rc.bottom = 160;
           GetRBAWindowRect(&rc);
           lpmmi = (LPMINMAXINFO)lparam;
           lpmmi->ptMinTrackSize.x = rc.right;
           lpmmi->ptMinTrackSize.y = rc.bottom;
       break;
       case WM_SETCURSOR:
           if(lcd.bFullScreen == 2){
               res = TRUE;
               bProc = FALSE;
               SetCursor(NULL);
           }
       break;
       case WM_DROPFILES:
           if(DragQueryFile((HDROP)wparam,(UINT)-1,NULL,NULL) != (UINT)-1){
               s1.Capacity(MAX_PATH+1);
               DragQueryFile((HDROP) wparam,0,s1.c_str(),MAX_PATH);
               if(LoadRom(s1.c_str()))
                   SetForegroundWindow(hWin);
           }
           DragFinish((HDROP)wparam);
           res = 0;
           bProc = FALSE;
       break;
       case WM_ENTERMENULOOP:
       case WM_ENTERSIZEMOVE:
           PauseRestartSound(0);
       break;
       case WM_EXITMENULOOP:
       case WM_EXITSIZEMOVE:
           dwlastTick = GetTickCount();
           dwFrame = 0;
           PauseRestartSound(0x11);
       break;
       case WM_ACTIVATE:
           InputActivateWindow(wparam,lparam);
           bProc = FALSE;
           res = 0;
       break;
       case WM_INITMENUPOPUP:
           hMenu = GetSubMenu(GetMenu(hWin),0);
           if(GetSubMenu(hMenu,POS_SM_FILESRECENT) == (HMENU)wparam){
               hMenu = (HMENU)wparam;
               i = GetMenuItemCount(hMenu);
               for(;i > 1;i--)
                   ::DeleteMenu(hMenu,i-1,MF_BYPOSITION);
               ZeroMemory(&mii,sizeof(MENUITEMINFO));
               mii.cbSize = sizeof(MENUITEMINFO);
               if(pRecentFile != NULL && pRecentFile->Count() > 0){
                   AppendMenu(hMenu,MF_SEPARATOR,0,NULL);
                   pString = (LString *)pRecentFile->GetLastItem(&dwPos);
                   i = ID_FILE_RECENT + pRecentFile->Count() - 1;
                   while(pString != NULL){
                       AppendMenu(hMenu,MF_STRING,i,pString->c_str());
                       pString = (LString *)pRecentFile->GetPrevItem(&dwPos);
                       i--;
                   }
               }
           }
           else if(GetSubMenu(hMenu,POS_SM_BACKUP) == (HMENU)wparam){
           	i = GetMenuItemCount((HMENU)wparam)-1;
               for(;i>=0;i--)
                   ::DeleteMenu((HMENU)wparam,i,MF_BYPOSITION);
           }
           else if(GetSubMenu(hMenu,POS_SM_LANGUAGE) == (HMENU)wparam){
               i = GetMenuItemCount((HMENU)wparam)-1;
               for(;i>=0;i--)
                   ::DeleteMenu((HMENU)wparam,i,MF_BYPOSITION);
               i = 0;
               wID = 0xC000;
               ZeroMemory(&mii,sizeof(MENUITEMINFO));
               mii.cbSize = sizeof(MENUITEMINFO);
               if(pLanguageList != NULL && pLanguageList->Count() > 0){
                   lpTrad = (LPTRAD)pLanguageList->GetFirstItem(&dwPos);
                   while(lpTrad != NULL){
                       mii.fMask = MIIM_TYPE|MIIM_STATE|MIIM_ID|MIIM_DATA;
                       mii.fType = MFT_STRING;
                       mii.fState = MFS_ENABLED;
                       mii.dwItemData = lpTrad->wLanguageID;
                       if(pLanguageList->GetLanguageID() == lpTrad->wLanguageID)
                           mii.fState |= MFS_CHECKED;
                       mii.dwTypeData = lpTrad->Name.c_str();
                       mii.wID = wID++;
                       InsertMenuItem((HMENU)wparam,i++,TRUE,&mii);
                       lpTrad = (LPTRAD)pLanguageList->GetNextItem(&dwPos);
                   }
               }
           }
           else{
               hMenu = GetSubMenu(GetMenu(hWin),2);
               if(GetSubMenu(hMenu,POS_SM_AUDIOPLUGIN) == (HMENU)wparam)
                   OnInitAudioPlugMenu((HMENU)wparam);
               else if(GetSubMenu(hMenu,POS_SM_SIOPLUGIN) == (HMENU)wparam)
                   OnInitSioPlugMenu((HMENU)wparam);
               else if(GetSubMenu(hMenu,POS_SM_VIDEOPLUGIN) == (HMENU)wparam)
                   OnInitVideoPlugMenu((HMENU)wparam);
               else{
                   hMenu = GetSubMenu(GetSubMenu(GetMenu(hWin),0),POS_SM_SAVESTATE);
                   if((HMENU)wparam == GetSubMenu(hMenu,POS_SM_SAVESTATE_LOAD))
                       OnInitMenuSaveState((HMENU)wparam,POS_SM_SAVESTATE_LOAD);
                   else if((HMENU)wparam == GetSubMenu(hMenu,POS_SM_SAVESTATE_SAVE))
                       OnInitMenuSaveState((HMENU)wparam,POS_SM_SAVESTATE_SAVE);
               }
           }
       break;
       case WM_POWERBROADCAST:
           if(wparam == PBT_APMQUERYSUSPEND){
               res = BROADCAST_QUERY_DENY;
               bProc = FALSE;
           }
       break;
       case WM_SYSCOMMAND:
           if((wparam & 0xFFF0) == SC_MONITORPOWER){
               bProc = FALSE;
               res = 1;
           }
       break;
       case WM_MOVE:
           RecalcLayout();
       break;
#ifdef _DEBUG
       case WM_WINDOWPOSCHANGED:
           if(((LPWINDOWPOS)lparam)->hwndInsertAfter != HWND_BOTTOM)
               BringDebugToTop(win);
       break;
#endif
       case WM_SIZE:
           SendMessage(hWndStatusBar,WM_SIZE,wparam,lparam);
           OnResizeWindow();
       break;
       case WM_COMMAND:
           wID = (int)LOWORD(wparam);
           wNotifyCode = HIWORD(wparam);
           if(wNotifyCode < 2){
               bProc = FALSE;
               res = 0;
               if(wID >= 0xC000 && wID <= 0xCFFF){
                   ZeroMemory(&mii,sizeof(MENUITEMINFO));
                   mii.cbSize = sizeof(MENUITEMINFO);
                   mii.fMask = MIIM_DATA;
                   GetMenuItemInfo(GetMenu(hWin),wID,FALSE,&mii);
                   pLanguageList->SetLanguageID((WORD)mii.dwItemData);
                   break;
               }
               else if(pRecentFile != NULL && (wID >= (int)ID_FILE_RECENT && wID < (int)(ID_FILE_RECENT+pRecentFile->Count()))){
                   pString = (LString *)pRecentFile->GetItem(wID-ID_FILE_RECENT + 1);
                   if(pString != NULL)
                       LoadRom(pString->c_str());
                   break;
               }
               else if(wID >= ID_SOUND_PLUG_START && wID <= ID_SOUND_PLUG_END){
                   EnableSoundPlug((WORD)wID);
                   break;
               }
               else if(wID >= ID_SIO_PLUG_START && wID <= ID_SIO_PLUG_END){
                   EnableSioPlug((WORD)wID);
                   break;
               }
               else if(wID >= ID_SRAM_START && wID <= ID_EEPROM_END){
                   SetGamePackID((WORD)wID);
                   break;
               }
               else if(wID >= ID_VIDEO_PLUG_START && wID <= ID_VIDEO_PLUG_END){
                   EnableVideoPlug((WORD)wID);
                   break;
               }
               else if(wID >= ID_FILE_STATE_START && wID <= ID_FILE_STATE_END){
                   wID -= ID_FILE_STATE_START;
                   if(wID < 15)
                       LoadState(wID);
                   else
                       SaveState(wID - 15);
               }
               switch(wID){
                   case ID_FILE_CHEAT_DISABLED:
                       bEnableCheatList = !bEnableCheatList;
                       CheckMenuItemInternal((WORD)wID,(BOOL)!bEnableCheatList);
                       ResetCheatSystem();
                   break;
                   case ID_APP_IPSPATCH:
                       bApplyIPSPath = (bApplyIPSPath & 0xFFFF0000)|((WORD)(!(WORD)bApplyIPSPath));
                       CheckMenuItemInternal((WORD)wID,(BOOL)(WORD)bApplyIPSPath);
                   break;
                   case ID_FILE_CHEAT_LOAD:
                       LoadCheatList(win);
                   break;
                   case ID_FILE_CHEAT_LIST:
                       ShowCheatDialog(win);
                   break;
                   case ID_FILE_STATE_REWINDGAME:
                       PlayGameState();
                   break;
                   case ID_FILE_STATE_RECORDGAME:
                       if((bRecordGame = !bRecordGame) == FALSE){
                           rewindState.Reset();
                           EnableMenuItem(GetMenu(hWin),ID_FILE_STATE_REWINDGAME,MF_BYCOMMAND|MF_GRAYED);
                       }
                       CheckMenuItemInternal((WORD)wID,(BOOL)bRecordGame);
                   break;
                   case ID_FILE_CHEAT_AUTOLOAD:
                       bLoadCheatList = !bLoadCheatList;
                       CheckMenuItemInternal((WORD)wID,(BOOL)bLoadCheatList);
                   break;
                   case ID_FILE_STATE_AUTOLOADMOSTRCNT:
                       bLoadRecent = !bLoadRecent;
                       CheckMenuItemInternal((WORD)wID,(BOOL)bLoadRecent);
                   break;
                   case ID_FILE_STATE_RESET:
                       ResetSaveState();
                   break;
                   case ID_FILE_STATE_SAVE:
                       SaveState(-1);
                   break;
                   case ID_SET_PROPERTY:
                       ShowPlugInProperty();
                   break;
                   case ID_KEY_CONFIG:
                       OnKeyConfig();
                   break;
                   case ID_SOUND_ALL:
                       hMenu = GetMenu(hWin);
                       for(i=0;i<4;i++){
                           pcm.gbcch[i].enableL &= (u8)~0x80;
                           pcm.gbcch[i].enableR &= (u8)~0x80;
                           CheckMenuItem(hMenu,i + ID_SOUND_GBC1,MF_BYCOMMAND|MF_CHECKED);
                       }
                       pcm.fifo[0].Enable &= (u8)~0x80;
                       CheckMenuItem(hMenu,ID_SOUND_FIFOA,MF_BYCOMMAND|MF_CHECKED);
                       pcm.fifo[1].Enable &= (u8)~0x80;
                       CheckMenuItem(hMenu,ID_SOUND_FIFOB,MF_BYCOMMAND|MF_CHECKED);
                   break;
                   case ID_LAYER_ALL:
                       hMenu = GetMenu(hWin);
                       for(i=0;i<7;i++){
                           lcd.VisibleLayers[i] = 1;
                           if(i < 4)
                               lcd.layers[i].Visible = 1;
                           CheckMenuItem(hMenu,ID_LAYER_0+i,MF_BYCOMMAND|MF_CHECKED);
                       }
                       FillListBackground();
                   break;
                   case ID_FILE_RESET:
                       pRecentFile->Clear();
                   break;
                   case ID_SOUND_GBC1:
                   case ID_SOUND_GBC2:
                   case ID_SOUND_GBC3:
                   case ID_SOUND_GBC4:
                       pcm.gbcch[i = wID - ID_SOUND_GBC1].enableL ^= 0x80;
                       i = (pcm.gbcch[i].enableR ^= 0x80);
                       CheckMenuItem(GetMenu(hWin),wID,MF_BYCOMMAND|(i >= 0 ? MF_CHECKED : MF_UNCHECKED));
                   break;
                   case ID_LAYER_0:
                   case ID_LAYER_1:
                   case ID_LAYER_2:
                   case ID_LAYER_3:
                   case ID_LAYER_OBJ:
                   case ID_LAYER_WIN0:
                   case ID_LAYER_WIN1:
                       i = (lcd.VisibleLayers[wID - ID_LAYER_0] ^= 1);
                       CheckMenuItemInternal((WORD)wID,(BOOL)i);
                       if(wID < ID_LAYER_OBJ)
                           lcd.layers[wID - ID_LAYER_0].Visible = (u8)i;
                       FillListBackground();
                   break;
                   case ID_EMU_AUTOSTART:
                       SetAutoStart(bAutoRun ^ 1);
                   break;
                   case ID_APP_USEBIOS:
                       SetUseBios(UseBiosFile(FALSE) ^ 1);
                   break;
                   case ID_SKIPBIOSINTRO:
                       SetSkipBiosIntro(SkipBiosIntro(FALSE) ^ 1);
                   break;
                   case ID_APP_RESETBIOS:
                       ResetBiosParam();
                   break;
                   case ID_SKIP_SYNCRO:
                       SetSyncro((u8)(bSincro ^ 1));
                   break;
                   case ID_SKIP_AUTO:
                       bAuto ^= 1;
                       CheckMenuItemInternal(ID_SKIP_AUTO,(BOOL)bAuto);
                   break;
                   case ID_BRIGHTNESS:
                       OnBrightness();
                   break;
                   case ID_FRAMEBUFFER_DDRAW:
                       ChangeFrameBuffer(BM_DIRECTDRAW);
                   break;
                   case ID_FRAMEBUFFER_GDI:
                       ChangeFrameBuffer(BM_GDI);
                   break;
                   case ID_FRAMEBUFFER_DDRAWFULLSCREEN:
                       ChangeFrameBuffer(BM_DIRECTDRAWFULLSCREEN);
                   break;
                   case ID_SKIP_0FRAME:
                       SettaVelocita(0);
                   break;
                   case ID_SKIP_1FRAME:
                       SettaVelocita(1);
                   break;
                   case ID_SKIP_2FRAME:
                       SettaVelocita(2);
                   break;
                   case ID_SKIP_3FRAME:
                       SettaVelocita(3);
                   break;
                   case ID_SKIP_4FRAME:
                       SettaVelocita(4);
                   break;
                   case ID_SKIP_CPU25:
                   case ID_SKIP_CPU50:
                   case ID_SKIP_CPU75:
                       EnableSpeedCPU((WORD)wID);
                   break;
                   case ID_SOUND_SAVE:
						hMenu = GetMenu(hWin);
                       ZeroMemory(&mii,sizeof(MENUITEMINFO));
                       mii.cbSize = sizeof(MENUITEMINFO);
                       mii.fMask = MIIM_STATE;
                       GetMenuItemInfo(hMenu,ID_SOUND_SAVE,FALSE,&mii);
                       if(mii.fState == MFS_CHECKED){
                       	StopSoundSave();
                           mii.fState = MFS_UNCHECKED;
                           PauseRestartSound(0x01);
                       }
                       else{
                       	if(EnableSoundSave())
                               mii.fState = MFS_CHECKED;
                           else
                               mii.fState = MFS_UNCHECKED;
                       }
                       SetMenuItemInfo(hMenu,ID_SOUND_SAVE,FALSE,&mii);
                   break;
                   case ID_SOUND:
                       hMenu = GetMenu(hWin);
                       ZeroMemory(&mii,sizeof(MENUITEMINFO));
                       mii.cbSize = sizeof(MENUITEMINFO);
                       mii.fMask = MIIM_STATE;
                       if((i = pcm.Enable >= 0 ? 1 : 0) != 0){
                           sound_off();
                           pcm.Enable |= 0x80;
                       }
                       else{
                           sound_on();
                           pcm.Enable &= (u8)~0x80;
                       }
                       mii.fState = (i != 0 ? MFS_UNCHECKED : MFS_CHECKED);
                       SetMenuItemInfo(hMenu,ID_SOUND,FALSE,&mii);
                   break;
                   case ID_SOUND_FIFOA:
                   case ID_SOUND_FIFOB:
                       hMenu = GetMenu(hWin);
                       i = pcm.fifo[wID - ID_SOUND_FIFOA].Enable & 0x80;
                       if(i != 0){
                           pcm.fifo[wID - ID_SOUND_FIFOA].Enable &= (u8)~0x80;
                           CheckMenuItem(hMenu,wID,MF_BYCOMMAND|MF_CHECKED);
                       }
                       else{
                           pcm.fifo[wID - ID_SOUND_FIFOA].Enable |= 0x80;
                           CheckMenuItem(hMenu,wID,MF_BYCOMMAND|MF_UNCHECKED);
                       }
                   break;
                   case ID_HELP_ABOUT:
                       PauseRestartSound(0x00);
                       ShowAbout();
                       PauseRestartSound(0x01);
                   break;
                   case ID_APP_BIOS:
                       ::SendMessage(hWin,WM_COMMAND,MAKEWPARAM(ID_EMU_RESET,0),0);
                       LoadBios();
                   break;
                   case ID_ZOOM_1:
                   case ID_ZOOM_2:
                   case ID_ZOOM_3:
                       ResizeWindow((u8)(wID - ID_ZOOM_1 + 1));
                   break;
#ifdef _DEBUG
                   case ID_DEBUG_DIS:
                       if(lcd.BlitMode == BM_DIRECTDRAWFULLSCREEN)
                           ChangeFrameBuffer(BM_DIRECTDRAW);
                       CreateDebugWindow(0);
                   break;
#endif
                   case ID_APP_LOAD:
                       LoadRom(NULL);
                   break;
                   case ID_APP_EXIT:
                       ::SendMessage(hWin,WM_CLOSE,0,0);
                   break;
                   case ID_EMU_RESET:
                       if(bThreadRun || bPause){
                           ::SendMessage(hWin,WM_COMMAND,MAKEWPARAM(ID_EMU_START,0),0);
                           PostMessage(hWin,WM_COMMAND,MAKEWPARAM(ID_EMU_RESET,0),0);
                       }
                       else{
#ifdef _DEBUG
                           ResetDebug();
#endif
                           DeallocRom();
                           if(lstrlen(bin.FileName))
                               LoadRom(bin.FileName);
                       }
                   break;
                   case ID_EMU_PAUSE:
                       bThreadRun = !bThreadRun;
                       bPause = !bPause;
                       PauseRestartSound(!bPause);
                       if(bPause)
                           s1 = TranslateGetMessage(ID_EMU_RESUME);
                       else
                           s1 = TranslateGetMessage(ID_EMU_PAUSE);
                       ::ZeroMemory(&mii,sizeof(MENUITEMINFO));
                       mii.cbSize = sizeof(MENUITEMINFO);
                       mii.fMask = MIIM_TYPE;
                       mii.fType = MFT_STRING;
                       mii.dwTypeData = s1.c_str();
                       mii.cch = s1.Length();
                       ::SetMenuItemInfo(GetMenu(hWin),ID_EMU_PAUSE,FALSE,&mii);
                   break;
                   case ID_EMU_START:
                       ::ZeroMemory(&mii,sizeof(MENUITEMINFO));
                       mii.cbSize = sizeof(MENUITEMINFO);
                       mii.fMask = MIIM_TYPE;
                       mii.fType = MFT_STRING;
                       mii.dwTypeData = s;
                       mii.cch = 49;
                       hMenu = ::GetMenu(hWin);
                       ::GetMenuItemInfo(hMenu,ID_EMU_START,FALSE,&mii);
                       if(!bThreadRun && !bPause){
                           bThreadRun = TRUE;
                           s1 = TranslateGetMessage(ID_EMU_STOP);
                           reset_gbaemu();
#ifdef _DEBUG
                           ResetDebug();
                           if(!bInLoop)
                               dwKey = 0;
#endif
                           dwFrame = dwlastTick = 0;
                           ExitWindowMode();
                       }
                       else{
#ifdef _DEBUG
                           dwKey = (DWORD)VK_F5;
#endif
                           EnterWindowMode();
                           bThreadRun = bPause = FALSE;
                           cpu_cycles = 0x0FFFFFFF;
                           VCOUNT = 300;
                           sound_off();
                           UpdateStatusBar("",-1);
                           s1 = TranslateGetMessage(ID_EMU_START);
                           BlackFrame(NULL);
                           rewindState.Reset();
                           EnableMenuItem(GetMenu(hWin),ID_FILE_STATE_REWINDGAME,MF_GRAYED);
                       }
                       mii.dwTypeData = s1.c_str();
                       ::SetMenuItemInfo(hMenu,ID_EMU_START,FALSE,&mii);
                       s1 = TranslateGetMessage((!bPause ? ID_EMU_PAUSE : ID_EMU_RESUME));
                       mii.dwTypeData = s1.c_str();
                       mii.fMask |= MIIM_STATE;
                       mii.fState = (bThreadRun ? MF_ENABLED : MF_GRAYED);
                       ::SetMenuItemInfo(hMenu,ID_EMU_PAUSE,FALSE,&mii);
                   break;
                   default:
                       bProc = TRUE;
                   break;
               }
           }
       break;
       case WM_MOUSEMOVE:
           if(!lcd.bFullScreen && pcm.bStart && (wparam & MK_LBUTTON))
               RunDragDrop(LOWORD(lparam),HIWORD(lparam));
       break;
       case WM_LBUTTONDOWN:
           if(lcd.bFullScreen != 1)
               EnterWindowMode();
           else
               ExitWindowMode();
       break;
       case WM_RBUTTONDOWN:
           if(wparam == MK_RBUTTON && OnRButtonDown()){
               bProc = FALSE;
               res = 0;
           }
       break;
       case WM_ERASEBKGND:
           if(bThreadRun || bPause){
               bProc = FALSE;
               res = 1;
           }
       break;
       case WM_PAINT:
           if((ps.hdc = (HDC)wparam) != 0)
               bProc = TRUE;
           else{
               BeginPaint(win,&ps);
               bProc = FALSE;
           }
           if(bThreadRun || bPause)
               BlitFrame(ps.hdc);
           else
               BlackFrame(ps.hdc);
           if(!bProc)
               EndPaint(win,&ps);
           bProc = FALSE;
           res = 0;
       break;
       case WM_DESTROY:
           if(bThreadRun)
               ::SendMessage(hWin,WM_COMMAND,MAKEWPARAM(ID_EMU_START,0),0);
           bQuit = TRUE;
       break;
       case WM_NOTIFY:
           lpNM = (LPNMMOUSE)lparam;
           if(lpNM->hdr.code == NM_RCLICK && abs(lpNM->dwItemSpec) == 2)
               OnRButtonDownStatusBar();
       break;
    }
    if(bProc)
       res = DefWindowProc(win, msg, wparam, lparam);
    return res;
}
//---------------------------------------------------------------------------
static BOOL InitApp(HINSTANCE this_inst)
{
   HWND win;
   RECT rc;
   int iWidth,iHeight;
   HMENU hMenu;

   InitCommonControls();
   if(!RegisterRBAClass(this_inst))
       return FALSE;
   if(!InitTranslation()){
       iSubCodeError = -10;
       ShowMessageError(TE_MAIN);
       iSubCodeError = 0;
   }
   ::SystemParametersInfo(SPI_GETWORKAREA,0,&rc,0);
   iWidth = 240 + GetSystemMetrics(SM_CXFIXEDFRAME) * 2;
   iHeight = 160 + GetSystemMetrics(SM_CYFIXEDFRAME) + GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYSIZEFRAME);
   rc.right -= iWidth;
   rc.bottom -= iHeight;
   rc.right >>= 1;
   rc.bottom >>= 1;
	win = CreateWindowEx(WS_EX_ACCEPTFILES,"RASCALBOY","RascalBoy Advance",
         WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_THICKFRAME,
         rc.right,rc.bottom,iWidth,iHeight,NULL,NULL,this_inst,NULL);
   if((hWin = win) == NULL)
       return FALSE;
   hWndStatusBar = CreateStatusWindow(WS_CHILD|WS_VISIBLE|CCS_BOTTOM|SBARS_SIZEGRIP|0x800,"",hWin,IDM_STATUSBAR);
   if(hWndStatusBar == NULL)
       return FALSE;
   ::GetWindowRect(hWndStatusBar,&rc);
   ::SetWindowPos(hWin,NULL,0,0,iWidth,iHeight + rc.bottom - rc.top,SWP_NOMOVE|SWP_NOREPOSITION);
   oldSBWndProc = (WNDPROC)SetWindowLong(hWndStatusBar,GWL_WNDPROC,(LONG)SBWndProc);
   hAccel = ::LoadAccelerators(hInstance,MAKEINTRESOURCE(IDR_MAINFRAME));
   if(hAccel == NULL)
       return FALSE;
   hMenu = GetMenu(win);
#ifndef _DEBUG
   ::DestroyMenu(::GetSubMenu(hMenu,1));
   ::DeleteMenu(hMenu,1,MF_BYPOSITION);
#else
   InitDebug();
#endif
   ResetMainPlugInMenu(hMenu);
   Translation(hWin,IDR_MAINFRAME,0);
   return TRUE;
}
//---------------------------------------------------------------------------
static void FireMessage()
{
   MSG msg;
   HWND hwnd;

   ::GetMessage(&msg,NULL,0,0);
   hwnd = ::GetActiveWindow();
   if(!::TranslateAccelerator(hwnd,hAccel,&msg)){
#ifdef _DEBUG
       if(hwnd == hWin || !IsDialogMessage(hwnd,&msg)){
#endif
           ::TranslateMessage(&msg);
           ::DispatchMessage(&msg);
#ifdef _DEBUG
       }
#endif
   }
}
//---------------------------------------------------------------------------
extern "C" void ProcessaMessaggi()
{
   MSG msg;

   while(PeekMessage(&msg,NULL,0,0,PM_NOREMOVE)){
       FireMessage();
       if(msg.message == WM_KEYDOWN && msg.wParam == VK_F5 && !bThreadRun && bin.bLoad)
           ::PostMessage(hWin,WM_COMMAND,MAKEWPARAM(ID_EMU_START,0),NULL);
#ifdef _DEBUG
       if(bInLoop)
           break;
#endif
   }
}
//---------------------------------------------------------------------------
void CleanUpAndPostQuit(void)
{
#ifdef _DEBUG
   DestroyDebug();
#endif
   DisableSound();
   clean_up();
   DestroyTranslation();
   if(pPlugInContainer != NULL)
       delete pPlugInContainer;
   pPlugInContainer = NULL;
   if(hWndStatusBar != NULL)
       ::DestroyWindow(hWndStatusBar);
   hWndStatusBar = NULL;
   if(hAccel != NULL)
        DestroyAcceleratorTable(hAccel);
   UnregisterRBAClass(hInstance);
   if(pBitCopy != NULL)
       GlobalFree((HGLOBAL)pBitCopy);
}
//---------------------------------------------------------------------------
BOOL ShowSaveDialog(char *lpstrTitle,char *lpstrFileName,char *lpstrFilter,HWND hwnd,LPFNCSAVEDLG pFnc)
{
   OPENFILENAME ofn;

   ZeroMemory(&ofn, sizeof(OPENFILENAME));
   ofn.lStructSize = sizeof(OPENFILENAME);
   if(hwnd == NULL)
       hwnd = hWin;
   ofn.hwndOwner = hwnd;
   ofn.lpstrFile = lpstrFileName;
   ofn.nMaxFile = MAX_PATH;
   if(lpstrFilter != NULL){
       ofn.lpstrFilter = lpstrFilter;
       ofn.nFilterIndex = 1;
   }
   ofn.Flags = OFN_FILEMUSTEXIST|OFN_EXPLORER|OFN_NOREADONLYRETURN|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
   ofn.lpstrTitle = lpstrTitle;
   if(pFnc != NULL){
       if(!pFnc(&ofn))
           return FALSE;
   }
   if(!::GetSaveFileName(&ofn))
       return FALSE;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL ShowOpenDialog(char *lpstrTitle,char *lpstrFileName,char *lpstrFilter,HWND hwnd)
{
   OPENFILENAME ofn;
   LString c;

   ZeroMemory(&ofn, sizeof(OPENFILENAME));
   ofn.lStructSize = sizeof(OPENFILENAME);
   ofn.hwndOwner = hwnd;
   ofn.lpstrFile = lpstrFileName;
   ofn.nMaxFile = MAX_PATH;
   if(lpstrFilter != NULL){
       ofn.lpstrFilter = lpstrFilter;
       ofn.nFilterIndex = 1;
   }
   ofn.Flags = OFN_FILEMUSTEXIST|OFN_EXPLORER|OFN_NOREADONLYRETURN|OFN_HIDEREADONLY;
   if(lpstrTitle != NULL){
       c = lpstrTitle;
       c += "...";
   }
   ofn.lpstrTitle = c.c_str();
   if (!::GetOpenFileName(&ofn))
       return FALSE;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LoadRom(char *pFileName)
{
   char szFile[MAX_PATH];
   HMENU hMenu;
   LString c;

   if(bThreadRun){
       ::SendMessage(hWin,WM_COMMAND,MAKEWPARAM(ID_EMU_START,0),0);
       EnableMenuItem(GetMenu(hWin),ID_EMU_START,MF_BYCOMMAND|MF_GRAYED);
   }
   if(pFileName == NULL){
       c = GetStringFromMenu(hWin,ID_APP_LOAD);
       MenuStringToString(c.c_str());
       c += "...";
       *((LPDWORD)szFile) = 0;
       if(!ShowOpenDialog(c.c_str(),szFile,"Gameboy Advance files(*.bin,*.gba,*.agb,*.zip)\0*.bin;*.gba;*.agb;*.zip\0Tutti (*.*)\0*.*\0\0\0\0\0",hWin))
           return FALSE;
   }
   else
       lstrcpy(szFile,pFileName);
   if(!load_bin(szFile)){
       iSubCodeError = -10;
       ShowMessageError(TE_MAIN,szFile);
       pRecentFile->DeleteString(szFile);
       sound_reset();
       return FALSE;
   }
   hMenu = ::GetMenu(hWin);
   ::EnableMenuItem(hMenu,ID_EMU_START,MF_BYCOMMAND|MF_ENABLED);
   ::EnableMenuItem(hMenu,ID_EMU_RESET,MF_BYCOMMAND|MF_ENABLED);
   ::EnableMenuItem(hMenu,ID_FILE_STATE_SAVE,MF_BYCOMMAND|MF_ENABLED);
   pRecentFile->Add(szFile);
   if((WORD)bApplyIPSPath != 0)
       OnApplyIPSPatch();
   reset_gbaemu();
   UpdateStatusBar(bin.Title,0);
   if(bAutoRun)
       SendMessage(hWin,WM_COMMAND,MAKEWPARAM(ID_EMU_START,0),NULL);
   LoadMostRecentSaveState();
   LoadCheatListFromRom(bLoadCheatList);
   ApplyStaticCheatList();
   return TRUE;
}
//---------------------------------------------------------------------------
int MenuStringToString(char *string)
{
   LString s,s1;
   int i,res;

   s = string;
   res = 0;
   if((i = s.Pos("&")) > 0){
       s1 = s.SubString(1,i-1);
       s = s1 + s.SubString(i+1,s.Length() - i);
       res = 1;
   }
   if((i = s.Pos("\t")) > 0){
       s1 = s.SubString(i+1,s.Length() - i);
       s = s.SubString(1,i-1);
       res = 2;
   }
   else
       s1 = "";
   lstrcpy(string,s.c_str());
   if(s1.IsEmpty())
       return res;
   i = lstrlen(string);
   string[i++] = 0;
   lstrcpy(&string[i],s1.c_str());
   return res;
}
//---------------------------------------------------------------------------
void SetSubCodeError(int err)
{
   iSubCodeError = err;
}
//---------------------------------------------------------------------------
void ShowMessageError(u8 Section,...)
{
   LString s;
   UINT wID;
   va_list ap;
   char string[400];

   va_start(ap,Section);
   switch(Section){
       case TE_LCD:
           iSubCodeError = GetLcdSubCodeError();
       break;
       case TE_MAIN:
       break;
       case TE_INPUT:
           iSubCodeError = GetInputSubCodeError();
       break;
       case TE_BIOS:
           iSubCodeError = GetBiosSubCodeError();
       break;
       case TE_SOUND:
           iSubCodeError = GetSoundSubCodeError();
       break;
       case TE_PLUGIN:
           iSubCodeError = pPlugInContainer->GetLastError();
       break;
       case TE_CHEAT:
           iSubCodeError = GetCheatSystemError();
       break;
   }
   wID = MAKEERRORGBA(Section,iSubCodeError);
   s = TranslateGetMessage(wID);
   vsprintf(string,s.c_str(),ap);
   MessageBox(hWin,string,"RascalBoy Advance",MB_ICONSTOP|MB_OK);
   va_end(ap);
}
//---------------------------------------------------------------------------
static LONG FAR PASCAL TBWndProc(HWND win, UINT msg, WPARAM wparam, LPARAM lparam)
{
   int i;

   switch(msg){
       case WM_LBUTTONDOWN:
           if(idTimer != 0)
               KillTimer(hWndStatusBar,idTimer);
           idTimer = 0;
       break;
       case WM_LBUTTONUP:
           idTimer = SetTimer(hWndStatusBar,0xEEEE,1000,NULL);
       break;
       case WM_DESTROY:
           if(idTimer != 0)
               KillTimer(hWndStatusBar,idTimer);
           idTimer = 0;
           if(pBitCopy)
               GlobalFree((HGLOBAL)pBitCopy);
           pBitCopy = NULL;
           lcd.LumStart = lcd.Luminosita = (s32)(100 + (SendMessage(hwndTrack, TBM_GETPOS, 0, 0) - 5) * 5);
           if(bPause){
               for(i=0;i<0x200;i++)
                   FillPalette((u16)i);
               SendMessage(hWin,WM_COMMAND,ID_EMU_PAUSE,0);
           }
       break;
   }
   return CallWindowProc(oldTBWndProc,win,msg,wparam,lparam);
}
//---------------------------------------------------------------------------
void UpdateBrightness()
{
   int y,x,r,g,b,i;
   u16 *p,*p1;

   if(!lcd.Enable)
       return;
   p = screen;
   p1 = pBitCopy;
   if((i = lcd.Luminosita) == 0)
       i = 100;
   for(y=0;y<160;y++){
       for(x=0;x<240;x++){
           r = (*p1 >> 10) & 0x1F;
           g = (*p1 >> 5) & 0x1F;
           b = *p1++ & 0x1F;
           if((r = r * i / 100) > 31)
               r = 31;
           if((g = g * i / 100) > 31)
               g = 31;
           if((b = b * i / 100) > 31)
               b = 31;
           *p++ = (u16)((r << 10) | (g << 5) | b);
       }
   }
}
//---------------------------------------------------------------------------
static LONG FAR PASCAL SBWndProc(HWND win, UINT msg, WPARAM wparam, LPARAM lparam)
{
   int iPos;
   TOOLINFO ti;
   char szText[20];
   HWND hwnd;
   float f;

   switch(msg){
       case WM_TIMER:
           DestroyWindow(hwndTrack);
           UpdateStatusBar(bin.Title,0);
           hwndTrack = NULL;
       break;
       case WM_HSCROLL:
           iPos = 100 + (SendMessage(hwndTrack, TBM_GETPOS, 0, 0) - 5) * 5;
           hwnd = (HWND)SendMessage(hwndTrack,TBM_GETTOOLTIPS,0,0);
           if(hwnd != NULL){
               ZeroMemory(&ti,sizeof(TOOLINFO));
               ti.cbSize = sizeof(TOOLINFO);
               if(SendMessage(hwnd,TTM_GETCURRENTTOOL,0,(LPARAM)&ti)){
                   wsprintf(szText,"%d%%",iPos);
                   ti.lpszText= szText;
                   SendMessage(hwnd,TTM_UPDATETIPTEXT,0,(LPARAM)&ti);
               }
           }
           if(lcd.Luminosita != iPos){
               f = lcd.LumStart != 0 ? (((float)iPos / (float)lcd.LumStart) * 100.0) : (float)iPos;
               lcd.Luminosita = (int)f;
               if(bThreadRun || bPause){
                   UpdateBrightness();
                   BlitFrame(NULL);
               }
           }
       break;
   }
   return CallWindowProc(oldSBWndProc,win,msg,wparam,lparam);
}
//---------------------------------------------------------------------------
static void OnBrightness()
{
   RECT rc;
   int i;

   SendMessage(hWndStatusBar,SB_GETRECT,0,(LPARAM)&rc);
   if(pBitCopy != NULL)
       GlobalFree((HGLOBAL)pBitCopy);
   if((i = rc.right - rc.left - 4) > 150)
       i = 150;
   rc.left = rc.left + (((rc.right - rc.left) - i) >> 1);
   hwndTrack = CreateWindow(TRACKBAR_CLASS,NULL,WS_CHILD|TBS_AUTOTICKS|TBS_HORZ|TBS_BOTTOM|TBS_TOOLTIPS,rc.left,rc.top+1,i,rc.bottom-rc.top - 2,hWndStatusBar,(HMENU)IDC_TRKBRIGHTNESS,hInstance,NULL);
   if(hwndTrack == NULL)
       return;
   if(bThreadRun || bPause){
       pBitCopy = (u16 *)GlobalAlloc(GPTR,240 * 160*sizeof(u16));
       if(pBitCopy == NULL){
           DestroyWindow(hwndTrack);
           hwndTrack = NULL;
           UpdateStatusBar(bin.Title,0);
           return;
       }
       CopyMemory(pBitCopy,screen,sizeof(u16) * 240 * 160);
       SendMessage(hWin,WM_COMMAND,ID_EMU_PAUSE,0);
   }
   UpdateStatusBar("",0);
   oldTBWndProc = (WNDPROC)SetWindowLong(hwndTrack,GWL_WNDPROC,(LONG)TBWndProc);
   SendMessage(hwndTrack, TBM_SETRANGE,(WPARAM)TRUE,(LPARAM) MAKELONG(0,10));
   i = lcd.Luminosita != 0 ? (lcd.Luminosita - 100) / 5 : 0;
   SendMessage(hwndTrack, TBM_SETPOS,(WPARAM)TRUE,(LPARAM)i + 5);
   ShowWindow(hwndTrack,SW_SHOW);
   SetFocus(hwndTrack);
   idTimer = SetTimer(hWndStatusBar,0xEEEE,2000,NULL);
}
//---------------------------------------------------------------------------
void DrawProgressBar(DWORD pos)
{
   WORD wPos,wMax;
   RECT rc;

   if(lcd.bFullScreen == 2)
       return;
   wPos = (WORD)(LOWORD(pos)+1);
   wMax = (WORD)(HIWORD(pos)+1);
   if(wMax > 0){
       if(hwndProgress == NULL){
           if(hwndTrack != NULL){
               DestroyWindow(hwndTrack);
               hwndTrack = NULL;
           }
           SendMessage(hWndStatusBar,SB_GETRECT,0,(LPARAM)&rc);
           hwndProgress = CreateWindow(PROGRESS_CLASS,NULL,WS_CHILD|PBS_SMOOTH|WS_VISIBLE,rc.left,rc.top+1,rc.right-rc.left,rc.bottom-rc.top - 2,hWndStatusBar,NULL,hInstance,NULL);
       }
       if(hwndProgress != NULL){
           wPos = (WORD)(100 * wPos / wMax);
           SendMessage(hwndProgress,PBM_SETPOS,(WPARAM)wPos,0);
       }
   }
   if(wPos == 100 && hwndProgress != NULL){
       DestroyWindow(hwndProgress);
       hwndProgress = NULL;
   }
   _EAX = _EAX;
   _EAX = _EAX;
   _EAX = _EAX;
   _EAX = _EAX;
/*   _EAX = _EAX;
   _EAX = _EAX;
   _EAX = _EAX;
   _EAX = _EAX;
   _EAX = _EAX;
   _EAX = _EAX;
/*   _EAX = _EAX;
   _EAX = _EAX;
   _EAX = _EAX;
   _EAX = _EAX;*/
}










