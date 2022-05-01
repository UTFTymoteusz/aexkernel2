IA32_EFER    equ 0xC0000080
IA32_GS_BASE equ 0xC0000101

; CR0 Flags
PE equ (1 << 0)  ; Protected Mode Enable
MP equ (1 << 1)  ; Monitor co-processor
EM equ (1 << 2)  ; FPU Emulation
TS equ (1 << 3)  ; Task switched
ET equ (1 << 4)  ; Extension type
NE equ (1 << 5)  ; FPU numeric error
WP equ (1 << 16) ; Ring 0 write protect
AM equ (1 << 18) ; Alignment mask
NW equ (1 << 29) ; Not-write through
CD equ (1 << 30) ; Cache disable
PG equ (1 << 31) ; Paging

; CR4 Flags
VME        equ (1 << 0)
PVI        equ (1 << 1)
TSD        equ (1 << 2)
DE         equ (1 << 3)
PSE        equ (1 << 4)
PAE        equ (1 << 5)
MCE        equ (1 << 6)
PGE        equ (1 << 7)
PCE        equ (1 << 8)
OSFXSR     equ (1 << 9)
OSXMMEXCPT equ (1 << 10)
UMIP       equ (1 << 11)
LA57       equ (1 << 12)
VMXE       equ (1 << 13)
SMXE       equ (1 << 14)
FSGSBASE   equ (1 << 16)
PCIDE      equ (1 << 17)
OSXSAVE    equ (1 << 18)
SMEP       equ (1 << 20)
SMAP       equ (1 << 21)
PKE        equ (1 << 22)

; EFER Flags
SCE    equ (1 << 0)  ; Syscall extensions
DPE    equ (1 << 1)  ; K6 Data Prefetch Enable
SEWBED equ (1 << 2)  ; K6 SEWBED
GEWBED equ (1 << 3)  ; K6 GEWBED
L2D    equ (1 << 4)  ; K6 L2D
LME    equ (1 << 8)  ; Long Mode Enable
LMA    equ (1 << 10) ; Long Mode Active
NXE    equ (1 << 11) ; No-execute Enable
SVME   equ (1 << 12) ; Secure Virtual Machine Enable
LMSLE  equ (1 << 13) ; Long Mode Segment Limit Enable
FFXSR  equ (1 << 14) ; Fast FXSAVE and FXRSTOR
TCE    equ (1 << 15) ; Translation cache extension