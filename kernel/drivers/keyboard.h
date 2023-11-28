#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <cpu/idt.h>
#include <cpu/io.h>
#include <typedefs.h>

void install_keyboard(void);
void add_keyboard(void);

#endif
