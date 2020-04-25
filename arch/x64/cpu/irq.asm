[BITS 64]

global irq_array
global irq_spurious

extern ipi_handle
extern common_irq_handler

SECTION .text
%macro pusha 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro popa 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

%macro irq 1
    global irq%1
    irq%1:
        push byte %1
        jmp irq_common
%endmacro

%macro irq_entry 1
    dq irq%1
%endmacro

irq_array:
    irq_entry  0
    irq_entry  1
    irq_entry  2
    irq_entry  3
    irq_entry  4
    irq_entry  5
    irq_entry  6
    irq_entry  7
    irq_entry  8
    irq_entry  9
    irq_entry 10
    irq_entry 11
    irq_entry 12

    dq irq_ipi

    irq_entry 14
    irq_entry 15

    %assign i 16
    %rep 16
        irq_entry i
        %assign i i + 1
    %endrep

%assign i 0
%rep 32
    irq i
    %assign i i + 1
%endrep

irq_spurious:
    ud2
    iretq

irq_common:
    pusha
    mov rdi, rsp
    
    call common_irq_handler

    popa

    add rsp, 8 ; Clean up pushed IRQ number
    iretq

irq_ipi:
    pusha

    call ipi_handle

    popa
    iretq