#include <windows.h>
#include "list.h"
//---------------------------------------------------------------------------
#ifndef fstreamH
#define fstreamH
//---------------------------------------------------------------------------
#define NULL_STREAM INVALID_HANDLE_VALUE

struct LStream
{
public:
   virtual BOOL Open(DWORD dwStyle=GENERIC_READ,DWORD dwCreation=OPEN_EXISTING) PURE;
   virtual void Close() PURE;
   virtual DWORD Read(LPVOID lpBuffer,DWORD dwBytes) PURE;
   virtual DWORD Write(LPVOID lpBuffer,DWORD dwBytes) PURE;
   virtual DWORD Seek(LONG dwDistanceToMove,DWORD dwMoveMethod) PURE;
   virtual BOOL SeekToBegin() PURE;
   virtual BOOL SeekToEnd() PURE;
   virtual DWORD Size(LPDWORD lpHigh = NULL) PURE;
   virtual BOOL SetEndOfFile(DWORD dw) PURE;
   virtual DWORD GetCurrentPosition() PURE;
   virtual BOOL IsOpen() PURE;
   virtual void Release() PURE;
};
//---------------------------------------------------------------------------
class LFile : public LStream
{
public:
   LFile(const char *name);
   ~LFile();
   void Release(){delete this;};
   BOOL Open(DWORD dwStyle=GENERIC_READ,DWORD dwCreation=OPEN_EXISTING);
   void Close();
   DWORD Read(LPVOID lpBuffer,DWORD dwBytes);
   DWORD Write(LPVOID lpBuffer,DWORD dwBytes);
   DWORD Seek(LONG dwDistanceToMove = 0,DWORD dwMoveMethod = FILE_BEGIN);
   inline BOOL SeekToBegin(){if(Seek() != 0xFFFFFFFF) return TRUE; return FALSE;};
   inline BOOL SeekToEnd(){if(Seek(0,FILE_END) != 0xFFFFFFFF) return TRUE; return FALSE;};
   DWORD Size(LPDWORD lpHigh = NULL);
   BOOL SetEndOfFile(DWORD dw);
   inline DWORD GetCurrentPosition(){return Seek(0,FILE_CURRENT);};
   inline HANDLE Handle(){return handle;};
   inline BOOL IsOpen(){return (BOOL)(handle != NULL_STREAM ? TRUE : FALSE);};
   DWORD WriteF(char *mes,...);
protected:
   char fileName[MAX_PATH];
   HANDLE handle;
};
//---------------------------------------------------------------------------
class LMemoryFile : public LStream, LList
{
public:
   LMemoryFile(DWORD dw = 8192);
   ~LMemoryFile();
   void Release(){delete this;};
   BOOL Open(DWORD dwStyle=0,DWORD dwCreation=0);
   void Close();
   DWORD Read(LPVOID lpBuffer,DWORD dwBytes);
   DWORD Write(LPVOID lpBuffer,DWORD dwBytes);
   DWORD Seek(LONG dwDistanceToMove = 0,DWORD dwMoveMethod = FILE_BEGIN);
   inline BOOL SeekToBegin(){if(Seek() != 0xFFFFFFFF) return TRUE; return FALSE;};
   inline BOOL SeekToEnd(){if(Seek(0,FILE_END) != 0xFFFFFFFF) return TRUE; return FALSE;};
   DWORD Size(LPDWORD lpHigh = NULL);
   BOOL SetEndOfFile(DWORD dw);
   inline DWORD GetCurrentPosition(){return Seek(0,FILE_CURRENT);};
   inline BOOL IsOpen(){return (BOOL)(nCount > 0 ? TRUE : FALSE);};
//   BOOL SaveToFile(const char *lpFileName);
protected:
//---------------------------------------------------------------------------
   struct buffer
   {
       LPBYTE buf;
       DWORD dwSize,dwPos,dwBytesWrite;
   public:
       buffer(DWORD dw = 8192);
   };
//---------------------------------------------------------------------------
   void DeleteElem(LPVOID ele);
   buffer *AddBuffer();
//---------------------------------------------------------------------------
   DWORD dwPos,dwSize,dwIndex,dwBufferSize;
   buffer *currentBuffer;
};

#endif
