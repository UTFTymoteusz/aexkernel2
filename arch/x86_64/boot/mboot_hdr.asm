extern _start_text
extern _start_bss
extern _end_bss
extern bootstrap

[BITS 32]
SECTION .bootstrap
ALIGN 4
mboot:
	MULTIBOOT_PAGE_ALIGN	equ 1 << 0
	MULTIBOOT_MEMORY_INFO	equ 1 << 1
	MULTIBOOT_GRAPHICS_INFO	equ 1 << 2
	MULTIBOOT_ADDRESS_INFO	equ 1 << 16
	MULTIBOOT_HEADER_MAGIC	equ 0x1BADB002
	MULTIBOOT_HEADER_FLAGS  equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_GRAPHICS_INFO | MULTIBOOT_ADDRESS_INFO
	MULTIBOOT_CHECKSUM	    equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

	dd MULTIBOOT_HEADER_MAGIC
	dd MULTIBOOT_HEADER_FLAGS
	dd MULTIBOOT_CHECKSUM

	dd mboot
	dd _start_text
	dd _start_bss
	dd 0x1000000 - 0x200000
	dd bootstrap

	; Graphics things
	dd 0
	dd 1280
	dd 720
	dd 32