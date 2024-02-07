.intel_syntax noprefix
.global copy_page_physical
.global loop_m
copy_page_physical:
   push ebx              
   pushf                 
                        
   cli                 
                      
   mov ebx, [esp+12] 
   mov ecx, [esp+16]

   mov edx, cr0        
   and edx, 0x7fffffff
   mov cr0, edx      

   mov edx, 1024    

.loop:
   mov eax, [ebx]  
   mov [ecx], eax 
   add ebx, 4    
   add ecx, 4   
   dec edx     
   jnz .loop

   mov edx, cr0 
   or  edx, 0x80000000 
   mov cr0, edx       

   popf              
   pop ebx          
   ret


.extern current_task_TCB
.global TCB

.section .data

    .struct 0
TCB: 
TCB.ESP:
    .space 4
TCB.CR3:
    .space 4
TCB.ESP0:
    .space 4

.global switch_to_task
.global insert_eip_on_stack
.global internal_fork
.extern create_process

.section .text

internal_fork:
    push ebp
    mov ecx, esp
    mov ebp, esp

    mov eax, [ebp+8] # current_task

    lea edx, after_internal_fork
    push edx
    push ecx
    push eax
    call create_process
    add esp, 0xC

    pop ebp
    ret
after_internal_fork:
    pop ebp
    mov eax, 0
    ret

insert_eip_on_stack:
    push ebp
    mov ebp, esp
    mov eax, [ebp+8] # cr3
    mov ecx, [ebp+8+4] # address
    mov edx, [ebp+8+4+4] # value

    push ebx

    mov ebx, cr3
    mov cr3, eax

    mov [ecx], edx

    mov cr3, ebx

    pop ebx
    pop ebp
    ret

# C declaration:
#    void switch_to_task(thread_control_block *next_thread);
# 
# WARNING: Caller is expected to disable IRQs before calling, and enable IRQs again after function returns

switch_to_task:
	cli
 
    # Save previous task's state
 
    # Notes:
    #   For cdecl; EAX, ECX, and EDX are already saved by the caller and don't need to be saved again
    #   EIP is already saved on the stack by the caller's "CALL" instruction
    #   The task isn't able to change CR3 so it doesn't need to be saved
    #   Segment registers are constants (while running kernel code) so they don't need to be saved
 
    push ebx
    push esi
    push edi
    push ebp
 
    mov edi,[current_task_TCB]    # edi = address of the previous task's "thread control block"
    mov [edi+TCB.ESP],esp         # Save ESP for previous task's kernel stack in the thread's TCB
 
    # Load next task's state
 
    mov esi,[esp+(4+1)*4]         # esi = address of the next task's "thread control block" (parameter passed on stack)
    mov [current_task_TCB],esi    # Current task's TCB is the next task TCB
 
    mov esp,[esi+TCB.ESP]         # Load ESP for next task's kernel stack from the thread's TCB
    mov eax,[esi+TCB.CR3]         # eax = address of page directory for next task
    mov ebx,[esi+TCB.ESP0]        # ebx = address for the top of the next task's kernel stack
#    mov [TSS.ESP0],ebx            # Adjust the ESP0 field in the TSS (used by CPU for for CPL=3 -> CPL=0 privilege level changes)
#    mov ecx,cr3                   # ecx = previous task's virtual address space
 
# FIXME: This branch gets a from the assembler, something about "relaxed branches".
# this branch would probably not be used anyway but should be checked on later anyway.
#    cmp eax,ecx                   # Does the virtual address space need to being changed?

#    je .doneVAS                   #  no, virtual address space is the same, so don't reload it and cause TLB flushes
    mov cr3,eax                   #  yes, load the next task's virtual address space
#.doneVAS:
 
    pop ebp
    pop edi
    pop esi
    pop ebx
 
    sti
    ret                           # Load next task's EIP from its kernel stack
