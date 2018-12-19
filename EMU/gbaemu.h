#define INITGUID

#include <windows.h>
#include "cpu.h"

#ifndef GBAEMU_HEADER
#define GBAEMU_HEADER

#define LINE_CYCLES    1261447
#define CYCLES_SHIFT   10

#define DISPCNT	    io_ram_u16[0x0]
#define DISPSTAT	    io_ram_u16[0x2]
#define BG0CNT		    io_ram_u16[0x4]
#define BG1CNT		    io_ram_u16[0x5]
#define BG2CNT		    io_ram_u16[0x6]
#define BG3CNT		    io_ram_u16[0x7]

#define BG2PA          io_ram_u16[0x10]
#define BG2PB          io_ram_u16[0x11]
#define BG2PC          io_ram_u16[0x12]
#define BG2PD          io_ram_u16[0x13]

#define BG3PA          io_ram_u16[0x18]
#define BG3PB          io_ram_u16[0x19]
#define BG3PC          io_ram_u16[0x1A]
#define BG3PD          io_ram_u16[0x1B]

#define BGFCNT         io_ram_u16[0x26] // MOSAIC
#define VCOUNT		    io_ram_u16[0x3]
#define BLENDCNT       io_ram_u16[0x28]
#define BLENDV         io_ram_u16[0x29]
#define BLENDY         io_ram_u16[0x2A]

#define WINXH          io_ram_u16[0x20]
#define WININ          io_ram_u16[0x24]
#define WINOUT         io_ram_u16[0x25]

#define INTR_VCR_BUF   wram_int_u32[0x1FFF]
#define INTR_CHECK     wram_int_u16[0x3FFC]
#define REG_IE         io_ram_u16[0x100]
#define REG_IF         io_ram_u16[0x101]
#define REG_IME        io_ram_u16[0x104]

#define KEYINPUT       io_ram_u16[0x98]
#define KEYCNT         io_ram_u16[0x99]

#define DM0SAD         io_ram_u32[0x2C]
#define DM0SAD_L	    io_ram_u16[0x58]
#define DM0SAD_H	    io_ram_u16[0x59]
#define DM0DAD         io_ram_u32[0x2D]
#define DM0DAD_L	    io_ram_u16[0x5A]
#define DM0DAD_H	    io_ram_u16[0x5B]
#define DM0CNT         io_ram_u32[0x2E]
#define DM0CNT_L	    io_ram_u16[0x5C]
#define DM0CNT_H	    io_ram_u16[0x5D]

#define DM1SAD         io_ram_u32[0x2F]
#define DM1SAD_L	    io_ram_u16[0x5E]
#define DM1SAD_H	    io_ram_u16[0x5F]
#define DM1DAD         io_ram_u32[0x30]
#define DM1DAD_L	    io_ram_u16[0x60]
#define DM1DAD_H	    io_ram_u16[0x61]
#define DM1CNT         io_ram_u32[0x31]
#define DM1CNT_L	    io_ram_u16[0x62]
#define DM1CNT_H	    io_ram_u16[0x63]

#define DM2SAD         io_ram_u32[0x32] // dma2
#define DM2SAD_L	    io_ram_u16[0x64]
#define DM2SAD_H	    io_ram_u16[0x65]
#define DM2DAD         io_ram_u32[0x33]
#define DM2DAD_L	    io_ram_u16[0x66]
#define DM2DAD_H	    io_ram_u16[0x67]
#define DM2CNT         io_ram_u32[0x34]
#define DM2CNT_L	    io_ram_u16[0x68]
#define DM2CNT_H	    io_ram_u16[0x69]

#define DM3SAD		    io_ram_u32[0x35] // 35
#define DM3SAD_L	    io_ram_u16[0x6A]
#define DM3SAD_H	    io_ram_u16[0x6B]
#define DM3DAD		    io_ram_u32[0x36] // 36
#define DM3DAD_L	    io_ram_u16[0x6C]
#define DM3DAD_H	    io_ram_u16[0x6D]
#define DM3CNT		    io_ram_u32[0x37] // 37
#define DM3CNT_L	    io_ram_u16[0x6E]
#define DM3CNT_H	    io_ram_u16[0x6F]

#define TM0D           io_ram_u16[0x80]
#define TM0CNT         io_ram_u16[0x81]
#define TM1D           io_ram_u16[0x82]
#define TM1CNT         io_ram_u16[0x83]
#define TM2D           io_ram_u16[0x84]
#define TM2CNT         io_ram_u16[0x85]
#define TM3D           io_ram_u16[0x86]
#define TM3CNT         io_ram_u16[0x87]

#define RCNT           io_ram_u16[0x9A] // 134
#define SCCNT_L        io_ram_u16[0x94] // 128
#define SCCNT_H        io_ram_u16[0x95]
#define SCD0           io_ram_u16[0x90]
#define SCD1           io_ram_u16[0x91]
#define SCD2           io_ram_u16[0x92]
#define SCD3           io_ram_u16[0x93]

#define JOYCNT         io_ram_u16[0xA0]
#define JOYSTAT        io_ram_u16[0xAC]
#define IR             io_ram_u16[0x9B]

#define WAITCNT        io_ram_u16[0x102]

#define SOUND1CNT_L    io_ram_u16[0x30]
#define SOUND1CNT_H    io_ram_u16[0x31]
#define SOUND1CNT_X    io_ram_u16[0x32]
#define SOUND2CNT_H    io_ram_u16[0x36]
#define SOUND3CNT_X    io_ram_u16[0x3A]

#define OPCODE	   	    arm.op
#define OPCODE_T  	    arm.op_t
#define CONDITION_MASK	(OPCODE>>28)&0xF


typedef struct BINARY_IMAGE {
	u32 rom_size_u8,maxIndex;
   char FileName[MAX_PATH],saveFileName[MAX_PATH];
   char Title[15];
   u8 bLoad,bLoadRam;
} BINARY_IMAGE;

extern ARMOPCODE opcode_handles[0x1002];
extern THUMBOPCODE opcode_handles_t[0x1002];
extern CPUEXEC exec;
extern BINARY_IMAGE bin;
extern IOROUTINE io_write_handles[0x400];

#ifdef __cplusplus
extern "C" {
#endif

int init_gbaemu();
void reset_gbaemu ();
void run_frame ();
void clean_up ();
void setup_tables ();
void fill_instruction_pipe(void);
void advance_instruction_pipe(void);
void tfill_instruction_pipe(void);
void tadvance_instruction_pipe(void);
void CheckEnterInterrupt(u16 r);
int SettaVelocita(u8 value);
int GetMainSubCodError();
void CheckExitInterrupt(void);

#if defined(__BORLANDC__)
int FASTCALL log2(u32 value);
#endif
void SwapBuffer(u8 xStart,u8 xEnd);

#ifdef __cplusplus
}
   #define RELEASE(a) if(a != NULL) a->Release(); a = NULL;
#else
   #define RELEASE(a) if(a != NULL) a->lpVtbl->Release(a); a = NULL;
#endif

#endif
