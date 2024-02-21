.intel_syntax noprefix
.global get_current_sp
.global get_current_sbp
.global halt
.global get_cr2
.global set_sp
.global set_sbp
.global flush_tlb
.global set_cr3
.global get_cr3
.global enable_paging

get_current_sp:
	mov eax, esp
	sub eax, 4
	ret

get_current_sbp:
	mov eax, ebp
	ret

set_sp:
	mov ecx, [esp] # store the return address in ecx
	mov eax, [esp + 4]
	mov esp, eax
	jmp ecx # jump to the return address instead of doing a ret as the stack values may have changed

set_sbp:
	mov eax, [esp + 4]
	mov ebp, eax
	ret

halt:
	hlt
	jmp $
	ret

get_cr2:
	mov eax, cr2
	ret

flush_tlb:
	mov eax, cr3
	mov cr3, eax
	ret

set_cr3:
	mov eax, [esp + 4]
	mov cr3, eax
	ret

get_cr3:
	mov eax, cr3
	ret

enable_paging:
	mov eax, cr0
        or eax, 0b10000000000000000000000000000000
	mov cr0, eax
	ret
