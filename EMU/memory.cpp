//---------------------------------------------------------------------------
#include <windows.h>
#pragma hdrstop

#include "fstream.h"
#include "memory.h"
#include "sound.h"
#include "lcd.h"
#include "sprite.h"
#include "gba.h"
#include "bios.h"
#include "debug.h"
#include "graphics.h"
#include "list.h"
#include "zipfile.h"
#include "exec.h"
#include "savestate.h"
#include "cheat.h"
#include "rtc.h"
#include "sio.h"
#include "pluginctn.h"

#if !defined(__BORLANDC__)
#include <math.h>
#endif
//---------------------------------------------------------------------------
u8  *rom_pages_u8[0x202];
u8  *wram_int_u8;
u16 *wram_int_u16;
u32 *wram_int_u32;
u8  *wram_ext_u8;
u16 *wram_ext_u16;
u32 *wram_ext_u32;
u8  *pal_ram_u8;
u16 *pal_ram_u16;
u32 *pal_ram_u32;
u8  *vram_u8;
u16 *vram_u16;
u32 *vram_u32;
u8  *oam_u8;
u16 *oam_u16;
u32 *oam_u32;
u8  *io_ram_u8;
u16 io_ram_u16[0x201];
u32 *io_ram_u32;
u8  *zero_page_u8;
u8  *cram_u8;
u32 LastData,ValueDataAbort,LastAddress,LastDataRead;
u8 LastBusAccess,bDataAbort,bUseNewAccessBios;
CNTPACKROM rom_pack[2];
LList *pSaveStateList;
//---------------------------------------------------------------------------
/*#define INC_IWRAM_CYCLES   arm.iwram_cycles.u.cS += (u8)8;arm.iwram_cycles.u.cN += (u8)8;\
                           arm.memCycles.u.cS = (u8)(arm.iwram_cycles.u.cS >> 4);arm.memCycles.u.cN = (u8)(arm.iwram_cycles.u.cN >> 4);\
                           arm.iwram_cycles.u.cS -= (u8)(arm.memCycles.u.cS << 4);arm.iwram_cycles.u.cN -= (u8)(arm.memCycles.u.cN<<4)
#define INC_BIOSRAM_CYCLES arm.biosram_cycles.u.cS += (u8)8;arm.biosram_cycles.u.cN += (u8)8;\
                           arm.memCycles.u.cS = (u8)(arm.biosram_cycles.u.cS >> 4);arm.memCycles.u.cN = (u8)(arm.biosram_cycles.u.cN >> 4);\
                           arm.biosram_cycles.u.cS -= (u8)(arm.memCycles.u.cS << 4);arm.biosram_cycles.u.cN -= (u8)(arm.memCycles.u.cN<<4)
#define INC_IORAM_CYCLES   arm.ioram_cycles.u.cS += (u8)8;arm.ioram_cycles.u.cN += (u8)8;\
                           arm.memCycles.u.cS = (u8)(arm.ioram_cycles.u.cS >> 4);arm.memCycles.u.cN = (u8)(arm.ioram_cycles.u.cN >> 4);\
                           arm.ioram_cycles.u.cS -= (u8)(arm.memCycles.u.cS << 4);arm.ioram_cycles.u.cN -= (u8)(arm.memCycles.u.cN<<4)*/

#define INC_IWRAM_CYCLES   arm.memCycles.value = 0x0101;
#define INC_BIOSRAM_CYCLES INC_IWRAM_CYCLES
#define INC_IORAM_CYCLES   INC_IWRAM_CYCLES

//---------------------------------------------------------------------------
enum eeCommand{eecNull,eecWrite,eecErase,eecSeek,eecNull2,eecUnlock,eecNull4,eecLock};
//---------------------------------------------------------------------------
static SRAMID sramID[]= {
   {0xd4bf,0,0},
   {0x1cc2,0,0},
   {0x1b32,0,0},
   {0x3d1f,0,0},
   {0x1362,0,1},
   {0x09c2,0,1},
   {0xd5bf,0,1},
   {0,0}
};
static u8 indexSRAM;
//---------------------------------------------------------------------------
static u32 WriteRomPack(LPCNTPACKROM pPack,u32 adress,u32 data,u8 mode);
static u32 ReadRomPack(LPCNTPACKROM pPack,u32 adress,u8 mode);
static int FileFlashROM(BOOL bRead);
static BOOL FileSRAM(BOOL bRead);
static void GetLastData();
static u8 ReadSRAM(u32 adress);
static void DataAbort(u32 data);
static void WriteSRAM(u32 adress,u8 data);
//---------------------------------------------------------------------------
u8 FASTCALL read_byte(u32 adress)
{
#ifdef _DEBPRO
   if(bDebug)
       ControlMemoryBreakPoint(adress,0x10,0);
#endif
	switch((adress >> 24) & 0xF){
		case 0:
           if((REG_PC & 0x0F000000)){
#ifdef _DEBPRO
               InsertException("Read Byte memory exception",adress);
#endif
               if(!bUseNewAccessBios) GetLastData();
               return (u8)0xFF;
           }
           INC_BIOSRAM_CYCLES;
           return (u8)(LastDataRead = (zero_page_u8[adress&0x3FFF]));
		case 0x2:
           arm.memCycles.value = 0x0303;
			return (u8)(LastDataRead = (wram_ext_u8[adress&0x3FFFF]));
		case 0x3:
           INC_IWRAM_CYCLES;
           return (u8)(LastDataRead = (wram_int_u8[adress&0x7FFF]));
		case 0x4:
           INC_IORAM_CYCLES;
			return (u8)(LastDataRead = (io_ram_u8[adress&0x3FF]));
		case 0x5:
           arm.memCycles.value = 0x0101;
			return (u8)(LastDataRead = (pal_ram_u8[adress&0x3FF]));
		case 0x6:
           arm.memCycles.value = 0x0101;
			return (u8)(LastDataRead = (vram_u8[adress&0x1FFFF]));
		case 0x7:
           arm.memCycles.value = 0x0101;
			return (u8)(LastDataRead = (oam_u8[adress&0x3FF]));
       case 0x8:
       case 0x9:
           arm.memCycles.value = arm.ws[0].value;
           if((adress & 0x1FFFFFF) < bin.rom_size_u8)
			    return (u8)(LastDataRead = (rom_pages_u8[(adress>>16)&0x1FF][(u16)adress]));
           return (u8)LastDataRead;
       case 0xA:
       case 0xB:
           arm.memCycles.value = arm.ws[1].value;
           return (u8)(LastDataRead = (u32)(ReadRomPack(&rom_pack[0],adress,AMM_BYTE)));
       case 0xC:
       case 0xD:
           arm.memCycles.value = arm.ws[2].value;
           return (u8)(LastDataRead = (u32)(ReadRomPack(&rom_pack[1],adress,AMM_BYTE)));
       case 0xE:
           arm.memCycles.value = arm.ws[3].value;
           return (u8)(LastDataRead = (u32)ReadSRAM(adress));
       default:
           return 0;
	}
}
//---------------------------------------------------------------------------
u16 FASTCALL read_hword(u32 adress)
{
#ifdef _DEBPRO
   if(bDebug)
       ControlMemoryBreakPoint(adress,0x20,0);
#endif
	switch((adress >> 24) & 0xF){
		case 0:
           if((REG_PC & 0x0F000000)){
#ifdef _DEBPRO
               InsertException("Read HalfWord memory exception 0x%08X",adress);
#endif
               if(!bUseNewAccessBios) GetLastData();
               return 0xFFFF;
           }
           INC_BIOSRAM_CYCLES;
           return (*((u16 *)(zero_page_u8 + (adress&0x3FFF))));
		case 0x2:
           arm.memCycles.value = 0x0303;
			return (u16)(LastDataRead = *((u16 *)(wram_ext_u8 + (adress&0x3FFFF))));
		case 0x3:
           INC_IWRAM_CYCLES;
			return (u16)(LastDataRead = *((u16 *)(wram_int_u8 + (adress&0x7FFF))));
		case 0x4:
           INC_IORAM_CYCLES;
			return (u16)(LastDataRead = *((u16 *)(io_ram_u8 + (adress&0x3FF))));
		case 0x5:
           arm.memCycles.value = 0x0101;
			return (u16)(LastDataRead = *((u16 *)(pal_ram_u8 + (adress&0x3FF))));
		case 0x6:
           arm.memCycles.value = 0x0101;
	  		return (u16)(LastDataRead = *((u16 *)(vram_u8 + (adress&0x1FFFF))));
		case 0x7:
           arm.memCycles.value = 0x0101;
			return (u16)(LastDataRead = *((u16 *)(oam_u8 + (adress&0x3FF))));
       case 0x8:
       case 0x9:
           arm.memCycles.value = arm.ws[0].value;
           if(adress == 0x80000c8)
               return rtcData.REG_ENABLE;
           else if(adress == 0x80000c6)
               return rtcData.REG_RW;
           else if(adress == 0x80000c4)
               return rtcData.REG_DATA;
           return (u16)(LastDataRead = *((u16 *)(rom_pages_u8[(adress>>16)&0x1FF] + (u16)adress)));
       case 0xA:
       case 0xB:
           arm.memCycles.value = arm.ws[1].value;
           return (u16)(ReadRomPack(rom_pack,adress,AMM_HWORD));
       case 0xC:
       case 0xD:
           arm.memCycles.value = arm.ws[2].value;
           return (u16)(ReadRomPack(&rom_pack[1],adress,AMM_HWORD));
       default:
           return 0;
	}
}
//---------------------------------------------------------------------------
u32 FASTCALL read_word(u32 adress)
{
#ifdef _DEBPRO
   if(bDebug)
       ControlMemoryBreakPoint(adress,0x40,0);
#endif
	switch((adress >> 24) & 0xF){
		case 0:
           if((REG_PC & 0x0F000000)){
#ifdef _DEBPRO
               InsertException("Read Word memory exception 0x%08X",adress);
#endif
               if(!bUseNewAccessBios) GetLastData();
               return 0xFFFFFFFF;
           }
           INC_BIOSRAM_CYCLES;
           return (*((u32 *)(zero_page_u8 + (adress&0x3FFF))));
		case 0x2:
           arm.memCycles.value = 0x0606;
			return LastDataRead =(*((u32 *)(wram_ext_u8 + (adress&0x3FFFF))));
		case 0x3:
           INC_IWRAM_CYCLES;
			return LastDataRead =(*((u32 *)(wram_int_u8 + (adress&0x7FFF))));
		case 0x4:
           INC_IORAM_CYCLES;
			return LastDataRead =(*((u32 *)(io_ram_u8 + (adress&0x3FF))));
		case 0x5:
           arm.memCycles.value = 0x0102;
			return LastDataRead =(*((u32 *)(pal_ram_u8 + (adress&0x3FF))));
		case 0x6:
           arm.memCycles.value = 0x0102;
	  		return LastDataRead =(*((u32 *)(vram_u8 + (adress&0x1FFFF))));
		case 0x7:
           arm.memCycles.value = 0x0101;
			return LastDataRead =(*((u32 *)(oam_u8 + (adress&0x3FF))));
       case 0x8:
       case 0x9:
           arm.memCycles.value = (u16)(arm.ws[0].u.cS + arm.ws[0].u.cN);
           arm.memCycles.value |= (u16)(arm.memCycles.value << 8);
           return LastDataRead =(*((u32 *)(rom_pages_u8[(adress>>16)&0x1FF] + (u16)adress)));
       case 0xA:
       case 0xB:
           arm.memCycles.value = (u16)(arm.ws[1].u.cS + arm.ws[1].u.cN);
           arm.memCycles.value |= (u16)(arm.memCycles.value << 8);
           return LastDataRead =((ReadRomPack(rom_pack,adress,AMM_WORD)));
       case 0xC:
       case 0xD:
           arm.memCycles.value = (u16)(arm.ws[2].u.cS + arm.ws[2].u.cN);
           arm.memCycles.value |= (u16)(arm.memCycles.value << 8);
           return LastDataRead =((ReadRomPack(&rom_pack[1],adress,AMM_WORD)));
       default:
           return 0;
	}
}
//---------------------------------------------------------------------------
void FASTCALL write_byte(u32 adress, u32 data)
{
#ifdef _DEBPRO
   if(bDebug)
       ControlMemoryBreakPoint(adress,0x11,(u8)data);
#endif
   if(data)
       LastData = (LastData << 8) | ((u8)data);
	switch((adress >> 24) & 0xF){
       case 0:
           DataAbort(data);
           INC_BIOSRAM_CYCLES;
       break;
		case 0x2:
			wram_ext_u8[adress&0x3FFFF] = (u8)data;
           arm.memCycles.value = 0x0303;
		break;
		case 0x3:
			wram_int_u8[adress&0x7FFF] = (u8)data;
           INC_IWRAM_CYCLES;
		break;
		case 0x4:
           io_ram_u8[(adress &= 0x3ff)] = (u8)data;
           io_write_handles[adress]((u16)adress,AMM_BYTE);
           INC_IORAM_CYCLES;
		break;
		case 0x5:
           arm.memCycles.value = 0x0101;
			pal_ram_u8[(adress &= 0x3FF)] = (u8)data;
           FillPalette((u16)(adress >> 1));
  		break;
		case 0x6:
           arm.memCycles.value = 0x0101;
           if(!(adress & 1))
               *((u16 *)(vram_u8 + (adress&0x1FFFE))) = (u16)(((u8)data << 8) | (u8)data);
           else
               *((u8 *)(vram_u8 + (adress&0x1FFFF))) = (u8)data;
		break;
		case 0x7:
           arm.memCycles.value = 0x0101;
           oam_u8[(adress &= 0x3FF)] = (u8)data;
           WriteSprite((u16)adress,AMM_BYTE);
		break;                                                                    //81e12ea
       case 0x8:
       case 0x9:
#ifdef _DEBPRO
           InsertException("Write memory exception 0x%08X",adress);
#endif
           arm.memCycles.value = arm.ws[0].value;
       break;
       case 0xE:
           arm.memCycles.value = arm.ws[3].value;
           WriteSRAM(adress,(u8)data);
       break;
       default:
       break;
	}
}
//---------------------------------------------------------------------------
void FASTCALL write_hword(u32 adress, u16 data)
{
#ifdef _DEBPRO
   if(bDebug)
       ControlMemoryBreakPoint(adress & ~1,0x21,(u16)data);
#endif
   if(data)
       LastData = (LastData << 16) | data;
	switch((adress >> 24) & 0xF){
       case 0:
           DataAbort(data);
           INC_BIOSRAM_CYCLES;
       break;
		case 0x2:
           arm.memCycles.value = 0x0303;
           *((u16 *)(wram_ext_u8 + (adress&0x3FFFE))) = data;
		break;
		case 0x3:
           *((u16 *)(wram_int_u8 + (adress&0x7FFE))) = data;
           INC_IWRAM_CYCLES;
		break;
		case 0x4:
           *((u16 *)(io_ram_u8 + (adress &= 0x3FE))) = data;   //30013a8
		    io_write_handles[adress]((u16)adress,AMM_HWORD);
           INC_IORAM_CYCLES;
       break;
		case 0x5:
           arm.memCycles.value = 0x0101;
           adress = (adress & 0x3FF) >> 1;
			pal_ram_u16[adress] = data;
           FillPalette((u16)adress);
		break;
		case 0x6:
/*           adress &= 0x1FFFF;
           if(adress > 0x8000 && adress < 0x1c000)
               i = i;
           i = adress;
           i1 = (int)((~(i & 0x10000)) << 15) >> 16;
           if(i > 0x14000)
               i = 0x14000;
           adress = i | (adress & 0x7FFF);*/
           arm.memCycles.value = 0x0101;
           adress = (adress & 0x10000) | (adress & (int)(((~(adress & 0x10000)) << 15) >> 16));
           *((u16 *)(vram_u8 + (adress&0x1FFFE))) = data;
       break;
		case 0x7:
           arm.memCycles.value = 0x0101;
			*((u16 *)(oam_u8 + (adress &= 0x3FE))) = data;
           WriteSprite((u16)adress,AMM_HWORD);
		break;
       case 0x8:
       case 0x9:
//           *((u16 *)(rom_pages_u8[(adress>>16)&0x1FF] + (u16)adress)) = data;
#ifdef _DEBPRO
           InsertException("Write memory exception 0x%08X",adress);
#endif
           if(adress > 0x80000c3 && adress < 0x80000cA)
               rtcWrite(adress,data);
           arm.memCycles.value = arm.ws[0].value;
       break;
		case 0xA:
       case 0xB:
			WriteRomPack(rom_pack,adress,data,AMM_HWORD);
           arm.memCycles.value = arm.ws[1].value;
       break;
       case 0xC:
		case 0xD:
           arm.memCycles.value = arm.ws[2].value;
       	WriteRomPack(&rom_pack[1],adress,data,AMM_HWORD);
       break;
       default:
       break;
	}
}
//---------------------------------------------------------------------------
void FASTCALL write_word(u32 adress,u32 data)
{
#ifdef _DEBPRO
   if(bDebug)
       ControlMemoryBreakPoint(adress & ~3,0x41,data);
#endif
   LastData = 0;
   if(REG_PC == adress)
       return;
	switch((adress >> 24) & 0xF){
       case 0:
           DataAbort(data);
           INC_BIOSRAM_CYCLES;
       break;
		case 0x2:
           *((u32 *)(wram_ext_u8 + (adress&0x3FFFC))) = data;
           arm.memCycles.value = 0x0606;
		break;
		case 0x3:
			*((u32 *)(wram_int_u8 + (adress&0x7FFC))) = data;
           INC_IWRAM_CYCLES;
		break;
		case 0x4:
			*((u32 *)(io_ram_u8 + (adress &= 0x3FC))) = data;
		    io_write_handles[(adress)]((u16)adress,AMM_WORD);
           INC_IORAM_CYCLES;
		break;
		case 0x5:
           arm.memCycles.value = 0x0102;
           *((u32 *)(pal_ram_u8 + (adress &= 0x3FC))) = data;
           adress >>= 1;
           FillPalette((u16)adress++);                          //30001d8
           FillPalette((u16)adress);
		break;
		case 0x6:
           arm.memCycles.value = 0x0102;
           adress = (adress & 0x10000) | (adress & (int)(((~(adress & 0x10000)) << 15) >> 16));
           *((u32 *)(vram_u8 + (adress & 0x1FFFC))) = data;
		break;
		case 0x7:
           arm.memCycles.value = 0x0101;
			*((u32 *)(oam_u8 + (adress &= 0x3FC))) = data;//200c984
           WriteSprite((u16)adress,AMM_WORD);
		break;
       case 0x8:
       case 0x9:
#ifdef _DEBPRO
           InsertException("Write memory exception 0x%08X",adress);
#endif
           arm.memCycles.value = (u16)(arm.ws[0].u.cS + arm.ws[0].u.cN);
           arm.memCycles.value |= (u16)(arm.memCycles.value << 8);
       break;
       case 0xA:
       case 0xB:
           arm.memCycles.value = (u16)(arm.ws[1].u.cS + arm.ws[1].u.cN);
           arm.memCycles.value |= (u16)(arm.memCycles.value << 8);
			WriteRomPack(rom_pack,adress,data,AMM_WORD);
       break;
       case 0xC:
       case 0xD:
           arm.memCycles.value = (u16)(arm.ws[2].u.cS + arm.ws[2].u.cN);
           arm.memCycles.value |= (u16)(arm.memCycles.value << 8);
			WriteRomPack(&rom_pack[1],adress,data,AMM_WORD);
       break;
       default:
       break;
	}
}
//-----------------------------------------------------------------------
void SetReadMemFunction(u8 value,u8 value1)
{
   switch(value){
       case 1:
           arm.pReadByte = read_byteCheat;
           arm.pReadHWord = read_hwordCheat;
           arm.pReadWord = read_wordCheat;
       break;
       default:
           arm.pReadByte = read_byte;
           arm.pReadHWord = read_hword;
           arm.pReadWord = read_word;
       break;
   }
   switch(value1){
       case 1:
           arm.pWriteByte = write_byteCheat;
           arm.pWriteHWord = write_hwordCheat;
           arm.pWriteWord = write_wordCheat;
       break;
       default:
           arm.pWriteByte = write_byte;
           arm.pWriteHWord = write_hword;
           arm.pWriteWord = write_word;
       break;
   }
}
//-----------------------------------------------------------------------
void EnableNewAccessBios(BOOL bFlag)
{
	bUseNewAccessBios = bFlag;
}
//-----------------------------------------------------------------------
BOOL AllocSRAM(BOOL bAlloc)
{
   int size;
   LPSRAM sram;

   if(cram_u8 != NULL)
       GlobalFree((HGLOBAL)cram_u8);
   cram_u8 = NULL;
   if(!bAlloc)
       return TRUE;
   if(IsSRAM128K())
       size = 128 * 1024;
   else
       size = 64*1024;
   if((cram_u8 = (u8 *)GlobalAlloc(GPTR,sizeof(SRAM)+size)) == NULL)
       return FALSE;
   MemoryAddress[10].Size = size;
   sram = (LPSRAM)cram_u8;
   sram->buffer = cram_u8 + sizeof(SRAM);
   sram->mask = size - 1;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL IsSRAM128K()
{
   if(indexSRAM > ID_SRAM_END - ID_SRAM_START)
       return FALSE;
   return (BOOL)(sramID[indexSRAM].mode == 1 ? TRUE : FALSE);
}
//---------------------------------------------------------------------------
WORD GetGamePackID()
{
   switch(indexSRAM){
       case 0:
           return ID_SRAM_SST64K;
       case 1:
           return ID_SRAM_MACRO64K;
       case 2:
           return ID_SRAM_PANASONIC64K;
       case 3:
           return ID_SRAM_ATMEL64K;
       case 4:
           return ID_SDRAM_SANYO128K;
       case 5:
           return ID_SRAM_MACRO128K;
       case 6:
           return ID_SRAM_SST128K;
       case 0x80:
           return ID_EEPROM_32K;
       case 0x81:
           return ID_EEPROM_128K;
   }
   return 0;
}
//---------------------------------------------------------------------------
void SetGamePackID(WORD wID)
{
   int i,i1;
   char string[100],string1[100],string2[100];
   u8 sizeCommand,bitCommand;
   MENUITEMINFO mii={0};

   lstrcpy(string,"S  RAM");
   switch(wID){
       case ID_SRAM_SST64K:
           indexSRAM = 0;
       break;
       case ID_SRAM_SST128K:
           indexSRAM = 6;
       break;
       case ID_SRAM_ATMEL64K:
           indexSRAM = 3;
       break;
       case ID_SDRAM_SANYO128K:
           indexSRAM = 4;
       break;
       case ID_SRAM_MACRO64K:
           indexSRAM = 1;
       break;
       case ID_SRAM_MACRO128K:
           indexSRAM = 5;
       break;
       case ID_SRAM_PANASONIC64K:
           indexSRAM = 2;
       break;
       case ID_EEPROM_32K:
           indexSRAM = 0x80;
           sizeCommand = 16;
           bitCommand = 3;
       break;
       case ID_EEPROM_128K:
           indexSRAM = 0x81;
           sizeCommand = 32;
           bitCommand = 3;
       break;
   }
   if(indexSRAM >= 0x80){
       lstrcpy(string,"E  EPROM");
       rom_pack[0].sizeCommand = sizeCommand;
       rom_pack[1].sizeCommand = sizeCommand;
       rom_pack[0].bitCommand = bitCommand;
       rom_pack[1].bitCommand = bitCommand;
   }
   for(i=ID_SRAM_START;i<=ID_EEPROM_END;i++)
       CheckMenuItem(GetMenu(hWin),i,MF_BYCOMMAND|(wID == i ? MF_CHECKED : MF_UNCHECKED));
   AllocSRAM((BOOL)(indexSRAM < 0x80 ? TRUE : FALSE));
   mii.cbSize = sizeof(MENUITEMINFO);
   mii.fMask = MIIM_TYPE;
   mii.dwTypeData = string2;
   mii.cch = 99;
   GetMenuItemInfo(GetMenu(hWin),wID,FALSE,&mii);
   for(i=i1=0;i<lstrlen(string2);i++){
       if(string2[i] != 9)
           string1[i1++] = string2[i];
       else{
           *((long *)(&string1[i1])) = 0x20202020;
           i1 += 4;
       }
   }
   string1[i1] = 0;
   lstrcpy(&string[lstrlen(string)+1],string1);
   UpdateStatusBar(string,2);
}
//---------------------------------------------------------------------------
BOOL InitMmu()
{
   int i;

	bUseNewAccessBios = FALSE;
   indexSRAM = 0x80;
   for (i=0; i<0x200; i++)
		rom_pages_u8[i] = NULL;
   ZeroMemory(&rom_pack[0],sizeof(CNTPACKROM));
   ZeroMemory(&rom_pack[1],sizeof(CNTPACKROM));
   rom_pack[0].sizeCommand = rom_pack[1].sizeCommand = 16;
   rom_pack[0].bitCommand = rom_pack[1].bitCommand = 3;
   cram_u8 = wram_int_u8  =  wram_ext_u8 = pal_ram_u8 = vram_u8 = oam_u8 = zero_page_u8 = NULL;
   pSaveStateList = NULL;
	if((wram_int_u8 = (u8 *)GlobalAlloc(GMEM_FIXED,445000)) == NULL)
       return FALSE;
	wram_ext_u8     = wram_int_u8   + 0x8002;
	pal_ram_u8      = wram_ext_u8   + 0x40002;
	vram_u8         = pal_ram_u8    + 0x402;
	oam_u8          = vram_u8       + 0x20002;
	zero_page_u8    = oam_u8        + 0x402;
	wram_int_u16    = (u16*)wram_int_u8;
	wram_int_u32    = (u32*)wram_int_u8;
	wram_ext_u16    = (u16*)wram_ext_u8;
	wram_ext_u32    = (u32*)wram_ext_u8;
	pal_ram_u16     = (u16*)pal_ram_u8;
	pal_ram_u32     = (u32*)pal_ram_u8;
	vram_u16        = (u16*)vram_u8;
	vram_u32        = (u32*)vram_u8;
	oam_u16         = (u16*)oam_u8;
	oam_u32         = (u32*)oam_u8;
   io_ram_u8       = (u8 *)io_ram_u16;
	io_ram_u32	    = (u32*)io_ram_u8;
   ResetMmu();
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL UseSRAM()
{
   return (BOOL)(indexSRAM <= ID_SRAM_END - ID_SRAM_START ? TRUE : FALSE);
}
//---------------------------------------------------------------------------
void DestroyMmu()
{
   if(pSaveStateList != NULL)
       delete pSaveStateList;
   pSaveStateList = NULL;
   DeallocRom();
	FreeRomPack(1);
   FreeRomPack(2);
   if(wram_int_u8 != NULL)
       GlobalFree((HGLOBAL)wram_int_u8);
   wram_int_u8     = NULL;
   wram_ext_u8     = NULL;
   pal_ram_u8      = NULL;
   vram_u8         = NULL;
   oam_u8          = NULL;
   zero_page_u8    = NULL;
   if(cram_u8 != NULL)
       GlobalFree((HGLOBAL)cram_u8);
   cram_u8 = NULL;
}
//---------------------------------------------------------------------------
void ResetMmu()
{
   ResetBios(NULL);
   ZeroMemory(pal_ram_u8,0x402);
   ZeroMemory(vram_u8,0x20002);
   ZeroMemory(wram_int_u8,0x8002);
   ZeroMemory(wram_ext_u8,0x40002);
	ZeroMemory(oam_u8,0x402);
	ZeroMemory(io_ram_u8,0x402);
   InitRTC();
   if(cram_u8 != NULL && !(bin.bLoadRam & 1)){
       if(IsSRAM128K())
           ZeroMemory(((LPSRAM)cram_u8)->buffer,128*1024);
       else
           ZeroMemory(((LPSRAM)cram_u8)->buffer,64*1024);
   }
   if(!(bin.bLoadRam & 2))
       FreeRomPack(1);
   if(!(bin.bLoadRam & 4))
	    FreeRomPack(2);
   bDataAbort = FALSE;
}
//---------------------------------------------------------------------------
static u32 ReadRomPack(LPCNTPACKROM pPack,u32 adress,u8 mode)
{
	u8 *p;
   u32 value;

   if(UseSRAM())
       return 0;
	switch(mode){
		case AMM_BYTE:
       	return 0;
       case AMM_HWORD:
           switch(pPack->mode){
               case 1:
                   return 1;
               case 0:
                   if((p = pPack->rom_pack[pPack->blocco].buffer) == NULL)
                       return 0;
                   if((p[pPack->byteIndex] & (1 << pPack->bitIndex)) != 0)
                       value = 1;
                   else
                       value = 0;
                   if((pPack->bitIndex = (u8)((pPack->bitIndex + 1) & 0x7)) == 0)
                       pPack->byteIndex++;
                   return value;
           }
       case AMM_WORD:
			return 0;
   }
	return 0;
}
//---------------------------------------------------------------------------
static u32 WriteRomPack(LPCNTPACKROM pPack,u32 adress,u32 data,u8 mode)
{
	u8 *p;

   if(UseSRAM())
       return 0;
	switch(mode){
       case AMM_HWORD:
       	if((adress = (u32)(u16)adress) == 0){
               pPack->com = 0x0;
               pPack->mode = 0;
               pPack->blocco = 0;
               pPack->byteIndex = 0;
               pPack->bitIndex = 0;
               pPack->bytesWrite = 0;
           }
           if(adress < pPack->sizeCommand){
               if((data & 1) != 0)
                   pPack->com |= (u32)(1 << (adress >> 1));
               if(adress == (pPack->sizeCommand - 2)){
                   pPack->blocco = (u16)(((pPack->com & 0x01FE000) >> 13));
                   if(pPack->rom_pack[pPack->blocco].buffer == NULL){
                       pPack->rom_pack[pPack->blocco].buffer = (u8 *)GlobalAlloc(GPTR,0x4000);
                       pPack->rom_pack[pPack->blocco].size = 0;
                   }
                   pPack->byteIndex = (u32)((pPack->com & (0x1FFF & ~pPack->bitCommand)) << 1);
                   pPack->bitIndex = (pPack->com & pPack->bitCommand) == eecWrite ? 4 : 0;
                   pPack->bytesWrite = 0;
               }
           }
           else{
               switch((pPack->com & pPack->bitCommand)){
                   case eecSeek:
                   break;
                   case eecWrite:
                       if(pPack->bytesWrite > 63){
                           pPack->isUsed = 1;
                           pPack->mode = 1;
                           break;
                       }
                       p = pPack->rom_pack[pPack->blocco].buffer;
                       if((data & 1))
                           p[pPack->byteIndex] |= (u8)(1 << pPack->bitIndex);
                       else
                           p[pPack->byteIndex] &= (u8)~(1 << pPack->bitIndex);
                       if((pPack->bitIndex = (u8)((pPack->bitIndex + 1) & 0x7)) == 0)
                           pPack->byteIndex++;
                       if(pPack->byteIndex > pPack->rom_pack[pPack->blocco].size)
                           pPack->rom_pack[pPack->blocco].size = pPack->byteIndex;
                       pPack->bytesWrite++;
                   break;
               }
           }
       break;
   }
	return TRUE;
}
//---------------------------------------------------------------------------
void FreeRomPack(u8 pack)
{
	u8 **p,*last;
	int i;
   LPPACKROM p1;

   if(!pack){
       p = rom_pages_u8;
       last = NULL;
       for(i=0;i<0x200;i++){
           if(p[i] != NULL && last != p[i]){
               last = p[i];
       	    GlobalFree((HGLOBAL)p[i]);
           }
           p[i] = NULL;
       }
   }
   else{
       pack = (u8)log2(pack);
     	p1 = rom_pack[pack].rom_pack;
       for(i=0;i<0x200;i++){
           if(p1[i].buffer != NULL)
               GlobalFree((HGLOBAL)p1[i].buffer);
           p1[i].buffer = NULL;
       }
       rom_pack[pack].com = 0x0;
       rom_pack[pack].mode = 0;
       rom_pack[pack].blocco = 0;
       rom_pack[pack].byteIndex = 0;
       rom_pack[pack].bitIndex = 0;
       rom_pack[pack].bytesWrite = 0;
   }
}
//---------------------------------------------------------------------------
void DeallocRom()
{
   FileSRAM(FALSE);
   FileFlashROM(FALSE);
   bin.bLoad = FALSE;
   bin.bLoadRam = 0;
   *((u32 *)bin.Title) = 0;
   FreeRomPack(0);
	FreeRomPack(1);
   FreeRomPack(2);
}
//---------------------------------------------------------------------------
BOOL WriteFileEEPROM(LStream *pFile)
{
   LPPACKROM p1;
   LPCNTPACKROM p;
   int i1,i,rom_size;
   u8 ver;

   if(pFile == NULL || !pFile->IsOpen())
       return FALSE;
   for(i1=0;i1<2;i1++){
       p = &rom_pack[i1];
       for(i=0;i<0x200;i++){
           if(p->rom_pack[i].buffer != NULL){
               if(!pFile->GetCurrentPosition()){
                   ver = 0xFF;
                   pFile->Write(&ver,1);
               }
               pFile->Write(&i1,sizeof(u8));
               pFile->Write(&i,sizeof(u16));
               p1 = ((LPPACKROM)&p->rom_pack[i]);
               rom_size = p1->size + 1;
               pFile->Write(&rom_size,sizeof(u32));
               pFile->Write(p1->buffer,rom_size);
           }
       }
   }
   if(pPlugInContainer && pPlugInContainer->GetBackupPlugInList()){
   	pFile->SeekToBegin();
		if(pPlugInContainer->GetBackupPlugInList()->Run(pFile,PIT_WRITE|PIT_EEPROM) > 0)
           return 0;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
DWORD ReadFileEEPROM(LStream *pFile)
{
   DWORD res;
   u8 ver,*pBuffer,value8;
   int i,i1,rom_size,i3,i4,i5,i2;
   u16 *pBuffer2,value;

   if(UseSRAM() || pFile == NULL || !pFile->IsOpen())
       return 0;
   if(pPlugInContainer && pPlugInContainer->GetBackupPlugInList()){
   	pFile->SeekToBegin();
		pPlugInContainer->GetBackupPlugInList()->Run(pFile,PIT_READ|PIT_EEPROM);
       pFile->SeekToBegin();
   }
   if(pFile->Read(&ver,1) != 1)
       return 0;
   if(ver != 0xFF)
       pFile->SeekToBegin();
   res = 0;
   do{
       i = rom_size = 0;
       i1 = 0;
       if(pFile->Read(&i1,sizeof(u8)) != 1)
           goto ex_ReadFileEEPROM;
       if(pFile->Read(&i,sizeof(u16)) != 2)
           goto ex_ReadFileEEPROM;
       if(ver != 0xFF){
           if(pFile->Read(&rom_size,sizeof(u16)) != 2)
               goto ex_ReadFileEEPROM;
           pBuffer = (u8 *)GlobalAlloc(GPTR,0x20000);
           if(pBuffer == NULL)
               goto ex_ReadFileEEPROM;
           pBuffer2 = (u16 *)GlobalAlloc(GPTR,0x20000);
           if(pBuffer2 == NULL){
               GlobalFree((HGLOBAL)pBuffer);
               goto ex_ReadFileEEPROM;
           }
           if(!pFile->Read((char *)pBuffer2,rom_size*sizeof(u16))){
               GlobalFree((HGLOBAL)pBuffer2);
               GlobalFree((HGLOBAL)pBuffer);
               goto ex_ReadFileEEPROM;
           }
           //Converte da old a new
           for(i4 = i3 = 0;i3< rom_size;i3+=16,i4+=8){
               for(i5 = 0;i5 < 9;i5++){
                   value = pBuffer2[i3+i5];
                   value8 = 0;
                   for(i2=0;i2<16;i2 += 2){
                       if((value & (1 << i2)) != 0)
                           value8 |= (u8)(1 << (i2 >> 1));
                   }
                   if(i3 != 0 && i5 == 0 && pBuffer[i4] != 0)
                       continue;
                   pBuffer[i4+i5] = value8;
               }
           }
           rom_size = i4;
           GlobalFree((HGLOBAL)pBuffer2);
           i1 = 1;
           i = 0;
/*           HANDLE fp1;
           fp1 = OpenStream("c:\\windows\\temp\\lino",GENERIC_WRITE,CREATE_ALWAYS);
           ver = 0xFF;
           WriteStream(fp1,&ver,1);
           WriteStream(fp1,&i1,sizeof(u8));
           WriteStream(fp1,&i,sizeof(u16));
           WriteStream(fp1,&rom_size,sizeof(u16));
           WriteStream(fp1,pBuffer,rom_size);
           CloseStream(fp1);*/
       }
       else{
           if(pFile->Read(&rom_size,sizeof(u32)) != 4)
               goto ex_ReadFileEEPROM;
           pBuffer = (u8 *)GlobalAlloc(GPTR,0x4000);
           if(pBuffer == NULL)
               goto ex_ReadFileEEPROM;
           if(!pFile->Read(pBuffer,rom_size)){
               GlobalFree((HGLOBAL)pBuffer);
               break;
           }
       }
       rom_pack[i1].rom_pack[i].buffer = pBuffer;
       rom_pack[i1].rom_pack[i].size = rom_size - 1;
       rom_pack[i1].isUsed = 1;
       res |= i1 == 0 ? 2 : 4;
   }while(1);
ex_ReadFileEEPROM:
   return res;
}
//---------------------------------------------------------------------------
static int FileFlashROM(BOOL bRead)
{
   LFile *fp;
   LString nameFile;
   BOOL res;

   if(UseSRAM())
       return FALSE;
   if(!nameFile.BuildFileName(bin.saveFileName,"erm",mbID))
       return FALSE;
   if((fp = new LFile(nameFile.c_str())) == NULL)
       return FALSE;
   res = FALSE;
   if(!bRead){
       if(!rom_pack[0].isUsed && !rom_pack[1].isUsed){
           res = TRUE;
           goto EX_FileFlashROM;
       }
       if(!fp->Open(GENERIC_WRITE,CREATE_ALWAYS))
           goto EX_FileFlashROM;
       res = WriteFileEEPROM(fp);
       rom_pack[0].isUsed = rom_pack[1].isUsed = 0;
   }
   else{
       if(!fp->Open(GENERIC_READ,OPEN_EXISTING))
           goto EX_FileFlashROM;
       res = ReadFileEEPROM(fp);
   }
EX_FileFlashROM:
   if(fp != NULL)
       delete fp;
   return res;
}
//---------------------------------------------------------------------------
BOOL WriteFileSRAM(LStream *pFile)
{
   DWORD dwSize;

   if(pFile == NULL || !pFile->IsOpen())
       return FALSE;
   if((dwSize = (DWORD)(((LPSRAM)cram_u8)->size + 1)) == 1)
       return TRUE;
   if(pFile->Write(((LPSRAM)cram_u8)->buffer,dwSize) != dwSize)
       return FALSE;
   if(pPlugInContainer && pPlugInContainer->GetBackupPlugInList()){
   	pFile->SeekToBegin();
		pPlugInContainer->GetBackupPlugInList()->Run(pFile,PIT_WRITE|PIT_SRAM);
   }
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL ReadFileSRAM(LStream *pFile)
{
   DWORD file_size;

   if(pFile == NULL || !pFile->IsOpen())
       return FALSE;
   if(!UseSRAM())
       return FALSE;
   if((file_size = pFile->Size() - 1) < 1)
       return FALSE;
   if(pPlugInContainer && pPlugInContainer->GetBackupPlugInList()){
   	pFile->SeekToBegin();
		pPlugInContainer->GetBackupPlugInList()->Run(pFile,PIT_READ|PIT_SRAM);
       pFile->SeekToBegin();
   }
   ((LPSRAM)cram_u8)->size = 0;
   if(file_size <= 0x20000){
       if(!IsSRAM128K() && file_size > 0xFFFF)
           SetGamePackID(ID_SRAM_MACRO128K);
       if(IsSRAM128K())
           ZeroMemory(((LPSRAM)cram_u8)->buffer,0x20000);
       else
           ZeroMemory(((LPSRAM)cram_u8)->buffer,0x10000);
       pFile->Read(((LPSRAM)cram_u8)->buffer,file_size+1);
       ((LPSRAM)cram_u8)->size = file_size;
       return TRUE;
   }
   return FALSE;
}
//---------------------------------------------------------------------------
static BOOL FileSRAM(BOOL bRead)
{
   LFile *fp;
   LString nameFile;
   BOOL res;

   if(!UseSRAM() || !nameFile.BuildFileName(bin.saveFileName,"srm",mbID))
       return FALSE;
   nameFile.LowerCase();
   if((fp = new LFile(nameFile.c_str())) == NULL)
       return FALSE;
   res = FALSE;
   if(bRead){
       if(fp->Open())
           res = ReadFileSRAM(fp);
   }
   else if(bin.bLoad && cram_u8 != NULL && ((LPSRAM)cram_u8)->size > 0){
       if(fp->Open(GENERIC_WRITE,CREATE_ALWAYS))
           res = WriteFileSRAM(fp);
   }
   if(fp != NULL)
       delete fp;
   return res;
}
//---------------------------------------------------------------------------
int load_bin(char *filename)
{
   LFile *fp;
	int file_size,n,i;
   u8 *last,*ext,bZip;
   LString s,s1;
   LZipFile zipFile;
   LPZIPFILEHEADER p;

   DeallocRom();
   s = filename;
   s.LowerCase();
   if(s.Pos(".zip") > 0){
       bZip = 1;
       if(!zipFile.Open(filename,FALSE))
           return 0;
       //Troviamo la rom e prendiamo la prima
       for(i=0;i < (int)zipFile.Count();i++){
           p = zipFile.GetZipFileHeader(i+1);
           s1 = p->nameFile;
           s1.LowerCase();
           if((n = s1.Pos(".gba")) > 0){
               if(s1.Length() == n + 3)
                   break;
           }
           else if((n = s1.Pos(".agb")) > 0){
               if(s1.Length() == n + 3)
                   break;
           }
           else if((n = s1.Pos(".bin")) > 0){
               if(s1.Length() == n + 3)
                   break;
           }
       }
       if(i >= zipFile.Count() || zipFile.OpenZipFile((WORD)(i+1)) == 0){
           zipFile.Close();
           return 0;
       }
       file_size = p->m_uUncomprSize;
       s1 = s = p->nameFile;
       s = s.FileName();
       s += s1.Ext();
       s1 = filename;
       s1 = s1.Path();
       lstrcpy(bin.saveFileName,s1.c_str());
       lstrcat(bin.saveFileName,"\\");
       lstrcat(bin.saveFileName,s.c_str());
   }
   else{
       s1 = filename;
       s1.LowerCase();
       if(s1.Pos(".gba") < 0 && s1.Pos(".agb") < 0 && s1.Pos(".bin") < 0)
           return 0;
       bZip = 0;
       if((fp = new LFile(filename)) == NULL)
           return 0;
       if(!fp->Open())
           return 0;
       file_size = fp->Size();
       lstrcpy(bin.saveFileName,filename);
   }
   bin.rom_size_u8 = file_size;
   lstrcpy(bin.FileName,filename);
   n = file_size >> 16;
   for(i=0;i<=n;i++){
       ProcessaMessaggi();
		if((rom_pages_u8[i] = (unsigned char *)GlobalAlloc(GPTR,0x10000)) == NULL)
           break;               
       if(!bZip)
           fp->Read(rom_pages_u8[i],0x10000);
       else
           zipFile.ReadZipFile(rom_pages_u8[i],0x10000);
       DrawProgressBar(MAKELONG(i,n));
	}
   if(!bZip)
       delete fp;
   else
       zipFile.Close();
   if(i < n)
       return 0;
   bin.maxIndex = (u32)n;
   if(i < 0x200){
       if((last = (u8 *)GlobalAlloc(GPTR,0x10000)) == NULL)
           return 0;
       for(;i<0x200;i++)
           rom_pages_u8[i] = last;
   }
   bin.bLoad = TRUE;
   bin.bLoadRam = 0;
   ZeroMemory(bin.Title,13);
   for(i=0;i<12;i++)
       bin.Title[i] = READBYTE(0x080000A0 + i);
   if(lstrlen(bin.Title) == 0){
       last = (u8 *)strrchr(filename,'\\');
       if(last != NULL){
           if((ext = (u8 *)strrchr(filename,'.')) != NULL){
               if((i = ext - ++last) > 12)
                   i = 12;
               lstrcpyn(bin.Title,(char *)last,i+1);
           }
           else
               lstrcpyn(bin.Title,(char *)++last,13);
       }
   }
//   if(mbID != 0)
//       rom_pages_u8[0][0xC5] = mbID;
   if(FileSRAM(TRUE))
       bin.bLoadRam |= 1;
   bin.bLoadRam |= (u16)FileFlashROM(TRUE);
#ifdef _DEBUG
   SetGamePakRomSize(0,(u32)file_size);
#endif
   ReadSaveStateList();
	return 1;
}
//---------------------------------------------------------------------------
static void GetLastData()
{
   switch(LastBusAccess){
       case 8:
           ValueDataAbort = (u8)(LastData >> 8);
           LastData = (LastData & ~(ValueDataAbort << 8)) << 8;
       break;
       case 16:
           ValueDataAbort = (u16)(LastData >> 16);
           LastData = (LastData & ~(ValueDataAbort << 16)) << 16;
       break;
       default:
           ValueDataAbort = LastData;
           LastData = 0;
       break;
   }
//   ValueDataAbort = 0;
   bDataAbort = TRUE;
}
//---------------------------------------------------------------------------
static void DataAbort(u32 data)
{
   if(data < 0x100)
       LastBusAccess = 8;
   else if(data < 0x10000){
       LastData = ((LastData >> 8) << 16) | ((u16)data);
       LastBusAccess = 16;
   }
   else{
       LastBusAccess = 32;
       LastData = data;
   }
#ifdef _DEBPRO
   InsertException("Data abort exception");
#endif
}
//---------------------------------------------------------------------------
static void WriteSRAM(u32 adress,u8 data)
{
   LPSRAM p;

   if(!UseSRAM())
       return;
   p = (LPSRAM)cram_u8;
   adress = (u16)adress;
   if(adress == 0x5555 && p->com != 0xA0){
       if(data == 0xF0 || data == 0xAA){
           if(p->mode != 2 || (p->com != 0x80 && p->com != 0xA0)){
               p->com = 0;
               p->mode = 0;
           }
           return;
       }
       if(p->mode == 1){
           p->com = (u16)data;
           p->mode = 2;
           if(data == 0xF0){
               p->com = 0;
               p->mode = 0;
           }
           return;
       }
   }
   else if(adress == 0x2AAA && p->com != 0xA0){
       if(p->mode == 0){
           if(data == 0x55){
               p->mode = 1;
               return;
           }
       }
       else if(p->mode == 2){
           if(data == 0x55 && p->com == 0x80)
               p->mode = 3;
           return;
       }
   }
   else if(p->mode == 3 && p->com != 0xA0){
       if(data == 0x30){
           //erase sector
           adress = (p->blocco << 16) | (adress & 0xF000);
           ZeroMemory(&p->buffer[adress],0x1000);
       }
       else
           ZeroMemory(p->buffer,IsSRAM128K() ? 0x20000 : 0x10000);
       p->mode = 0;
       return;
   }
   else if(p->mode == 2 && p->com == 0xB0){
       p->blocco = (u16)(u8)data;
       p->mode = 0;
       return;
   }
   adress |= (p->blocco << 16);
   if((s32)adress > p->size)
       p->size = adress;
   p->buffer[adress] = (u8)data;
   if(p->mode == 2 && p->com == 0xA0){
       p->mode = 0;
       p->com = 0;
   }
}
//---------------------------------------------------------------------------
static u8 ReadSRAM(u32 adress)
{
   LPSRAM p;
   LPSRAMID p1;

   if(!UseSRAM())
       return 0;
   p = (LPSRAM)cram_u8;
   adress = (u32)(u16)adress;
   switch(p->com){
       case 0x80:
           p->com = 0;
           p->mode = 0;
           return 0xFF;
       case 0x90:
           if(adress < 2){
               p1 = &sramID[indexSRAM];
               if((adress & 1))
                   return *((u8 *)(&p1->ID) + 1);
               else
                   return *((u8 *)(&p1->ID));
           }
       break;
   }
   return p->buffer[(p->blocco << 16) | adress];
}
//---------------------------------------------------------------------------

