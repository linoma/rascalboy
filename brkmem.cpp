//---------------------------------------------------------------------------
#include <windows.h>
#pragma hdrstop
#include <commctrl.h>

#include "brkmem.h"
#include "debug.h"
#include "debug1.h"
#include "resource.h"
#include "string.h"

//---------------------------------------------------------------------------
#ifdef _DEBPRO
//---------------------------------------------------------------------------
extern HINSTANCE hInstance;
static BOOL CALLBACK DlgProcBRKMEM(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
static void OnCommand(HWND hwndDlg,WORD notifyCode,WORD item,HWND hwnd);
static void OnInitDialog(HWND hwndDlg);
static void OnListView1Notify(LPNMHDR p);
static void RefreshWindow();
static void OnInsertBreakPoint(char *s,int item);
static void OnDeleteAllItems();
//---------------------------------------------------------------------------
static HWND hwndDialog;
static int nCurrentBreakPoint;
//---------------------------------------------------------------------------
static BOOL CALLBACK DlgProcBRKMEM(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   switch(uMsg){
       case WM_INITDIALOG:
           OnInitDialog(hwndDlg);
       break;
       case WM_CLOSE:
           EndDialog(hwndDlg,1);
       break;
       case WM_COMMAND:
           OnCommand(hwndDlg,HIWORD(wParam),LOWORD(wParam),(HWND)lParam);
       break;
       case WM_NOTIFY:
           if(wParam == IDC_BREAKPOINTADRESS)
               OnListView1Notify((LPNMHDR)lParam);
       break;
   }
   return FALSE;
}
//---------------------------------------------------------------------------
static void OnListView1Notify(LPNMHDR p)
{
   NM_LISTVIEW *p1;
   LV_DISPINFO *p2;

   switch(p->code){
       case LVN_ENDLABELEDIT:
           p2 = (LV_DISPINFO *)p;
           OnInsertBreakPoint(p2->item.pszText,p2->item.iItem);
       break;
       case LVN_ITEMCHANGED:
           p1 = (NM_LISTVIEW *)p;
           if(p1->uChanged != 0){
               if((p1->uNewState & 0x3000)){
                   if(nCurrentBreakPoint != -1)
                       ((LPBREAKPOINT)p1->lParam)->bEnable = (char)((p1->uNewState & 0x1000) ? 0 : 1);
               }
               else if((p1->uNewState & 0x2)){
                   if(p1->lParam != NULL){
                       nCurrentBreakPoint = (int)(((char *)p1->lParam - (char *)MAIN_MESSAGE.breakpoint) / sizeof(BREAKPOINT));
                       RefreshWindow();
                   }
               }
           }
       break;
   }
}
//---------------------------------------------------------------------------
static void OnInitDialog(HWND hwndDlg)
{
   LPBREAKPOINT p;
   int i,i1;
   char s[30];
   HWND hwnd;
   DWORD dwExStyle;
   LVITEM item;
   LVCOLUMN col;

   nCurrentBreakPoint = -1;
   hwndDialog = hwndDlg;
   hwnd = GetDlgItem(hwndDlg,IDC_BREAKPOINTADRESS);
   dwExStyle = ListView_GetExtendedListViewStyle(hwnd);
   dwExStyle |= LVS_EX_FULLROWSELECT|LVS_EX_CHECKBOXES;
   ListView_SetExtendedListViewStyle(hwnd,dwExStyle);

   ZeroMemory(&col,sizeof(LVCOLUMN));
   col.mask = LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM;
   col.fmt = LVCFMT_LEFT;
   col.cx = 250;
   col.pszText = "Address";
   ListView_InsertColumn(hwnd,0,&col);

   ListView_DeleteAllItems(hwnd);

   p = MAIN_MESSAGE.breakpoint;
   for(i1=i=0;i<MAX_BREAK;i++){
       if(p[i].Type != BT_MEMORY)
           continue;
       wsprintf(s,"0x%08X",p[i].adress);
       ZeroMemory(&item,sizeof(LVITEM));
       item.mask = LVIF_TEXT|LVIF_PARAM;
       item.iItem = i1;
       item.pszText = s;
       item.lParam = (LPARAM)&p[i];
       ListView_InsertItem(hwnd,&item);
       ListView_SetItemState(hwnd,i1++,INDEXTOSTATEIMAGEMASK(p[i].bEnable ? 2:1), LVIS_STATEIMAGEMASK)
   }
   if(i1 != 0)
       ListView_SetItemState(hwnd,0,LVIS_SELECTED,0xFF);
   SetFocus(hwnd);
}
//---------------------------------------------------------------------------
static void RefreshWindow()
{
   PMEMORYBRK p;
   LPBREAKPOINT p1;

   if(nCurrentBreakPoint < 0)
       return;
   p1 = &listBreakPoint[nCurrentBreakPoint];
   p = (PMEMORYBRK)p1->Condition;
   SendDlgItemMessage(hwndDialog,IDC_RADIO1,BM_SETCHECK,(WPARAM)(p->bWrite ? BST_CHECKED : BST_UNCHECKED),0);
   SendDlgItemMessage(hwndDialog,IDC_RADIO2,BM_SETCHECK,(WPARAM)(p->bRead ? BST_CHECKED : BST_UNCHECKED),0);
   SendDlgItemMessage(hwndDialog,IDC_RADIO3,BM_SETCHECK,(WPARAM)(p->bModify ? BST_CHECKED : BST_UNCHECKED),0);
   SendDlgItemMessage(hwndDialog,IDC_RADIO4,BM_SETCHECK,(WPARAM)(p->bBreak ? BST_CHECKED : BST_UNCHECKED),0);
   SendDlgItemMessage(hwndDialog,IDC_RADIO5,BM_SETCHECK,(WPARAM)((p->mAccess & 1) ? BST_CHECKED : BST_UNCHECKED),0);
   SendDlgItemMessage(hwndDialog,IDC_RADIO6,BM_SETCHECK,(WPARAM)((p->mAccess & 2) ? BST_CHECKED : BST_UNCHECKED),0);
   SendDlgItemMessage(hwndDialog,IDC_RADIO7,BM_SETCHECK,(WPARAM)((p->mAccess & 4) ? BST_CHECKED : BST_UNCHECKED),0);
}
//---------------------------------------------------------------------------
static void OnInsertBreakPoint(char *s,int item)
{
   DWORD dw;
   LPBREAKPOINT p;
   int i;
   HWND hwnd;
   LVITEM lvitem;
   LONG lStyle;

   if(MAIN_MESSAGE.nBreak >= MAX_BREAK - 1 || s == NULL)
       return;
   hwnd = GetDlgItem(hwndDialog,IDC_BREAKPOINTADRESS);
   dw = StrToHex(s);
   p = MAIN_MESSAGE.breakpoint;
   for(i=0;i<MAX_BREAK;i++){
       if(p[i].Type != BT_MEMORY)
           continue;
       if(p[i].adress == dw)
           break;
   }
   if(i < MAX_BREAK){
       ListView_DeleteItem(hwnd,item);
       return;
   }
   nCurrentBreakPoint = -1;
   wsprintf(s,"0x%08X",dw);
   ZeroMemory(&lvitem,sizeof(LVITEM));
   lvitem.mask = LVIF_TEXT|LVIF_STATE|LVIF_PARAM;
   lvitem.stateMask = 0xF0FF;
   lvitem.state = LVIS_SELECTED|0x2000;
   lvitem.pszText = s;
   lvitem.iItem = item;
   p = &listBreakPoint[(nCurrentBreakPoint = MAIN_MESSAGE.nBreak++)];
   p->adress = dw;
   p->Type = BT_MEMORY;
   p->bEnable = 0;
   ZeroMemory(p->Condition,30);
   ((PMEMORYBRK)p->Condition)->mAccess = 0xFF;
   lvitem.lParam = (LPARAM)p;
   ListView_SetItem(hwnd,&lvitem);
   lStyle = GetWindowLong(hwnd,GWL_STYLE);
   lStyle &= ~LVS_EDITLABELS;
   SetWindowLong(hwnd,GWL_STYLE,lStyle);
   RefreshWindow();
}
//---------------------------------------------------------------------------
static void OnDeleteItem()
{
   int i;
   HWND hwnd;

   i = ListView_GetNextItem((hwnd = GetDlgItem(hwndDialog,IDC_BREAKPOINTADRESS)),-1,LVNI_ALL|LVNI_SELECTED);
   if(i != -1){
       ListView_DeleteItem(hwnd,i);
       MAIN_MESSAGE.breakpoint[nCurrentBreakPoint].adress = 0;
       MAIN_MESSAGE.breakpoint[nCurrentBreakPoint].bEnable = 0;
       nCurrentBreakPoint = -1;
       RefillBreakPointList();
       FillComboBreakPoint();
       UpdateDebugToolBar();
       ListView_SetItemState(hwnd,0,LVIS_SELECTED,0xFF);
   }
}
//---------------------------------------------------------------------------
static void OnDeleteAllItems()
{
   int i,i1;
   HWND hwnd;
   LV_ITEM it;
   LPBREAKPOINT p;

   hwnd = GetDlgItem(hwndDialog,IDC_BREAKPOINTADRESS);
   i = ListView_GetItemCount(hwnd);
   for(i1 = 0;i1 < i;i1++){
       ZeroMemory(&it,sizeof(LV_ITEM));
       it.mask = LVIF_PARAM;
       it.iItem = i1;
       ListView_GetItem(hwnd,&it);
       if((p = (LPBREAKPOINT)it.lParam) == NULL)
           continue;
       ZeroMemory(p,sizeof(BREAKPOINT));
   }
   ListView_DeleteAllItems(hwnd);
   nCurrentBreakPoint = -1;
   RefillBreakPointList();
   FillComboBreakPoint();
   UpdateDebugToolBar();
   ListView_SetItemState(hwnd,0,LVIS_SELECTED,0xFF);
}
//---------------------------------------------------------------------------
static void OnDisableAllItems()
{
   int i,i1;
   HWND hwnd;

   hwnd = GetDlgItem(hwndDialog,IDC_BREAKPOINTADRESS);
   i = ListView_GetItemCount(hwnd);
   for(i1 = 0;i1 < i;i1++)
       ListView_SetItemState(hwnd,i1,INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK)
}
//---------------------------------------------------------------------------
int ShowDialogBRKMEM(HWND parent)
{
   return ::DialogBox(hInstance,MAKEINTRESOURCE(IDD_DIALOG11),parent,(DLGPROC)DlgProcBRKMEM);
}
//---------------------------------------------------------------------------
static void OnCommand(HWND hwndDlg,WORD notifyCode,WORD item,HWND hwnd)
{
   PMEMORYBRK p;
   LVITEM lvitem;
   int i;
   LONG lStyle;

   if(!notifyCode && hwnd == NULL){
           switch(item){
               case ID_BRKMEM_NEW:
                   ZeroMemory(&lvitem,sizeof(LVITEM));
                   lvitem.iItem = ListView_GetItemCount(GetDlgItem(hwndDlg,IDC_BREAKPOINTADRESS));
                   nCurrentBreakPoint = -1;
                   ::SetFocus(GetDlgItem(hwndDlg,IDC_BREAKPOINTADRESS));
                   i = ListView_InsertItem(GetDlgItem(hwndDlg,IDC_BREAKPOINTADRESS),&lvitem);
                   if(i != -1){
                       lStyle = GetWindowLong(GetDlgItem(hwndDlg,IDC_BREAKPOINTADRESS),GWL_STYLE);
                       lStyle |= LVS_EDITLABELS;
                       SetWindowLong(GetDlgItem(hwndDlg,IDC_BREAKPOINTADRESS),GWL_STYLE,lStyle);
                       ListView_EditLabel(GetDlgItem(hwndDlg,IDC_BREAKPOINTADRESS),i);
                   }
               break;
               case ID_BRKMEM_DELETE:
                   OnDeleteItem();
               break;
               case ID_BRKMEM_DELETEALL:
                   OnDeleteAllItems();
               break;
               case ID_BRKMEM_DISABLEDALL:
                   OnDisableAllItems();
               break;
           }
       return;
   }
   switch(notifyCode){
       case BN_CLICKED:
           switch(item){
               case IDOK:
                   EndDialog(hwndDlg,1);
               break;
               case IDCANCEL:
                   EndDialog(hwndDlg,0);
               break;
               case IDC_RADIO1:
                   if(nCurrentBreakPoint == -1)
                       return;
                   p = (PMEMORYBRK)MAIN_MESSAGE.breakpoint[nCurrentBreakPoint].Condition;
                   p->bWrite = !p->bWrite;
                   MAIN_MESSAGE.bBreakChanged = TRUE;
               break;
               case IDC_RADIO2:
                   if(nCurrentBreakPoint == -1)
                       return;
                   p = (PMEMORYBRK)MAIN_MESSAGE.breakpoint[nCurrentBreakPoint].Condition;
                   p->bRead = !p->bRead;
                   MAIN_MESSAGE.bBreakChanged = TRUE;
               break;
               case IDC_RADIO3:
                   if(nCurrentBreakPoint == -1)
                       return;
                   p = (PMEMORYBRK)MAIN_MESSAGE.breakpoint[nCurrentBreakPoint].Condition;
                   p->bModify = !p->bModify;
                   MAIN_MESSAGE.bBreakChanged = TRUE;
               break;
               case IDC_RADIO4:
                   if(nCurrentBreakPoint == -1)
                       return;
                   p = (PMEMORYBRK)MAIN_MESSAGE.breakpoint[nCurrentBreakPoint].Condition;
                   p->bBreak = !p->bBreak;
                   MAIN_MESSAGE.bBreakChanged = TRUE;
               break;
               case IDC_RADIO5:
               case IDC_RADIO6:
               case IDC_RADIO7:
                   if(nCurrentBreakPoint == -1)
                       return;
                   p = (PMEMORYBRK)MAIN_MESSAGE.breakpoint[nCurrentBreakPoint].Condition;
                   p->mAccess ^= (char)(1 << ((item - IDC_RADIO5)));
                   MAIN_MESSAGE.bBreakChanged = TRUE;
               break;
           }
       break;
   }
}
#endif

