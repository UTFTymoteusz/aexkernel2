global context_test

context_test:
    mov eax, 0x11111111
    mov ebx, 0x22222222
    mov ecx, 0x33333333
    mov edx, 0x44444444
    mov edi, 0x66666666
    mov esi, 0x77777777
    mov r8d , 0x88888888
    mov r9d , 0x99999999
    mov r10d, 0xAAAAAAAA
    mov r11d, 0xBBBBBBBB
    mov r12d, 0xCCCCCCCC
    mov r13d, 0xDDDDDDDD
    mov r14d, 0xEEEEEEEE
    mov r15d, 0xFFFFFFFF

    .loop:
        cmp eax, 0x11111111
        jne .fail

        cmp ebx, 0x22222222
        jne .fail

        cmp ecx, 0x33333333
        jne .fail

        cmp edx, 0x44444444
        jne .fail

        cmp edi, 0x66666666
        jne .fail

        cmp esi, 0x77777777
        jne .fail

        cmp r8d , 0x88888888
        jne .fail

        cmp r9d , 0x99999999
        jne .fail

        cmp r10d, 0xAAAAAAAA
        jne .fail

        cmp r11d, 0xBBBBBBBB
        jne .fail

        cmp r12d, 0xCCCCCCCC
        jne .fail

        cmp r13d, 0xDDDDDDDD
        jne .fail

        cmp r14d, 0xEEEEEEEE
        jne .fail

        cmp r15d, 0xFFFFFFFF
        jne .fail

        jmp .loop

    .fail:
        ud2