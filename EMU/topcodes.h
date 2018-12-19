#include "opcodes.h"
#include "exec.h"

#ifndef topcodesH
#define topcodesH

#ifdef __cplusplus
extern "C" {
#endif
u8 tins_sbc(void);
u8 tins_mov_reg(void);
u8 tins_swi(void);
u8 tins_cmn_reg(void);
u8 tins_asr_reg(void);
u8 tins_mvn_reg(void);
u8 tins_ldrsb_reg(void);
u8 tins_adc_reg(void);
u8 tins_tst(void);
u8 tins_ror_reg(void);
u8 tins_mul(void);
u8 tins_lsl_reg(void);
u8 tins_ldmia(void);
u8 tins_add_rel_sp(void);
u8 tins_ldrsh(void);
u8 tins_ldr_reg(void);
u8 tins_strh_reg(void);
u8 tins_str_reg(void);
u8 tins_ldrb_reg(void);
u8 tins_stmia(void);
u8 tins_strb_reg(void);
u8 tins_sub_reg_imm(void);
u8 tins_sub_sp(void);
u8 tins_eor(void);
u8 tins_cmp_reg(void);
u8 tins_sp_rel_ldr (void);
u8 tins_add_pc(void);
u8 tins_cmp(void);
u8 tins_bx(void);
u8 tins_sub(void);
u8 tins_bu(void); // opcode 0x380 b senza condizione
u8 tins_b(void); // opcode 0x340 branch con condizione
u8 tins_neg(void);
u8 tins_unknown(void);
u8 tins_mov_lo_hi(void);
u8 tins_mov_hi_lo(void);
u8 tins_mov_hi_hi(void);
u8 tins_add_lo_hi(void);
u8 tins_add_hi_lo(void);
u8 tins_add_hi_hi(void);
u8 tins_add_sp(void);
u8 tins_mov_imm(void);
u8 tins_sp_rel_str(void);
u8 tins_pc_rel_ldr(void);
u8 tins_str_imm(void);
u8 tins_strb_imm(void);
u8 tins_ldr_imm(void);
u8 tins_ldrb_imm(void);
u8 tins_lsl_imm(void);
u8 tins_lsr_reg(void);
u8 tins_lsr_imm(void);
u8 tins_asr_imm(void);
u8 tins_add(void);
u8 tins_add_imm(void);
u8 tins_cmp_imm(void);
u8 tins_ble(void);
u8 tins_strh_imm(void);
u8 tins_ldrh_reg(void);
u8 tins_ldrh_imm(void);
u8 tins_and(void);
u8 tins_orr(void);
u8 tins_bic(void);
u8 tins_mvn(void);
u8 tins_add_short_imm(void);
u8 tins_sub_imm(void);
u8 tins_push(void);
u8 tins_pop(void);
u8 tins_bl(void);

#ifdef __cplusplus
}
#endif

#endif