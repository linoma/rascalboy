//---------------------------------------------------------------------------
#include <windows.h>
#pragma hdrstop

#include "list.h"
#include "lregkey.h"
//---------------------------------------------------------------------------
LList::LList()
{
   nCount = 0;
   First = Last = NULL;
   iErrore = 0;
}
//---------------------------------------------------------------------------
LList::~LList()
{
   Clear();
}
//---------------------------------------------------------------------------
LList::elem_list::elem_list(LPVOID ele,LPVOID next,LPVOID last)
{
   Next = (elem_list *)next;
   Last = (elem_list *)last;
   Ele = ele;
}
//---------------------------------------------------------------------------
LPVOID LList::GetItem(DWORD item)
{
   elem_list *tmp;

   tmp = CercaItem(item);
   if(tmp)
       return tmp->Ele;
   else
       return NULL;
}
//---------------------------------------------------------------------------
LList::elem_list * LList::CercaItem(DWORD item)
{
   elem_list *tmp;
   DWORD i;

   if(nCount == 0 || First == NULL || Last == NULL || item > nCount || item < 1)
       return NULL;
   tmp = First;
   for(i=1;tmp != NULL && i < item;i++)
       tmp = tmp->Next;
   return tmp;
}
//---------------------------------------------------------------------------
void LList::DeleteElem(LPVOID ele)
{
   if(ele != NULL)
       delete ele;
}
//---------------------------------------------------------------------------
void LList::Clear()
{
   elem_list *tmp,*tmp1;

   tmp = First;
   while(tmp){
       tmp1 = tmp->Next;
       DeleteElem(tmp->Ele);
       delete tmp;
       tmp = tmp1;                                 
   }
   nCount = 0;
   First = Last = NULL;
}
//---------------------------------------------------------------------------
DWORD LList::IndexFromEle(LPVOID ele)
{
   DWORD index,i;
   elem_list *tmp;

   index = (DWORD)-1;
   tmp = First;
   i = 1;
   while(tmp != NULL){
       if(tmp->Ele == ele){
           index = i;
           break;
       }
       tmp = tmp->Next;
       i++;
   }
   return index;
}
//---------------------------------------------------------------------------
BOOL LList::SetItem(DWORD item,LPVOID ele)
{
   elem_list *tmp;

   tmp = CercaItem(item);
   if(tmp ==  NULL)
       return FALSE;
   tmp->Ele = ele;
   return TRUE;
}
//---------------------------------------------------------------------------
LList::elem_list *LList::Remove(DWORD item)
{
   elem_list *tmp;

   tmp = CercaItem(item);
   if(tmp == NULL)
       return NULL;
   if(tmp->Last)
       tmp->Last->Next = tmp->Next;
   if(tmp->Next)
       tmp->Next->Last = tmp->Last;
   if(tmp == First)
       First = tmp->Next;
   if(tmp == Last)
       Last = tmp->Last;
   nCount--;
   return tmp;
}
//---------------------------------------------------------------------------
BOOL LList::Delete(DWORD item)
{
   elem_list *tmp;

   tmp = Remove(item);
   if(tmp == NULL)
       return FALSE;
   DeleteElem(tmp->Ele);
   delete tmp;   
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LList::Add(LPVOID ele)               
{
   elem_list *tmp;

   tmp = new elem_list(ele,NULL,Last);
   if(tmp == NULL){
       iErrore = LE_ALLOCMEM;
       return FALSE;
   }
   if(Last)
       Last->Next = tmp;
   Last = tmp;
   if(First == NULL)
       First = tmp;
   nCount++;
   return TRUE;
}
//---------------------------------------------------------------------------
LPVOID LList::GetLastItem(LPDWORD pos)
{
   elem_list *tmp;

   tmp = Last;
   if(tmp){
       *pos = (DWORD)tmp;
       return tmp->Ele;
   }
   else{
       *pos = 0;
       return NULL;
   }
}
//---------------------------------------------------------------------------
LPVOID LList::GetPrevItem(LPDWORD pos)
{
   LPVOID res;

   if(pos == NULL || *pos == 0)
       return NULL;
   *pos = (DWORD)((elem_list *)*pos)->Last;
   if(*pos)
       res = ((elem_list *)*pos)->Ele;
   else
       res = NULL;
   return res;
}
//---------------------------------------------------------------------------
LPVOID LList::GetFirstItem(LPDWORD pos)
{
   elem_list *tmp;

   tmp = First;
   if(tmp){
       *pos = (DWORD)tmp;
       return tmp->Ele;
   }
   else{
       *pos = 0;
       return NULL;
   }
}
//---------------------------------------------------------------------------
LPVOID LList::GetNextItem(LPDWORD pos)
{
   LPVOID res;

   if(pos == NULL || *pos == 0)
       return NULL;
   *pos = (DWORD)((elem_list *)*pos)->Next;
   if(*pos)
       res = ((elem_list *)*pos)->Ele;
   else
       res = NULL;
   return res;
}
//---------------------------------------------------------------------------
void LList::Sort(DWORD start,DWORD end,LPLISTSORTFNC sortFunction)
{
   LPVOID p,p1;
   DWORD i,i1,i2;
   BOOL flag;

   i1 = nCount;
   flag = TRUE;
   while(flag){
       flag = FALSE;
       for(i=1;i < i1;i++){
           p = CercaItem(i)->Ele;
           p1 = CercaItem(i+1)->Ele;
           if(sortFunction(p,p1) <= 0)
               continue;
           SetItem(i,p1);
           SetItem(i+1,p);
           flag = TRUE;
           i2 = i;
       }
       i1 = i2;
   }
}
//---------------------------------------------------------------------------
LStringList::LStringList() : LList()
{
}
//---------------------------------------------------------------------------
LStringList::~LStringList()
{
   Clear();
}
//---------------------------------------------------------------------------
BOOL LStringList::Add(char *string)
{
   LString *s;

   s = new LString(string);
   return LList::Add((LPVOID)s);
}
//---------------------------------------------------------------------------
BOOL LStringList::DeleteString(char *pString)
{
   DWORD dw;

   dw = FindString(pString);
   if(dw == (DWORD)-1)
       return FALSE;
   if(LList::Delete(dw))
       return TRUE;
   else
       return FALSE;
}
//---------------------------------------------------------------------------
DWORD LStringList::FindString(char *pString)
{
   DWORD res,i;
   LString *s;

   res = (DWORD)-1;
   if(pString == NULL || nCount == 0)
       return res;
   for(i=1;i<=nCount;i++){
       s = (LString *)LList::GetItem(i);
       if(lstrcmp(s->c_str(),pString) == 0){
           res = i;
           break;
       }
   }
   return res;
}
//---------------------------------------------------------------------------
BOOL LStringList::Add(LString string)
{
   LString *s;

   if((s = new LString(string)) == NULL)
   	return FALSE;
   return LList::Add((LPVOID)s);
}
//---------------------------------------------------------------------------
void LStringList::DeleteElem(LPVOID ele)
{
   if(ele != NULL)
       delete (LString *)ele;
}
//---------------------------------------------------------------------------
LRecentFile::LRecentFile(int max) : LStringList()
{
   maxElem = max;
}
//---------------------------------------------------------------------------
int LRecentFile::Read(char *pKey)
{
   DWORD dwType,dwLen;
   int i;
   LString s,FileName;
   LRegKey regKey;

   if(!regKey.Open(pKey))
       return -1;
   Clear();
   i = 0;
   do{
       s = "File";
       s += i+1;
       if(!regKey.Read(s.c_str(),NULL,&dwType,&dwLen))
           break;
       if((int)dwLen > 1){
           FileName.Capacity(dwLen + 1);
           regKey.Read(s.c_str(),(LPBYTE)FileName.c_str(),&dwType,&dwLen);
           Add(FileName.c_str());
       }
       i++;
   }while(1 == 1);
   regKey.Close();
   return i;
}
//---------------------------------------------------------------------------
int LRecentFile::Save(char *pKey)
{
   LString s,FileName;
   int i;
   LRegKey regKey;

   if(!regKey.Open(pKey))
       return -1;
   for(i = 1;i <= (int)Count();i++){
       s = "File";
       s += (int)i;
       FileName = ((LString *)GetItem(i))->c_str();
       regKey.Write(s.c_str(),(LPBYTE)FileName.c_str(),FileName.Length());
   }
   for(;i<=maxElem;i++){
       s = "File";
       s += (int)i;
       regKey.Write(s.c_str(),(LPBYTE)"",0);
   }
   regKey.Close();
   return (int)Count();
}
//---------------------------------------------------------------------------
BOOL LRecentFile::Add(char *pNome)
{
   if(pNome == NULL)
       return FALSE;
   if(FindString(pNome) != (DWORD)-1)
       return FALSE;
   if((int)Count() == maxElem)
       Delete(1);
   return LStringList::Add(pNome);
}

