#include "gbaemu.h"
#include "resource.h"

#ifndef execH
#define execH

#define OPCODE_MASK	((OPCODE&0xFF00000)>>16)|((OPCODE&0xF0)>>4)
#define OPCODE_MASK_T	(OPCODE_T>>6)

#ifdef __cplusplus
extern "C" {
#endif

u8 arm_exec(void);
u8 thumb_exec(void);
u8 arm_exec_d(void);
u8 thumb_exec_d(void);

void FASTCALL SetExecFunction(u8 i);

u32 FASTCALL CalcSubFlags(u32,u32);
u32 FASTCALL CalcAddFlags(u32,u32);
u32 FASTCALL CalcAddcFlags(u32,u32);
void FASTCALL SET_DP_LOG_FLAGS(u32 value);

#ifdef __cplusplus
}
#endif

#endif
