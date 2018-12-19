	.file	"dp.c"
	.text
	.p2align 4,,15
.globl _DP_IMM_OPERAND
    .def   _DP_IMM_OPERAND;  .scl    2;  .type   32; .endef
_DP_IMM_OPERAND:
    movl    _arm, %ecx
    movzbl  %cl,%eax
    andl    $3840,%ecx
    shrl    $7, %ecx
    rorl    %cl,%eax
	ret
	.p2align 4,,15
.globl _DP_IMM_OPERAND_UPD
    .def    _DP_IMM_OPERAND_UPD;  .scl    2;  .type   32; .endef
_DP_IMM_OPERAND_UPD:
    movl    _arm, %ecx
    movzbl  %cl,%eax
    andl    $3840,%ecx
    shrl    $7, %ecx
    jz L1
    rorl    %cl,%eax
    setcb   _arm+18
L1:
	ret
	.p2align 4,,15
.globl _floattoint
	.def	_floattoint;	.scl	2;	.type	32;	.endef
_floattoint:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$16, %esp
	fnstcw	-2(%ebp)
	movzwl	-2(%ebp), %edx
	flds	8(%ebp)
	orw	$3072, %dx
	movw	%dx, -4(%ebp)
	fldcw	-4(%ebp)
	fistpq	-16(%ebp)
	fldcw	-2(%ebp)
	movl	-16(%ebp), %eax
	movl	%ebp, %esp
	popl	%ebp
	ret
	.p2align 4,,15
.globl _CalcAddFlags
    .def    _CalcAddFlags;    .scl    2;  .type   32; .endef
_CalcAddFlags:
    addl  %edx,%eax
    setzb _arm+16
    setsb _arm+17
    setcb _arm+18
    setob _arm+19
	ret
	.p2align 4,,15
.globl _CalcSubFlags
    .def    _CalcSubFlags;    .scl    2;  .type   32; .endef
_CalcSubFlags:
    subl  %edx,%eax
    setzb _arm+16
    setsb _arm+17
    setncb _arm+18
    setob _arm+19
	ret
	.p2align 4,,15
.globl _SET_DP_LOG_FLAGS
    .def    _SET_DP_LOG_FLAGS;    .scl    2;  .type   32; .endef
_SET_DP_LOG_FLAGS:
    cmpl $0,%eax
    setzb _arm+16
    setsb _arm+17
	ret
	.p2align 4,,15
.globl _power
	.def	_power;	.scl	2;	.type	32;	.endef
_power:
	pushl	%ebp
	fldz
	movl	%esp, %ebp
	popl	%ebp
	ret
	.p2align 4,,15
.globl _VerifyFile
	.def	_VerifyFile;	.scl	2;	.type	32;	.endef
_VerifyFile:
	pushl	%ebp
	movl	%esp, %ebp
	popl	%ebp
	ret
