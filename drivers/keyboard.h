#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <cpu/io.h>
#include <cpu/idt.h>

void install_keyboard(void);
void add_keyboard(void);

#endif
