#include <windows.h>

#ifndef cpuH
#define cpuH

#define USER_MODE		    0x10
#define FIQ_MODE		    0x11
#define IRQ_MODE		    0x12
#define SUPERVISOR_MODE	0x13
#define ABORT_MODE		    0x17
#define UNDEFINED_MODE	    0x1B
#define SYSTEM_MODE		0x1F
#define THUMB_BIT		    0x00000020
#define FIQ_BIT			0x00000040
#define IRQ_BIT			0x00000080
#define N_BIT		        0x80000000
#define Z_BIT		        0x40000000
#define C_BIT		        0x20000000
#define V_BIT		        0x10000000
#define T_BIT		        0x00000020

#define CPSR	   	    arm.cpsr
#define ZFLAG		    arm.z_flag
#define NFLAG		    arm.n_flag
#define CFLAG		    arm.c_flag
#define VFLAG		    arm.v_flag
#define GP_REG         gp_reg
#define REG_PC         GP_REG[15]

#define FIFOBUFFER_SIZE    4096
#define CPU_FREQ           (1 << 24)

#define AMM_NULL	0
#define AMM_BYTE   1
#define AMM_HWORD  2
#define AMM_WORD   4
#define AMM_BIT    8
#define AMM_ALL	0xFF

#define SAVESTATE_SIZEARM7TDMI 856

#define cycS   arm.memCycles.u.cS
#define cycN   arm.memCycles.u.cN
#define cycI   1
#define cycP   arm.memCyclesPipe.u.cS
#define cycPN  arm.memCyclesPipe.u.cN

#define LDR_RET        (u8)(cycI + cycP + cycPN)
#define LDRPC_RET      LDR_RET
#define LDRBASE_RET    LDR_RET
#define LDRBASEPC_RET  LDR_RET
#define STR_RET        (u8)(cycP + cycPN)
#define STRBASE_RET    (u8)(cycP + cycPN)

typedef unsigned char  byte;
typedef unsigned short hword;
typedef unsigned long  word;
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned long  u32;
typedef signed char  s8;
typedef signed short s16;
typedef signed long  s32;

#if defined __BORLANDC__
#define FASTCALL __fastcall
#else
#define FASTCALL __attribute__ ((regparm(2)))
#endif

typedef u8 (*CPUEXEC)(void);
typedef u8 (*ARMOPCODE)(void);
typedef u8 (*THUMBOPCODE)(void);
typedef void (*IOROUTINE)(u16,u8);
typedef u8 FASTCALL (*LPREADBYTE)(u32);
typedef u16 FASTCALL (*LPREADHWORD)(u32);
typedef u32 FASTCALL (*LPREADWORD)(u32);

typedef void FASTCALL (*LPWRITEBYTE)(u32,u32);
typedef void FASTCALL (*LPWRITEHWORD)(u32,u16);
typedef void FASTCALL (*LPWRITEWORD)(u32,u32);
//-----------------------------------------------------------------------
typedef struct{
   u32 lr;
   u32 ra;
   u8  irq;
   u8  mode;
} LR,*PLR;
//-----------------------------------------------------------------------
typedef union{
   struct{
       u8 cS;
       u8 cN;
   } u;
   u32 value;
} WAITSTATE;
#define IWRAM_CYCLES   WAITSTATE
#define BIOSRAM_CYCLES WAITSTATE
#define IORAM_CYCLES   WAITSTATE
//-----------------------------------------------------------------------
typedef struct {
	u32 op;
	u32 op_t;
   u32 *gp_reg;
	u32 cpsr;
	u8  z_flag;
	u8  n_flag;
	u8  c_flag;
	u8  v_flag;
   u32 ra;
   u32 reg[7][17];
   u8  mode;
   LR  lrv[17];
   u8  index,stop;
   u16 irq,IntrWait,irqSuspend;
   u8 bInc,bEnableSpeedCPU;
   u8 useDMA;
   CPUEXEC exec[2][4];
   LPREADBYTE pReadByte;
   LPREADHWORD pReadHWord;
   LPREADWORD pReadWord;
   LPWRITEBYTE pWriteByte;
   LPWRITEHWORD pWriteHWord;
   LPWRITEWORD pWriteWord;
   WAITSTATE memCycles,memCyclesPipe;
   u8 bRefillPipe,nPluginCallback;
   u16 timerCycles;
   WAITSTATE ws[5];
   IWRAM_CYCLES iwram_cycles;
   BIOSRAM_CYCLES biosram_cycles;
   IORAM_CYCLES ioram_cycles;
} ARM7TDMI,*PARM7TDMI;
//-----------------------------------------------------------------------
typedef struct{
   u8 Index;
   u16 *Control;
   u16 *Count;
   u16 CountOffset;
   u16 Freq;
   u8 Enable;
   u8 Irq;
   u8 Cascade;
   s32 Value;
   u16 ResetValue;
   u16 Diff,Remainder;
   u8 Inc;
} TIMER,*PTIMER,*LPTIMER;
//-----------------------------------------------------------------------
typedef struct{
   u8 Index;
   u32 *Source;
   u16 offSrc;
   u32 *Dest;
   u16 offDst;
   u32 *Control;
   u32 Dst,Src;
//   u16 Count;
   u16 MaxCount;
   u16 InternalCount;
   u8 Enable;
   u8 Start;
   u8 Repeat;
   u8 Reload;
   u8 Irq;
   s8 IncS,IncD;
   u8 Mode;
} DMA,*PDMA,*LPDMA;
//-----------------------------------------------------------------------
typedef struct{
   u8 Index;
   s8 Enable;
   u8 Volume;
   u8 Timer;
   u8 Dma;
   u8 Start,Max;
   u32 Freq;
   u32 Resample,Pos;
   u32 Inc,MaxPos;
   u8 *lpAddress,*lpBuffer;
} FIFOCHANNEL,*PFIFOCHANNEL,*LPFIFOCHANNEL;
//-----------------------------------------------------------------------
typedef struct
{
   u8 Index;
	u8 on,timed,writeable;
   s8 enableL,enableR;
	u32 pos,frcnt,dutyCycle;
	u32 cnt, encnt, swcnt;
	u32 len, enlen, swlen;
   s8 endir,swdir;
	int swfreq;
	u32 freq;
	s8 envol;
   u8 value,value1,value2;
} GBCCHANNEL,*LPGBCCHANNEL;
//-----------------------------------------------------------------------
typedef struct {
   u8 Stereo;
   u8 bStart,isPlay;
   s8 Enable,EnableL,EnableR;
   GBCCHANNEL gbcch[4];
   u8 wave[16];
   u32 Freq,hz,bitsPerSample;
   u32 LastWriteBytes,LastPlay,LastWriteEnd;
   u8 Update;
   u8 *lpBuffer;
   u32 Inc,MaxPos;
   int cycles,rate,fps;
   double bytesFPS;
   u8 volL,volR,pos;
   FIFOCHANNEL fifo[2];
   void (*fpWriteSound)(LPVOID,DWORD,LPVOID,int,int,int,int);
   void (*fpSoundMix)(u8);
} PCM,*PPCM,*LPPCM;
//-----------------------------------------------------------------------
extern ARM7TDMI arm;
extern TIMER timer[4];
extern DMA dma[4];
extern PCM pcm;
extern LPFIFOCHANNEL fifo;
extern LPGBCCHANNEL gbcch;
extern u32 cpu_cycles,*gp_reg;
extern char *cpu_mode_strings[0x20];
extern CRITICAL_SECTION crSection;
extern u8 mbID;

#ifdef __cplusplus
extern "C" {
#endif

u8 InitFifo();
void DestroyFifo(void);
void SetFifoFreq(u8 i,u32 value);
void ResetFifo(void);
void CalcFifoFreq(PTIMER p);

u8 InitCpu();
u8 SwitchCpuMode(u16 value);
void ResetCpu(void);
void DestroyCpu();
void SetInterrupt(u16 value);
void SetInterruptSuspend(u16 value);
int EnableSpeedCPU(WORD wID);
int IDToCpuInc(int value);
int CpuIncToID(int value);

void ResetTimer();
void RenderTimer(void);
void SetTimerGBA(LPTIMER p);
void SetTimerCount(LPTIMER p);

void SetDMAGBA(u8 i,u16 adress);
void ResetDMA();
void ExecDMA(u8 i);

#ifdef __cplusplus
}
#endif

#endif
