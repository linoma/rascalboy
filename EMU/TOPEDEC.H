#ifndef topedecH
#define topedecH

#ifdef __cplusplus
extern "C" {
#endif

void standard_debug_handle_t(u16 op, u32 adress, char *dest);
void immediate_debug_handle_t(u16 op, u32 adress, char *dest);
void bcond_debug_handle_t(u16 op, u32 adress, char *dest);
void hireg_debug_handle_t(u16 op, u32 adress, char *dest);
void bl_debug_handle_t(u16 op, u32 adress, char *dest);
void immlong_debug_handle_t(u16 op, u32 adress, char *dest);
void immshort_debug_handle_t(u16 op, u32 adress, char *dest);
void reg_debug_handle_t(u16 op, u32 adress, char *dest);
void bx_debug_handle_t(u16 op, u32 adress, char *dest);
void mdt_debug_handle_t(u16 op, u32 adress, char *dest);
void stack_debug_handle_t(u16 op, u32 adress, char *dest);
void swi_debug_handle_t(u16 op,u32 adress,char *dest);
void rs_debug_handle_t(u16 op,u32 adress,char *dest);
void b_debug_handle_t(u16 op,u32 adress,char *dest);
void unknown_debug_handle_t(u16 op, u32 adress, char *dest);
void FillMultipleRegisterString(BYTE *s1,char *dest);

#ifdef __cplusplus
}
#endif

#endif
