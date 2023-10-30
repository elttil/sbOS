.intel_syntax noprefix
.global jump_signal_handler
jump_signal_handler:
	mov ebx, [esp+8]
	mov ecx, [esp+4]
	mov ax, (4 * 8) | 3 # ring 3 data with bottom 2 bits set for ring 3
	mov ds, ax
	mov es, ax 
	mov fs, ax 
	mov gs, ax # SS is handled by iret
 
	mov eax, ebx
	push (4 * 8) | 3 # data selector
	push eax
	pushf
	push (3 * 8) | 3
	push ecx
	iret
