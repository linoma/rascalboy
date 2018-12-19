#ifndef opedecH
#define opedecH

#ifdef __cplusplus
extern "C" {
#endif

void second_operand (u32 op, u32 adress, char *dest);
void standard_debug_handle (u32 op, u32 adress, char *dest);
void b_debug_handle (u32 op, u32 adress, char *dest);
void bx_debug_handle (u32 op, u32 adress, char *dest);
void dpsingle_debug_handle (u32 op, u32 adress, char * dest);
void dpnw_debug_handle (u32 op, u32 adress, char * dest);
void dp_debug_handle (u32 op, u32 adress, char * dest);
void mul_debug_handle (u32 op, u32 adress, char *dest);
void mull_debug_handle (u32 op, u32 adress, char *dest);
void sdt_debug_handle (u32 op, u32 adress, char *dest);
void hwdt_debug_handle (u32 op, u32 adress, char *dest);
void mdt_debug_handle (u32 op, u32 adress, char *dest);
void msr_debug_handle (u32 op, u32 adress, char *dest);
void mrs_debug_handle (u32 op, u32 adress, char *dest);
void swp_debug_handle (u32 op, u32 adress, char *dest);
void swi_debug_handle (u32 op, u32 adress, char *dest);

#ifdef __cplusplus
}
#endif

#endif