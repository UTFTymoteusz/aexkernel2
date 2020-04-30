[BITS 64]

global proc_timer_tick
global proc_reshed

extern proc_timer_tick_ext
extern proc_reshed_manual_ext

%macro save_context 0
    push rcx
    push rbx
    push rax

    mov rcx, rbx
    mov rbx, rax

    mov qword rax, [gs:0x08]
    mov qword [rax + 0x70], rbx
    mov qword [rax + 0x68], rcx
    
    pop rax
    pop rbx
    pop rcx

    mov rbx, rsp

    mov qword rsp, [gs:0x08]
    add rsp, 0x08 * 13

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

    mov rsp, rbx

    mov r10, cr3
    pop r15 ; rip
    pop r14 ; cs
    pop r13 ; rflags
    pop r12 ; rsp
    pop r11 ; ss

    mov qword rsp, [gs:0x08]
    add rsp, 0x08 * 21

    push r11
    push r12
    push r13
    push r14
    push r15
    push r10
    
    mov rsp, rbx
%endmacro

%macro enter_context 0
    mov qword rax, [gs:0x08]

    mov qword rbx, [rax + 0x08 * 15]
    xchg bx, bx
    mov cr3, rbx

    mov qword rsp, [gs:0x08]

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

    add rsp, 8

    iretq
%endmacro

proc_timer_tick:
    save_context

    call proc_timer_tick_ext
    
    enter_context

proc_reshed_manual:
    save_context

    call proc_reshed_manual_ext
    
    enter_context


proc_reshed:
    mov rdi, rsp

    mov  rax, ss
    push rax

    push rdi

    pushfq

    mov  rax, cs
    push rax

    push ret

    cli

    jmp proc_reshed_manual

    ret:
        retq