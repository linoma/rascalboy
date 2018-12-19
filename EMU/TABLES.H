#include "debug.h"
#ifndef tablesH
#define tablesH

#ifdef __cplusplus
extern "C" {
#endif

void setup_hwdt_handles2(u32 base,ARMOPCODE handle,ARMOPCODE handle2,ARMOPCODE handle3,ARMOPCODE handle4,ARMOPCODE handle2wb,ARMOPCODE handle4wb);
void setup_sdt_handles2(u32 base,ARMOPCODE handle,ARMOPCODE handle2,ARMOPCODE handle3,ARMOPCODE handle4,ARMOPCODE handle3wb,ARMOPCODE handle4wb);
void setup_dp_handle(u32 base,ARMOPCODE ins,ARMOPCODE ins_reg,ARMOPCODE ins_imm);
void setup_handle_tables (void);
void setup_io_handle_tables (void);
void setup_dp_strings (u32 base, char *string);
void setup_sdt_strings (u32 base, char *string);
void setup_string_tables(void);
void setup_dp_debug_handle (u32 base, DECODEARM handle);
void setup_sdt_debug_handles (u32 base, DECODEARM handle);
void setup_hwdt_debug_handles (u32 base, DECODEARM handle);
void setup_debug_handles(void);

#ifdef __cplusplus
}
#endif
#endif

