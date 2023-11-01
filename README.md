# sbOS

A mostly from scratch, UNIX like x86 hobbyist operaing system. Kernel,
libc and the rest of the userland are written from scratch. It only
requires a bootloader that supports the first version of multiboot(such
as GRUB).

## Features

* Paging
* Interrupts
* Process scheduling
* VBE graphics
* PS2 Mouse/Keyboard
* VFS
* ext2 filesystem
* DevFS
* UNIX sockets
* libc
* Window Manager
* Terminal Emulator
* Simple Text Editor(ante)
* Very basic shell
* Shell utilities(cat, yes, echo etc)
* PCI (it works for what it is currently used for)
* rtl8139 Network Card (works ish)
* ARP/Ethernet/IPv4/UDP
* A very simple TCP implementation

and some other stuff.

## Screenshot

![sbOS running DOOM](doom.png "sbOS running DOOM")

sbOS running DOOM, more specifically [doomgeneric](https://github.com/ozkl/doomgeneric) created by [ozkl](https://github.com/ozkl)

## How do I run it?

I don't know why you would, it is not well supported and it does not
have anything intreasting to look at. But if you really want to then you
can build the toolchain by running.
`meta/toolchain.sh`
and build the full system using
`meta/build.sh`

You need the packages listed by the
[osdev](https://wiki.osdev.org/GCC_Cross-Compiler#Installing_Dependencies)
wiki and some other packages for setting up grub that I forgot what they
are.

I have only tested this a few times when distro hopping so no clue if it
still works.

## Why?

fun
