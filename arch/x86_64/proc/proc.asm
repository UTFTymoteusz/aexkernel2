[BITS 64]
section .text

global proc_reshed
global proc_reshed_nosave
global proc_ctxsave
global proc_ctxload
global proc_ctxsave_int
global proc_ctxload_int
global safe_mxcsr

global proc_timer_tick
global proc_sched_nosave_int

extern proc_timer_tick_ext
extern proc_sched_int_ext

safe_mxcsr:
    dd 0x1f80

%macro save_context 0
    push rcx
    push rbx
    ; push rax

    mov rcx, rbx
    mov rbx, rax

    mov qword rax, [gs:0x08]
    mov qword [rax + 0x70], rbx
    mov qword [rax + 0x68], rcx

    mov rax, rbp
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
    add rsp, 0x08 * 22

    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 0x08
    push r10

    fxsave [rsp + 0x08 * 7]

    mov rsp, rbx
%endmacro

%macro save_context_hw 0
    sub rsp, 0x10

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

    mov rax, cr3
    mov [rsp + 0x08 * 15], qword rax
    
    fxsave [rsp + 0x08 * 22]

    mov rsp, qword [gs:0x30]
%endmacro

enter_context:
    mov qword rsp, [gs:0x08]

    fxrstor [rsp + 0x08 * 22]

    mov qword rbx, [rsp + 0x08 * 15] ; Thread-to-enter cr3
    mov rcx, cr3                     ; Current cr3

    ;cmp rcx, rbx 
    ;je .no_reload

    mov cr3, rbx

    .no_reload:
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

    add rsp, 16
    iretq

proc_timer_tick:
    save_context

    push rax ; rbp is saved to rax
    mov rbp, rsp

    mov ax, 0x10
    mov ss, ax

    ldmxcsr [safe_mxcsr]
    call proc_timer_tick_ext

    pop rbp

    jmp enter_context

proc_sched_nosave_int:
    push rbp
    mov rbp, rsp

    mov ax, 0x10
    mov ss, ax

    ldmxcsr [safe_mxcsr]
    call proc_sched_int_ext

    pop rbp

    jmp enter_context

proc_ctxsave_int:
    save_context

    mov ax, 0x10
    mov ss, ax

    jmp enter_context

proc_ctxload_int:
    mov ax, 0x10
    mov ss, ax

    jmp enter_context

proc_reshed:
    int 0x70
    ret

proc_reshed_nosave:
    int 0x71
    ret

proc_ctxsave:
    int 0x72
    ret

proc_ctxload:
    int 0x73
    ret