#include "windows.h"
#include "lstring.h"

//---------------------------------------------------------------------------
#ifndef __listH__
#define __listH__
//---------------------------------------------------------------------------
#define LE_ALLOCMEM 1
//---------------------------------------------------------------------------
typedef int (*LPLISTSORTFNC)(LPVOID ele1,LPVOID ele2);

class LList
{
protected:
   struct elem_list
   {
       LPVOID Ele;
       elem_list *Next,*Last;
   public:
       elem_list(LPVOID ele = NULL,LPVOID next=NULL,LPVOID last=NULL);
   };
   virtual void DeleteElem(LPVOID ele);
   elem_list *CercaItem(DWORD item);
   elem_list *First,*Last;
   DWORD nCount,iErrore;
public:
   LList();
   ~LList();
   DWORD Count(){return nCount;};
   virtual void Clear();
   virtual BOOL Add(LPVOID ele);
   DWORD GetLastError(){return iErrore;};
   void SetLastError(DWORD err){iErrore = err;};
   virtual LPVOID GetItem(DWORD item);
   virtual BOOL Delete(DWORD item);
   elem_list *Remove(DWORD item);
   BOOL SetItem(DWORD item,LPVOID ele);
   virtual LPVOID GetFirstItem(LPDWORD pos);
   virtual LPVOID GetNextItem(LPDWORD pos);
   virtual LPVOID GetLastItem(LPDWORD pos);
   virtual LPVOID GetPrevItem(LPDWORD pos);
   DWORD IndexFromEle(LPVOID ele);
   void Sort(DWORD start,DWORD end,LPLISTSORTFNC sortFunction);
};
//---------------------------------------------------------------------------
class LStringList : public LList
{
public:
   LStringList();
   ~LStringList();
   virtual BOOL Add(char *string);
   virtual BOOL Add(LString string);
   DWORD FindString(char *pString);
   BOOL DeleteString(char *pString);
protected:
   void DeleteElem(LPVOID ele);
};
//---------------------------------------------------------------------------
class LRecentFile : public LStringList
{
public:
   LRecentFile(int max=4);
   int Read(char *pKey);
   int Save(char *pKey);
   BOOL Add(char *string);
protected:
   int maxElem;
};
#endif
