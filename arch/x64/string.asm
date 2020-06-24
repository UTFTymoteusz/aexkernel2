global _ZN3AEX6memcpyEPvPKvm
global _ZN3AEX6memsetEPvcm
global _ZN3AEX8memset16EPvtm
global _ZN3AEX8memset32EPvjm
global _ZN3AEX8memset64EPvmm

_ZN3AEX6memcpyEPvPKvm:
    cld

    mov r8, rdi
    mov r9, rsi

    mov rcx, rdx
    shr rcx, 0x03

    rep movsq

    mov rdi, r8
    mov rsi, r9

    mov rcx, rdx
    and rcx, 0x07
    and rdx, ~0x7

    add rdi, rdx
    add rsi, rdx
    
    rep movsb

    ret

_ZN3AEX6memsetEPvcm:
    cld

    mov al , sil
    mov rcx, rdx

    rep stosb

    ret

_ZN3AEX8memset16EPvtm:
    cld

    mov ax , si
    mov rcx, rdx

    rep stosw

    ret

_ZN3AEX8memset32EPvjm:
    cld

    mov eax, esi
    mov rcx, rdx

    rep stosd

    ret

_ZN3AEX8memset64EPvmm:
    cld

    mov rax, rsi
    mov rcx, rdx

    rep stosq

    ret