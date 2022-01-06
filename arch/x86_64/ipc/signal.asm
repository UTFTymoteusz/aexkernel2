global sigret_trampoline_func

sigret_trampoline_func:
    mov r12, 51
    syscall

    ud2