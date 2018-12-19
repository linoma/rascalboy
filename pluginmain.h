#include <windows.h>

/***********************************************************************************************************
****    Questo e' il file include per i vari plug-in audio, video e
****    della porta di comunicazione per il RascalBoy Advance.
****
****    Si tenga presente che ogni tipo di plug-in ha la sua funzione RunFunc.
****    I vari plug-in in base al tipo saranno accodati al menu' Filtri Audio per il plug-in 
****    audio, menu' Filtri Video per quelli video e nel 
****    menu' Porta di comunicazione quelli della SIO.
****
****
****    Sono liberamente distribuibili senza nessuna riserva o altro vincolo.
****    Per eventuali problemi potete contattarmi 
****    a rascalboy@nzone.it oppure a linoma@inwind.it.
***********************************************************************************************************/

#ifndef __PLUGINMAINH__
#define __PLUGINMAINH__

#define SIZEOFPLUGINFO10       44
#define SIZEOFPLUGINFO11       48
#define SIZEOFPLUGINFO12       52

#define PIT_AUDIO              1
#define PIT_VIDEO              2
#define PIT_SIO                3
#define PIT_BACKUP				4

#define PIT_DYNAMIC            0x100
#define PIT_NOEXCLUSIVE        0x200
#define PIT_ASYNC              0x400
#define PIT_ASYNCEVENT         0x800
#define PIT_BITBLT             0x400
#define PIT_NOMODIFY           0x800
#define PIT_READ				0x400
#define PIT_WRITE				0x800
#define PIT_EEPROM				0x1000
#define PIT_SRAM				0x2000
#define PIT_VIDEOAUDIO         0x1000
#define PIT_ENABLERUN			0x4000
#define PIT_NOMENU				0x8000

#define PIS_ENABLE             0x1
#define PIS_RUN                0x2
#define PIS_PAUSE              0x4
#define PIS_FULLSCREEN         0x8
#define PIS_DDRAW              0x10
#define PIS_SIZEMOVE           0x20
#define PIS_RUNMASK            0x6
#define PIS_ENABLEMASK         0x1
#define PIS_VIDEOMODEMASK      0x38

#define PLUGINISENABLE(p)      ((p->dwState & PIS_ENABLE) && (p->dwStateMask & PIS_ENABLEMASK))
#define PLUGINISSTOP(p)        !(p->dwState & PIS_RUNMASK)
#define PLUGINISRUN(p)         ((p->dwState & PIS_RUN) && (p->dwStateMask & PIS_RUNMASK))
#define PLUGINISPAUSE(p)       ((p->dwState & PIS_PAUSE) && (p->dwStateMask & PIS_RUNMASK))
#define PLUGINISFULLSCREEN(p)  ((p->dwState & PIS_FULLSCREEN) && (p->dwStateMask & PIS_VIDEOMODEMASK))
#define PLUGINISDDRAW(p)       ((p->dwState & PIS_DDRAW) && (p->dwStateMask & PIS_VIDEOMODEMASK))
//---------------------------------------------------------------------------
typedef struct _pluginfo{
   DWORD   cbSize;
   WORD    wType;
   WORD    wIndex;
   WORD    wID;
   DWORD   dwLanguageID;
   LPWSTR  pszText;
   int     cchTextMax;
   GUID    guidID;
   DWORD   dwState;
   DWORD   dwStateMask;
   LPARAM  lParam;
   LPVOID  pServiceInterface;
} PLUGININFO,*LPPLUGININFO;
//---------------------------------------------------------------------------
// ProperyPage for plug-in
//---------------------------------------------------------------------------
typedef struct {
   HWND hwndOwner;
   PLUGININFO info;
} SETPROPPLUGIN,*LPSETPROPPLUGIN;
//---------------------------------------------------------------------------
typedef struct _video2{
   int x;
   int y;
   int cx;
   int cy;
   int mode;
   int frameSkip;
   LPVOID pCanvas;
} VIDEO2,*LPVIDEO2;
//---------------------------------------------------------------------------
typedef union _videoaudio{
   struct {
       unsigned short *InBuffer;
       unsigned short *OutBuffer;
   } video;
   struct {
       LPVOID mem1;
       DWORD dwSize1;
       LPVOID mem2;
       DWORD dwSize2;
   } audio;
} VIDEOAUDIO,*LPVIDEOAUDIO;
//---------------------------------------------------------------------------
typedef struct _serialIO
{
   WORD d0,d1,d2,d3;   //0x120,0x122,0x124,0x126
   WORD DATA;          // 0x12A
   union{
       struct{
           unsigned int other : 13;
           unsigned int mode  : 2;
       } regx;
       struct{
           unsigned int other : 14;
           unsigned int mode  : 1;
       } regn;
       struct{
           unsigned int scb       : 1;
           unsigned int sdb       : 1;
           unsigned int sib       : 1;
           unsigned int sob       : 1;
           unsigned int scd       : 1;
           unsigned int sdd       : 1;
           unsigned int sid       : 1;
           unsigned int sod       : 1;
           unsigned int irq       : 1;
           unsigned int none      : 5;
           unsigned int mode      : 2;
       } regg;
       WORD value;
   } REG_CNT; //0x134
   union{
       struct{
           unsigned int baud      : 2;
           unsigned int is        : 1;
           unsigned int so        : 1;
           unsigned int id        : 2;
           unsigned int err       : 1;
           unsigned int status    : 1;
           unsigned int none      : 4;
           unsigned int mode      : 2;// set 2
           unsigned int irq       : 1;
       } mp;
       struct{
           unsigned int sc        : 1;
           unsigned int isc       : 1;
           unsigned int si        : 1;
           unsigned int so        : 1;
           unsigned int none      : 3;
           unsigned int status    : 1;
           unsigned int none1     : 4;
           unsigned int length    : 1;
           unsigned int mode      : 1;// set 0
           unsigned int irq       : 1;
       } nm;
       struct{
           unsigned int baud      : 2;
           unsigned int cts       : 1;
           unsigned int parity    : 1;
           unsigned int sd        : 1;
           unsigned int rd        : 1;
           unsigned int err       : 1;
           unsigned int data      : 1;
           unsigned int fifo      : 1;
           unsigned int pf        : 1;
           unsigned int sdf       : 1;
           unsigned int rdf       : 1;
           unsigned int mode      : 2;// set 3
           unsigned int irq       : 1;
       } uart;
       WORD value;
   } CNT; // 0x128
   DWORD mMask;
   DWORD dwCallback[2];
   DWORD dwResult;
} SERIALIO,*LPSERIALIO;
//---------------------------------------------------------------------------
typedef void WINAPI (*LPSAVESTATECALLBACK)(int);
//---------------------------------------------------------------------------
struct LStream;
struct LISaveState;
struct ServiceInterface;
//---------------------------------------------------------------------------
typedef BOOL WINAPI (*LPPLUGINRESET)();
typedef BOOL WINAPI (*LPPLUGINDELETE)();
typedef DWORD WINAPI (*LPPLUGINGETINFO)(LPPLUGININFO);
typedef BOOL WINAPI (*LPPLUGINSETINFO)(LPPLUGININFO);
typedef BOOL WINAPI (*LPPLUGINSETPROPERTY)(LPSETPROPPLUGIN);
typedef BOOL WINAPI (*LPAUDIOPLUGRUN)(WORD index,LPVOID mem1,DWORD dwSize1,LPVOID mem2,DWORD dwSize2);
typedef BOOL WINAPI (*LPVIDEOPLUGRUN)(WORD index,unsigned short *InBuffer,unsigned short *OutBuffer);
typedef BOOL WINAPI (*LPVIDEOAUDIOPLUGRUN)(WORD index,LPVIDEOAUDIO);
typedef BOOL WINAPI (*LPSIOPLUGRUN)(LPSERIALIO);
typedef void WINAPI (*LPSIOCALLBACKFUNC)(LPSERIALIO);
typedef BOOL WINAPI (*LPBACKUPPLUGRUN)(WORD index,LStream *p);
//---------------------------------------------------------------------------
DEFINE_GUID(IID_IMemoryFile,0x4DCBA39A,0x2B55,0x4405,0x95,0x32,0x60,0xF0,0x84,0xA4,0x9A,0x6B);
DEFINE_GUID(IID_IZipFile,0xD19284B5,0x3F21,0x4141,0xB9,0x6D,0xE7,0x01,0x62,0x12,0xC8,0x5C);
DEFINE_GUID(IID_ISaveState,0x9219C5E9,0x0EB1,0x455D,0x9C,0x67,0x5D,0x3E,0x0A,0xDD,0x00,0xBA);
//---------------------------------------------------------------------------

#endif
