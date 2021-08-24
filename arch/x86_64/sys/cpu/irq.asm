[BITS 64]

global irq_array
global irq_spurious

extern ipi_handle
extern irq_handler
extern irq_marker
extern safe_mxcsr

SECTION .text
%macro pusha 0
    push rbp
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
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
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    pop rbp
%endmacro

%macro irq 1
    global irq%1
    irq%1:
        push qword 0 ; Need this for SSE alignment
        push qword %1
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
    %rep 15
        irq_entry i
        %assign i i + 1
    %endrep

    dq _irq_marker

%assign i 0
%rep 32
    irq i
    %assign i i + 1
%endrep

irq_spurious:
    iretq

irq_common:
    pusha

    sub rsp, 512
    fxsave [rsp]
    
    mov rax, 0x0002
    push rax
    popfq

    mov rdi, rsp
    add rdi, 512
    xor rbp, rbp

    ldmxcsr [safe_mxcsr]
    call irq_handler

    fxrstor [rsp]
    add rsp, 512

    popa

    add rsp, 16 ; Clean up pushed IRQ number and SSE padder
    iretq

_irq_marker:
    pusha

    mov rdi, rsp
    xor rbp, rbp
    
    mov rax, 0x0002
    push rax
    popfq
    
    ldmxcsr [safe_mxcsr]
    call irq_marker

    popa
    iretq

irq_ipi:
    pusha

    sub rsp, 512
    fxsave [rsp]

    xor rbp, rbp
    
    mov rax, 0x0002
    push rax
    popfq
    
    ldmxcsr [safe_mxcsr]
    call ipi_handle

    fxrstor [rsp]
    add rsp, 512

    popa
    iretq