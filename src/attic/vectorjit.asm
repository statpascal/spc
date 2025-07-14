/*
Vector conversions

These must be set to define operations and sizes

%define srcsize -4
%define srctype s64|...
%define dstsize 8
%define dsttype s64|...

Parameters

 RDI: void *src
 RSI: void *dst
 RDX: std::size_t count

Registers used:
 RAX: accumulator

*/

const char *asm_conv_src = R"ASM(
bits 64
	or rdx, rdx
	jz conv_done

conv_loop:
	%ifidn srctype, s64
		mov rax, [rdi]
	%elifidn srctype, s32
		movsx rax, dword [rdi]
	%elifidn srctype, s16
		movsx rax, word [rdi]
	%elifidn srctype, s8
		movsx rax, byte [rdi]
	%elifidn srctype, u8
		movzx rax, byte [rdi]
	%elifidn srctype, u16
		movzx rax, word [rdi]
	%elifidn srctype, u32
		mov eax, dword [rdi]
	%endif

	%ifidn dsttype, s64
		mov [rsi], rax
	%elifidn dsttype, s32
		mov [rsi], eax
	%elifidn dsttype, s16
		mov [rsi], ax
	%elifidn dsttype, s8
		mov [rsi], al
	%elifidn dsttype, u8
		mov [rsi], al
	%elifidn dsttype, u16
		mov [rsi], ax
	%elifidn dsttype, u32
		mov [rsi], eax
	%elifidn dsttype, single
		%ifidn srctype, real
			movsd xmm0, [rdi]
			cvtsd2ss xmm0, xmm0
		%else
			cvtsi2ss xmm0, rax
		%endif
		movss [rsi], xmm0
	%elifidn dsttype, real
	    	%ifidn srctype, single
			movss xmm0, [rdi]
			cvtss2sd xmm0, xmm0
		%else
			cvtsi2sd xmm0, rax
		%endif
		movsd [rsi], xmm0
	%endif

	add rdi, srcsize
	add rsi, dstsize
	dec rdx
	jnz conv_loop

conv_done:
	ret
)ASM";

/* 
Vector operations

These must be set to define operations and sizes

%define src1size -4
%define src1type s64|...
%define src2size -2
%define src2type s64|
%define int_op 0|1
%define cmp_op 0|1
%macro perform_op 0
	add rax, rbx
%endmacro

Parameters

 RDI: void *src1, 
 RSI: std::size_t src1len, 
 RDX: void *src2, 
 RCX: std::size_t src2len, 
 R8:  void *dst, 
 R9:  void *dstend;

Registers used:

 RAX, xmm0: accumulator
 RBX, xmm1: temp for second operand
 R10: src1 iter
 R11: src2 iter
 R12: end of src1/src2 range (shorter one)
 R13: copy of parameter RDX (required for DIV)
 R14: error flag

*/

const char *asm_op_src = R"ASM(
%macro operation 0
	loadfirst
	loadsecond
	perform_op
%endmacro

start_vecop:
	or rsi, rsi
        jz done
        or rcx, rcx
        jz done
        cmp r8, r9
        jz done

        mov r10, rdi
	mov r11, rdx

	push rbx
	push r12
	push r14
	xor r14, r14

%if int_op = 0
	vzeroall
%endif

	cmp rcx, 1
	je second_constant
	cmp rsi, rcx
	je equal_length
        jl first_smaller


second_smaller:
	lea r12, [rdx + src2size * rcx]
	push r13
	mov r13, rdx

second_smaller_loop:
	operation
	add r10, src1size
	add r11, src2size

        cmp r11, r12
	jne second_smaller_2
	mov r11, r13

second_smaller_2:
	cmp r8, r9
	jne second_smaller_loop

	pop r13
	jmp done


second_constant:
	loadsecond

second_constant_loop:
	loadfirst
	perform_op

	add r10, src1size
	cmp r8, r9
	jl second_constant_loop

	jmp done


equal_length:
	operation
	add r10, src1size
	add r11, src2size

	cmp r8, r9
	jl equal_length

	jmp done


first_smaller:
	lea r12, [rdi + src1size * rsi]

first_smaller_loop:
	operation
	add r10, src1size
	add r11, src2size

        cmp r10, r12
	jne first_smaller_2
	mov r10, rdi

first_smaller_2:
	cmp r8, r9
	jne first_smaller_loop

done:
	mov rax, r14
	pop r14
	pop r12
	pop rbx
	ret

)ASM";










/********

ALT:

%macro load 5
	%ifidn   %1, s64
		mov %2, [%3]
	%elifidn %1, s32
		movsx %2, dword [%3]
	%elifidn %1, s16
		movsx %2, word [%3]
	%elifidn %1, s8
		movsx %2, byte [%3]
	%elifidn %1, real
		movsd %4, [%3]
	%elifidn %1, single
		movss %4, [%3]
		cvtss2sd %4, %4
	%elifidn %1, u8
		movzx %2, byte [%3]
	%elifidn %1, u16
		movzx %2, word [%3]
	%elifidn %1, u32
		mov %5, dword [%3]
	%elifidn %1, u64
		mov %2, [%3]  
	%endif

        %if int_op = 0
            %ifnidn %1, real
		%ifnidn %1, single
		    cvtsi2sd %4, %2
		%endif
	    %endif
	%endif
%endmacro

%macro store_res 0
	%if cmp_op = 0
	    %if int_op = 0
		movsd [r8], xmm0
	    %else
		mov [r8], rax
	    %endif
		add r8, 8
	%else
		inc r8
	%endif
%endmacro

%macro operation_alt 0
;        load src1type, rax, r10, xmm0, eax
;	load src2type, rbx, r11, xmm1, ebx

;	perform_op
;	store_res

;	add r10, src1size
;        add r11, src2size

;	pxor xmm0, xmm0

;	cvtsi2sd xmm0, dword [r10]

;        movsd xmm0, [r10]
;	addsd xmm0, [r11]

;	movapd xmm0, [r10]
;	addpd xmm0, [r11]

;	vmovupd ymm0, yword [r10]
;	vmovupd ymm1, yword [r11]
;	vaddpd ymm2, ymm0, ymm1

;	add r10, 32
;	add r11, 32

;	movsd [r8], xmm0
;	movapd [r8], xmm0
;	vmovupd yword [r8], ymm2
;	add r8, 32

;        add rax, rbx
;	imul rax, rbx
;	cqo
;	idiv ebx
;	cdqe

%endmacro

start_vecop:
	or rsi, rsi
        jz done
        or rcx, rcx
        jz done
        cmp r8, r9
        jz done

        mov r10, rdi
	mov r11, rdx

	push rbx
	push r12

%if int_op = 0
	vzeroall
%endif

;	cmp rcx, 1
;	je second_constant
	cmp rsi, rcx
	je equal_length
        jl first_smaller

second_smaller:
	lea r12, [rdx + src2size * rcx]
	push r13
	mov r13, rdx

second_smaller_loop:
	operation

        cmp r11, r12
	jne second_smaller_2
	mov r11, r13

second_smaller_2:
	cmp r8, r9
	jne second_smaller_loop

	pop r13
	jmp done


;second_constant:
;        load src2type, rbx, r11, xmm1, ebx

;second_constant_loop:
;	load src1type, rax, r10, xmm0, eax
;	perform_op
;	store_res

;	add r10, src1size
;	cmp r8, r9
;	jl second_constant_loop

;	jmp done


equal_length:
	operation

	cmp r8, r9
	jl equal_length

	jmp done


first_smaller:
	lea r12, [rdi + src1size * rsi]

first_smaller_loop:
	operation

        cmp r10, r12
	jne first_smaller_2
	mov r10, rdi

first_smaller_2:
	cmp r8, r9
	jne first_smaller_loop

done:
	pop r14
	pop r12
	pop rbx
	ret

)ASM";


*////