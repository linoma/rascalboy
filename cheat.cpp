//---------------------------------------------------------------------------
#include <windows.h>
#pragma hdrstop

#include "cheat.h"
#include "gbaemu.h"
#include "debug.h"
#include "resource.h"
#include "trad.h"
#include "inputtext.h"

#define MAX_SIZE_DESCRCOLUMN   150

static LCheatList *pReadCheatList = NULL,*pWriteCheatList = NULL,*pLVCheatList = NULL;
static LCheatList *pStaticCheatList = NULL;
extern BOOL bEnableCheatList;
//---------------------------------------------------------------------------
static u8 v3_deadtable1[256] = {
   0xD0, 0xFF, 0xBA, 0xE5, 0xC1, 0xC7, 0xDB, 0x5B, 0x16, 0xE3, 0x6E, 0x26, 0x62, 0x31, 0x2E, 0x2A,
   0xD1, 0xBB, 0x4A, 0xE6, 0xAE, 0x2F, 0x0A, 0x90, 0x29, 0x90, 0xB6, 0x67, 0x58, 0x2A, 0xB4, 0x45,
   0x7B, 0xCB, 0xF0, 0x73, 0x84, 0x30, 0x81, 0xC2, 0xD7, 0xBE, 0x89, 0xD7, 0x4E, 0x73, 0x5C, 0xC7,
   0x80, 0x1B, 0xE5, 0xE4, 0x43, 0xC7, 0x46, 0xD6, 0x6F, 0x7B, 0xBF, 0xED, 0xE5, 0x27, 0xD1, 0xB5,
   0xD0, 0xD8, 0xA3, 0xCB, 0x2B, 0x30, 0xA4, 0xF0, 0x84, 0x14, 0x72, 0x5C, 0xFF, 0xA4, 0xFB, 0x54,
   0x9D, 0x70, 0xE2, 0xFF, 0xBE, 0xE8, 0x24, 0x76, 0xE5, 0x15, 0xFB, 0x1A, 0xBC, 0x87, 0x02, 0x2A,
   0x58, 0x8F, 0x9A, 0x95, 0xBD, 0xAE, 0x8D, 0x0C, 0xA5, 0x4C, 0xF2, 0x5C, 0x7D, 0xAD, 0x51, 0xFB,
   0xB1, 0x22, 0x07, 0xE0, 0x29, 0x7C, 0xEB, 0x98, 0x14, 0xC6, 0x31, 0x97, 0xE4, 0x34, 0x8F, 0xCC,
   0x99, 0x56, 0x9F, 0x78, 0x43, 0x91, 0x85, 0x3F, 0xC2, 0xD0, 0xD1, 0x80, 0xD1, 0x77, 0xA7, 0xE2,
   0x43, 0x99, 0x1D, 0x2F, 0x8B, 0x6A, 0xE4, 0x66, 0x82, 0xF7, 0x2B, 0x0B, 0x65, 0x14, 0xC0, 0xC2,
   0x1D, 0x96, 0x78, 0x1C, 0xC4, 0xC3, 0xD2, 0xB1, 0x64, 0x07, 0xD7, 0x6F, 0x02, 0xE9, 0x44, 0x31,
   0xDB, 0x3C, 0xEB, 0x93, 0xED, 0x9A, 0x57, 0x05, 0xB9, 0x0E, 0xAF, 0x1F, 0x48, 0x11, 0xDC, 0x35,
   0x6C, 0xB8, 0xEE, 0x2A, 0x48, 0x2B, 0xBC, 0x89, 0x12, 0x59, 0xCB, 0xD1, 0x18, 0xEA, 0x72, 0x11,
   0x01, 0x75, 0x3B, 0xB5, 0x56, 0xF4, 0x8B, 0xA0, 0x41, 0x75, 0x86, 0x7B, 0x94, 0x12, 0x2D, 0x4C,
   0x0C, 0x22, 0xC9, 0x4A, 0xD8, 0xB1, 0x8D, 0xF0, 0x55, 0x2E, 0x77, 0x50, 0x1C, 0x64, 0x77, 0xAA,
   0x3E, 0xAC, 0xD3, 0x3D, 0xCE, 0x60, 0xCA, 0x5D, 0xA0, 0x92, 0x78, 0xC6, 0x51, 0xFE, 0xF9, 0x30
};
//---------------------------------------------------------------------------
static u8 v3_deadtable2[256] = {
   0xAA, 0xAF, 0xF0, 0x72, 0x90, 0xF7, 0x71, 0x27, 0x06, 0x11, 0xEB, 0x9C, 0x37, 0x12, 0x72, 0xAA,
   0x65, 0xBC, 0x0D, 0x4A, 0x76, 0xF6, 0x5C, 0xAA, 0xB0, 0x7A, 0x7D, 0x81, 0xC1, 0xCE, 0x2F, 0x9F,
   0x02, 0x75, 0x38, 0xC8, 0xFC, 0x66, 0x05, 0xC2, 0x2C, 0xBD, 0x91, 0xAD, 0x03, 0xB1, 0x88, 0x93,
   0x31, 0xC6, 0xAB, 0x40, 0x23, 0x43, 0x76, 0x54, 0xCA, 0xE7, 0x00, 0x96, 0x9F, 0xD8, 0x24, 0x8B,
   0xE4, 0xDC, 0xDE, 0x48, 0x2C, 0xCB, 0xF7, 0x84, 0x1D, 0x45, 0xE5, 0xF1, 0x75, 0xA0, 0xED, 0xCD,
   0x4B, 0x24, 0x8A, 0xB3, 0x98, 0x7B, 0x12, 0xB8, 0xF5, 0x63, 0x97, 0xB3, 0xA6, 0xA6, 0x0B, 0xDC,
   0xD8, 0x4C, 0xA8, 0x99, 0x27, 0x0F, 0x8F, 0x94, 0x63, 0x0F, 0xB0, 0x11, 0x94, 0xC7, 0xE9, 0x7F,
   0x3B, 0x40, 0x72, 0x4C, 0xDB, 0x84, 0x78, 0xFE, 0xB8, 0x56, 0x08, 0x80, 0xDF, 0x20, 0x2F, 0xB9,
   0x66, 0x2D, 0x60, 0x63, 0xF5, 0x18, 0x15, 0x1B, 0x86, 0x85, 0xB9, 0xB4, 0x68, 0x0E, 0xC6, 0xD1,
   0x8A, 0x81, 0x2B, 0xB3, 0xF6, 0x48, 0xF0, 0x4F, 0x9C, 0x28, 0x1C, 0xA4, 0x51, 0x2F, 0xD7, 0x4B,
   0x17, 0xE7, 0xCC, 0x50, 0x9F, 0xD0, 0xD1, 0x40, 0x0C, 0x0D, 0xCA, 0x83, 0xFA, 0x5E, 0xCA, 0xEC,
   0xBF, 0x4E, 0x7C, 0x8F, 0xF0, 0xAE, 0xC2, 0xD3, 0x28, 0x41, 0x9B, 0xC8, 0x04, 0xB9, 0x4A, 0xBA,
   0x72, 0xE2, 0xB5, 0x06, 0x2C, 0x1E, 0x0B, 0x2C, 0x7F, 0x11, 0xA9, 0x26, 0x51, 0x9D, 0x3F, 0xF8,
   0x62, 0x11, 0x2E, 0x89, 0xD2, 0x9D, 0x35, 0xB1, 0xE4, 0x0A, 0x4D, 0x93, 0x01, 0xA7, 0xD1, 0x2D,
   0x00, 0x87, 0xE2, 0x2D, 0xA4, 0xE9, 0x0A, 0x06, 0x66, 0xF8, 0x1F, 0x44, 0x75, 0xB5, 0x6B, 0x1C,
   0xFC, 0x31, 0x09, 0x48, 0xA3, 0xFF, 0x92, 0x12, 0x58, 0xE9, 0xFA, 0xAE, 0x4F, 0xE2, 0xB4, 0xCC
};
//---------------------------------------------------------------------------
static u32 seeds[4],bModified;
static int iSubCodError;
//---------------------------------------------------------------------------
static void decrypt_code(u32 *address, u32 *value);
static void encrypt_code(u32 *address, u32 *value);
static void deadface(u16 value);
static u32 seed_gen(u8 upper, u8 seed, u8 *deadtable1, u8 *deadtable2);
//---------------------------------------------------------------------------
static void decrypt_code(u32 *address, u32 *value)
{
   int i;
   u32 rollingseed = 0xC6EF3720;

   for(i = 0; i < 32; i++) {
       *value -= ((((*address << 4) + seeds[2]) ^ (*address + rollingseed)) ^ ((*address >> 5) + seeds[3]));
       *address -= ((((*value << 4) + seeds[0]) ^ (*value + rollingseed)) ^ ((*value >> 5) + seeds[1]));
       rollingseed -= 0x9E3779B9;
   }
   if (*address == 0xDEADFACE)
       deadface((u16)*value);
}
//---------------------------------------------------------------------------
static void encrypt_code(u32 *address, u32 *value)
{
   int i;
   u32 rollingseed = 0, oldaddr = *address, oldval = *value;

   for (i = 0; i < 32; i++) {
       rollingseed += 0x9E3779B9;
       *address += ((((*value << 4) + seeds[0]) ^ (*value + rollingseed)) ^ ((*value >> 5) + seeds[1]));
       *value += ((((*address << 4) + seeds[2]) ^ (*address + rollingseed)) ^ ((*address >> 5) + seeds[3]));
   }
   if (oldaddr == 0xDEADFACE)
       deadface((u16)oldval);
}
//---------------------------------------------------------------------------
static void deadface(u16 value)
{
   int i;

   for(i = 0; i < 4; i++)
       seeds[i] = seed_gen((u8)((value & 0xFF00) >> 8),(u8)((value & 0xFF) + i), v3_deadtable1, v3_deadtable2);
}
//---------------------------------------------------------------------------
static u32 seed_gen(u8 upper, u8 seed, u8 *deadtable1, u8 *deadtable2)
{
   int i;
   u32 newseed;

   newseed = 0;
   for (i = 0; i < 4; i++)
       newseed = ((newseed << 8) | ((deadtable1[(i + upper) & 0xFF] + deadtable2[seed]) & 0xFF));
   return newseed;
}
//---------------------------------------------------------------------------
static BOOL EnumCheatProc(LPCHEATHEADER p,LPARAM lParam)
{
   LVITEM item;
   int i;
   HWND hwndListView;

   hwndListView = (HWND)lParam;
   i = ListView_GetItemCount(hwndListView);
   ZeroMemory(&item,sizeof(LVITEM));
   item.mask = LVIF_TEXT|LVIF_PARAM;
   item.iItem = i;
   item.pszText = p->descr;
   item.lParam = (LPARAM)p;
   ListView_InsertItem(hwndListView,&item);
   ListView_SetItemText(hwndListView,i,1,p->codeString);
   ListView_SetItemText(hwndListView,i,2,(p->type == GameShark ? "GameShark" : "Codebreaker"));
   ListView_SetItemState(hwndListView,i,INDEXTOSTATEIMAGEMASK(p->Enable ? 2 : 1), LVIS_STATEIMAGEMASK)
   return TRUE;
}
//---------------------------------------------------------------------------
static LCheatList *GetCheatListFromCode(DWORD id)
{
   switch(id){
       case CLID_READ:
           return pReadCheatList;
       case CLID_WRITE:
           return pWriteCheatList;
       case CLID_STATIC:
           return pStaticCheatList;
       default:
           return NULL;
   }
}
//---------------------------------------------------------------------------
static void OnInitDialog(HWND hwndDlg)
{
   LString s;
   HWND hwnd1;
   LVCOLUMN col;
   HMENU hMenu;

   hMenu = LoadMenu(FindResourceInternal(IDR_CHEAT,RT_MENU),MAKEINTRESOURCE(IDR_CHEAT));
   if(hMenu != NULL)
       SetMenu(hwndDlg,hMenu);   
   Translation(hwndDlg,IDR_CHEAT,IDD_DIALOG16);
   hwnd1 = GetDlgItem(hwndDlg,IDC_LIST1);
   ListView_SetExtendedListViewStyle(hwnd1,ListView_GetExtendedListViewStyle(hwnd1)|LVS_EX_FULLROWSELECT|LVS_EX_CHECKBOXES);
   ZeroMemory(&col,sizeof(LVCOLUMN));
   col.mask = LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM;
   col.fmt = LVCFMT_LEFT;
   col.cx = MAX_SIZE_DESCRCOLUMN;
   col.pszText = "Description";
   ListView_InsertColumn(hwnd1,0,&col);
   col.cx = 130;
   col.pszText = "Code";
   ListView_InsertColumn(hwnd1,1,&col);
   col.cx = 80;
   col.pszText = "Type";
   ListView_InsertColumn(hwnd1,2,&col);
   if(pLVCheatList != NULL)
       pLVCheatList->EnumCheat(EnumCheatProc,(LPARAM)hwnd1);
   SetFocus(hwnd1);
}
//---------------------------------------------------------------------------
static LString SearchCheatCode(const char *lpCode,int *len)
{
   LString s,s1,res,s2;
   int i;

   res = "";
   s = lpCode;
   *len = 0;
   if(s.IsEmpty())
       goto ex_SearchCheatCode;
   s2 = "0123456789ABCDEF";
   do{
       s1 = s.NextToken(13);
       if(s1.IsEmpty()){
           s1 = s.RemainderToken();
           if(s1.IsEmpty())
               break;
       }
       else
           (*len)++;
       *len += s1.Length();
       for(i=1;i<=s1.Length();i++){
           if(s1[i] == 32 || s2.Pos(s1[i]) < 1)
               continue;
           res += s1[i];
       }
   }while(res.Length() < 9);
ex_SearchCheatCode:
   return res;
}
//---------------------------------------------------------------------------
void OnNewCheat(HWND hwndDlg,BOOL isGameShark)
{
   LVITEM item;
   int i,pos,len;
   HWND hwndListView,hwndHeader;
   RECT rcItem,rc,rcHeader;
   LString s,s1,s3;
   DWORD res;

   hwndListView = GetDlgItem(hwndDlg,IDC_LIST1);
   ZeroMemory(&item,sizeof(LVITEM));
   i = ListView_GetItemCount(hwndListView);
   item.mask = LVIF_TEXT;
   item.iItem = i;
   item.pszText = "";
   ListView_InsertItem(hwndListView,&item);
   ListView_EnsureVisible(hwndListView,i,FALSE);
   ListView_GetItemRect(hwndListView,i,&rcItem,LVIR_LABEL);
   CopyRect(&rc,&rcItem);
   s.Capacity(MAX_CHEATNAME+10);
   s.c_str()[0] = 0;
   if(!InputText(hwndListView,&rc,0,s.c_str(),MAX_CHEATNAME))
       goto ex_OnNewCheatError;
   ListView_SetItemText(hwndListView,i,0,s.c_str());
   rc.left += rc.right;
   rc.right += rc.right;
   s1.Capacity(2000);
   s1.c_str()[0] = 0;
   GetClientRect(hwndListView,&rcItem);
   hwndHeader = ListView_GetHeader(hwndListView);
   if(hwndHeader != NULL){
       GetWindowRect(hwndHeader,&rcHeader);
       rc.top = rcHeader.bottom - rcHeader.top;
   }
   else
       rc.top = rcItem.top;
   if(ListView_GetSubItemRect(hwndListView,i,1,0,&rcHeader))
       rc.right = rcHeader.right;
   rc.bottom = rcItem.bottom;
   if(!InputText(hwndListView,&rc,ES_UPPERCASE|ES_MULTILINE|ES_WANTRETURN|ES_AUTOVSCROLL,s1.c_str(),1999))
       goto ex_OnNewCheatError;
   pos = 0;
   do{
       s3 = SearchCheatCode(s1.c_str() + pos,&len);
       if(s3.IsEmpty())
           break;
       ListView_SetItemText(hwndListView,i,1,s3.c_str());
       ListView_SetItemText(hwndListView,i,2,(isGameShark ? "GameShark" : "Codebreaker"));
       if(isGameShark)
           res = AddGameShark(s3.c_str(),s.c_str());
       else
           res = AddCodeBreak(s3.c_str(),s.c_str());
       pos += len;
       if(!res)
           continue;
       item.mask = LVIF_PARAM|LVIF_STATE;
       item.stateMask = LVIS_STATEIMAGEMASK;
       item.state = INDEXTOSTATEIMAGEMASK(2);
       item.lParam = (LPARAM)res;
       ListView_SetItem(hwndListView,&item);
       bModified = TRUE;
       item.mask = LVIF_TEXT;
       item.pszText = s.c_str();
       item.iItem = ++i;
       ListView_InsertItem(hwndListView,&item);
   }while(pos < s1.Length());
ex_OnNewCheatError:
   ListView_DeleteItem(hwndListView,i);
}
//---------------------------------------------------------------------------
static void OnDeleteCheat(HWND hwndDlg)
{
   HWND hwndListView;
   int item,index,index1;
   LV_ITEM lvitem;
   LCheatList *pList;
   LPCHEATHEADER cheatHeader;

   hwndListView = GetDlgItem(hwndDlg,IDC_LIST1);
   item = ListView_GetNextItem(hwndListView,-1,LVNI_ALL|LVNI_SELECTED);
   if(item == -1)
       return;
   ZeroMemory(&lvitem,sizeof(LV_ITEM));
   lvitem.mask = LVIF_PARAM;
   lvitem.iItem = item;
   if(!ListView_GetItem(hwndListView,&lvitem))
       return;
   cheatHeader = (LPCHEATHEADER)lvitem.lParam;
   if(cheatHeader == NULL)
       return;
   pList = (LCheatList *)cheatHeader->pList;
   if(pList == NULL)
       return;
   index = pList->IndexFromEle((LPVOID)cheatHeader);
   if(index == -1)
       return;
   index1 = pLVCheatList->IndexFromEle((LPVOID)cheatHeader);
   if(index1 == -1)
       return;
   pLVCheatList->Remove(index1);
   pList->Delete(index);
   ListView_DeleteItem(hwndListView,item);
   ResetCheatSystem();
   bModified = TRUE;
}
//---------------------------------------------------------------------------
static void OnDeleteAllCheat(HWND hwndDlg)
{
   ListView_DeleteAllItems(GetDlgItem(hwndDlg,IDC_LIST1));
   if(pLVCheatList != NULL)
       pLVCheatList->Clear();
   if(pWriteCheatList != NULL)
       pWriteCheatList->Clear();
   if(pReadCheatList != NULL)
       pReadCheatList->Clear();
   if(pStaticCheatList != NULL)
       pStaticCheatList->Clear();
   ResetCheatSystem();
   bModified = TRUE;
}
//---------------------------------------------------------------------------
static void OnNotifyListView1(LPNMHDR pHeader)
{
   NM_LISTVIEW *p;
   LV_ITEM item;
   LPCHEATHEADER p1;

   p = (NM_LISTVIEW *)pHeader;
   switch(pHeader->code){
       case LVN_ITEMCHANGED:
           if((p->uChanged & 0x8) && p->iSubItem == 0 && p->uOldState != 0){
               ZeroMemory(&item,sizeof(LV_ITEM));
               item.iItem = p->iItem;
               item.mask = LVIF_PARAM;
               ListView_GetItem(p->hdr.hwndFrom,&item);
               if((p1 = (LPCHEATHEADER)item.lParam) != NULL){
                   p1->Enable = (u8)((p->uNewState & 0x2000) ? 1 : 0);
               }
           }
       break;
   }
}
//---------------------------------------------------------------------------
static void OnSaveCheatList(HWND hwndDlg)
{
   SaveCheatList(hwndDlg);
}
//---------------------------------------------------------------------------
static void OnLoadCheatList(HWND hwndDlg)
{
   HWND hwndListView;

   hwndListView = GetDlgItem(hwndDlg,IDC_LIST1);
   if(!LoadCheatList(hwndDlg))
       return;
   ListView_DeleteAllItems(hwndListView);
   if(pLVCheatList != NULL)
       pLVCheatList->EnumCheat(EnumCheatProc,(LPARAM)hwndListView);
}
//---------------------------------------------------------------------------
static BOOL CALLBACK DlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   BOOL res;
   UINT i;

   res = FALSE;
   switch(uMsg){
       case WM_INITMENU:
           i = MF_BYCOMMAND;
           if(ListView_GetSelectedCount(GetDlgItem(hwndDlg,IDC_LIST1)) > 0)
               i |= MF_ENABLED;
           else
               i |= MF_GRAYED;
           EnableMenuItem((HMENU)wParam,ID_CHEAT_DELETE,i);
           i = MF_BYCOMMAND;
           if(ListView_GetItemCount(GetDlgItem(hwndDlg,IDC_LIST1)) > 0)
               i |= MF_ENABLED;
           else
               i |= MF_GRAYED;
           EnableMenuItem((HMENU)wParam,ID_CHEAT_DELETE_ALL,i);
           i = (bModified != 0 ? MF_ENABLED : MF_GRAYED);
           EnableMenuItem((HMENU)wParam,ID_FILE_CHEAT_SAVE,i|MF_BYCOMMAND);
       break;
       case WM_INITDIALOG:
           OnInitDialog(hwndDlg);
       break;
       case WM_NOTIFY:
           if(wParam == IDC_LIST1)
               OnNotifyListView1((LPNMHDR)lParam);
       break;
       case WM_CLOSE:
           EndDialog(hwndDlg,0);
       break;
       case WM_COMMAND:
           i = (UINT)LOWORD(wParam);
           if(!HIWORD(wParam) && !lParam){
               switch(i){
                   case ID_CHEAT_NEW_GAMESHARK:
                   case ID_CHEAT_NEW_CODEBREAKER:
                       OnNewCheat(hwndDlg,(i == ID_CHEAT_NEW_CODEBREAKER ? FALSE : TRUE));
                   break;
                   case ID_CHEAT_DELETE:
                       OnDeleteCheat(hwndDlg);
                   break;
                   case ID_CHEAT_DELETE_ALL:
                       OnDeleteAllCheat(hwndDlg);
                   break;
                   case ID_FILE_CHEAT_LOAD:
                       OnLoadCheatList(hwndDlg);
                   break;
                   case ID_FILE_CHEAT_SAVE:
                       OnSaveCheatList(hwndDlg);
                   break;
               }
               return FALSE;
           }
           switch(HIWORD(wParam)){
               case BN_CLICKED:
                   switch(i){
                       case IDOK:
                           EndDialog(hwndDlg,1);
                       break;
                   }
               break;
           }
       break;
       case WM_DESTROY:
           i = (UINT)GetMenu(hwndDlg);
           if(i != NULL)
               DestroyMenu((HMENU)i);
       break;
   }
   return res;
}
//---------------------------------------------------------------------------
BOOL ShowCheatDialog(HWND hwndParent)
{
   DialogBox(FindResourceInternal(IDD_DIALOG16,RT_DIALOG),MAKEINTRESOURCE(IDD_DIALOG16),hwndParent,DlgProc);
   return TRUE;
}
//---------------------------------------------------------------------------
static BOOL EnumCheatProcResetCheat(LPCHEATHEADER p,LPARAM lParam)
{
   LPCHEATHEADER p1;
   DWORD index;

   if(lParam == 0)
       return FALSE;
   if(p->pNext == (LPCHEATHEADER)-1 && pLVCheatList != NULL){
       index = pLVCheatList->IndexFromEle((LPVOID)p);
       if(index != (DWORD)-1){
           p1 = (LPCHEATHEADER)pLVCheatList->GetItem(index+1);
           if(p1 != NULL){
               p->pNext = p1;
               p1->skip = 1;
           }
       }
   }
   if(p->Enable)
       (*((LPDWORD)lParam))++;
   return TRUE;
}
//---------------------------------------------------------------------------
void ResetCheatSystem()
{
   u8 value,value1;

   if(!bEnableCheatList){
       SetReadMemFunction(0,0);
       return;
   }
   value = 0;
   if(pStaticCheatList != NULL)
       pStaticCheatList->EnumCheat(EnumCheatProcResetCheat,(LPARAM)&value);
   value = value1 = 0;
   if(pReadCheatList != NULL){
       pReadCheatList->EnumCheat(EnumCheatProcResetCheat,(LPARAM)&value);
       if(value != 0)
           value = 1;
   }
   if(pWriteCheatList != NULL){
       pWriteCheatList->EnumCheat(EnumCheatProcResetCheat,(LPARAM)&value1);
       if(value1 != 0)
           value1 = 1;
   }
   SetReadMemFunction(value,value1);
}
//---------------------------------------------------------------------------
BOOL LoadCheatInternal(const char *lpFileName)
{
   LFile *pFile;
   DWORD dw;
   BOOL res;

   if(lpFileName == NULL)
       return FALSE;
   pFile = new LFile(lpFileName);
   if(pFile == NULL)
       return FALSE;
   res = FALSE;
   if(!pFile->Open())
       goto ex_LoadCheatInternal;
   res = FALSE;
   if(pFile->Read(&dw,sizeof(DWORD)) != sizeof(DWORD))
       goto ex_LoadCheatInternal;
   if(dw != 1)
       goto ex_LoadCheatInternal;
   if(pFile->Read(&dw,sizeof(DWORD)) != sizeof(DWORD))
       goto ex_LoadCheatInternal;
   if(pLVCheatList == NULL)
       goto ex_LoadCheatInternal;
   if((res = pLVCheatList->Load(pFile)) != 0)
       bModified = FALSE;
ex_LoadCheatInternal:
   delete pFile;
   return res;
}
//---------------------------------------------------------------------------
BOOL SaveCheatInternal(const char *lpFileName)
{
   LFile *pFile;
   DWORD dw;
   BOOL res;

   if(lpFileName == NULL)
       return FALSE;
   if(pWriteCheatList->Count() < 1)
       return TRUE;
   pFile = new LFile(lpFileName);
   if(pFile == NULL)
       return FALSE;
   res = FALSE;
   if(!pFile->Open(GENERIC_WRITE,CREATE_ALWAYS))
       goto ex_SaveCheatInternal;
   dw = 1;
   pFile->Write(&dw,sizeof(DWORD));
   if((res = pLVCheatList->Save(pFile)) != 0)
       bModified = FALSE;
ex_SaveCheatInternal:
   delete pFile;
   return res;
}
//---------------------------------------------------------------------------
void LoadCheatListFromRom(BOOL bAuto)
{
   LString nameFile;

   if(!bAuto)
       return;
   if(!nameFile.BuildFileName(bin.saveFileName,"cht"))
       return;
   if(pWriteCheatList != NULL)
       pWriteCheatList->Clear();
   if(pReadCheatList != NULL)
       pReadCheatList->Clear();
   if(pLVCheatList != NULL)
       pLVCheatList->Clear();
   LoadCheatInternal(nameFile.c_str());
}
//---------------------------------------------------------------------------
BOOL SaveCheatList(HWND hwndParent)
{
   char szFile[MAX_PATH];
   LString s,nameFile;

   if(nameFile.BuildFileName(bin.saveFileName,"cht"))
       lstrcpy(szFile,nameFile.c_str());
   else
       *((LPDWORD)szFile) = 0;
   if(!ShowSaveDialog(NULL,szFile,"RBA CheatList(*.cht)\0*.cht\0\0\0\0\0",hwndParent,NULL))
       return FALSE;
   return SaveCheatInternal(szFile);
}
//---------------------------------------------------------------------------
BOOL LoadCheatList(HWND hwndParent)
{
   char szFile[MAX_PATH];
   LString nameFile;

   if(nameFile.BuildFileName(bin.saveFileName,"cht"))
       lstrcpy(szFile,nameFile.c_str());
   else
       *((LPDWORD)szFile) = 0;
   if(!ShowOpenDialog(NULL,szFile,"RBA CheatList(*.cht)\0*.cht\0\0\0\0\0",hwndParent))
       return FALSE;
   return LoadCheatInternal(szFile);
}
//---------------------------------------------------------------------------
BOOL InitCheatSystem()
{
//   u32 value,adr;

   bModified = FALSE;
   pWriteCheatList = NULL;
   pLVCheatList = NULL;
   pReadCheatList = new LCheatList(CLID_READ);
   if(pReadCheatList == NULL)
       return FALSE;
   pWriteCheatList = new LCheatList(CLID_WRITE);
   if(pWriteCheatList == NULL)
       return FALSE;
   pLVCheatList = new LCheatList(CLID_LISTVIEW,TRUE);
   if(pLVCheatList == NULL)
       return FALSE;
   pStaticCheatList = new LCheatList(CLID_STATIC);
   if(pStaticCheatList == NULL)
       return FALSE;
   deadface(0);
//   adr = (2 << 0x19) | ( 1 << 0x1b) | (0x6000000 >> 4) | (0x600025c & 0x3FFFF);
//   value = 0xe3a01000;//cfd57142 34ca50cb
//   encrypt_code(&adr,&value);
//   adr = (2 << 0x19) | (0 << 0x1b) | (0x6000000 >> 4) | (0x600025c & 0x3FFFF);
//   value = 0xe3a010FF;//
//   encrypt_code(&adr,&value);
//cfd57142 34ca50cb
//2af0fdba 479c97ad

   return TRUE;
}
//---------------------------------------------------------------------------
static int sortCheatList(LPVOID ele,LPVOID ele1)
{
   u32 adr0,adr1;

   adr0 = ((LPCHEATHEADER)ele)->adr;
   adr1 = ((LPCHEATHEADER)ele1)->adr;
   if(adr0 < adr1)
       return -1;
   else if(adr0 > adr1)
       return 1;
   return 0;
}
//---------------------------------------------------------------------------
void DestroyCheatSystem()
{
   bModified = FALSE;
   if(pReadCheatList != NULL){
       delete pReadCheatList;
       pReadCheatList = NULL;
   }
   if(pWriteCheatList != NULL){
       delete pWriteCheatList;
       pWriteCheatList = NULL;
   }
   if(pLVCheatList != NULL){
       delete pLVCheatList;
       pLVCheatList = NULL;
   }
   if(pStaticCheatList != NULL){
       delete pStaticCheatList;
       pStaticCheatList = NULL;
   }
}
//---------------------------------------------------------------------------
void ApplyStaticCheatList()
{
   DWORD dwPos;
   LPCHEATHEADER cheatHeader;

   if(!bEnableCheatList || pStaticCheatList == NULL || pStaticCheatList->Count() < 1)
       return;
   cheatHeader = (LPCHEATHEADER)pStaticCheatList->GetFirstItem(&dwPos);
   while(cheatHeader != NULL){
       if(cheatHeader->pEvaluateFunc != NULL)
           cheatHeader->pEvaluateFunc(cheatHeader,cheatHeader->adr,AMM_WORD);
       cheatHeader = (LPCHEATHEADER)pStaticCheatList->GetNextItem(&dwPos);
   }
}
//---------------------------------------------------------------------------
void ApplyAllCheatList()
{
   DWORD dwPos;
   LPCHEATHEADER cheatHeader;

   if(!bEnableCheatList || pWriteCheatList == NULL || pWriteCheatList->Count() < 1)
       return;
   cheatHeader = (LPCHEATHEADER)pWriteCheatList->GetFirstItem(&dwPos);
   while(cheatHeader != NULL){
       if(cheatHeader->pEvaluateFunc != NULL)
           cheatHeader->pEvaluateFunc(cheatHeader,cheatHeader->adr,AMM_WORD);
       cheatHeader = (LPCHEATHEADER)pWriteCheatList->GetNextItem(&dwPos);
   }
}
//---------------------------------------------------------------------------
void FASTCALL write_byteCheat(u32 address,u32 data)
{
   write_byte(address,data);
   pWriteCheatList->EvaluateCheat(address,AMM_BYTE);
}
//---------------------------------------------------------------------------
void FASTCALL write_hwordCheat(u32 address,u16 data)
{
   write_hword(address,data);
   pWriteCheatList->EvaluateCheat(address,AMM_HWORD);
}
//---------------------------------------------------------------------------
void FASTCALL write_wordCheat(u32 address,u32 data)
{
   write_word(address,data);
   pWriteCheatList->EvaluateCheat(address,AMM_WORD);
}
//---------------------------------------------------------------------------
u8 FASTCALL read_byteCheat(u32 address)
{
   pWriteCheatList->EvaluateCheat(address,AMM_BYTE);
   return read_byte(address);
}
//---------------------------------------------------------------------------
u16 FASTCALL read_hwordCheat(u32 address)
{
   pWriteCheatList->EvaluateCheat(address,AMM_HWORD);
   return read_hword(address);
}
//---------------------------------------------------------------------------
u32 FASTCALL read_wordCheat(u32 address)
{
   pWriteCheatList->EvaluateCheat(address,AMM_WORD);
   return read_word(address);
}
//---------------------------------------------------------------------------
static void EvaluateCodeBreak(LPCHEATHEADER p,u32 address,u8 accessMode)
{
   u32 i,adr;

   switch(p->code){
       case 3:
           write_byte(p->adr,(u8)p->value);
       break;
       case 8:
           write_hword(p->adr,(u16)p->value);
       break;
       case 4:
           if(p->pNext == NULL || p->pNext == (LPCHEATHEADER)-1)
               return;
           adr = p->adr;
           for(i=0;i<p->pNext->adr;i++,adr+= p->pNext->value)
               write_hword(adr,(u16)p->value);
       break;
       case 7:
           if(p->pNext == NULL || p->pNext == (LPCHEATHEADER)-1)
               return;
           if(read_hword(p->adr) == (u16)p->value)
               EvaluateCodeBreak(p->pNext,address,accessMode);
       break;
   }
}
//---------------------------------------------------------------------------
static void EvaluateGameShark(LPCHEATHEADER p,u32 address,u8 accessMode)
{
   switch(p->code){
       case 0:
           write_byte(p->adr,(u8)p->value);
       break;
       case 1:
           write_hword(p->adr,(u16)p->value);
       break;
       case 2:
           write_word(p->adr,p->value);
       break;
       case 4:
           if(p->pNext == NULL || p->pNext == (LPCHEATHEADER)-1)
               return;
           if(read_byte(p->adr) == (u8)p->value)
               EvaluateGameShark(p->pNext,address,accessMode);
       break;
       case 5:
           if(p->pNext == NULL || p->pNext == (LPCHEATHEADER)-1)
               return;
           if(read_hword(p->adr) == (u16)p->value)
               EvaluateGameShark(p->pNext,address,accessMode);
       break;
       case 6:
           if(p->pNext == NULL || p->pNext == (LPCHEATHEADER)-1)
               return;
           if(read_word(p->adr) == (u32)p->value)
               EvaluateGameShark(p->pNext,address,accessMode);
       break;
       case 0x62:
           write_hword(0x8000000|p->adr,(u16)p->value);
       break;
       case 0xDE:
           write_word(0x080000AC,p->value);
       break;
   }
}
//---------------------------------------------------------------------------
static BOOL CheckCheatCode(u32 *x,u32 *y,const char *lpCode,BOOL isGameShark)
{
   LString s,s1;
   int i,i1;

   if(x == NULL || y == NULL || lpCode == NULL)
       return FALSE;
   s = lpCode;
   i = isGameShark ? 16 : 12;
   i = (i1 = s.Pos(" ")) > 0 ? i+1 : i;
   if(s.Length() != i){
       iSubCodError = -1;
       ShowMessageError(TE_CHEAT,"");
       return FALSE;
   }
   if(i1 < 1){
       s1 = s.SubString(1,8) + " ";
       s1 += s.SubString(9,s.Length() - 8);
       s = s1;
   }
   *x = *y = 0;
   s1 = "0x";
   s1 += s.NextToken(32);
   *x = StrToHex(s1.c_str());
   s1 = "0x";
   s1 += s.RemainderToken();
   *y = StrToHex(s1.c_str());
   return TRUE;
}
//---------------------------------------------------------------------------
int GetCheatSystemError()
{
   return iSubCodError;
}
//---------------------------------------------------------------------------
DWORD AddCodeBreak(const char *lpCode,const char *lpDescr)
{
   LPCODEBREAK p;
   u32 adr,value;
   LCheatList *pList;

   if(pReadCheatList == NULL || pWriteCheatList == NULL || !CheckCheatCode(&adr,&value,lpCode,FALSE))
       return 0;
   p = new CODEBREAK[1];
   if(p == NULL)
       return 0;
   ZeroMemory(p,sizeof(CODEBREAK));
   lstrcpy(p->codeString,lpCode);
   if(lpDescr != NULL)
       lstrcpyn(p->descr,lpDescr,MAX_CHEATNAME+1);
   p->code = (u8)(adr >> 28);
   p->type = CodeBreak;
   p->Enable = TRUE;
   p->adr = (u32)(adr & 0x0FFFFFFF);
   p->pEvaluateFunc = EvaluateCodeBreak;
   switch(p->code){
       case 3:
           p->value = (u32)((u8)value);
           pList = pWriteCheatList;
       break;
       case 4:
       case 7:
           p->pNext = (LPCHEATHEADER)-1;
       case 8:
           p->value = (u32)((u16)value);
           pList = pWriteCheatList;
       break;
       default:
           p->value = value;
           pList = pReadCheatList;
       break;
   }
   if(pList == NULL || !pList->Add((LPCHEATHEADER)p)){
       delete p;
       return 0;
   }
   pLVCheatList->Add((LPCHEATHEADER)p);
   pList->Sort(1,pList->Count(),sortCheatList);
   ResetCheatSystem();
   p->pList = pList;
   return (DWORD)p;
}
//---------------------------------------------------------------------------
DWORD AddGameShark(const char *lpCode,const char *lpDescr)
{
   LPGAMESHARK p;
   u32 adr,value;
   LCheatList *pList;
   u16 code;

   if(pReadCheatList == NULL || pWriteCheatList == NULL || !CheckCheatCode(&adr,&value,lpCode,TRUE))
       return 0;
   p = new GAMESHARK[1];
   if(p == NULL)
       return 0;
   ZeroMemory(p,sizeof(GAMESHARK));
   lstrcpy(p->codeString,lpCode);
   if(lpDescr != NULL)
       lstrcpyn(p->descr,lpDescr,MAX_CHEATNAME+1);
   p->type = GameShark;
   decrypt_code(&adr,&value);
   p->Enable = TRUE;
   p->value = value;
   p->adrEnd = (u32)-1;
   p->adr = (((adr & 0xf00000) << 4)) | (adr & 0x3ffff);
   code = (u16)((adr >> 0x19) & 3);
   code |= (u16)(((adr >> 0x1B) & 7) << 2);
   code |= (u16)(((adr >> 0x1E) & 3) << 5);
   code |= (u16)(((adr >> 0x18) & 1) << 7);
   p->code = code;
   p->pEvaluateFunc = EvaluateGameShark;
   if(value == 0x001DC0DE){
       p->code = 0xDE;
       p->value = adr;
       pList = pStaticCheatList;                    
       goto AddGameShark_1;
   }
   else if(adr == 0xDEADFACE){
       deadface((u16)value);
       return 0;
   }
   switch(p->code){
       case 0:
       case 1:
       case 2:
           pList = pWriteCheatList;
       break;
       case 4:
       case 5:
       case 6:
           p->pNext = (LPCHEATHEADER)-1;
           pList = pWriteCheatList;
       break;
       case 0x62:
           pList = pStaticCheatList;
       break;
       default:
           pList = pReadCheatList;
       break;
   }
AddGameShark_1:
   if(pList == NULL || !pList->Add((LPCHEATHEADER)p)){
       delete p;
       return 0;
   }
   pLVCheatList->Add((LPCHEATHEADER)p);
   p->pList = pList;
   pList->Sort(1,pList->Count(),sortCheatList);
   ResetCheatSystem();
   return (DWORD)p;
}
//---------------------------------------------------------------------------
LCheatList::LCheatList(u32 i,BOOL b) : LList()
{
   maxAdr = 0;
   minAdr = (u32)-1;
   id = i;
   isClone = b;
}
//---------------------------------------------------------------------------
LCheatList::~LCheatList()
{
   Clear();
}
//---------------------------------------------------------------------------
void LCheatList::DeleteElem(LPVOID ele)
{
   if(isClone)
       return;
   if(ele != NULL)
       delete ele;
}
//---------------------------------------------------------------------------
void LCheatList::Clear()
{
   LList::Clear();
   maxAdr = 0;
   minAdr = (u32)-1;
}
//---------------------------------------------------------------------------
BOOL LCheatList::EnumCheat(LPENUMCHEAT pEnumCheatProc,LPARAM lParam)
{
   elem_list *tmp;
   if(pEnumCheatProc == NULL)
       return FALSE;
   if(nCount == 0)
       return TRUE;
   tmp = First;
   while(tmp != NULL){
       if(!pEnumCheatProc((LPCHEATHEADER)tmp->Ele,lParam))
           return FALSE;
       tmp = tmp->Next;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LCheatList::Load(LStream *pFile)
{
   DWORD dw,dwSize,dwType;
   char buffer[100];

   if(pFile == NULL || !pFile->IsOpen())
       return FALSE;
   if(pFile->Read(&dw,sizeof(DWORD)) != sizeof(DWORD) || dw == 0)
       return FALSE;
   Clear();
   for(;dw > 0;dw--){
       pFile->Read(&dwType,sizeof(DWORD));
       pFile->Read(&dwSize,sizeof(DWORD));
       pFile->Read(buffer,LOWORD(dwSize) + HIWORD(dwSize));
       if(dwType == GameShark)
           AddGameShark(&buffer[LOWORD(dwSize)],buffer);
       else
           AddCodeBreak(&buffer[LOWORD(dwSize)],buffer);
   }
   if(dw == 0)
       return TRUE;
   return FALSE;
}
//---------------------------------------------------------------------------
BOOL LCheatList::Save(LStream *pFile)
{
   elem_list *tmp;
   LPCHEATHEADER cheatHeader;
   DWORD dw;

   if(pFile == NULL || !pFile->IsOpen())
       return FALSE;
   if(nCount < 1)
       return TRUE;
   pFile->Write(&id,sizeof(DWORD));
   pFile->Write(&nCount,sizeof(DWORD));
   tmp = First;
   while(tmp != NULL){
       cheatHeader = (LPCHEATHEADER)tmp->Ele;
       dw = cheatHeader->type;
       pFile->Write(&dw,sizeof(DWORD));
       dw = MAKELONG(MAX_CHEATNAME + 1,20);
       pFile->Write(&dw,sizeof(DWORD));
       pFile->Write(cheatHeader,MAX_CHEATNAME + 21);
       tmp = tmp->Next;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LCheatList::Add(LPCHEATHEADER p)
{
   if(!LList::Add((LPVOID)p))
       return FALSE;
   if(p->adrEnd == (u32)-1){
       if(p->adr > maxAdr)
           maxAdr = p->adr + 4;
       if(p->adr < minAdr)
           minAdr = p->adr;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
void __fastcall LCheatList::EvaluateCheat(u32 address,u8 accessMode)
{
   elem_list *tmp;
   LPCHEATHEADER cheatHeader;

   if(address > maxAdr || address < minAdr)
       return;
   tmp = First;
   do{
       cheatHeader = (LPCHEATHEADER)tmp->Ele;
       if(!cheatHeader->Enable || cheatHeader->skip != 0)
           goto EvaluateCheat_2;
       if(address < cheatHeader->adr && (cheatHeader->adrEnd == 0xFFFFFFFF || address < cheatHeader->adrEnd))
           return;
       if(cheatHeader->adrEnd == 0xFFFFFFFF){
           if(address == cheatHeader->adr){
               if(cheatHeader->pEvaluateFunc != NULL)
                   cheatHeader->pEvaluateFunc(cheatHeader,address,accessMode);
           }
       }
       else if(address < cheatHeader->adrEnd)
           return;
EvaluateCheat_2:
       tmp = tmp->Next;
   }while(tmp != NULL);
}

