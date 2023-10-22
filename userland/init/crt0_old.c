#include <stdint.h>
int main(int argc, char **argv);
//int main();

void _start(int a, int b, int argc, char** argv)
{
	(void)a;
	kprintf("argc : %x\n", &argc);
	kprintf("argv : %x\n", &argv);
	for(;;);
//	main(argc, t);
}
