.intel_syntax noprefix
.global outsw
.global outb
.global outw
.global outl
.global inb
.global inw
.global inl
.global rep_outsw
.global rep_insw
.global flush_tss
.global load_idtr

# ebx, esi, edi, ebp, and esp;
outsw:
        push ebp
	mov ebp, esp
	push esi
	mov dx, [ebp + 4+4]
	mov esi, [ebp + 8+4]
	outsw
	pop esi
	mov esp, ebp
	pop ebp
	ret

outl:
	mov eax, [esp + 8]
	mov dx, [esp + 4]
	out dx, eax
	ret

outb:
	mov al, [esp + 8]
	mov dx, [esp + 4]
	out dx, al
	ret

outw:
	mov ax, [esp + 8]
	mov dx, [esp + 4]
	out dx, ax
	ret

inl:
	mov dx, [esp + 4]
	in  eax, dx
	ret

inw:
	mov dx, [esp + 4]
	in  ax, dx
	ret

inb:
	mov dx, [esp + 4]
	in  al, dx
	ret

rep_outsw:
        push ebp
	mov ebp, esp
	push edi
	mov ecx, [ebp + 4+4]      #ECX is counter for OUTSW
	mov edx, [ebp + 8+4]      #Data port, in and out
	mov edi, [ebp + 12+4]		#Memory area
	rep outsw               #in to [RDI]
	pop edi
	mov esp, ebp
	pop ebp
	ret

rep_insw:
        push ebp
	mov ebp, esp
	push edi
	mov ecx, [ebp + 4+4]      #ECX is counter for INSW
	mov edx, [ebp + 8+4]      #Data port, in and out
	mov edi, [ebp + 12+4]		#Memory area
	rep insw                #in to [RDI]
	pop edi
	mov esp, ebp
	pop ebp
	ret

flush_tss:
	mov ax, 40
	ltr ax
	ret

load_idtr:
	mov edx, [esp + 4]
	lidt [edx]
	ret

test_user_function:
	mov eax, 0x00
	int 0x80
	mov ecx, 0x03
	mov ebx, 0x04
	mov eax, 0x02
	int 0x80
	mov ebx, eax
	mov eax, 0x01
	int 0x80
loop:
	jmp loop
	ret

.global spin_lock
.global spin_unlock
.extern locked

spin_lock:
	mov eax, 1
	xchg eax, ebx
	test eax, eax     
	jnz spin_lock
	ret

spin_unlock:
	xor eax, eax
	xchg eax, ebx
	ret


.global jump_usermode
jump_usermode:
	mov ax, (4 * 8) | 3 # user data segment with RPL 3
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax # sysexit sets SS
 
	# setup wrmsr inputs
	xor edx, edx # not necessary; set to 0
	mov eax, 0x100008 # SS=0x10+0x10=0x20, CS=0x8+0x10=0x18
	mov ecx, 0x174 # MSR specifier: IA32_SYSENTER_CS
	wrmsr # set sysexit segments
 
	# setup sysexit inputs
	mov edx, [esp + 4] # to be loaded into EIP
	mov ecx, [esp + 8] # to be loaded into ESP
	mov esp, ecx
	mov ebp, ecx
	sti
	sysexit
