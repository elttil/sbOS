.intel_syntax noprefix

.global get_tsc
.global get_hz

# 1.193182 MHz
# So 0xFFFF is roughly 0.05492 seconds
# So take the result times 18 and you got your Hz

get_tsc:
	rdtsc
	ret

get_hz:
	cli
	# Disable the gate for channel 2 so the clock can be set.
	# This should only matter if the channel already has count
	inb al, 0x61
	and al, 0xFE
	or al, 0x1
	outb 0x61, al

	# Set mode
	mov al, 0b10110010
	outb 0x43, al

	# 0x2e9b = 11931 which is close to the PIT Hz divided by 100
	mov al, 0x9b
	outb 0x42, al
	mov al, 0x2e
	outb 0x42, al

	rdtsc
	mov ecx, eax
	mov esi, edx

	# Set the gate for channel 2
	inb al, 0x61
	or al, 0x1
	outb 0x61, al

	# The fifth bit will(seems to) flip when the count is low.
	and al, 0x20
	jnz none_zero_check

zero_check:
	inb al, 0x61
	andb al, 0x20
	cmp al, 0
	jz zero_check
	jmp end

none_zero_check:
	inb al, 0x61
	andb al, 0x20
	cmp al, 0
	jnz none_zero_check
end:
	rdtsc

	sub eax, ecx
	sub edx, esi
	ret
