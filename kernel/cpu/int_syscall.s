.intel_syntax noprefix
.global int_syscall
.extern syscall_function_handler
int_syscall:
	push ebp
	push edi
	push esi
	push edx
	push ecx
	push ebx
	push eax
        call syscall_function_handler
	add esp, 4
	pop ebx
	pop ecx
	pop edx
	pop esi
	pop edi
	pop ebp
    iretd
