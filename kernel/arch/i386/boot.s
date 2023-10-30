.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set VIDEO_MODE,  1<<2
.set FLAGS,    VIDEO_MODE|ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)
 
#.section .multiboot
.align 16, 0
.section .multiboot.data, "aw"
	.align 4
	.long MAGIC
	.long FLAGS
	.long CHECKSUM
	# unused(padding)
	.long 0,0,0,0,0
	# Video mode
	.long   0       # Linear graphics
	.long   0       # Preferred width
	.long   0       # Preferred height
	.long   32      # Preferred pixel depth

# Allocate the initial stack.
.section .bootstrap_stack, "aw", @nobits
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

.global boot_page_directory
.global boot_page_table1
.section .bss, "aw", @nobits
	.align 4096
boot_page_directory:
	.skip 4096
boot_page_table1:
	.skip 4096

.section .multiboot.text, "a"
.global _start
.extern _kernel_end
.type _start, @function
# Kernel entry
_start:
    # edi contains the buffer we wish to modify
	movl $(boot_page_table1 - 0xC0000000), %edi

    # for loop
	movl $0, %esi
	movl $1024, %ecx
1:
    # esi = address
    # If we are in kernel range jump to "label 2"
	cmpl $_kernel_start, %esi
	jl 2f

    # If we are past the kernel jump to the final stage
    # "label 3"
	cmpl $(_kernel_end - 0xC0000000), %esi
	jge 3f
    
2:
    # Add permission to the pages
	movl %esi, %edx
	orl $0x003, %edx
	movl %edx, (%edi)

	# Size of page is 4096 bytes.
	addl $4096, %esi
	# Size of entries in boot_page_table1 is 4 bytes.
	addl $4, %edi
	# Loop to the next entry if we haven't finished.
	loop 1b

3:
	# Map the page table to both virtual addresses 0x00000000 and 0xC0000000.
	movl $(boot_page_table1 - 0xC0000000 + 0x003), boot_page_directory - 0xC0000000 + 0
	movl $(boot_page_table1 - 0xC0000000 + 0x003), boot_page_directory - 0xC0000000 + 768 * 4

	# Set cr3 to the address of the boot_page_directory.
	movl $(boot_page_directory - 0xC0000000), %ecx
	movl %ecx, %cr3

	# Enable paging
	movl %cr0, %ecx
	orl $0x80000000, %ecx
	movl %ecx, %cr0

	# Jump to higher half with an absolute jump. 
	lea 4f, %ecx
	jmp *%ecx

.size _start, . - _start

.section .text

4:
	# At this point, paging is fully set up and enabled.

	# Unmap the identity mapping as it is now unnecessary. 
	movl $0, boot_page_directory + 0

	# Reload crc3 to force a TLB flush so the changes to take effect.
	movl %cr3, %ecx
	movl %ecx, %cr3

	# Set up the stack.
	mov $stack_top, %esp

        # Reset EFLAGS.
        pushl   $0
        popf

	pushl %esp

        /* Push the pointer to the Multiboot information structure. */
        pushl   %ebx
        /* Push the magic value. */
        pushl   %eax

	mov $_kernel_end, %eax
        pushl   %eax
	# Enter the high-level kernel.
	call kernel_main

	# Infinite loop if the system has nothing more to do.
	cli
1:	hlt
	jmp 1b
