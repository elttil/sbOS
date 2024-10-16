#ifndef VBE_H
#define VBE_H
#include <multiboot.h>

void vbe_drawfont(u32 px, u32 py, const u8 c);
int display_driver_init(multiboot_info_t *mb);
void display_driver_cross(multiboot_info_t *mbi);
void add_vbe_device(void);
#endif // VBE_H
