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

    fxsave [rsp + 0x08 * 7]
    
    mov rsp, rbx
%endmacro

enter_context:
    mov qword rax, [gs:0x08]

    fxrstor [rax + 0x08 * 22]

    mov qword rbx, [rax + 0x08 * 15] ; Thread-to-enter cr3
    mov rcx, cr3                     ; Current cr3

    cmp rcx, rbx  ; Let's check if they are equal, and if they are, skip to not anger the cache.
    je .no_reload

    mov cr3, rbx

    .no_reload:
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
    

proc_timer_tick:
    save_context

    call proc_timer_tick_ext
    
    jmp enter_context

proc_reshed_manual:
    save_context

    call proc_reshed_manual_ext
    
    jmp enter_context


proc_reshed:
    mov rdi, rsp

    mov  rax, ss
    push rax

    push rdi

    pushfq

    mov  rax, cs
    push rax

    push .ret

    cli 

    jmp proc_reshed_manual

    .ret:
        ret