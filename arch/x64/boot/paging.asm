[BITS 32]

global paging_init
global pml4

PAGE_FLAGS equ 0x003

SECTION .bootstrap

paging_init:
    mov ecx, pml4
    .pml4init:
        mov dword [ecx], 0
        add ecx, 4

        cmp ecx, 4096
        jl .pml4init

    mov edx, pdp0
    or  edx, PAGE_FLAGS
    mov dword [pml4 + 8 * 0], edx

    mov edx, pdp0.pd0
    or  edx, PAGE_FLAGS
    mov dword [pdp0 + 8 * 0], edx

    mov edx, pdp0.pt0_3
    or  edx, PAGE_FLAGS

    mov dword [pdp0.pd0 + 8 * 0], edx
    add edx, 4096

    mov dword [pdp0.pd0 + 8 * 1], edx
    add edx, 4096

    mov dword [pdp0.pd0 + 8 * 2], edx
    add edx, 4096

    mov dword [pdp0.pd0 + 8 * 3], edx
    add edx, 4096


    mov ebx, pdp0.pt0_3
    xor ecx, ecx
    mov edx, PAGE_FLAGS

    .ptinit1:
        mov dword [ebx], edx
        add ebx, 8
        inc ecx
        add edx, 4096

        cmp ecx, 2048
        jl .ptinit1


    mov edx, pdp511
    or  edx, PAGE_FLAGS
    mov dword [pml4 + 8 * 511], edx

    mov edx, pdp511.pd511
    or  edx, PAGE_FLAGS
    mov dword [pdp511 + 8 * 510], edx

    mov edx, pdp511.pt0_3
    or  edx, PAGE_FLAGS
    mov dword [pdp511.pd511 + 8 * 0], edx

    mov edx, pdp511.pt0_3
    or  edx, PAGE_FLAGS

    mov dword [pdp511.pd511 + 8 * 0], edx
    add edx, 4096

    mov dword [pdp511.pd511 + 8 * 1], edx
    add edx, 4096

    mov dword [pdp511.pd511 + 8 * 2], edx
    add edx, 4096

    mov dword [pdp511.pd511 + 8 * 3], edx
    add edx, 4096


    mov ebx, pdp511.pt0_3
    xor ecx, ecx
    mov edx, PAGE_FLAGS

    .ptinit2:
        mov dword [ebx], edx
        add ebx, 8
        inc ecx
        add edx, 4096

        cmp ecx, 2048
        jl .ptinit2


	; Enable PAE
	mov eax, cr4
    or  eax, 1 << 5
    mov cr4, eax

	; Make cr3 point to our PML4
	mov eax, pml4
	mov cr3, eax

    ret


SECTION .bootstrap.bss

align 4096
pml4:
    resb 4096

pdp0:
    resb 4096
    .pd0:
        resb 4096
        .pt0_3:
            resb 4096 * 4


pdp511:
    resb 4096
    .pd511:
        resb 4096
        .pt0_3:
            resb 4096 * 4