//---------------------------------------------------------------------------
#include <windows.h>
#pragma hdrstop

#include "lstring.h"

#define CHECK_POINTER(p) if(p == NULL) return;
#define CHECK_POINTERR(p) if(p == NULL) return *this;

//---------------------------------------------------------------------------
LString::LString()
{
   pToken = pData = NULL;
}
//---------------------------------------------------------------------------
LString::LString(const LString &string)
{
   pData = NULL;
   CopyString((char *)string.pData,lstrlen(string.pData));
}
//---------------------------------------------------------------------------
LString::LString(const int len)
{
   pData = NULL;
   AllocBuffer(len);
}
//---------------------------------------------------------------------------
LString::~LString()
{
   if(pData != NULL)
       ::GlobalFree((HGLOBAL)pData);
   pData = NULL;
}
//---------------------------------------------------------------------------
LString::LString(const char *pString)
{
   pData = NULL;
   CHECK_POINTER(pString);
   CopyString((char *)pString,lstrlen(pString));
}
//---------------------------------------------------------------------------
void LString::ResetToken()
{
   pToken = pData;
}
//---------------------------------------------------------------------------
LString LString::RemainderToken()
{
   LString s;
   int i;

   s = "";
   if(pToken != NULL && (i = lstrlen(pToken)) > 0){
       s.Capacity(i+1);
       s = pToken;
       pToken = NULL;
   }
   return s;
}
//---------------------------------------------------------------------------
const LString &LString::AddEXT(char *ext)
{
   int i;

   if((i = Pos(".")) > 0)
       Length(i-1);
   CatString(ext,lstrlen(ext));
   return *this;
}
//---------------------------------------------------------------------------
LString LString::Path()
{
   LString c;
   char *p;

   c = "";
   p = strrchr(pData,'\\');
   if(p != NULL)
       c = SubString(1,p-pData);
   return c;
}
//---------------------------------------------------------------------------
LString LString::Ext()
{
   LString c;
   char *p;

   if((p = strrchr(pData,'.')) != NULL)
       c = p;
   return c;
}
//---------------------------------------------------------------------------
LString LString::FileName()
{
   LString c;
   char *p,*p1;

   c = pData;
   if((p = strrchr(pData,'\\')) == NULL)
       p = strrchr(pData,'/');
   if(p != NULL)
       c = ++p;
   else
       p = pData;
   if((p1 = strrchr(p,'.')) != NULL)
       c.Length((p1 - p));
   return c;
}
//---------------------------------------------------------------------------
BOOL LString::BuildFileName(const LPSTR fileName,const LPSTR fileExt,int ID)
{
   LString s,nameFile;

   if(fileName == NULL)
       return FALSE;
   s = fileName;
   if(s.Length() == 0)
       return FALSE;
   nameFile = s.Path() + "\\";
   nameFile += s.FileName().AllTrim();
   if(ID != -0)
       nameFile += (int)ID;
   if(fileExt != NULL){
       nameFile += ".";
       nameFile += fileExt;
   }
   CopyString(nameFile.c_str(),nameFile.Length());
   return (BOOL)(nameFile.Length() > 0 ? TRUE : FALSE);
}
//---------------------------------------------------------------------------
LString LString::NextToken(BYTE c)
{
   char *p;
   LString s;
   int i;

   s = "";
   if(pToken == NULL)
       return s;
   p = strchr(pToken,c);
   if(p != NULL){
       i = p - pToken;
       s.Capacity(i+1);
       lstrcpyn(s.c_str(),pToken,i+1);
       pToken = p + 1;
   }
   else if(pToken != pData){
       i = lstrlen(pToken);
       s.Capacity(i+1);
       lstrcpyn(s.c_str(),pToken,i+1);
       pToken = NULL;
   }
   return s;
}
//---------------------------------------------------------------------------
const BOOL LString::LoadString(UINT uID,HINSTANCE hInst)
{
   int i;
   BOOL res;

   res = FALSE;
   if(hInst == NULL)
       hInst = GetModuleHandle(NULL);
   AllocBuffer(100);
   if(pData == NULL)
       return res;
   i = ::LoadString(hInst,uID,pData,100);
   if(i < 100 && i > 0)
       Length(i);
   if(i > 0)
       res = TRUE;
   return res;
}
//---------------------------------------------------------------------------
LString LString::SubString(int index,int count) const
{
   LString c;
   c.CopyString(&pData[index-1],count);
   return c;
}
//---------------------------------------------------------------------------
const LString& LString::RemoveAllChar(const BYTE c)
{
   LString s;
   int i,iLen;

   s.Capacity((iLen = Length()));
   for(i=0;i<iLen;i++){
       if(pData[i] != c)
           s += pData[i];
   }
   CopyString(s.c_str(),s.Length());
   return *this;
}
//---------------------------------------------------------------------------
const LString& LString::AllTrim()
{
   return RemoveAllChar(32);
}
//---------------------------------------------------------------------------
const LString& LString::operator =(const LString& string)
{
   CopyString(string.pData,string.Length());
   return *this;
}
//---------------------------------------------------------------------------
const LString &LString::operator=(const char *pString)
{
   CHECK_POINTERR(pString);
   CopyString((char *)pString,lstrlen(pString));
   pToken = pData;
   return *this;
}
//---------------------------------------------------------------------------
int LString::Length() const
{
   if(pData != NULL)
       return lstrlen(pData);
   else
       return 0;
}
//---------------------------------------------------------------------------
int LString::Length(int len)
{
   LString s;

   if(pData == NULL)
       return 0;
   s = pData;
   CopyString(s.c_str(),len);
   return lstrlen(pData);
}
//---------------------------------------------------------------------------
int LString::ToInt()
{
   if(pData != NULL)
       return atoi(pData);
   else return 0;
}
//---------------------------------------------------------------------------
const LString &LString::operator +(const LString& string)
{
   CatString(string.pData,string.Length());
   return *this;
}
//---------------------------------------------------------------------------
const LString& LString::operator +=(const LString& string)
{
   CatString(string.pData,string.Length());
   return *this;
}
//---------------------------------------------------------------------------
BOOL LString::operator <(const char *pString)
{
   if(pData == NULL || pString == NULL)
       return FALSE;
   if(lstrcmp(pData,pString) < 0)
       return TRUE;
   else
       return FALSE;
}
//---------------------------------------------------------------------------
BOOL LString::operator >(const char *pString)
{
   if(pData == NULL || pString == NULL)
       return FALSE;
   if(lstrcmp(pData,pString) > 0)
       return TRUE;
   else
       return FALSE;
}
//---------------------------------------------------------------------------
BOOL LString::operator ==(const char *pString)
{
   if(pData == NULL || pString == NULL)
       return 0;
   if(lstrcmp(pData,pString) == 0)
       return TRUE;
   else
       return FALSE;
}
//---------------------------------------------------------------------------
const LString &LString::operator +(const char car)
{
   pData[Length()] = car;
   return *this;
}
//---------------------------------------------------------------------------
const LString &LString::operator +=(const int value)
{
   char s[30];

   wsprintf(s,"%d",value);
   CatString(s,lstrlen(s));
   return *this;
}
//---------------------------------------------------------------------------
const LString &LString::operator +=(const char car)
{
   char s[4];

   *((long *)s) = 0;
   s[0] = car;
   CatString(s,1);
   return *this;
}
//---------------------------------------------------------------------------
const LString &LString::operator +(const char *pString)
{
   CHECK_POINTERR(pString);
   CatString((char *)pString,lstrlen(pString));
   return *this;
}
//---------------------------------------------------------------------------
char LString::operator[](const int index)
{
   if(pData == NULL || index > Length())
       return 0;
   return pData[index-1];
}
//---------------------------------------------------------------------------
const LString &LString::UpperCase()
{
   CHECK_POINTERR(pData);
   CharUpperBuff(pData,Length());
   return *this;
}
//---------------------------------------------------------------------------
const LString &LString::LowerCase()
{
   CHECK_POINTERR(pData);
   CharLowerBuff(pData,Length());
   return *this;
}
//---------------------------------------------------------------------------
int LString::Pos(const char Search)
{
   char c[2] = {0,0};

   c[0] = Search;
   return Pos(c);
}
//---------------------------------------------------------------------------
int LString::Pos(char *Search)
{
   int len,i,length,i1;
   char *p,*p1,*p2;

   length = lstrlen(pData);
   if(pData == NULL || length == 0 || Search == NULL)
       return 0;
   len = lstrlen(Search);
   if(len > length)
       return 0;
   p = pData;
   for(i = 0;i<length;i++,p++){
       if(*p == *Search){
           p1 = p + 1;
           p2 = Search + 1;
           for(i1=1;i1<len;i1++){
               if(*p1++ != *p2++)
                   break;
           }
           if(i1 == len)
               break;
       }
   }
   if(i == length)
       return -1;
   else
       return i+1;
}
//---------------------------------------------------------------------------
const LString& LString::operator +=(const char *pString)
{
   CatString((char *)pString,lstrlen(pString));
   return *this;
}
//---------------------------------------------------------------------------
const LString& LString::operator =(const int value)
{
   AllocBuffer(20);
   wsprintf(pData,"%d",value);
   return *this;
}
//---------------------------------------------------------------------------
const LString& LString::Capacity(const int len)
{
	LString s;

	if(pData != NULL)
   	s = pData;
   AllocBuffer(len);
   if(s.Length() > 0)
   	lstrcpyn(pData,s.c_str(),len);
   return *this;
}
//---------------------------------------------------------------------------
void LString::AllocBuffer(int len)
{
   if(pData)
       ::GlobalFree((HGLOBAL)pData);
   if((pData = (char *)GlobalAlloc(GPTR,len+1)) == NULL)
       return;
   pToken = pData;
}
//---------------------------------------------------------------------------
void LString::CopyString(char *pSrc,int len)
{
   AllocBuffer(len+1);
   if(pData)
       lstrcpyn(pData,pSrc,len+1);
}
//---------------------------------------------------------------------------
void LString::CatString(char *pSrc,int len)
{
   LString l(pData);
   AllocBuffer(len+l.Length());
   if(l.pData)
       lstrcpy(pData,l.pData);
   lstrcat(pData,pSrc);
}

