//---------------------------------------------------------------------------
#include <windows.h>
#pragma hdrstop

#include "lregkey.h"

//---------------------------------------------------------------------------
LRegKey::LRegKey()
{
   hKey = NULL;
}
//---------------------------------------------------------------------------
LRegKey::~LRegKey()
{
   Close();
}
//---------------------------------------------------------------------------
BOOL LRegKey::Open(char *key,const HKEY keyRoot,BOOL bOpenAlways)
{
   DWORD KeyWord;

   if(!Close())
       return FALSE;
   if(bOpenAlways){
       if(RegCreateKeyEx(keyRoot,key,0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKey,&KeyWord) != ERROR_SUCCESS)
           return FALSE;
       else
           return TRUE;
   }
   if(RegOpenKeyEx(keyRoot,key,0,KEY_ALL_ACCESS,&hKey) != ERROR_SUCCESS)
       return FALSE;
   else
       return TRUE;
}
//---------------------------------------------------------------------------
BOOL LRegKey::Close()
{
   LONG res;

   res = ERROR_SUCCESS;
   if(hKey != NULL)
       res = ::RegCloseKey(hKey);
   if(res != ERROR_SUCCESS)
       return FALSE;
   else{
       hKey = NULL;
       return TRUE;
   }
}
//---------------------------------------------------------------------------
BOOL LRegKey::Write(char *key,LPBYTE lpData,DWORD cbData,DWORD type)
{
   LONG res;

   if(hKey == NULL)
       return FALSE;
   res = ::RegSetValueEx(hKey,key,0,type,lpData,cbData);
   if(res != ERROR_SUCCESS)
       return FALSE;
   else
       return TRUE;
}
//---------------------------------------------------------------------------
BOOL LRegKey::Delete(const char *key,const HKEY keyRoot)
{
   LONG res;

   res = ::RegDeleteKey(keyRoot,key);
   if(res != ERROR_SUCCESS)
       return FALSE;
   else
       return TRUE;
}
//---------------------------------------------------------------------------
BOOL LRegKey::Read(char *key,LPBYTE lpData,LPDWORD lpType,LPDWORD lpLen)
{
   LONG res;

   if(hKey == NULL)
       return FALSE;
   res = ::RegQueryValueEx(hKey,key,0,lpType,lpData,lpLen);
   if(res != ERROR_SUCCESS)
       return FALSE;
   else
       return TRUE;
}
//---------------------------------------------------------------------------
DWORD LRegKey::ReadBinaryData(char *key,char *buffer,DWORD len)
{
   DWORD dwType,dwLen;
   DWORD res;

   res = (DWORD)-1;
   if(Read(key,NULL,&dwType,&dwLen)){
       if(dwType == REG_BINARY && dwLen <= len){
           Read(key,(LPBYTE)buffer,&dwType,&dwLen);
           res = dwLen;
       }
   }
   return res;
}
//---------------------------------------------------------------------------
LString LRegKey::ReadString(char *key,char *def)
{
   DWORD dwType,dwLen;
   LString res;

   res = def;
   if(Read(key,NULL,&dwType,&dwLen)){
       if(dwType == REG_SZ){
           res.Length(dwLen + 1);
           Read(key,(LPBYTE)res.c_str(),&dwType,&dwLen);
       }
   }
   return res;
}
//---------------------------------------------------------------------------
DWORD LRegKey::ReadLong(char *key,DWORD def)
{
   DWORD res,dwType,dwLen;

   res = def;
   if(Read(key,NULL,&dwType,&dwLen)){
       if(dwLen == 4 && dwType == REG_DWORD)
           Read(key,(LPBYTE)&res,&dwType,&dwLen);
   }
   return res;
}
