	.section .text.entry
	.globl _entry
_entry:
	li t0, 32768
	mul t0, a0, t0
	la t1, boot_stack_top
	sub sp, t1, t0
	call main

	.section .bss.stack
	.globl boot_stack
boot_stack:
	.space 4096 * 16
	.globl boot_stack_top
boot_stack_top:
