#include "gbaemu.h"
#include "memory.h"

#ifndef opcodesH
#define opcodesH

#if defined(__BORLANDC__)
typedef DWORDLONG INT64;                      
typedef DWORDLONG UINT64;
#endif

#ifdef __cplusplus
extern "C" {
#endif

//u32 DP_IMM_OPERAND(char bUpdate);
u32 DP_REG_OPERAND_IMM(void);
u32 DP_REG_OPERAND(void);
u32 DP_REG_OPERAND_UPD(u8 shift);

u32 FASTCALL DP_IMM_OPERAND(void);
u32 FASTCALL DP_IMM_OPERAND_UPD(void);

u8 sdt(void);
u8 hsdt(void);
u8 dp(void);                                          
u8 bdt(void);

u8 ldm(void);
u8 stm(void);

u8 ldr_predownwb(void);
u8 ldr_preupwb(void);
u8 ldr_predownimmwb(void);
u8 ldr_preupimmwb(void);
u8 str_predownwb(void);
u8 str_preupwb(void);
u8 str_predownimmwb(void);
u8 str_preupimmwb(void);
u8 ldrh_preupwb(void);
u8 ldrh_predownwb(void);
u8 ldrh_preupimmwb(void);
u8 ldrh_predownimmwb(void);
u8 ldrsh_preupwb(void);
u8 ldrsh_predownwb(void);
u8 ldrsh_predownimmwb(void);
u8 ldrsh_preupimmwb(void);
u8 ldrsb_preupimmwb(void);
u8 ldrsb_predownimmwb(void);
u8 ldrsb_preupwb(void);
u8 ldrsb_predownwb(void);
u8 strh_predownwb(void);
u8 strh_predownimmwb(void);
u8 strh_preupwb(void);
u8 strh_preupimmwb(void);
u8 ldrb_predownwb(void);
u8 ldrb_preupwb(void);
u8 ldrb_predownimmwb(void);
u8 ldrb_preupimmwb(void);
u8 strb_predownwb(void);
u8 strb_preupwb(void);
u8 strb_predownimmwb(void);
u8 strb_preupimmwb(void);

u8 ldr_predown(void);
u8 ldr_preup(void);
u8 ldr_postdown(void);
u8 ldr_postup(void);
u8 ldr_predownimm(void);
u8 ldr_preupimm(void);
u8 ldr_postdownimm(void);
u8 ldr_postupimm(void);
u8 str_predown(void);
u8 str_preup(void);
u8 str_postdown(void);
u8 str_postup(void);
u8 str_predownimm(void);
u8 str_preupimm(void);
u8 str_postdownimm(void);
u8 str_postupimm(void);

u8 ldrh_preup(void);
u8 ldrh_postup(void);
u8 ldrh_preupimm(void);
u8 ldrh_postupimm(void);
u8 ldrh_predown(void);
u8 ldrh_postdown(void);
u8 ldrh_predownimm(void);
u8 ldrh_postdownimm(void);

u8 ldrsh_preup(void);
u8 ldrsh_postup(void);
u8 ldrsh_preupimm(void);
u8 ldrsh_postupimm(void);
u8 ldrsh_predown(void);
u8 ldrsh_postdown(void);
u8 ldrsh_predownimm(void);
u8 ldrsh_postdownimm(void);

u8 ldrsb_preup(void);
u8 ldrsb_postup(void);
u8 ldrsb_preupimm(void);
u8 ldrsb_postupimm(void);
u8 ldrsb_predown(void);
u8 ldrsb_postdown(void);
u8 ldrsb_predownimm(void);
u8 ldrsb_postdownimm(void);

u8 strh_predown(void);
u8 strh_postdown(void);
u8 strh_predownimm(void);
u8 strh_postdownimm(void);

u8 strh_preup(void);
u8 strh_postup(void);
u8 strh_preupimm(void);
u8 strh_postupimm(void);

u8 ldrb_predown(void);
u8 ldrb_preup(void);
u8 ldrb_postdown(void);
u8 ldrb_postup(void);
u8 ldrb_predownimm(void);
u8 ldrb_preupimm(void);
u8 ldrb_postdownimm(void);
u8 ldrb_postupimm(void);
u8 strb_predown(void);
u8 strb_preup(void);
u8 strb_postdown(void);
u8 strb_postup(void);
u8 strb_predownimm(void);
u8 strb_preupimm(void);
u8 strb_postdownimm(void);
u8 strb_postupimm(void);

u8 cmp(void);
u8 cmp_reg(void);
u8 cmp_imm(void);

u8 cmn(void);
u8 cmn_reg(void);
u8 cmn_imm(void);

u8 tst(void);
u8 tst_reg(void);
u8 tst_imm(void);

u8 teq(void);
u8 teq_reg(void);
u8 teq_imm(void);

u8 adc(void);
u8 adc_reg(void);
u8 adc_imm(void);
u8 adcs(void);
u8 adcs_reg(void);
u8 adcs_imm(void);

u8 rsc(void);

u8 mov(void);
u8 mov_reg(void);
u8 mov_imm(void);
u8 movs(void);
u8 movs_reg(void);
u8 movs_imm(void);

u8 mvn(void);
u8 mvn_reg(void);
u8 mvn_imm(void);
u8 mvns(void);
u8 mvns_reg(void);
u8 mvns_imm(void);

u8 bic(void);
u8 bic_reg(void);
u8 bic_imm(void);
u8 bics(void);
u8 bics_reg(void);
u8 bics_imm(void);

u8 add(void);
u8 add_reg(void);
u8 add_imm(void);
u8 adds(void);
u8 adds_reg(void);
u8 adds_imm(void);

u8 sub(void);
u8 sub_reg(void);
u8 sub_imm(void);
u8 subs(void);
u8 subs_reg(void);
u8 subs_imm(void);

u8 subcs(void);
u8 subcs_reg(void);
u8 subcs_imm(void);
u8 subc(void);
u8 subc_reg(void);
u8 subc_imm(void);

u8 rs(void);
u8 rs_reg(void);
u8 rs_imm(void);
u8 rss(void);
u8 rss_reg(void);
u8 rss_imm(void);

u8 and_(void);
u8 and_reg(void);
u8 and_imm(void);
u8 ands(void);
u8 ands_reg(void);
u8 ands_imm(void);

u8 orr(void);
u8 orr_reg(void);
u8 orr_imm(void);
u8 orrs(void);
u8 orrs_reg(void);
u8 orrs_imm(void);

u8 eor(void);
u8 eor_reg(void);
u8 eor_imm(void);
u8 eors(void);
u8 eors_reg(void);
u8 eors_imm(void);

u8 unknown_opcode(void);
u8 ins_bpl(void);
u8 ins_bmi(void);
u8 ins_blpl(void);
u8 ins_blmi(void);
u8 ins_bx(void);
u8 ins_mul(void);
u8 ins_muls(void);
u8 ins_mla(void);
u8 ins_mlas(void);
u8 ins_mull(void);
u8 ins_mulls(void);
u8 ins_mull_unsigned(void);
u8 ins_mulls_unsigned(void);
u8 ins_mlal(void);
u8 ins_mlals(void);
u8 ins_mlal_unsigned(void);
u8 ins_mlals_unsigned(void);
u8 ins_and(void);
u8 ins_mrs_cpsr(void);
u8 ins_msr_cpsr(void);
u8 ins_mrs_spsr(void);
u8 ins_msr_spsr(void);
u8 ins_msr_cpsr_imm(void);
u8 ins_msr_spsr_imm(void);
u8 ins_swi(void);
u8 ins_swp(void);
u8 ins_swpb(void);

#ifdef __cplusplus
}
#endif

#endif
