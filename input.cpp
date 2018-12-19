#include "input.h"
#include "debug.h"
#include "gba.h"
#include "C:\Borland\include\dinput.h"
#include "memory.h"
#include "trad.h"

//---------------------------------------------------------------------------
static LRESULT CALLBACK KeyboardProc(int code,WPARAM wParam,LPARAM lParam);
static BOOL CALLBACK DIEnumDeviceObjectsCallback(LPCDIDEVICEINSTANCE lpddoi,LPVOID pvRef);
static BOOL CALLBACK EnumJoystickCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi,LPVOID pvRef);
static void SendInputInterrupt(void);
static void ChangeEditKeyName(LPARAM lParam);
static BOOL CALLBACK KeyDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
//---------------------------------------------------------------------------
static HHOOK hHook;
static LPDIRECTINPUT7 lpDI;
static LPDIRECTINPUTDEVICE7 lpJoy;
static GUID guidJoy;
static int iSubCodeError,KeyPressed;
short KeyField;
static SHORT keyCode[11],keyValue;
static const SHORT keyCodeReset[11] = {0x1e,0x2c,0x2a,0x1c,0x4d,0x4b,0x48,0x50,0x2d,0x1f,0};
static BYTE bModifyKey,keySelected;
static WNDPROC oldWindowProc;
static HWND hwndDialog;

static DIOBJECTDATAFORMAT rgodf[ ] = {
  {&GUID_XAxis,FIELD_OFFSET(MYDATA,lX),DIDFT_ABSAXIS|DIDFT_MAKEINSTANCE(0),0},
  {&GUID_YAxis,FIELD_OFFSET(MYDATA,lY),DIDFT_ABSAXIS|DIDFT_MAKEINSTANCE(1),0},
  {&GUID_Button,FIELD_OFFSET(MYDATA,rgbButtons[0]),DIDFT_BUTTON|DIDFT_MAKEINSTANCE(0), 0, },
  {&GUID_Button,FIELD_OFFSET(MYDATA,rgbButtons[1]),DIDFT_BUTTON|DIDFT_MAKEINSTANCE(1), 0, },
  {&GUID_Button,FIELD_OFFSET(MYDATA,rgbButtons[2]),DIDFT_BUTTON|DIDFT_MAKEINSTANCE(2), 0, },
  {&GUID_Button,FIELD_OFFSET(MYDATA,rgbButtons[3]),DIDFT_BUTTON|DIDFT_MAKEINSTANCE(3), 0, },
  {&GUID_Button,FIELD_OFFSET(MYDATA,rgbButtons[4]),DIDFT_BUTTON|DIDFT_MAKEINSTANCE(4), 0, },
  {&GUID_Button,FIELD_OFFSET(MYDATA,rgbButtons[5]),DIDFT_BUTTON|DIDFT_MAKEINSTANCE(5), 0, },
};
//---------------------------------------------------------------------------
u8 InitInput()
{                 
   HRESULT hr;
   DWORD dw;
   DIPROPDWORD dipd;                             
   DIDEVCAPS  DIJoyCaps;
   DIDATAFORMAT df;

   KeyPressed = 0;
   bModifyKey = 0;
   CopyMemory(keyCode,keyCodeReset,sizeof(keyCodeReset));
   lpJoy = NULL;
   hHook = SetWindowsHookEx(WH_KEYBOARD,KeyboardProc,hInstance,GetCurrentThreadId());
   iSubCodeError = -1;
   if(hHook == NULL)
       return 0;
   hr = CoCreateInstance(CLSID_DirectInput,NULL,CLSCTX_INPROC_SERVER,IID_IDirectInput7,(LPVOID *)&lpDI);
   iSubCodeError = -2;
   if(hr != DI_OK)
       return 0;
   hr = lpDI->Initialize(hInstance,DIRECTINPUT_VERSION);
   if(hr != DI_OK)
       return 0;
   dw = 0;
   hr = lpDI->EnumDevices(0,(LPDIENUMDEVICESCALLBACK)DIEnumDeviceObjectsCallback,&dw,DIEDFL_ATTACHEDONLY);
   iSubCodeError = -3;
   if(hr != DI_OK)
       return 0;
   iSubCodeError = -4;
   if(dw == 0)
       return 0;
   hr = lpDI->CreateDevice(guidJoy,(LPDIRECTINPUTDEVICE *)&lpJoy,NULL);
   iSubCodeError = -5;
   if(hr != DI_OK)
       return 0;
   dw = 0;
   hr = lpJoy->EnumObjects(EnumJoystickCallback,&dw,DIDFT_AXIS);
   iSubCodeError = -7;
   if(hr != DI_OK)
       return 0;
   DIJoyCaps.dwSize = sizeof(DIDEVCAPS);
   hr = lpJoy->GetCapabilities(&DIJoyCaps);
   if(hr != DI_OK)
       return 0;
   ZeroMemory(&df,sizeof(DIDATAFORMAT));
   df.dwSize = sizeof(DIDATAFORMAT);
   df.dwObjSize = sizeof(DIOBJECTDATAFORMAT);
   df.dwFlags = DIDF_ABSAXIS;
   df.dwDataSize = sizeof(MYDATA);
   df.dwNumObjs = DIJoyCaps.dwAxes + DIJoyCaps.dwButtons;
   df.rgodf = rgodf;
   hr = lpJoy->SetDataFormat(&df);
   iSubCodeError = -8;
   if(hr != DI_OK)
       return 0;
   hr = lpJoy->SetCooperativeLevel(hWin,DISCL_EXCLUSIVE|DISCL_FOREGROUND);
   iSubCodeError = -9;
   if(hr != DI_OK)
       return 0;
   ZeroMemory(&dipd,sizeof(DIPROPDWORD));
   dipd.diph.dwSize = sizeof(DIPROPDWORD);
   dipd.diph.dwHeaderSize = sizeof(DIPROPHEADER);
   dipd.diph.dwHow = DIPH_DEVICE;
   dipd.dwData = 5000;
   hr = lpJoy->SetProperty(DIPROP_DEADZONE,&dipd.diph);
   iSubCodeError = -10;
   if(hr != DI_OK)
       return 0;
   iSubCodeError = 0;
   return 1;
}
//---------------------------------------------------------------------------
void DestroyInput()
{
   if(hHook != NULL)
       UnhookWindowsHookEx(hHook);
   RELEASE(lpJoy);
   RELEASE(lpDI);
}
//---------------------------------------------------------------------------
int GetInputSubCodeError()
{
   return iSubCodeError;
}
//---------------------------------------------------------------------------
static BOOL CALLBACK EnumJoystickCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi,LPVOID pvRef)
{
   DIPROPRANGE diprg;

   diprg.diph.dwSize       = sizeof(DIPROPRANGE);
   diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
   diprg.diph.dwHow        = DIPH_BYID;
   diprg.diph.dwObj        = lpddoi->dwType;
   diprg.lMin              = -1000;
   diprg.lMax              = +1000;
   IDirectInputDevice7_SetProperty(lpJoy,DIPROP_RANGE, &diprg.diph);
   (*((DWORD *)pvRef))++;

   return TRUE;
}
//---------------------------------------------------------------------------
BOOL CALLBACK DIEnumDeviceObjectsCallback(LPCDIDEVICEINSTANCE lpddoi,LPVOID pvRef)
{
   if(DIDFT_GETTYPE(lpddoi->dwDevType) == DIDEVTYPE_JOYSTICK){
       CopyMemory(&guidJoy,&lpddoi->guidInstance,sizeof(GUID));
       (*((DWORD *)pvRef))++;
       return DIENUM_STOP;
   }
   return DIENUM_CONTINUE;
}
//---------------------------------------------------------------------------
void InputActivateWindow(WPARAM wParam,LPARAM lParam)
{
   if(lpJoy == NULL || wParam == WA_INACTIVE)
       return;
   IDirectInputDevice7_Acquire(lpJoy);
}
//---------------------------------------------------------------------------
void Acquire()
{
   HRESULT hr;
   MYDATA js;
   u16 key;

   if(lpJoy == NULL)
       return;
   hr = IDirectInputDevice7_Poll(lpJoy);
   if(hr != DI_OK && hr != DI_NOEFFECT){
       do{
           hr = IDirectInputDevice7_Acquire(lpJoy);
       }while(hr == DIERR_INPUTLOST);
       return;
   }
   hr = IDirectInputDevice7_GetDeviceState(lpJoy,sizeof(MYDATA),&js);
   if(hr != DI_OK)
       return;
   key = 0x3FF;
   if(js.lX < 0)
       key &= ~32;
   else if (js.lX > 0)
       key &= ~16;
   if (js.lY < 0)
       key &= ~64;
   else if (js.lY > 0)
       key &= ~128;
   if((js.rgbButtons[0] & 0x80) != 0)
       key &= ~1;
   if((js.rgbButtons[1] & 0x80) != 0)
       key &= ~2;
   if((js.rgbButtons[2] & 0x80) != 0)
       key &= ~256;
   if((js.rgbButtons[3] & 0x80) != 0)
       key &= ~512;
   if((js.rgbButtons[4] & 0x80) != 0)
       key &= ~4;
   if((js.rgbButtons[5] & 0x80) != 0)
       key &= ~8;
   if(key != 0x3FF)
       KeyPressed = 0;
   if(KeyPressed)
       return;
   KeyField = key;
   SendInputInterrupt();
}
//---------------------------------------------------------------------------
int OnKeyPressed(WPARAM wParam,LPARAM lParam)
{
   char flag;
   BYTE key;

   flag = 0;
   key = (u8)(lParam >> 16);
   if((lParam >> 31) != 0){
       if(key == (u8)keyCode[0])
           KeyField |= 1;
       else if(key == (u8)keyCode[1])
           KeyField |= 2;
       else if(key == (u8)keyCode[2])
           KeyField |= 4;
       else if(key == (u8)keyCode[3])
           KeyField |= 8;
       else if(key == (u8)keyCode[4])
           KeyField |= 16;
       else if(key == (u8)keyCode[5])
           KeyField |= 32;
       else if(key == (u8)keyCode[6])
           KeyField |= 64;
       else if(key == (u8)keyCode[7])
           KeyField |= 128;
       else if(key == (u8)keyCode[8])
           KeyField |= 256;
       else if(key == (u8)keyCode[9])
           KeyField |= 512;
       return 0;
   }
   if(key == (u8)keyCode[0]){
       KeyField &= ~1;
       flag = 1;
   }
   else if(key == (u8)keyCode[1]){
        KeyField &= ~2;
        flag = 1;
   }
   else if(key == (u8)keyCode[2]){
        KeyField &= ~4;
        flag = 1;
   }
   else if(key == (u8)keyCode[3]){
       KeyField &= ~8;
       flag = 1;
   }
   else if(key == (u8)keyCode[4]){
       KeyField &= ~16;
       flag = 1;
   }
   else if(key == (u8)keyCode[5]){
       KeyField &= ~32;
       flag = 1;
   }
   else if(key == (u8)keyCode[6]){
       KeyField &= ~64;
       flag = 1;
   }
   else if(key == (u8)keyCode[7]){
       KeyField &= ~128;
       flag = 1;
   }
   else if(key == (u8)keyCode[8]){
       KeyField &= ~256;
       flag = 1;
   }
   else if(key == (u8)keyCode[9]){
       KeyField &= ~512;
       flag = 1;
   }
   if(flag){
       KeyPressed = 1;
       SendInputInterrupt();
   }
   return 1;
}
//---------------------------------------------------------------------------
LRESULT CALLBACK KeyboardProc(int code,WPARAM wParam,LPARAM lParam)
{
   if(code != HC_ACTION)
       return CallNextHookEx(hHook,code,wParam,lParam);
   if(wParam == VK_RETURN && bModifyKey && !(lParam >> 31) ){
       ChangeEditKeyName(lParam);
       return 1;
   }
   if(!OnKeyPressed(wParam,lParam))
       return ::CallNextHookEx(hHook, code, wParam, lParam);
#ifdef _DEBUG
   dwKey = wParam;
#endif
   return ::CallNextHookEx(hHook, code, wParam, lParam);
}
//---------------------------------------------------------------------------
static void SendInputInterrupt()
{
   u16 mask,key;

   if(!(KEYCNT & 0x4000))
       return;
   mask = (u16)(KEYCNT & 0x3FF);
   key = (u16)(0x3FF & ~KeyField);
   if((!(KEYCNT & 0x8000) && !(mask & key)) || ((mask & key) != mask))
       return;
   SetInterrupt(0x1000);
}
//---------------------------------------------------------------------------
char *GetKeyConfig()
{
   keyCode[11] = 0;
   return (char *)keyCode;
}
//---------------------------------------------------------------------------
void SetKeyConfig(char *src)
{
   if(src == NULL || *src == 0)
       return;
   CopyMemory(keyCode,src,10 * sizeof(BYTE));
}
//---------------------------------------------------------------------------
static void ChangeEditKeyName(LPARAM lParam)
{
   char szText[100];
   int i;
   SHORT k;
   HWND hwnd;

   k = (SHORT)((lParam >> 16) & 0xE1FF);
   for(i=0;i<10;i++){
       if(keySelected == i)
           continue;
       if(k == keyCode[i]){
           MessageBeep(MB_ICONHAND);
           hwnd = GetDlgItem(hwndDialog,IDC_BUTTON_A+i);
           EnableWindow(hwnd,FALSE);
           SleepEx(60,FALSE);
           EnableWindow(hwnd,TRUE);
           SleepEx(60,FALSE);
           return;
       }
   }
   GetKeyNameText(lParam,szText,100);
   SetWindowText(GetDlgItem(hwndDialog,IDC_EDIT1),szText);
   keyValue = keyCode[keySelected] = (u16)k;
}
//---------------------------------------------------------------------------
static void OnChangeKeyState(HWND hwndDlg,WORD wID)
{
   int i;

   if(SendMessage(GetDlgItem(hwndDlg,wID),BM_GETCHECK,0,0) == BST_UNCHECKED){
       SetWindowText(GetDlgItem(hwndDlg,IDC_EDIT1),"");
       bModifyKey = FALSE;
       return;
   }
   for(i=0;i<10;i++)
       SendMessage(GetDlgItem(hwndDlg,IDC_BUTTON_A+i),BM_SETCHECK,wID != IDC_BUTTON_A+i ? BST_UNCHECKED : BST_CHECKED,0);
   keySelected = (u8)(wID - IDC_BUTTON_A);
   ChangeEditKeyName(keyCode[keySelected]<<16);
   bModifyKey = TRUE;
   SetFocus(GetDlgItem(hwndDlg,IDC_EDIT1));
}
//---------------------------------------------------------------------------
static LRESULT CALLBACK WindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   switch(uMsg){
       case WM_CHAR:
       case WM_KEYDOWN:
           if(!(lParam & 0x40000000))
               ChangeEditKeyName(lParam);
           return 0;
   }
   HideCaret(hwnd);
   return CallWindowProc(oldWindowProc,hwnd,uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
static BOOL CALLBACK KeyDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   WORD wID;
   int notifyCode;
   BOOL res;

   res = FALSE;
   switch(uMsg){
       case WM_INITDIALOG:
           SendMessage(GetDlgItem(hwndDlg,IDC_BUTTON_RIGHT),BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)LoadBitmap(hInstance,MAKEINTRESOURCE(IDI_BUTTON_RIGHT)));
           SendMessage(GetDlgItem(hwndDlg,IDC_BUTTON_LEFT),BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)LoadBitmap(hInstance,MAKEINTRESOURCE(IDI_BUTTON_LEFT)));
           SendMessage(GetDlgItem(hwndDlg,IDC_BUTTON_UP),BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)LoadBitmap(hInstance,MAKEINTRESOURCE(IDI_BUTTON_UP)));
           SendMessage(GetDlgItem(hwndDlg,IDC_BUTTON_DOWN),BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)LoadBitmap(hInstance,MAKEINTRESOURCE(IDI_BUTTON_DOWN)));
           hwndDialog = hwndDlg;
           oldWindowProc = (WNDPROC)SetWindowLong(GetDlgItem(hwndDlg,IDC_EDIT1),GWL_WNDPROC,(LONG)WindowProc);
           Translation(hwndDlg,0,IDD_DIALOG13);
       break;
       case WM_CLOSE:
           EndDialog(hwndDlg,0);
       break;
       case WM_COMMAND:
           wID = LOWORD(wParam);
           notifyCode = (int)HIWORD(wParam);
           switch(notifyCode){
               case BN_CLICKED:
                   switch(wID){
                       case IDC_BUTTON_RIGHT:
                       case IDC_BUTTON_LEFT:
                       case IDC_BUTTON_UP:
                       case IDC_BUTTON_DOWN:
                       case IDC_BUTTON_START:
                       case IDC_BUTTON_SELECT:
                       case IDC_BUTTON_A:
                       case IDC_BUTTON_B:
                       case IDC_BUTTON_L:
                       case IDC_BUTTON_R:
                           OnChangeKeyState(hwndDlg,wID);
                       break;
                       case IDOK:
                           EndDialog(hwndDlg,1);
                       break;
                       case IDCANCEL:
                           EndDialog(hwndDlg,0);
                       break;
                       case IDC_BUTTON3:
                           for(int i=0;i<10;i++)
                               SendMessage(GetDlgItem(hwndDlg,IDC_BUTTON_A+i),BM_SETCHECK,BST_UNCHECKED,0);
                           SetWindowText(GetDlgItem(hwndDlg,IDC_EDIT1),"");
                           bModifyKey = TRUE;                               
                           SetKeyConfig((char *)keyCodeReset);
                       break;
                   }
               break;
           }
       break;
   }
   return res;
}
//---------------------------------------------------------------------------
void OnKeyConfig()
{
   SHORT oldKeyCode[10];

   bModifyKey = FALSE;
   CopyMemory(oldKeyCode,keyCode,10*sizeof(SHORT));
   if(!DialogBox(hInstance,MAKEINTRESOURCE(IDD_DIALOG13),hWin,KeyDlgProc))
       CopyMemory(keyCode,oldKeyCode,10*sizeof(SHORT));
   bModifyKey = FALSE;
}
