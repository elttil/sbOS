#ifndef SETJMP_H
#define SETJMP_H
typedef unsigned long __jmp_buf[6];
typedef struct __jmp_buf_tag {
	__jmp_buf __jb;
	unsigned long __fl;
	unsigned long __ss[128/sizeof(long)];
} jmp_buf[1];
typedef jmp_buf sigjmp_buf;

void _longjmp(jmp_buf, int);
void longjmp(jmp_buf, int);
void siglongjmp(sigjmp_buf, int);
#endif
