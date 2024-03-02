.intel_syntax noprefix
.global int_syscall
.extern syscall_functions
int_syscall:
sti
push   ebp
mov    ebp,esp
push   edi
push   esi
push   ebx

mov    edx,DWORD PTR [ebp+0x8] # reg_t*
mov    eax,DWORD PTR [edx+0x20] # syscall number
mov    eax,DWORD PTR [eax*4+syscall_functions] # function pointer

mov    edi,DWORD PTR [edx+0x4]
push edi
mov    edi,DWORD PTR [edx+0x8]
push edi
mov    edi,DWORD PTR [edx+0x18]
push edi
mov    edi,DWORD PTR [edx+0x1c]
push edi
mov    edi,DWORD PTR [edx+0x14]
push edi

call   eax
add    esp,20
mov    edx,DWORD PTR [ebp+0x8] # reg_t*
mov    DWORD PTR [edx+0x20],eax
lea    esp,[ebp-0xc]

pop    ebx
pop    esi
pop    edi
pop    ebp
ret
