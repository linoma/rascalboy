#include "gbaemu.h"
#include "memory.h"
#include "gba.h"
#include "input.h"

#ifndef __ioH__
#define __ioH__

#ifdef __cplusplus
extern "C" {
#endif

void joy_io_handle(u16 adress,u8 accessMode);
void wait_io_handle(u16 adress,u8 accessMode);
void com_io_handle(u16 adress,u8 accessMode);
void standard_io_write_handle(u16 adress,u8 accessMode);
void reg_dm3cnt_write(u16 adress,u8 accessMode);
void reg_dm2cnt_write(u16 adress,u8 accessMode);
void reg_dm1cnt_write(u16 adress,u8 accessMode);
void reg_dm0cnt_write(u16 adress,u8 accessMode);
void timer_io_handle(u16 adress,u8 accessMode);
void ime_io_handle(u16 adress,u8 accessMode);
void if_io_handle(u16 adress,u8 accessMode);
void power_mode(u16 adress,u8 accessMode);
void exec_dma(u16 value);
void ie_io_handle(u16 adress,u8 accessMode);
void reg_keyinput_write(u16 adress,u8 accessMode);

#ifdef __cplusplus
}
#endif

#endif