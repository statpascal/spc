	.file "settest3.sp"
// Begin asmlist al_procedures

.section .text.n_main
	.balign 8
.globl	PASCALMAIN
	.type	PASCALMAIN,@function
PASCALMAIN:
.globl	main
	.type	main,@function
main:
	stp	x29,x30,[sp, #-16]!
	mov	x29,sp
	stp	x19,x20,[sp, #-16]!
	bl	fpc_initializeunits
	movz	w0,#0
	adrp	x0,:got:_$CHARSET$_Ld1
	ldr	x0,[x0, :got_lo12:_$CHARSET$_Ld1]
	adrp	x3,U_$P$CHARSET_$$_S
	add	x3,x3,:lo12:U_$P$CHARSET_$$_S
	ldp	x4,x2,[x0]
	ldp	x1,x0,[x0, #16]
	stp	x4,x2,[x3]
	stp	x1,x0,[x3, #16]
	movz	w19,#255
	.balign 4
.Lj3:
	add	w0,w19,#1
	uxtb	w0,w0
	mov	w19,w0
	mov	w0,w19
	mov	w1,w0
	lsr	x1,x1,#3
	and	x0,x0,#7
	adrp	x2,U_$P$CHARSET_$$_S
	add	x2,x2,:lo12:U_$P$CHARSET_$$_S
	ldrb	w1,[x1, x2]
	lsrv	x1,x1,x0
	and	x1,x1,#1
	cmp	w1,#0
	b.eq	.Lj7
	bl	fpc_get_output
	mov	x20,x0
	mov	w2,w19
	mov	x1,x20
	movz	w0,#2
	bl	fpc_write_text_char
	bl	fpc_iocheck
	mov	x0,x20
	bl	fpc_write_end
	bl	fpc_iocheck
.Lj7:
	cmp	w19,#255
	b.lo	.Lj3
	bl	fpc_do_exit
	ldp	x19,x20,[sp], #16
	ldp	x29,x30,[sp], #16
	ret
.Le0:
	.size	main, .Le0 - main

.section .text
// End asmlist al_procedures
// Begin asmlist al_globals

.section .bss
	.type U_$P$CHARSET_$$_CH,@object
	.size U_$P$CHARSET_$$_CH,1
U_$P$CHARSET_$$_CH:
	.zero 1

.section .bss
	.balign 8
	.type U_$P$CHARSET_$$_S,@object
	.size U_$P$CHARSET_$$_S,32
U_$P$CHARSET_$$_S:
	.zero 32

.section .data.n_INITFINAL
	.balign 8
.globl	INITFINAL
	.type	INITFINAL,@object
INITFINAL:
	.quad	2,0
	.quad	INIT$_$SYSTEM
	.quad	0,0
	.quad	FINALIZE$_$OBJPAS
.Le1:
	.size	INITFINAL, .Le1 - INITFINAL

.section .data.n_FPC_THREADVARTABLES
	.balign 8
.globl	FPC_THREADVARTABLES
	.type	FPC_THREADVARTABLES,@object
FPC_THREADVARTABLES:
	.long	1
	.quad	THREADVARLIST_$SYSTEM$indirect
.Le2:
	.size	FPC_THREADVARTABLES, .Le2 - FPC_THREADVARTABLES

.section .data.n_FPC_RESOURCESTRINGTABLES
	.balign 8
.globl	FPC_RESOURCESTRINGTABLES
	.type	FPC_RESOURCESTRINGTABLES,@object
FPC_RESOURCESTRINGTABLES:
	.quad	0
.Le3:
	.size	FPC_RESOURCESTRINGTABLES, .Le3 - FPC_RESOURCESTRINGTABLES

.section .data.n_FPC_WIDEINITTABLES
	.balign 8
.globl	FPC_WIDEINITTABLES
	.type	FPC_WIDEINITTABLES,@object
FPC_WIDEINITTABLES:
	.quad	0
.Le4:
	.size	FPC_WIDEINITTABLES, .Le4 - FPC_WIDEINITTABLES

.section .data.n_FPC_RESSTRINITTABLES
	.balign 8
.globl	FPC_RESSTRINITTABLES
	.type	FPC_RESSTRINITTABLES,@object
FPC_RESSTRINITTABLES:
	.quad	0
.Le5:
	.size	FPC_RESSTRINITTABLES, .Le5 - FPC_RESSTRINITTABLES

.section .fpc.n_version
	.balign 16
	.type	__fpc_ident,@object
__fpc_ident:
	.ascii	"FPC 3.2.2 [2021/10/01] for aarch64 - Linux"
.Le6:
	.size	__fpc_ident, .Le6 - __fpc_ident

.section .data.n___stklen
	.balign 8
.globl	__stklen
	.type	__stklen,@object
__stklen:
	.quad	8388608
.Le7:
	.size	__stklen, .Le7 - __stklen

.section .data.n___heapsize
	.balign 8
.globl	__heapsize
	.type	__heapsize,@object
__heapsize:
	.quad	0
.Le8:
	.size	__heapsize, .Le8 - __heapsize

.section .data.n___fpc_valgrind
	.balign 8
.globl	__fpc_valgrind
	.type	__fpc_valgrind,@object
__fpc_valgrind:
	.byte	0
.Le9:
	.size	__fpc_valgrind, .Le9 - __fpc_valgrind

.section .data.n_FPC_RESLOCATION
	.balign 8
.globl	FPC_RESLOCATION
	.type	FPC_RESLOCATION,@object
FPC_RESLOCATION:
	.quad	0
.Le10:
	.size	FPC_RESLOCATION, .Le10 - FPC_RESLOCATION
// End asmlist al_globals
// Begin asmlist al_typedconsts

.section .rodata.n__$CHARSET$_Ld1
	.balign 8
.globl	_$CHARSET$_Ld1
_$CHARSET$_Ld1:
	.byte	0,0,0,0,0,0,0,0,126,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
.Le11:
	.size	_$CHARSET$_Ld1, .Le11 - _$CHARSET$_Ld1
// End asmlist al_typedconsts
.section .note.GNU-stack,"",%progbits

