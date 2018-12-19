//---------------------------------------------------------------------------
#pragma hdrstop

#if defined(__BORLANDC__)
#define DWORD_PTR      DWORD
#define LONG_PTR       DWORD
#endif
#pragma warn -use

#include "sound.h"
#include "fstream.h"
#include "memory.h"
#include "gba.h"
#include "C:\Borland\include\dsound.h"
#include "lstring.h"
#include "trad.h"
#include <dlgs.h>
#include <commdlg.h>
#include <dshow.h>
#include <dshowasf.h>
#include "debug.h"
#include "pluginctn.h"

#define SOUND_LENGTH   44100 * 2
#define GBA_FPS        59.7275

#define WAVE pcm.wave
#define RATE pcm.rate
#define S1 gbcch[0]
#define S2 gbcch[1]
#define S3 gbcch[2]
#define S4 gbcch[3]
#define PCM0 fifo[0]
#define PCM1 fifo[1]

static LPDIRECTSOUND lpDS;
static LPDIRECTSOUNDBUFFER lpBuffer;
static WAVEFORMATEX wf;
static DSBUFFERDESC dsBD;
static LFile *fpWave;
static DWORD dwLenWave;
static int iSubCodeError;
static BOOL bSaveWave = FALSE,bResampleMix = FALSE,bSaveCompressed = FALSE;
static LString waveFileName;
static AudioPlugList *pAudioPlugList;

//Tabella lfsr
static u8 lfsr7[20];
static u16 lfsr15[2190];
static float percDuty[]={0.125,0.25,0.50,0.75};

const GUID WMProfile_V70_96Audio   = {0xa9d4b819,0x16cc,0x4a59,{0x9f, 0x37, 0x69, 0x3d, 0xbb, 0x3, 0x2, 0xd6}};
const GUID WMProfile_V70_64Audio   = {0xb29cffc6,0xf131,0x41db,{0xb5, 0xe8, 0x99, 0xd8, 0xb0, 0xb9, 0x45, 0xf4}};
const GUID IID_IGraphBuilder       = {0x56a868a9,0x0ad4,0x11ce,{0xb0,0x3a,0x00,0x20,0xaf,0x0b,0xa7,0x70}};
const GUID IID_IBaseFilter         = {0x56a86895,0x0ad4,0x11ce,{0xb0,0x3a,0x00,0x20,0xaf,0x0b,0xa7,0x70}};
const GUID IID_IFileSinkFilter     = {0xa2104830,0x7c70,0x11cf,{0x8b,0xce,0x00,0xaa,0x00,0xa3,0xf1,0xa6}};
const GUID IID_IConfigAsfWriter    = {0x45086030,0xF7E4,0x486a,{0xB5,0x04,0x82,0x6B,0xB5,0x79,0x2A,0x3B}};
const GUID IID_IMediaFilter        = {0x56a86899,0x0ad4,0x11ce,{0xb0,0x3a,0x00,0x20,0xaf,0x0b,0xa7,0x70}};

//---------------------------------------------------------------------------
static BOOL SleepAt(LPDWORD pdwPlay,LPDWORD pdwWrite);
static void InitGBChannel(LPGBCCHANNEL p,BOOL bEnable);
static void WriteFifoChannel(LPFIFOCHANNEL pFifo);
static BOOL WINAPI InitSaveDlg(LPOPENFILENAME pofn);
static UINT APIENTRY OFNHookProc(HWND hdlg,UINT uiMsg,WPARAM wParam,LPARAM lParam);
static int toWma(char *FileName,LPWAVEFORMATEX wf,DWORD dwLen);
static void writeWavHeader(LStream *fp,DWORD len);
//---------------------------------------------------------------------------
static void s1_freq()
{
   LPGBCCHANNEL p;

   pcm.fpSoundMix(1);
   p = &gbcch[0];
   p->freq = (2048 - (SOUND1CNT_X & 0x7FF)) << 7;
   p->dutyCycle = p->freq * percDuty[R_NR11 >> 6];
}
//---------------------------------------------------------------------------
static void s2_freq()
{
   LPGBCCHANNEL p;

   pcm.fpSoundMix(1);
   p = &gbcch[1];
   p->freq = (2048 - (SOUND2CNT_H & 0x7FF)) << 7;
   p->dutyCycle = p->freq * percDuty[R_NR21 >> 6];
}
//---------------------------------------------------------------------------
static void s3_freq()
{
   pcm.fpSoundMix(1);
   gbcch[2].freq = (2048 - (SOUND3CNT_X & 0x7FF)) << 4;
}
//---------------------------------------------------------------------------
static void s4_freq()
{
   u32 freqtab[8] = {524288 * 2,524288,524288 / 2, 524288 / 3,524288 / 4,524288 / 5,524288 / 6,524288 / 7};
   u8 div;

   pcm.fpSoundMix(1);
   if((div = (u8)(R_NR43 >> 4)) > 13)
       div = 13;
	gbcch[3].freq = (CPU_FREQ / (freqtab[R_NR43 & 7] >> ++div));
}
//---------------------------------------------------------------------------
static void CalcEnergy(LPGBCCHANNEL p)
{
   p->value = (u8)(p->envol >> 1);
   if((p->envol & 1))
       p->value++;
}
//---------------------------------------------------------------------------
void sound_dirty()
{
   int i;

	s1_freq();
	s2_freq();
	s3_freq();
	s4_freq();
   for(i=0;i<4;i++){
       pcm.gbcch[i].Index = (u8)(i + 1);
       pcm.gbcch[i].enableL = (s8)((pcm.gbcch[i].enableL & 0x80) | 1);
       pcm.gbcch[i].enableR = (s8)((pcm.gbcch[i].enableR & 0x80) | 1);
       InitGBChannel(&pcm.gbcch[i],FALSE);
   }
}
//---------------------------------------------------------------------------
void sound_off()
{
   R_NR52 = 0xF0;
   gbcch[0].on = gbcch[1].on = gbcch[2].on = gbcch[3].on = 0;
   pcm.Enable &= 0x80;
   if(lpBuffer != NULL)
       IDirectSoundBuffer_Stop(lpBuffer);
   pcm.bStart = pcm.isPlay = FALSE;
}
//---------------------------------------------------------------------------
void sound_on()
{
   pcm.Enable |= 1;
//   fifo[0].Enable |= 1;
//   fifo[1].Enable |= 1;
}
//---------------------------------------------------------------------------
void sound_reset()
{
   pcm.bStart = pcm.isPlay =FALSE;

	R_NR10 = 0x80;
	R_NR11 = 0xBF;
	R_NR12 = 0xF3;
	R_NR14 = 0xBF;
	R_NR21 = 0x3F;
	R_NR22 = 0xF0;
	R_NR24 = 0xBF;
	R_NR30 = 0xFF;
	R_NR31 = 0xFF;
	R_NR32 = 0x1F;
	R_NR33 = 0xBF;
	R_NR41 = 0xFF;
	R_NR42 = 0xF0;
	R_NR43 = 0x00;
	R_NR44 = 0xBF;

	R_NR50 = 0x77;
	R_NR51 = 0xFF;
	R_NR52 = 0xF0;
	sound_dirty();

   pcm.gbcch[0].on = pcm.gbcch[1].on = pcm.gbcch[2].on = pcm.gbcch[3].on = 0;
   RATE = pcm.bytesFPS * GBA_FPS;
   pcm.fps = 0;
   pcm.Inc = 0;
   pcm.LastWriteBytes = 0;
   ZeroMemory(pcm.wave,16);
   ResetPlayFifo();
}
//---------------------------------------------------------------------------
void no_sound_mix(u8 forceUpdate)
{
   pcm.Inc = 0;
}
//---------------------------------------------------------------------------
void sound_mix(u8 forceUpdate)
{
	int f;
   u8 *p,l,r,s,maskVolumeL,maskVolumeR;
   u16 valueAdd;
   LPGBCCHANNEL pch;
   int cycles;

   if((cycles = (pcm.cycles / RATE) * RATE) == 0 && forceUpdate)
       cycles = pcm.cycles;
   pcm.cycles -= cycles;
   if(pcm.Enable <= 0)
       return;
   p = &pcm.lpBuffer[pcm.Inc];
   switch(io_ram_u8[0x82] & 3){
       case 0:
           maskVolumeL = 25;
       break;
       case 1:
           maskVolumeL = 50;
       break;
       case 2:
           maskVolumeL = 100;
       break;
       case 3:
           maskVolumeL = 0;
       break;
   }
   maskVolumeR = (u8)(maskVolumeL * pcm.volR >> 3);
   maskVolumeL = (u8)(maskVolumeL * pcm.volL >> 3);
   for(;cycles > 0;cycles -= RATE){
       l = r = 0;
       pch = pcm.gbcch;
       valueAdd = (u16)(cycles > RATE ? RATE : cycles);
	    if(pch->on){
           if(pch->pos < pch->dutyCycle)
               s = (u8)pch->value;
           else
               s = (u8)(0 - pch->value);
           pch->pos += valueAdd;
           if((pch->frcnt += valueAdd) >= pch->freq)
               pch->pos = (pch->frcnt %= pch->freq);
           if(pch->timed){
               if((pch->cnt += valueAdd) >= pch->len)
                   pch->on = 0;
           }
		    if(pch->enlen && (pch->encnt += valueAdd) >= pch->enlen){
               pch->encnt %= pch->enlen;
			    pch->envol += pch->endir;
               pch->writeable = 1;
               if (pch->envol < 1){
                   pch->envol = 0;
                   pch->writeable = 0;
               }
               else if (pch->envol > 15)
                   pch->envol = 15;
               CalcEnergy(pch);
           }
           if(pch->swlen && (pch->swcnt += valueAdd) >= pch->swlen){
               pch->swcnt %= pch->swlen;
               if((f = R_NR10 & 7) != 0){
                   pch->freq = (2048 - (SOUND1CNT_X & 0x7FF)) << 7;
                   f = 2048 - ((pch->freq + (int)((pch->freq >> f) * pch->swdir)) >> 7);
                   if(f > 2047)
                       pch->on = 0;
                   else if(f > 0){
                       R_NR13 = (u8)f;
				        R_NR14 = (u8)((R_NR14 & 0xF8) | (f >> 8));
                       s1_freq();
                   }
               }
           }
           if(pch->writeable){
		        if(pch->enableL > 0)
                   r += s;
               if(pch->enableR > 0)
                   l += s;
           }
       }
	    if((++pch)->on){
           if(pch->pos < pch->dutyCycle)
               s = (u8)pch->value;
           else
               s = (u8)(0 - pch->value);
           pch->pos += valueAdd;
           if((pch->frcnt += valueAdd) >= pch->freq)
               pch->pos = (pch->frcnt %= pch->freq);
           if(pch->timed){
               if((pch->cnt += valueAdd) >= pch->len)
                   pch->on = 0;
           }
		    if(pch->enlen && (pch->encnt += valueAdd) >= pch->enlen){
			    pch->encnt %= pch->enlen;
			    pch->envol += pch->endir;
               pch->writeable = 1;
               if (pch->envol < 1){
                   pch->envol = 0;
                   pch->writeable = 0;
               }
               else if (pch->envol > 15)
                   pch->envol = 15;
               CalcEnergy(pch);
           }
           if(pch->writeable){
		        if (pch->enableL > 0)
                   r += s;
               if (pch->enableR > 0)
                   l += s;
           }
       }
       if((++pch)->on){
           s = WAVE[pch->pos & 0xF];
		    if((pch->pos & 1))
               s &= 15;
           else
               s >>= 4;
           if((pch->frcnt += valueAdd) >= pch->freq){
               if(!(pch->pos & 0x7))
                   R_NR30 &= ~0x40;
               else
                   R_NR30 |= 0x40;
               pch->pos++;
               pch->frcnt %= pch->freq;
           }
           if(pch->timed){
               if((pch->cnt += valueAdd) >= pch->len)
                   pch->on = 0;
           }
           pch->writeable = (u8)((pch->envol != 0) ? 1 : 0);
           s = (u8)(s * pch->envol / 100);
           if(pch->writeable){
               if(pch->enableL > 0)
                   r += s;
               if(pch->enableR > 0)
                   l += s;
           }
       }
	    if((++pch)->on){
           if((pch->frcnt += valueAdd) >= pch->freq){
               if(pch->value2)
                   s = (u8)((lfsr7[pch->pos % 19] >> (pch->pos & 0x7)) & 1);
               else
                   s = (u8)((lfsr15[pch->pos % 2185] >> (pch->pos & 0xF)) & 1);
               if(s)
                   pch->value1 = (u8)pch->value;
               else
                   pch->value1 = (u8)-pch->value;
               pch->pos++;
               if(pch->freq)
                   pch->frcnt %= pch->freq;
           }
           s = pch->value1;
           if(pch->timed){
               if((pch->cnt += valueAdd) >= pch->len)
                   pch->on = 0;
           }
		    if(pch->enlen && (pch->encnt += valueAdd) >= pch->enlen){
			    pch->encnt %= pch->enlen;
			    pch->envol += pch->endir;
               pch->writeable = 1;
               if (pch->envol < 1){
                   pch->envol = 0;
                   pch->writeable = 0;
               }
               else if (pch->envol > 15)
                   pch->envol = 15;
               CalcEnergy(pch);
           }
           if(pch->writeable){
               if(pch->enableL > 0)
                   r += s;
               if(pch->enableR > 0)
                   l += s;
           }
       }
       pcm.Update = 1;
	    *p++ = (u8)(l * maskVolumeL / 25);
	    *p++ = (u8)(r * maskVolumeR / 25);
       if((pcm.Inc += 2) >= 0x7FFF){
           UpdateFifo();
           p = pcm.lpBuffer;
       }
   }
   R_NR52 = (u8)((R_NR52 & 0xf0)| gbcch[0].on | (gbcch[1].on<<1) | (gbcch[2].on << 2) | (gbcch[3].on<<3));
}
//---------------------------------------------------------------------------
static void EnableFifo(LPFIFOCHANNEL pFifo,u8 value)
{
	u8 start,max;

	pFifo->Enable = (u8)((pFifo->Enable & 0x80) | value);
	switch(value){
   	case 1:
       	start = 0;
           max = 4;
       break;
       case 2:
       	start = 0;
           max = 4;
       break;
       case 3:
       	start = 0;
       	max = 4;
       break;
   }
   pFifo->Start = start;
   pFifo->Max = max;
}
//---------------------------------------------------------------------------
static void InitGBChannel(LPGBCCHANNEL p,BOOL bEnable)
{
   u8 i,i1;

   switch(p->Index){
       case 1:
           p->dutyCycle = ((p->freq * percDuty[R_NR11 >> 6]));
		    p->swlen = ((R_NR10 >> 4) & 7) << 17;
           p->swdir = (s8)((R_NR10 & 0x8) ? -1 : 1);
		    p->len = (64 - (R_NR11 & 0x3F)) << 16;
           p->endir = (s8)((R_NR12 & 0x8) ? 1 : -1);
           p->envol = (u8)(R_NR12 >> 4);
           CalcEnergy(p);
		    p->enlen = (R_NR12 & 7) << 18;
           p->timed = (u8)((R_NR14 & 0x40) >> 6);
           p->encnt = 0;
           p->cnt = 0;
           p->swcnt = 0;
       break;
       case 2:
           p->dutyCycle = ((p->freq * percDuty[R_NR21 >> 6]));
		    p->len = (64 - (R_NR21 & 0x3F)) << 16;
		    p->envol = (u8)(R_NR22 >> 4);
		    p->enlen = (R_NR22 & 7) << 18;
           p->endir = (s8)((R_NR22 & 0x8) ? 1 : -1);
           p->timed = (u8)((R_NR24 & 0x40) >> 6);
           CalcEnergy(p);
           p->encnt = 0;
           p->cnt = 0;
       break;
       case 3:
           p->len = (256 - R_NR31) << 16;
           p->cnt = 0;
           p->timed = (u8)((R_NR34 & 0x40) >> 6);
           if((R_NR30 & 0x20))
               p->value = 0;
           else{
               if((i = (u8)(((io_ram_u8[0x70] & 0x40) >> 6) << 5)) != p->value){
                   p->value = i;
                   for(i=0;i<15;i++){
                       i1 = WAVE[i];
                       WAVE[i] = io_ram_u8[i+0x90];
                       io_ram_u8[i+0x90] = i1;
                   }
               }
           }
           if(bEnable){
               for(i=0;i<15;i++)
                   io_ram_u8[0x90+i] = 0;
           }
           switch((R_NR32 >> 5)){
               case 1:
                   p->envol = 100;
               break;
               case 2:
                   p->envol = 50;
               break;
               case 3:
                   p->envol = 25;
               break;
               case 4:
                   p->envol = 75;
               break;
               default:
                   p->envol = 0;
               break;

           }
       break;
       case 4:
		    p->len = (0x40 - (R_NR41 & 0x3F)) << 16;
           p->envol = (u8)(R_NR42 >> 4);
           CalcEnergy(p);
		    p->endir = (s8)((R_NR42 & 0x8) ? 1 : -1);
		    p->enlen = (R_NR42 & 7) << 18;
           p->timed = (u8)((R_NR44 & 0x40) >> 6);
           p->encnt = 0;
           p->cnt = 0;
       break;
   }
   if(bEnable){
       p->on = TRUE;
       if(p->envol > 0 || (p->enlen > 0 && p->timed))
           p->writeable = TRUE;
       else
           p->writeable = FALSE;
   }
}
//---------------------------------------------------------------------------
void sound_write(u16 r,u8 accessMode)
{
	switch (r){
	    case RI_NR10:
	    case RI_NR11:
       case RI_NR12:
           InitGBChannel(&gbcch[0],FALSE);
       break;
	    case RI_NR13:
       case RI_NR14:
		    s1_freq();
           if((R_NR14 & 0x80)){
               InitGBChannel(&gbcch[0],TRUE);
               R_NR14 &= (u8)~0x80;
               R_NR10 &= ~8;
           }
		break;
	    case RI_NR21:
       case RI_NR22:
           InitGBChannel(&gbcch[1],FALSE);
		break;
	    case RI_NR23:
       case RI_NR24:
		    s2_freq();
           if((R_NR24 & 0x80)){
               InitGBChannel(&gbcch[1],TRUE);
               R_NR24 &= (u8)~0x80;
           }
		break;
	    case RI_NR30:
		    if(!(R_NR30 & 128))
               S3.on = 0;
           else
               InitGBChannel(&gbcch[2],FALSE);
		break;
	    case RI_NR31:
       case RI_NR32:
           InitGBChannel(&gbcch[2],FALSE);
       break;
	    case RI_NR33:
       case RI_NR34:
		    s3_freq();
		    if((R_NR34 & 0x80)){
               InitGBChannel(&gbcch[2],(R_NR30 >> 7));
               R_NR34 &= (u8)~0x80;
           }
		break;
	    case RI_NR41:
       case RI_NR42:
           InitGBChannel(&gbcch[3],FALSE);
       break;
	    case RI_NR43:
		    s4_freq();
           gbcch[3].value2 = (u8)(R_NR43 & 0x8);
		    if(R_NR44 & 0x80){
               R_NR44 &= (u8)~0x80;
               InitGBChannel(&gbcch[3],TRUE);
           }
       break;
       case RI_NR50:
       case RI_NR51:
           r = R_NR50;
           pcm.volR = (u8)(((r & 7) << 3) / 7);
           pcm.volL = (u8)((((r >> 4) & 7) << 3) / 7);
           r = R_NR51;
           gbcch[0].enableL = (u8)((gbcch[0].enableL & 0x80) | ((r & 1) != 0 ? 1 : 0));
           gbcch[1].enableL = (u8)((gbcch[1].enableL & 0x80) | ((r & 2) != 0 ? 1 : 0));
           gbcch[2].enableL = (u8)((gbcch[2].enableL & 0x80) | ((r & 4) != 0 ? 1 : 0));
           gbcch[3].enableL = (u8)((gbcch[3].enableL & 0x80) | ((r & 8) != 0 ? 1 : 0));
           gbcch[0].enableR = (u8)((gbcch[0].enableR & 0x80) | ((r & 16) != 0 ? 1 : 0));
           gbcch[1].enableR = (u8)((gbcch[1].enableR & 0x80) | ((r & 32) != 0 ? 1 : 0));
           gbcch[2].enableR = (u8)((gbcch[2].enableR & 0x80) | ((r & 64) != 0 ? 1 : 0));
           gbcch[3].enableR = (u8)((gbcch[3].enableR & 0x80) | ((r & 128) != 0 ? 1 : 0));
       break;                                                        //803034a
	    case RI_NR52:
		    if(!(R_NR52 & 0x80))
			    sound_off();
           else
               sound_on();
       break;
       case RI_SGCNT0_H:
       case 0x83:
           if((SGCNT0_H & 0x800)){
               SGCNT0_H &= ~0x800;
     			SGCNT0_H |= 0x300;                                 //80001354
           }
           if((SGCNT0_H & 0x8000)){
               SGCNT0_H &= (u16)~0x8000;
     			SGCNT0_H |= 0x3000;
           }
           fifo[0].Volume = (u8)((SGCNT0_H & 0x4) ? 0 : 1);
			EnableFifo(&fifo[0],(u8)((SGCNT0_H & 0x300) >> 8));
           fifo[0].Timer = (u8)((SGCNT0_H & 0x400) >> 10);
           fifo[1].Volume = (u8)((SGCNT0_H & 0x8) ? 0 : 1);
			EnableFifo(&fifo[1],(u8)((SGCNT0_H & 0x3000) >> 12));
           fifo[1].Timer = (u8)((SGCNT0_H & 0x4000) >> 14);
           CalcFifoFreq(&timer[fifo[0].Timer]);
           CalcFifoFreq(&timer[fifo[1].Timer]);
       break;
       case RI_SGBIAS:
           switch(((SG_BIAS >> 14) & 3)){
               case 0:
                   pcm.Freq = 32768;
               break;
               case 1:
                   pcm.Freq = 65536;
               break;
               case 2:
                   pcm.Freq = 131072;
               break;
               case 3:
                   pcm.Freq = 262144;
               break;
           }
//           SetFifoFreq(0,fifo[0].Freq);
//           SetFifoFreq(1,fifo[1].Freq);
       break;
       case 0xA0:
           WriteFifoChannel(&fifo[0]);
       break;
       case 0xA4:
           WriteFifoChannel(&fifo[1]);
       break;
	}
}
//---------------------------------------------------------------------------
static void WriteFifoChannel(LPFIFOCHANNEL pFifo)
{
   u8 *p,*p1,i,vol,start;
   u32 f,max;

   if(pcm.Enable <= 0 || pFifo->Enable <= 0 || pFifo->Resample == 0)
       return;
   vol = pFifo->Volume;
   p = &pFifo->lpBuffer[pFifo->Inc];
   p1 = pFifo->lpAddress + (start = pFifo->Start);
	max = (pFifo->Max - start) << 8;
	for(i=0,f = pFifo->Pos;f < max;i++){                                 
		*p++ = (u8)((s8)p1[f >> 8] >> vol);
       f += pFifo->Resample;
   }
   pFifo->Pos = f - max;
   pcm.Update = 1;
   if((pFifo->Inc += i) >= pFifo->MaxPos)
       UpdateFifo();
}
//---------------------------------------------------------------------------
void ResetPlayFifo()
{
   HRESULT res;
   DWORD dwByte1,dwByte2,dwPlay;
   LPVOID mem1,mem2;

   if(pPlugInContainer != NULL)
       pAudioPlugList = pPlugInContainer->GetAudioPlugInList();
   if(lpBuffer != NULL){
       if(pcm.bStart != 0){
           res = IDirectSoundBuffer_GetCurrentPosition(lpBuffer,&dwPlay,NULL);
           if(res == DS_OK)
               SleepAt(&dwPlay,NULL);
       }
       PauseRestartSound(0x10);
       res = IDirectSoundBuffer_Lock(lpBuffer,0,0,&mem1,&dwByte1,&mem2,&dwByte2,DSBLOCK_ENTIREBUFFER|DSBLOCK_FROMWRITECURSOR);
       if(res == DS_OK){
           FillMemory(mem1,dwByte1,0x80);
           if(mem2 != NULL)
               FillMemory(mem2,dwByte2,0x80);
           IDirectSoundBuffer_Unlock(lpBuffer,mem1,dwByte1,mem2,dwByte2);
       }
       pAudioPlugList->Reset();
   }
   pcm.LastWriteEnd = pcm.LastPlay  = pcm.LastWriteBytes = 0;
   pcm.bStart = pcm.isPlay = 0;
}
//---------------------------------------------------------------------------
HRESULT PauseRestartSound(u8 flag)
{
   HRESULT res;
   u8 com,subcom;

   if(lpBuffer == NULL)
       return E_FAIL;
   com = (u8)(flag & 0xF);
   subcom = (u8)(flag >> 4);
   if(!com){
       res = IDirectSoundBuffer_Stop(lpBuffer);
       pcm.bStart = 0;
       if(pcm.isPlay)
           pPlugInContainer->NotifyState((DWORD)-1,PIS_PAUSE);
       if(!subcom)
           pcm.isPlay = 0;
   }
   else{
       if((!pcm.bStart && !subcom) || (pcm.isPlay && subcom == 1)){
           res = IDirectSoundBuffer_Play(lpBuffer,0,0,DSBPLAY_LOOPING);
           if(pAudioPlugList != NULL && pcm.Inc)
               pAudioPlugList->NotifyState(PIS_RUN);
           pPlugInContainer->NotifyState(PIL_VIDEO|PIL_SIO,PIS_RUN);
           pcm.isPlay = pcm.bStart = 1;
       }
   }
   pcm.LastWriteEnd = pcm.LastPlay  = pcm.LastWriteBytes = 0;
   return res;
}
//---------------------------------------------------------------------------
u8 StartPlayFifo()
{
   if(pcm.bStart != 0 || PauseRestartSound(1) == DS_OK)
       return 1;
   return 0;
}
/*//---------------------------------------------------------------------------
static int GetResampleSize(int i,int f0Inc,int f1Inc,int gcInc,LPDWORD presFifo,LPDWORD presGbc)
{
   int res,i2;
   u32 resFifo,resGbc;
   u8 resampleAll;

   resFifo = resGbc = 0x100;
   i2 = res = i;
   if(gcInc == 0 || i == 0 || i == gcInc)
       goto ex_GetResampleSize;
   resampleAll = 0;
   if(gcInc > i2){
       resGbc = (gcInc << 8) / i2;
       resFifo = 0x100;
       if(resGbc > 0x140)
           resampleAll = 1;
   }
   else if(gcInc < i2){
       resGbc = 0x100;
       resFifo = (i2 << 8) / gcInc;
       if(resFifo > 0x140)
           resampleAll = 1;
   }
   if(resampleAll){
       if((res = (gcInc + i2) >> 1) & 1)
           res++;
       resGbc = (gcInc << 8) / res;
       resFifo = (i2 << 8) / res;
   }
ex_GetResampleSize:
   if(presFifo)
       *presFifo = resFifo;
   if(presGbc)
       *presGbc = resGbc;
   return res;
}
//---------------------------------------------------------------------------
static void WriteSoundEx(LPVOID mem1,DWORD dwByte1,LPVOID mem2,int i,int f0Inc,int f1Inc,int gcInc)
{
   u8 *p,*p1,*p2,valueL,valueR,value,value1;
   int i2,sample,i1;
   u16 *p3;
   u32 rGbc,pGbc,pGbc2;
   u32 rFifo,pFifoL,pFifoL2;
   u32 pFifoR,pFifoR2;

   p = (u8 *)mem1;
   p1 = fifo[0].lpBuffer;
   p2 = fifo[1].lpBuffer;
   p3 = (u16 *)pcm.lpBuffer;

   pGbc = pGbc2 = 0;
   value = (u8)*p3;
   value1 = (u8)(*p3 >> 8);

   GetResampleSize(i,f0Inc,f1Inc,gcInc,&rFifo,&rGbc);

   pFifoL = pFifoL2 = pFifoR = pFifoR2 = 0;
   valueL = *p1;
   valueR = *p2;
   i2 = (int)dwByte1 > i ? i : dwByte1;
   for(i -= i2;i2 > 1;i2 -= 2){
       if(f0Inc > 0){
           if(((pFifoL2 += rFifo) >> 8) != pFifoL){
               valueL = p1[i1 = pFifoL];
               pFifoL = pFifoL2 >> 8;
               f0Inc -= (pFifoL - i1);
           }
       }
       else
           valueL = 0;
       if(f1Inc > 0){
           if(((pFifoR2 += rFifo) >> 8) != pFifoR){
               valueR = p2[i1 = pFifoR];
               pFifoR = pFifoR2 >> 8;
               f1Inc -= (pFifoR - i1);
           }
       }
       else
           valueR = 0;
       if(gcInc > 0){
           if(((pGbc2 += rGbc) >> 8) != pGbc){
               value = (u8)(sample = p3[i1 = pGbc]);
               value1 = (u8)(sample >> 8);
               pGbc = pGbc2 >> 8;
               gcInc -= (pGbc - i1) << 1;
           }
           if((sample = (int)(s8)valueL + (s8)value) > 127)
               sample = 127;
           else if(sample < -128)
               sample = -128;
           valueL = (u8)sample;
           if((sample = (int)(s8)valueR + (s8)value1) > 127)
               sample = 127;
           else if(sample < -128)
               sample = -128;
           valueR = (u8)sample;
       }
       *p++ = (u8)(valueL - 128);
       *p++ = (u8)(valueR - 128);
   }
   for(p = (u8 *)mem2;i > 1;i -= 2){
       if(f0Inc > 0){
           if(((pFifoL2 += rFifo) >> 8) != pFifoL){
               valueL = p1[i1 = pFifoL];
               pFifoL = pFifoL2 >> 8;
               f0Inc -= (pFifoL - i1);
           }
       }
       else
           valueL = 0;
       if(f1Inc > 0){
           if(((pFifoR2 += rFifo) >> 8) != pFifoR){
               valueR = p2[i1 = pFifoR];
               pFifoR = pFifoR2 >> 8;
               f1Inc -= (pFifoR - i1);
           }
       }
       else
           valueR = 0;
       if(gcInc > 0){
           if(((pGbc2 += rGbc) >> 8) != pGbc){
               value = (u8)(sample = p3[i1 = pGbc]);
               value1 = (u8)(sample >> 8);
               pGbc = pGbc2 >> 8;
               gcInc -= (pGbc - i1) << 1;
           }
           if((sample = (int)(s8)valueL + (s8)value) > 127)
               sample = 127;
           else if(sample < -128)
               sample = -128;
           valueL = (u8)sample;
           if((sample = (int)(s8)valueR + (s8)value1) > 127)
               sample = 127;
           else if(sample < -128)
               sample = -128;
           valueR = (u8)sample;
       }
       *p++ = (u8)(valueL - 128);
       *p++ = (u8)(valueR - 128);
   }
}*/
//---------------------------------------------------------------------------
static void WriteSound(LPVOID mem1,DWORD dwByte1,LPVOID mem2,int i,int f0Inc,int f1Inc,int gcInc)
{
   u8 *p,*p1,*p2,valueL,valueR,value,value1;
   int i2,sample,i1;
   u16 *p3;
   u32 rGbc,pGbc,pGbc2;
   DWORD dwSize1,dwSize2;

   p = (u8 *)mem1;
   p1 = fifo[0].lpBuffer;
   p2 = fifo[1].lpBuffer;
   p3 = (u16 *)pcm.lpBuffer;

   pGbc = pGbc2 = 0;
   value = (u8)*p3;
   value1 = (u8)(*p3 >> 8);
   if(i && gcInc){
       if((rGbc = (gcInc << 8) / i) < 0x100)
           rGbc = 0x100;
   }
   else
       rGbc = 0x100;
   i2 = (int)dwByte1 > i ? i : dwByte1;
   dwSize1 = (DWORD)i2;
   dwSize2 = (DWORD)(i - i2);
   for(i -= i2;i2 > 1;i2 -= 2){
       valueL = (u8)(f0Inc-- > 0 ? *p1++ : 0x0);
       valueR = (u8)(f1Inc-- > 0 ? *p2++ : 0x0);
       if(gcInc > 0){
           if(((pGbc2 += rGbc) >> 8) != pGbc){
               value = (u8)(sample = p3[i1 = pGbc]);
               value1 = (u8)(sample >> 8);
               pGbc = pGbc2 >> 8;
               gcInc -= (pGbc - i1) << 1;
           }
           if((sample = (int)(s8)valueL + (s8)value) > 127)
               sample = 127;
           else if(sample < -128)
               sample = -128;
           valueL = (u8)sample;
           if((sample = (int)(s8)valueR + (s8)value1) > 127)
               sample = 127;
           else if(sample < -128)
               sample = -128;
           valueR = (u8)sample;
       }
       *p++ = (u8)(valueL-128);
       *p++ = (u8)(valueR-128);
   }
   for(p = (u8 *)mem2;i > 1;i -= 2){
       valueL = (u8)(f0Inc-- > 0 ? *p1++ : 0x0);
       valueR = (u8)(f1Inc-- > 0 ? *p2++ : 0x0);
       if(gcInc > 0){
           if(((pGbc2 += rGbc) >> 8) != pGbc){
               value = (u8)(sample = p3[i1 = pGbc]);
               value1 = (u8)(sample >> 8);
               pGbc = pGbc2 >> 8;
               gcInc -= (pGbc - i1) << 1;
           }
           if((sample = (int)(s8)valueL + (s8)value) > 127)
               sample = 127;
           else if(sample < -128)
               sample = -128;
           valueL = (u8)sample;
           if((sample = (int)(s8)valueR + (s8)value1) > 127)
               sample = 127;
           else if(sample < -128)
               sample = -128;
           valueR = (u8)sample;
       }
       *p++ = (u8)(valueL-128);
       *p++ = (u8)(valueR-128);
   }
   pAudioPlugList->Run(mem1,dwSize1,mem2,dwSize2);
}
//---------------------------------------------------------------------------
static BOOL SleepAt(LPDWORD pdwPlay,LPDWORD pdwWrite)
{
   DWORD dwPause,dwWrite;
   BOOL res;

   res = FALSE;
   if(!pcm.LastWriteEnd)
       return res;
   dwPause = 0;
   if(pcm.LastPlay <= pcm.LastWriteEnd){
       if(*pdwPlay <= pcm.LastWriteEnd){
           dwWrite = pcm.LastWriteEnd;
           dwPause = pcm.LastWriteEnd - *pdwPlay;
           res = TRUE;
       }
   }
   else{
       if(*pdwPlay <= pcm.LastWriteEnd || *pdwPlay > pcm.LastPlay){
           dwWrite = pcm.LastWriteEnd;
           res = TRUE;
           if(*pdwPlay <= pcm.LastWriteEnd)
               dwPause = pcm.LastWriteEnd - *pdwPlay;
           else
               dwPause = (SOUND_LENGTH - *pdwPlay) + pcm.LastWriteEnd;
       }
   }
   if(dwPause > 3584 && dwPause < (SOUND_LENGTH * 2 / 3)){
       SleepEx(dwPause >> 7,FALSE);
       IDirectSoundBuffer_GetCurrentPosition(lpBuffer,pdwPlay,NULL);
   }
   if(res && pdwWrite)
      *pdwWrite = dwWrite;
   return res;
}
//---------------------------------------------------------------------------
void UpdateFifo()
{
   HRESULT res;
   u8 *p,Update,bErase;
   DWORD dwByte1,dwByte2,dwWrite,dwPlay,dwLock,dwErase,dw;
   LPVOID mem1,mem2;
   int i,f0Inc,f1Inc,gcinc;//i1;

   i = (f0Inc = fifo[0].Inc) + (f1Inc = fifo[1].Inc) + (gcinc = pcm.Inc);
   if(f0Inc != f1Inc)
       i = ((f0Inc > f1Inc ? f0Inc : f1Inc) << 1) + gcinc;
   if(bSaveWave){
       if(fpWave == NULL)
           return;
       if(i > gcinc)
           i -= gcinc;
       if(i == 0)
           return;
       if(dwLenWave == 0)
           writeWavHeader(fpWave,0);
       if((p = (u8*)GlobalAlloc(GMEM_FIXED,i)) != NULL){
           pcm.fpWriteSound((LPVOID)p,i,NULL,i,f0Inc,f1Inc,gcinc);
           fpWave->Write(p,i);
           GlobalFree((HGLOBAL)p);
           dwLenWave += i;
       }
       fifo[0].Inc = fifo[1].Inc = pcm.Inc = 0;
       return;
   }
   if(pcm.Enable <= 0)
       return;
   if(!bPass) goto ex_UpdateFifo;
   Update = pcm.Update;
   if(i == 0 && Update == 0){
       ResetPlayFifo();
       return;
   }
   if(pcm.bStart == 0){
       if((pcm.bStart = StartPlayFifo()) == 0)
           return;
   }
   pcm.Update = 0;
   if(!gcinc && i < FIFOBUFFER_SIZE - 16 && Update == 1)
      return;
   IDirectSoundBuffer_GetCurrentPosition(lpBuffer,&dwPlay,&dwWrite);
   bErase = 1;
   dwLock = DSBLOCK_ENTIREBUFFER;
   if(IsSyncroEnabled()){
       if(SleepAt(&dwPlay,&dwWrite)){
           bErase = 0;
           dwLock = 0;
       }
   }
   else
       bErase = 2;
   if(i > gcinc)
       i -= gcinc;
   if(i > (SOUND_LENGTH / 2))
       i = SOUND_LENGTH / 2;
   res = IDirectSoundBuffer_Lock(lpBuffer,dwWrite,i,&mem1,&dwByte1,&mem2,&dwByte2,dwLock);
   if(res != DS_OK)
       return;
   if(bErase == 1){
       FillMemory(mem1,dwByte1,128);
       if(mem2)
           FillMemory(mem2,dwByte2,128);
   }
   else if(bErase == 2){
       if(dwPlay <= dwWrite)
           dwErase = SOUND_LENGTH - (dwWrite - dwPlay);
       else
           dwErase = dwPlay + dwWrite;
       if((dw = dwErase) > dwByte1)
           dw = dwByte1;
       FillMemory(mem1,dw,128);
       dwErase -= dw;
       if(mem2 && dwErase){
           if(dwErase > dwByte2)
               dwErase = dwByte2;
           FillMemory(mem2,dwErase,128);
       }
   }
   pcm.fpWriteSound(mem1,dwByte1,mem2,i,f0Inc,f1Inc,gcinc);
   IDirectSoundBuffer_Unlock(lpBuffer,mem1,dwByte1,mem2,dwByte2);
   pcm.LastPlay = dwPlay;
   if((dwWrite += i) >= SOUND_LENGTH)
       dwWrite -= SOUND_LENGTH;
   pcm.LastWriteEnd = dwWrite;
   pcm.LastWriteBytes = i;
ex_UpdateFifo:
   pcm.Inc = fifo[0].Inc = fifo[1].Inc = 0;
}
//---------------------------------------------------------------------------
int GetSoundSubCodeError()
{
   return iSubCodeError;
}
//---------------------------------------------------------------------------
BOOL InitSSM()
{
   unsigned short noise,bit,value,valueShift;
   int i,i1;

   noise = value = 0x7F;
   i = i1 = 0;
   valueShift = 6;
   do{
       if(!(i++ % 7))
           lfsr7[i1++] = (u8)noise;
       bit = (unsigned short)(noise & 1);
       bit = (unsigned short)((bit ^ ((noise >>= 1) & 1)) << valueShift);
       noise |= bit;
   }while(noise != value);

   noise = value = 0x7FFF;
   i = i1 = 0;
   valueShift = 14;
   do{
       if(!(i++ % 15))
           lfsr15[i1++] = noise;
       bit = (unsigned short)(noise & 1);
       bit = (unsigned short)((bit ^ ((noise >>= 1) & 1)) << valueShift);
       noise |= bit;
   }while(noise != value);
   pcm.lpBuffer = (u8*)GlobalAlloc(GPTR,0x8000);
   if(pcm.lpBuffer == NULL)
       return FALSE;
   for(i=0;i<4;i++)
       pcm.gbcch[i].Index = (u8)(i + 1);
   ZeroMemory(pcm.wave,16);
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL EnableSound()
{
   HRESULT res;

   pcm.lpBuffer = NULL;
   ZeroMemory(&wf,sizeof(WAVEFORMATEX));
   lpBuffer = NULL;
   iSubCodeError = -1;
   res = ::CoCreateInstance(CLSID_DirectSound,NULL,CLSCTX_INPROC_SERVER,IID_IDirectSound,(LPVOID *)&lpDS);
   if(res != DS_OK)
       return FALSE;
   res = lpDS->Initialize(NULL);                                      
   if(res != DS_OK)
       return FALSE;
   iSubCodeError = -2;
   res = lpDS->SetCooperativeLevel(hWin,DSSCL_PRIORITY);
   if(res != DS_OK)
       return FALSE;

   wf.wFormatTag       = WAVE_FORMAT_PCM;
   wf.nChannels        = 2;
   wf.nSamplesPerSec   = 22050;
   wf.nAvgBytesPerSec  = 44100;
   wf.nBlockAlign      = 2;
   wf.wBitsPerSample   = 8;

	ZeroMemory(&dsBD,sizeof(DSBUFFERDESC));
	dsBD.dwSize = sizeof(dsBD);
	dsBD.dwFlags = DSBCAPS_GETCURRENTPOSITION2|DSBCAPS_GLOBALFOCUS;
   dsBD.dwBufferBytes = SOUND_LENGTH;
	dsBD.lpwfxFormat = (LPWAVEFORMATEX)&wf;

   iSubCodeError = -3;
   res = lpDS->CreateSoundBuffer(&dsBD,(LPDIRECTSOUNDBUFFER *)&lpBuffer,NULL);
   if(res != DS_OK)
       return FALSE;
   iSubCodeError = -5;
   pcm.hz = 22050;
   pcm.bytesFPS = (280896.0 / (double)pcm.hz) * .95;
   pcm.Stereo = 1;
   RATE = pcm.bytesFPS * GBA_FPS;
   pcm.bitsPerSample = 8;
   if(!InitSSM())
       return FALSE;
   SetResampleFunction((u8)bResampleMix);
   SetSoundGbcFunction(0);
   fpWave = NULL;
   dwLenWave = 0;
   iSubCodeError = 0;

   return TRUE;
}
//---------------------------------------------------------------------------
BOOL OnInitAudioPlugMenu(HMENU menu)
{
   return pPlugInContainer->GetAudioPlugInList()->OnInitMenu(menu);
}
//---------------------------------------------------------------------------
BOOL EnableSoundPlug(WORD wID)
{
   return pPlugInContainer->GetAudioPlugInList()->OnEnablePlug(wID);
}
//---------------------------------------------------------------------------
void SetSoundGbcFunction(u8 value)
{
   UINT flag;
   HMENU hMenu;

   if(!value){
       pcm.fpSoundMix = sound_mix;
       flag = MF_ENABLED;
   }
   else{
       pcm.fpSoundMix = no_sound_mix;
       flag = MF_GRAYED;
   }
   hMenu = GetMenu(hWin);
   for(value = 0;value < 4;value++)
       EnableMenuItem(hMenu,ID_SOUND_GBC1 + value,MF_BYCOMMAND|flag);
}
//---------------------------------------------------------------------------
void SetResampleFunction(u8 value)
{
   if(!value)
       bResampleMix = 0;
   else
       bResampleMix = 1;
   if(!bResampleMix)
       pcm.fpWriteSound = WriteSound;
   else
       pcm.fpWriteSound = WriteSound;//WriteSoundEx;
}
//---------------------------------------------------------------------------
void SetFrequencyRate(int value)
{
   if(abs(pcm.fps - ((pcm.fps + value) >> 1)) < 3 || abs(pcm.fps - value) > 15)
       return;
   pcm.fps = value;
   if((RATE = pcm.bytesFPS * value) > 1232)
       RATE = 1232;
   else if(RATE < 64)
       RATE = 64;
}
//---------------------------------------------------------------------------
void DisableSound()
{
	StopSoundSave();
   ResetPlayFifo();
   if(pcm.lpBuffer != NULL)
       GlobalFree((HGLOBAL)pcm.lpBuffer);
   pcm.lpBuffer = NULL;
   RELEASE(lpBuffer);
   RELEASE(lpDS);
}
//---------------------------------------------------------------------------
BOOL EnableSoundSave()
{
   char szFile[MAX_PATH];
   BOOL res;
   LString c;

   if(fpWave != NULL)
       return TRUE;
   dwLenWave = 0;
   bSaveWave = FALSE;
   lstrcpy(szFile,"rascal.wav");
   c = TranslateGetMessage(0xF002);
   PauseRestartSound(0x10);
   res = ShowSaveDialog(c.c_str(),szFile,"Files audio (*.wav)\0*.wav\0\0\0\0\0",NULL,(LPFNCSAVEDLG)InitSaveDlg);
   if(!res){
       PauseRestartSound(1);
       return FALSE;
   }
   iSubCodeError = -4;
   c = szFile;
   c.AddEXT(".wav");
   waveFileName = "";
   fpWave = new LFile(c.c_str());
   if(fpWave == NULL || !fpWave->Open(GENERIC_READ|GENERIC_WRITE,CREATE_ALWAYS)){
       ShowMessageError(TE_SOUND,waveFileName.c_str());
       iSubCodeError = 0;
       if(fpWave != NULL)
           delete fpWave;
       fpWave = NULL;
       return FALSE;
   }
   waveFileName = c;
   bSaveWave = TRUE;
   iSubCodeError = 0;
	ResetPlayFifo();
   return TRUE;
}
//---------------------------------------------------------------------------
static UINT APIENTRY OFNHookProc(HWND hdlg,UINT uiMsg,WPARAM wParam,LPARAM lParam)
{
   RECT rc,rc1,rcParent;
   HWND hwnd,hwndParent;
   int y;
   LString s;
   LPOPENFILENAME pofn;

   switch(uiMsg){
       case WM_NOTIFY:
           if(((LPNMHDR)lParam)->code == CDN_INITDONE){
               hwndParent = GetParent(hdlg);
               GetWindowRect(GetDlgItem(hwndParent,cmb1),&rc);
               hwnd = GetDlgItem(hdlg,IDC_WMA);
               s = TranslateGetMessage(IDC_WMA);
               SetWindowText(hwnd,s.c_str());
               GetWindowRect(hwnd,&rc1);
               rc1.left = rc.left;
               rc1.bottom -= rc1.top;
               GetWindowRect(hwndParent,&rcParent);
               rc1.top = rc.bottom + 2;
               ScreenToClient(hwndParent,(LPPOINT)&rc1);
               SetWindowPos(hwnd,NULL,rc1.left,rc1.top,0,0,SWP_NOSIZE|SWP_NOSENDCHANGING|SWP_NOREPOSITION);
           }
       break;
       case WM_COMMAND:
           switch(LOWORD(wParam)){
               case IDC_WMA:
                   bSaveCompressed = SendMessage((HWND)lParam,BM_GETCHECK,0,0) == BST_CHECKED ? TRUE : FALSE;
               break;
           }
       break;
   }
   return 0;
}
//---------------------------------------------------------------------------
static BOOL WINAPI InitSaveDlg(LPOPENFILENAME pofn)
{
   pofn->Flags |= OFN_EXPLORER|OFN_ENABLEHOOK|OFN_ENABLETEMPLATE;
   pofn->hInstance = hInstance;
   pofn->lpTemplateName = MAKEINTRESOURCE(IDD_DIALOG12);
   pofn->lpfnHook = OFNHookProc;
//   pofn->FlagsEx = 1;
   bSaveCompressed = FALSE;
   return TRUE;
}
//---------------------------------------------------------------------------
void StopSoundSave()
{
   int err;

	bSaveWave = FALSE;
   pcm.fps = 0;
   if(fpWave == NULL)
       return;
   writeWavHeader(fpWave,dwLenWave);
   delete fpWave;
   fpWave = NULL;
   if(bSaveCompressed && !waveFileName.IsEmpty()){
       if((err = toWma(waveFileName.c_str(),&wf,dwLenWave)) < 0){
           iSubCodeError = -6;
           ShowMessageError(TE_SOUND,err);
       }
   }
   waveFileName = "";
   dwLenWave = 0;
}
//---------------------------------------------------------------------------
void writeWavHeader(LStream *fp,DWORD len)
{
   WAVHDR wav;

   if(fp == NULL || !fp->IsOpen())
       return;
   CopyMemory(wav.riff,"RIFF",4);
   wav.len = len + 44 - 8;
   CopyMemory(wav.cWavFmt,"WAVEfmt ",8);
   wav.dwHdrLen        = 16;
   wav.wFormat         = WAVE_FORMAT_PCM;
   wav.wNumChannels    = wf.nChannels;
   wav.dwSampleRate    = wf.nSamplesPerSec;
   wav.dwBytesPerSec   = wf.nAvgBytesPerSec;
   wav.wBlockAlign     = wf.nBlockAlign;
   wav.wBitsPerSample  = wf.wBitsPerSample;
   CopyMemory(wav.cData,"data", 4);
   wav.dwDataLen = len;
   fp->SeekToBegin();
   fp->Write(&wav,sizeof(wav));
}
//---------------------------------------------------------------------------
static int toWma(char *FileName,LPWAVEFORMATEX wf,DWORD dwLen)
{
   wchar_t wDestFileName[MAX_PATH],wSourceFileName[MAX_PATH];
   HRESULT hr;
   LString c;
   LONG_PTR lp1, lp2;
   IGraphBuilder *pGraph;
   IFileSinkFilter *pFileSink;
   IBaseFilter *pASFWriter;
   IConfigAsfWriter *pConfigAsfWriter;
   IMediaControl *pMC;
   IMediaFilter *pFilter;
   IMediaEvent *pME;
   IMediaPosition *pMP;
   HANDLE hEvent;
   LONG levCode;
   int err;
   DWORD dw,dw2,dw3;
   REFTIME tm;

   pME = NULL;
   pMC = NULL;
   pFilter = NULL;
   pGraph = NULL;
   pFileSink = NULL;
   pASFWriter = NULL;
   pConfigAsfWriter = NULL;
   pMP = NULL;

   err = -1;
   hr = ::CoCreateInstance(CLSID_FilterGraph,NULL,CLSCTX_INPROC_SERVER,IID_IGraphBuilder,(LPVOID *)&pGraph);
   if(hr != S_OK)
       goto ex_towma;
   err = -2;
   hr = ::CoCreateInstance(CLSID_WMAsfWriter,NULL,CLSCTX_INPROC_SERVER,IID_IBaseFilter,(LPVOID *) &pASFWriter);
   if(hr != S_OK)
       goto ex_towma;
   err = -3;
   hr = pASFWriter->QueryInterface(IID_IFileSinkFilter,(LPVOID *)&pFileSink);
   if(hr != S_OK)
       goto ex_towma;
   err = -4;
   c = waveFileName;
   c.AddEXT(".wma");
   ZeroMemory(wDestFileName,MAX_PATH);
   MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,c.c_str(),-1,wDestFileName,MAX_PATH);
   hr = pFileSink->SetFileName(wDestFileName, NULL);
   if(hr != S_OK)
       goto ex_towma;
   err = -5;
   hr = pGraph->AddFilter(pASFWriter, L"Mux");
   if(hr != S_OK)
       goto ex_towma;
   err = -6;
   hr = pASFWriter->QueryInterface(IID_IConfigAsfWriter,(LPVOID *)&pConfigAsfWriter);
   if(hr != S_OK)
       goto ex_towma;
   err = -7;
   hr = pConfigAsfWriter->ConfigureFilterUsingProfileGuid(WMProfile_V70_64Audio);
   if(hr != S_OK)
       goto ex_towma;
   err = -8;
   hr = pConfigAsfWriter->SetIndexMode(FALSE);
   if(hr != S_OK)
       goto ex_towma;
   err = -9;
   ZeroMemory(wSourceFileName,MAX_PATH);
   MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,waveFileName.c_str(),-1,wSourceFileName,MAX_PATH);
   hr = pGraph->RenderFile(wSourceFileName, NULL);
   if(hr != S_OK)
       goto ex_towma;
   err = -10;
   hr = pGraph->QueryInterface(IID_IMediaControl,(LPVOID *)&pMC);
   if(hr != S_OK)
       goto ex_towma;
   err = -11;
   hr = pGraph->QueryInterface(IID_IMediaFilter,(LPVOID *)&pFilter);
   if(hr != S_OK)
       goto ex_towma;
   err = -12;
   hr = pFilter->SetSyncSource(NULL);
   if(hr != S_OK)
       goto ex_towma;
   err = -13;
   hr = pMC->Run();
   if(hr != S_OK)
       goto ex_towma;
   err = -14;
   hr = pGraph->QueryInterface(IID_IMediaEvent,(LPVOID *)&pME);
   if(hr != S_OK)
       goto ex_towma;
   err = -15;
   hr = pME->GetEventHandle((OAEVENT *)&hEvent);
   if(hr != S_OK)
       goto ex_towma;
   for(;;){
       while(WaitForSingleObject(hEvent,100) != WAIT_OBJECT_0){
           hr = pGraph->QueryInterface(IID_IMediaPosition,(LPVOID *)&pMP);
           if(hr == S_OK){
               pMP->get_CurrentPosition(&tm);
               dw2 = (tm * wf->nAvgBytesPerSec * 100 / dwLen);
               DrawProgressBar(MAKELONG(dw2,100));
               pMP->Release();
           }
           ProcessaMessaggi();
       }
       if(pME->GetEvent(&levCode, &lp1, &lp2, 0) == S_OK){
           pME->FreeEventParams(levCode, lp1, lp2);
           if(EC_COMPLETE == levCode)
               break;
           else if(EC_ERRORABORT == levCode)
               break;
           else if(EC_USERABORT == levCode)
               break;
       }
   }
   pMC->Stop();
   if(levCode == EC_COMPLETE)
       err = 0;
   DrawProgressBar(MAKELONG(100,100));
ex_towma:
   if(pME != NULL)
       pME->Release();
   if(pMC != NULL)
       pMC->Release();
   if(pFilter != NULL)
       pFilter->Release();
   if(pFileSink != NULL)
       pFileSink->Release();
   if(pConfigAsfWriter != NULL)
       pConfigAsfWriter->Release();
   if(pASFWriter != NULL)
       pASFWriter->Release();
   if(pGraph != NULL)
       pGraph->Release();
   return err;
}















