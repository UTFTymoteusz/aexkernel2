[BITS 32]

SECTION .bootstrap

global _start

extern main
extern paging_init
extern pml4
extern sse_init

_start:
    jmp bootstrap

ALIGN 4
mboot:
	MULTIBOOT_PAGE_ALIGN	equ 1 << 0
	MULTIBOOT_MEMORY_INFO	equ 1 << 1
	MULTIBOOT_GRAPHICS_INFO	equ 1 << 2
	MULTIBOOT_HEADER_MAGIC	equ 0x1BADB002
	MULTIBOOT_HEADER_FLAGS  equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_GRAPHICS_INFO
	MULTIBOOT_CHECKSUM	    equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

	dd MULTIBOOT_HEADER_MAGIC
	dd MULTIBOOT_HEADER_FLAGS
	dd MULTIBOOT_CHECKSUM

	dd 0
	dd 0
	dd 0
	dd 0
	dd 0

	; Graphics things
	dd 0
	dd 1280
	dd 720
	dd 32

fail:
	ud2

bootstrap:
    mov esp, boot_stack_end

	push eax
	push ebx
	push ecx
	push edx
	
	; Let's check if we have long mode
	mov eax, 0x80000001
	cpuid

	and edx, 1 << 29
	cmp edx, 1
	jl  fail

	; Let's setup paging
	call paging_init

	; Setting the long mode bit
	mov ecx, 0xC0000080
	rdmsr
	or eax, 1 << 8
	wrmsr

	; Actually enabling paging
	mov eax, cr0
	or  eax, (1 << 31)     ; PG
	and eax, ~0x60000000   ; clear CD and NW
	mov cr0, eax

	pop edx
	pop ecx
	pop ebx
	pop eax

	; Let's load a temporal GDT
	lgdt [gdt32to64.ptr]
    jmp gdt32to64.code:bootstrap64

[BITS 64]
bootstrap64:
	mov rsp, stack_end
	push rax

	; Let's setup SSE
    mov rax, cr0
    and rax, ~(1 << 2)
    or  rax, (1 << 1)
    mov rax, cr0

    mov rax, cr4
    or  rax, (0b11 << 9)
    mov cr4, rax

	; Let's set our non-code segment registers
	mov ax, gdt64.data
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov ss, ax

	xor rax, rax
	mov gs, rax
	mov fs, rax

	; Let's fake the CPU structure
	mov rcx, 0xC0000101

	mov rax, fake_cpu_class_ptr
	mov rdx, rax
	ror rdx, 32

	wrmsr

	; Let's load our actual GDT
	lgdt [gdt64.ptr]

	pop rax

	mov rdi, rbx
	mov rsi, rax

	xor rbp, rbp
	; sub rsp, 8 ; Gotta align the stack to 16 bytes or SSE explodes

    call main

    cli
	.hcf:
		hlt
		jmp .hcf

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
	db 0b10101111 ; Granularity - 64 bits, limit 19:16
	db 0x00       ; Base (high)
	.data: equ $ - gdt32to64
	dw 0xFFFF     ; Limit
	dw 0x00       ; Base (low)
	db 0x00       ; Base (middle)
	db 0b10010010 ; Access (read/write)
	db 0b00000000 ; Granularity
	db 0x00       ; Base (high)
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
	db 0b10101111 ; Granularity - 64 bits, limit 19:16
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
	db 0b10101111 ; Granularity - 64 bits, limit 19:16
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
boot_stack:
    resb 2048
	boot_stack_end:

SECTION .bss
stack:
    resb 16384
	stack_end: