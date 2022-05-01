[BITS 32]

global setup_early_idt_32
extern early_fault_handler_32

SECTION .bootstrap

%macro flt_err 1
    flt%1_32:
        push dword %1
        jmp early_fault_wrapper_32
%endmacro

%macro flt_noerr 1
    flt%1_32:
        push dword 0
        push dword %1
        jmp early_fault_wrapper_32
%endmacro

%macro flt_entry 1
    dd flt%1_32
%endmacro

flt_array_32:
    %assign i 0
    %rep 32
        flt_entry i
        %assign i i + 1
    %endrep

flt_noerr  0
flt_noerr  1
flt_noerr  2
flt_noerr  3
flt_noerr  4
flt_noerr  5
flt_noerr  6
flt_noerr  7
flt_err    8
flt_noerr  9
flt_err   10
flt_err   11
flt_err   12
flt_err   13
flt_err   14
flt_noerr 15
flt_noerr 16
flt_noerr 17
flt_noerr 18
flt_noerr 19
flt_noerr 20
flt_noerr 21
flt_noerr 22
flt_noerr 23
flt_noerr 24
flt_noerr 25
flt_noerr 26
flt_noerr 27
flt_noerr 28
flt_noerr 29
flt_noerr 30
flt_noerr 31

early_idt_ptr_32:
	dw 32 * 8 - 1
	dd early_idt_32

early_idt_32:
	resb 32 * 8

setup_early_idt_32:
	push ebp
	mov ebp, esp

	mov ebx, early_idt_32
	xor ecx, ecx
    mov edi, flt_array_32

	.loop:
        mov dword edx, [edi]

		; First part of the offset
		mov esi, edx
		and esi, 0xFFFF
		mov word [ebx + 0x00], si

		; Code selector
		mov word [ebx + 0x02], 0x18

		; IST
		mov byte [ebx + 0x04], 0x00

		; Type and attributes
		mov byte [ebx + 0x05], 0x8E

		; Second part of the offset
		mov esi, edx
		shr esi, 16
		and esi, 0xFFFF
		mov word [ebx + 0x06], si

		add ebx, 8
        add edi, 4
		inc ecx
		cmp ecx, 32
		jl .loop

	lidt [early_idt_ptr_32]

	pop ebp
	ret

early_fault_wrapper_32:
	pop ecx
	pop edx

	mov esi, ecx
	xor edi, edi

	mov eax, ecx
	xor edx, edx

	mov ebx, 10
	idiv ebx

	add eax, '0'
	add edx, '0'

	mov byte [0xB8000], 'f'
	mov byte [0xB8002], 'a'
	mov byte [0xB8004], 'u'
	mov byte [0xB8006], 'l'
	mov byte [0xB8008], 't'
	mov byte [0xB800A], ' '
	mov byte [0xB800C], al
	mov byte [0xB800E], dl
	mov byte [0xB8010], ' '
	mov byte [0xB8012], ':'
	mov byte [0xB8014], '('
	mov byte [0xB8016], ' '

	mov byte [0xB8013], 0x0C
	mov byte [0xB8015], 0x0C

    push ebp
	mov ebp, esp
	
	sub esp, 512
    mov edi, esp

	hlt
	; call early_fault_handler
	