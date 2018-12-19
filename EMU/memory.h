#include "cpu.h"
#include "resource.h"

//---------------------------------------------------------------------------
#ifndef memoryH
#define memoryH
//---------------------------------------------------------------------------
typedef struct{
   SYSTEMTIME time;
   char fileName[12];
} SSFILE,*LPSSFILE;
//---------------------------------------------------------------------------
typedef struct{
   u8 *buffer;
   u32 size;
   u32 com;
   u16 blocco;
   u8 mode;
   u32 mask;
} SRAM,*LPSRAM;
//---------------------------------------------------------------------------
typedef struct{
   u8 *buffer;
   u32 size;
} PACKROM,*LPPACKROM;
//---------------------------------------------------------------------------
typedef struct{
   u32 com;
   u16 blocco;
   u8 mode;
   u32 byteIndex;
   u8 bitIndex,bytesWrite,sizeCommand,isUsed,bitCommand;
   PACKROM rom_pack[0x200];
} CNTPACKROM,*LPCNTPACKROM;
//---------------------------------------------------------------------------
typedef struct{
   u16 ID;
   u16 addrTest;
   u8 mode;
} SRAMID,*LPSRAMID;

//---------------------------------------------------------------------------
extern u8  *oam_u8;
extern u16 *oam_u16;
extern u32 *oam_u32;
extern u8  *rom_pages_u8[0x202];
extern CNTPACKROM rom_pack[2];
extern u8  *wram_int_u8;
extern u16 *wram_int_u16;
extern u32 *wram_int_u32;
extern u8  *wram_ext_u8;
extern u16 *wram_ext_u16;
extern u32 *wram_ext_u32;
extern u8  *zero_page_u8;
extern u8  *pal_ram_u8;
extern u16 *pal_ram_u16;
extern u32 *pal_ram_u32;
extern u8  *io_ram_u8;
extern u16 io_ram_u16[0x201];
extern u32 *io_ram_u32;
extern u8  *vram_u8;
extern u16 *vram_u16;
extern u32 *vram_u32;
extern u8 *cram_u8;
extern u32 LastData,ValueDataAbort,LastDataRead;
extern u8 LastBusAccess;
extern u8 bDataAbort;

#ifdef __cplusplus
extern "C" {
#endif

void FreeRomPack(u8 pack);
BOOL IsSRAM128K();
WORD GetGamePackID();
BOOL InitMmu();
void ResetMmu();
void DestroyMmu();
void DeallocRom();
int load_bin (char *filename);
u8  FASTCALL read_byte (u32 adress);
u16 FASTCALL read_hword (u32 adress);
u32 FASTCALL read_word (u32 adress);
void FASTCALL write_byte(u32 adress, u32 data);
void FASTCALL write_hword(u32 adress, u16 data);
void FASTCALL write_word(u32 adress, u32 data);
void SetGamePackID(WORD wID);
BOOL UseSRAM();
void SetReadMemFunction(u8 value,u8 value1);
void EnableNewAccessBios(BOOL bFlag);

#ifdef __cplusplus
}
#endif

/*#define READBYTE(a)    arm.pReadByte(a)
#define READHWORD(a)   arm.pReadHWord(a)
#define READWORD(a)    arm.pReadWord(a)*/

#define READBYTE(a)    read_byte(a)
#define READHWORD(a)   read_hword(a)
#define READWORD(a)    read_word(a)


#define WRITEBYTE(a,b)     arm.pWriteByte(a,b)
#define WRITEHWORD(a,b)    arm.pWriteHWord(a,b)
#define WRITEWORD(a,b)     arm.pWriteWord(a,b)

/*#define WRITEBYTE(a,b)     write_byte(a,b)
#define WRITEHWORD(a,b)    write_hword(a,b)
#define WRITEWORD(a,b)     write_word(a,b)*/

#endif
