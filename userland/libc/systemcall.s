.global syscall
.global s_syscall
syscall:
  push %ebp
  mov %esp,%ebp
  push %edi
  push %esi
  push %ebx
  mov 0x1C(%ebp), %edi
  mov 0x18(%ebp), %esi
  mov 0x14(%ebp), %edx
  mov 0x10(%ebp), %ecx
  mov 0xc(%ebp), %ebx
  mov 0x8(%ebp), %eax
  int $0x80
  pop %ebx
  pop %esi
  pop %edi
  leave
  ret

s_syscall:
  push %ebp
  mov %esp,%ebp
  movl 0x8(%ebp), %eax
  int $0x80
  leave
  ret
