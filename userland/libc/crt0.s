.global _start
.extern main
_start:
	call main
	mov %eax, %ebx
	mov $8, %eax
	int $0x80
l:
	nop
	nop
	nop
	nop
	jmp l
