#include "gbaemu.h"

//---------------------------------------------------------------------------
#ifndef soundH
#define soundH

#define RI_NR10        0x60
#define RI_NR11        0x62
#define RI_NR12        0x63
#define RI_NR13        0x64
#define RI_NR14        0x65
#define RI_NR21        0x68
#define RI_NR22        0x69
#define RI_NR23        0x6c
#define RI_NR24        0x6d
#define RI_NR30        0x70
#define RI_NR31        0x72
#define RI_NR32        0x73
#define RI_NR33        0x74
#define RI_NR34        0x75
#define RI_NR41        0x78
#define RI_NR42        0x79
#define RI_NR43        0x7c
#define RI_NR44        0x7d
#define RI_NR50        0x80
#define RI_NR51        0x81
#define RI_SGCNT0_H    0x82
#define RI_SGBIAS      0x88
#define RI_NR52        0x84

#define SGCNT0_H   io_ram_u16[0x41]
#define SG_BIAS    io_ram_u16[0x44]
#define R_NR10     io_ram_u8[0x60]
#define R_NR11     io_ram_u8[0x62]
#define R_NR12     io_ram_u8[0x63]
#define R_NR13     io_ram_u8[0x64]
#define R_NR14     io_ram_u8[0x65]
#define R_NR21     io_ram_u8[0x68]
#define R_NR22     io_ram_u8[0x69]
#define R_NR23     io_ram_u8[0x6c]
#define R_NR24     io_ram_u8[0x6d]
#define R_NR30     io_ram_u8[0x70]
#define R_NR31     io_ram_u8[0x72]
#define R_NR32     io_ram_u8[0x73]
#define R_NR33     io_ram_u8[0x74]
#define R_NR34     io_ram_u8[0x75]
#define R_NR41     io_ram_u8[0x78]
#define R_NR42     io_ram_u8[0x79]
#define R_NR43     io_ram_u8[0x7c]
#define R_NR44     io_ram_u8[0x7d]
#define R_NR50     io_ram_u8[0x80]
#define R_NR51     io_ram_u8[0x81]
#define R_NR52     io_ram_u8[0x84]

#pragma option push -a
#pragma option -a1
typedef struct
{
  BYTE  riff[4];
  DWORD len;
  BYTE  cWavFmt[8];
  DWORD dwHdrLen;
  WORD  wFormat;
  WORD  wNumChannels;
  DWORD dwSampleRate;
  DWORD dwBytesPerSec;
  WORD  wBlockAlign;
  WORD  wBitsPerSample;
  BYTE  cData[4];
  DWORD dwDataLen;
} WAVHDR, *PWAVHDR, *LPWAVHDR;
#pragma option pop
//---------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

void SetSoundGbcFunction(u8 value);
void no_sound_mix(u8 forceUpdate);
void SetResampleFunction(u8 value);
void sound_mix(u8 forceUpdate);
void sound_on();
void DisableSound();
BOOL EnableSound();
void sound_write(u16 r,u8 accessMode);
void sound_reset();
void sound_off();
void sound_dirty();
void UpdateFifo();
u8 StartPlayFifo();
void ResetPlayFifo();
HRESULT PauseRestartSound(u8 flag);
int GetSoundSubCodeError();
void StopSoundSave();
BOOL EnableSoundSave();
void SetFrequencyRate(int value);
void SetResampleFunction(u8 value);
BOOL EnableSoundPlug(WORD wID);
BOOL OnInitAudioPlugMenu(HMENU menu);

#ifdef __cplusplus
}
#endif

#endif
