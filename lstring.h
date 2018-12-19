#include <windows.h>
#include <stdio.h>
//---------------------------------------------------------------------------
#ifndef lstringH
#define lstringH
//---------------------------------------------------------------------------

class LString
{
public:
   LString();
   LString(const int len);
   LString(const char *pString);
   LString(const LString& string);
   ~LString();
   const LString &LString::AddEXT(char *ext);
   const BOOL IsEmpty(){if(pData == NULL || lstrlen(pData) == 0) return TRUE; else return FALSE;};
   const BOOL LoadString(UINT uID,HINSTANCE hInst = NULL);
   const LString& AllTrim();
   const LString& RemoveAllChar(const BYTE c);
   const LString& Capacity(const int len);
   const LString &operator =(const char *pString);
   const LString &operator =(const int value);
   const LString &operator +(const LString& string);
   const LString &operator +(const char *pString);
   const LString &operator +(const char car);
   const LString &operator +=(const char car);
   const LString &operator +=(const LString& string);
   const LString &operator =(const LString& string);
   const LString &operator +=(const char *pString);
   const LString &operator +=(const int value);
   LString Path();
   LString FileName();   
   BOOL operator ==(const char *pString);
   BOOL operator <(const char *pString);
   BOOL operator >(const char *pString);
   char operator[](const int index);
   char *c_str(){return pData;};
   int Length() const;
   int Length(int len);
   const LString &UpperCase();
   const LString &LowerCase();
   int Pos(char *Search);
   int Pos(const char Search);
   LString SubString(int index,int count) const;
   int ToInt();
   LString NextToken(BYTE c);
   LString RemainderToken();
   void ResetToken();
   LString LString::Ext();
   BOOL BuildFileName(const LPSTR fileName,const LPSTR fileExt = NULL,int ID = 0);   
protected:
   char *pData,*pToken;
private:
   void AllocBuffer(int len);
   void CopyString(char *pSrc,int len);
   void CatString(char *pSrc,int len);
};
#endif


