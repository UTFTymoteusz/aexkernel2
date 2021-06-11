[BITS 64]

global exc_array

extern fault_handler

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

%macro exc_err 1
    global exc%1
    exc%1:
        push byte %1
        jmp exc_common
%endmacro

%macro exc_noerr 1
    global exc%1
    exc%1:
        push byte 0
        push byte %1
        jmp exc_common
%endmacro

%macro exc_entry 1
    dq exc%1
%endmacro

exc_array:
    %assign i 0
    %rep 32
        exc_entry i
        %assign i i + 1
    %endrep

exc_noerr  0
exc_noerr  1
exc_noerr  2
exc_noerr  3
exc_noerr  4
exc_noerr  5
exc_noerr  6
exc_noerr  7
exc_err    8
exc_noerr  9
exc_err   10
exc_err   11
exc_err   12
exc_err   13
exc_err   14
exc_noerr 15
exc_noerr 16
exc_noerr 17
exc_noerr 18
exc_noerr 19
exc_noerr 20
exc_noerr 21
exc_noerr 22
exc_noerr 23
exc_noerr 24
exc_noerr 25
exc_noerr 26
exc_noerr 27
exc_noerr 28
exc_noerr 29
exc_noerr 30
exc_noerr 31

exc_common:
    pusha

    sub rsp, 512
    fxsave [rsp]

    ; Fake stack frame so the stack trace works properly
    mov rax, qword [rsp + 512 + 17 * 8]
    push rax
    push rbp
    mov rbp, rsp
    
    mov rax, 0x0002
    push rax
    popfq

    mov rdi, rsp
    add rdi, 512 + 16
    call fault_handler

    pop rbp
    pop rax

    fxrstor [rsp]
    add rsp, 512

    popa

    add rsp, 16 ; Clean up pushed error code and ISR number
    iretq