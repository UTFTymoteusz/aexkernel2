[BITS 64]

global setjmp
global longjmp
global testjmp
global nojmp

; rdi - context
setjmp:
    push rbp
    mov rbp, rsp

    mov rax, rsp
    mov rsp, rdi
    add rsp, 15 * 8

    push 0x6d656d65
    push rbx
    push rcx
    push 0x00646e61
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

    mov dword rdx, [rax + 8]
    mov rcx, rax
    add rcx, 16
    
    add rsp, 22 * 8
    mov r8, ss
    push r8      ; ss
    push rcx     ; rsp
    pushfq       ; rflags
    mov r8, cs
    push r8      ; cs
    push rdx     ; rip
    push 0       ; padding

    mov rdx, cr3
    push rdx     ; cr3

    add rsp, 7 * 8
    fxsave [rsp]

    mov rsp, rax
    xor rax, rax

    pop rbp
    retq

; rdi - context, rsi - val
longjmp:
    push rbp
    mov rbp, rsp

    cmp rsi, 0
    jne .statusvalid
    mov rsi, 1

.statusvalid:
    mov rax, rsi
    mov rsp, rdi

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
    mov dword [rsp], 0x00000000
    add rsp, 8 ; rdx
    pop rcx
    pop rbx
    mov dword [rsp], 0x00000000
    add rsp, 8 ; rax

    pop rdx
    mov cr3, rdx

    fxrstor [rsp + 6 * 8]

    add rsp, 8 ; padding
    iretq

; rdi - context
testjmp:
    push rbp
    mov rbp, rsp

    xor rax, rax

    cmp dword [rdi + 0x70], 0x6d656d65
    jne .fail
    cmp dword [rdi + 0x58], 0x00646e61
    jne .fail

    add rax, 1
    pop rbp
    ret

.fail:
    pop rbp
    ret

; rdi - context
nojmp:
    push rbp
    mov rbp, rsp

    mov dword [rdi + 0x70], 0x00000000
    mov dword [rdi + 0x58], 0x00000000

    pop rbp
    ret
