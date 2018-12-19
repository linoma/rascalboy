#include <windows.h>
#include "lstring.h"
//---------------------------------------------------------------------------
#ifndef lregkeyH
#define lregkeyH
//---------------------------------------------------------------------------
class LRegKey
{
public:
   LRegKey();
   ~LRegKey();
   BOOL Open(char *key,const HKEY keyRoot = HKEY_CURRENT_USER,BOOL bOpenAlways = TRUE);
   BOOL Close();
   BOOL Write(char *key,LPBYTE lpData,DWORD cbData,DWORD type = REG_SZ);
   BOOL Read(char *key,LPBYTE lpData,LPDWORD lpType,LPDWORD lpLen);
   DWORD ReadLong(char *key,DWORD def = 0);
   LString ReadString(char *key,char *def);
   BOOL WriteLong(char *key,DWORD value){ return Write(key,(LPBYTE)&value,sizeof(DWORD),REG_DWORD);};
   BOOL WriteString(char *key,char *value){return Write(key,(LPBYTE)value,lstrlen(value),REG_SZ);};
   BOOL WriteBinaryData(char *key,char *value,DWORD len){return Write(key,(LPBYTE)value,len,REG_BINARY);};
   DWORD ReadBinaryData(char *key,char *buffer,DWORD len);
   BOOL Delete(const char *key,const HKEY keyRoot = HKEY_CURRENT_USER);
   inline const HKEY Handle(){return hKey;}; 
protected:
   HKEY hKey;
};

#endif
