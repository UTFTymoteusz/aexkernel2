%include "arch/x86_64/boot/common.mac"

[BITS 32]
SECTION .bootstrap

global bootstrap

extern setup_early_idt_32
extern paging_init
extern setup_early_idt
extern kmain

bootstrap:
    mov esp, boot_stack_end
	pushad

	pushad
	call setup_early_idt_32
	popad
	
	; Let's check if we have long mode
	mov eax, 0x80000001
	cpuid

	and edx, 1 << 29
	cmp edx, 1
	jl  fail

	call paging_init

	mov ecx, IA32_EFER
	rdmsr
	or eax, SCE
	or eax, LME
	wrmsr

	mov eax, cr0
	or  eax, PG
	or  eax, AM
	and eax, ~(NW | CD)
	mov cr0, eax

	popad

	lgdt [gdt32to64.ptr]
    jmp gdt32to64.code:bootstrap64
	
fail:
	ud2

[BITS 64]
bootstrap64:
	mov rsp, stack_end
	mov rdi, rbx
	mov rsi, rax

    mov rax, cr0
    and rax, ~EM
    or  rax, MP
    mov rax, cr0

    mov rax, cr4
    or  rax, OSXMMEXCPT | OSFXSR
    mov cr4, rax

	mov ax, gdt64.data
	mov ds, ax
	mov es, ax
	mov ss, ax

	xor rax, rax
	mov gs, rax
	mov fs, rax

	mov rcx, IA32_GS_BASE
	mov rax, fake_cpu_class_ptr
	mov rdx, rax
	ror rdx, 32
	wrmsr

	; Let's load our actual GDT
	lgdt [gdt64.ptr]

	; Let's setup an early IDT to catch some early faults (ahem ACPI ahem)
	call setup_early_idt

	xor rbp, rbp
    call kmain

fake_cpu_class_ptr:
	dq fake_cpu_class

fake_cpu_class:
	dd 0
	dd 0

	dq fake_cpu_class

	dq 0
	dq 0
	dq 0
	dq 0
	dq 0
	dq 0
	dq 0
	dq 0
	
align 16
gdt32to64:
	.null: equ $ - gdt32to64
	dw 0xFFFF     ; Limit
	dw 0x00       ; Base (low)
	db 0x00       ; Base (middle)
	db 0x00       ; Access
	db 0x01       ; Granularity
	db 0x00       ; Base (high)
	.code: equ $ - gdt32to64
	dw 0xFFFF     ; Limit
	dw 0x00       ; Base (low)
	db 0x00       ; Base (middle)
	db 0b10011010 ; Access (exec/read)
	db 0b10101111 ; Granularity, 64 bits, limit 19:16
	db 0x00       ; Base (high)
	.data: equ $ - gdt32to64
	dw 0xFFFF     ; Limit
	dw 0x00       ; Base (low)
	db 0x00       ; Base (middle)
	db 0b10010010 ; Access (read/write)
	db 0b10000000 ; Granularity
	db 0x00       ; Base (high)
	.code32: equ $ - gdt32to64
	dw 0xFFFF     ; Limit
	dw 0x00       ; Base (low)
	db 0x00       ; Base (middle)
	db 0b10011010 ; Access (exec/read)
	db 0b11001111 ; Granularity, 32 bits, limit 19:16
	db 0x00       ; Base (high)
	.data32: equ $ - gdt32to64
	dw 0xFFFF     ; Limit
	dw 0x00       ; Base (low)
	db 0x00       ; Base (middle)
	db 0b10010010 ; Access (read/write)
	db 0b11000000 ; Granularity, 32 bits, 
	db 0x00       ; Base (high)
	.tss: equ $ - gdt32to64
	dw 0xFFFF     ; Limit
	dw 0x00       ; Base (low)
	db 0x00       ; Base (middle)
	db 0b10001001 ; Access (read/write)
	db 0b11000000 ; Granularity, 32 bits, 
	db 0x00       ; Base (high)
	dd 0x00       ; 64 bit address
	.ptr:
	dw $ - gdt32to64 - 1 ; Limit
	dq gdt32to64         ; Base

SECTION .data
gdt64:
	.null: equ $ - gdt64
	dw 0xFFFF     ; Limit
	dw 0x00       ; Base (low)
	db 0x00       ; Base (middle)
	db 0x00       ; Access
	db 0x01       ; Granularity
	db 0x00       ; Base (high)
	.code: equ $ - gdt64
	dw 0xFFFF     ; Limit
	dw 0x00       ; Base (low)
	db 0x00       ; Base (middle)
	db 0b10011010 ; Access (exec/read)
	db 0b10101111 ; Granularity, 64 bits, limit 19:16
	db 0x00       ; Base (high)
	.data: equ $ - gdt64
	dw 0xFFFF     ; Limit
	dw 0x00       ; Base (low)
	db 0x00       ; Base (middle)
	db 0b10010010 ; Access (read/write)
	db 0b00000000 ; Granularity
	db 0x00       ; Base (high)
	.usrcode: equ $ - gdt64
	dw 0xFFFF     ; Limit
	dw 0x00       ; Base (low)
	db 0x00       ; Base (middle)
	db 0b11111010 ; Access (ring3/exec/read)
	db 0b10101111 ; Granularity, 64 bits, limit 19:16
	db 0x00       ; Base (high)
	.usrdata: equ $ - gdt64
	dw 0xFFFF     ; Limit
	dw 0x00       ; Base (low)
	db 0x00       ; Base (middle)
	db 0b11110010 ; Access (ring3/read/write)
	db 0b00000000 ; Granularity
	db 0x00       ; Base (high)
	.tss: equ $ - gdt64
	dw 0xFFFF     ; Limit
	dw 0x00       ; Base (low)
	db 0x00       ; Base (middle)
	db 0b10001001 ; Access (ring3/read/write)
	db 0b00000000 ; Granularity
	db 0x00       ; Base (high)
	dd 0x00000000 ; Extra TSS things
	dd 0x00000000 ; 
	.ptr:
	dw $ - gdt64 - 1 ; Limit
	dq gdt64         ; Base

SECTION .bootstrap.bss
ALIGN 16
boot_stack:
    resb 4096
	boot_stack_end:

SECTION .bss
ALIGN 16
stack:
    resb 16384
	stack_end: