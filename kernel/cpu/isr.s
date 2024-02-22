.intel_syntax noprefix
.section .text
# Due to x86 not giving the interrupt number these isr stubs are
# created. This is to allow a general isr stub for all interrupts. This
# stub will act as a # multiplexer to the correct interrupt.
.macro ISR_NOERRCODE num
  .global isr\num
  isr\num:
    cli 
    push 0
    push \num
    jmp isr_common_stub
.endm

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_NOERRCODE 8
ISR_NOERRCODE 9
ISR_NOERRCODE 10
ISR_NOERRCODE 11
ISR_NOERRCODE 12
ISR_NOERRCODE 13
ISR_NOERRCODE 14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_NOERRCODE 17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31
ISR_NOERRCODE 32
ISR_NOERRCODE 33
ISR_NOERRCODE 34
ISR_NOERRCODE 35
ISR_NOERRCODE 36
ISR_NOERRCODE 37
ISR_NOERRCODE 38
ISR_NOERRCODE 39
ISR_NOERRCODE 40
ISR_NOERRCODE 41
ISR_NOERRCODE 42
ISR_NOERRCODE 43
ISR_NOERRCODE 44
ISR_NOERRCODE 45
ISR_NOERRCODE 46
ISR_NOERRCODE 47
ISR_NOERRCODE 48
ISR_NOERRCODE 49
ISR_NOERRCODE 50
ISR_NOERRCODE 51
ISR_NOERRCODE 52
ISR_NOERRCODE 53
ISR_NOERRCODE 54
ISR_NOERRCODE 55
ISR_NOERRCODE 56
ISR_NOERRCODE 57
ISR_NOERRCODE 58
ISR_NOERRCODE 59
ISR_NOERRCODE 60
ISR_NOERRCODE 61
ISR_NOERRCODE 62
ISR_NOERRCODE 63
ISR_NOERRCODE 64
ISR_NOERRCODE 65
ISR_NOERRCODE 66
ISR_NOERRCODE 67
ISR_NOERRCODE 68
ISR_NOERRCODE 69
ISR_NOERRCODE 70
ISR_NOERRCODE 71
ISR_NOERRCODE 72
ISR_NOERRCODE 73
ISR_NOERRCODE 74
ISR_NOERRCODE 75
ISR_NOERRCODE 76
ISR_NOERRCODE 77
ISR_NOERRCODE 78
ISR_NOERRCODE 79
ISR_NOERRCODE 80
ISR_NOERRCODE 81
ISR_NOERRCODE 82
ISR_NOERRCODE 83
ISR_NOERRCODE 84
ISR_NOERRCODE 85
ISR_NOERRCODE 86
ISR_NOERRCODE 87
ISR_NOERRCODE 88
ISR_NOERRCODE 89
ISR_NOERRCODE 90
ISR_NOERRCODE 91
ISR_NOERRCODE 92
ISR_NOERRCODE 93
ISR_NOERRCODE 94
ISR_NOERRCODE 95
ISR_NOERRCODE 96
ISR_NOERRCODE 97
ISR_NOERRCODE 98
ISR_NOERRCODE 99
ISR_NOERRCODE 100
ISR_NOERRCODE 101
ISR_NOERRCODE 102
ISR_NOERRCODE 103
ISR_NOERRCODE 104
ISR_NOERRCODE 105
ISR_NOERRCODE 106
ISR_NOERRCODE 107
ISR_NOERRCODE 108
ISR_NOERRCODE 109
ISR_NOERRCODE 110
ISR_NOERRCODE 111
ISR_NOERRCODE 112
ISR_NOERRCODE 113
ISR_NOERRCODE 114
ISR_NOERRCODE 115
ISR_NOERRCODE 116
ISR_NOERRCODE 117
ISR_NOERRCODE 118
ISR_NOERRCODE 119
ISR_NOERRCODE 120
ISR_NOERRCODE 121
ISR_NOERRCODE 122
ISR_NOERRCODE 123
ISR_NOERRCODE 124
ISR_NOERRCODE 125
ISR_NOERRCODE 126
ISR_NOERRCODE 127
ISR_NOERRCODE 128
ISR_NOERRCODE 129
ISR_NOERRCODE 130
ISR_NOERRCODE 131
ISR_NOERRCODE 132
ISR_NOERRCODE 133
ISR_NOERRCODE 134
ISR_NOERRCODE 135
ISR_NOERRCODE 136
ISR_NOERRCODE 137
ISR_NOERRCODE 138
ISR_NOERRCODE 139
ISR_NOERRCODE 140
ISR_NOERRCODE 141
ISR_NOERRCODE 142
ISR_NOERRCODE 143
ISR_NOERRCODE 144
ISR_NOERRCODE 145
ISR_NOERRCODE 146
ISR_NOERRCODE 147
ISR_NOERRCODE 148
ISR_NOERRCODE 149
ISR_NOERRCODE 150
ISR_NOERRCODE 151
ISR_NOERRCODE 152
ISR_NOERRCODE 153
ISR_NOERRCODE 154
ISR_NOERRCODE 155
ISR_NOERRCODE 156
ISR_NOERRCODE 157
ISR_NOERRCODE 158
ISR_NOERRCODE 159
ISR_NOERRCODE 160
ISR_NOERRCODE 161
ISR_NOERRCODE 162
ISR_NOERRCODE 163
ISR_NOERRCODE 164
ISR_NOERRCODE 165
ISR_NOERRCODE 166
ISR_NOERRCODE 167
ISR_NOERRCODE 168
ISR_NOERRCODE 169
ISR_NOERRCODE 170
ISR_NOERRCODE 171
ISR_NOERRCODE 172
ISR_NOERRCODE 173
ISR_NOERRCODE 174
ISR_NOERRCODE 175
ISR_NOERRCODE 176
ISR_NOERRCODE 177
ISR_NOERRCODE 178
ISR_NOERRCODE 179
ISR_NOERRCODE 180
ISR_NOERRCODE 181
ISR_NOERRCODE 182
ISR_NOERRCODE 183
ISR_NOERRCODE 184
ISR_NOERRCODE 185
ISR_NOERRCODE 186
ISR_NOERRCODE 187
ISR_NOERRCODE 188
ISR_NOERRCODE 189
ISR_NOERRCODE 190
ISR_NOERRCODE 191
ISR_NOERRCODE 192
ISR_NOERRCODE 193
ISR_NOERRCODE 194
ISR_NOERRCODE 195
ISR_NOERRCODE 196
ISR_NOERRCODE 197
ISR_NOERRCODE 198
ISR_NOERRCODE 199
ISR_NOERRCODE 200
ISR_NOERRCODE 201
ISR_NOERRCODE 202
ISR_NOERRCODE 203
ISR_NOERRCODE 204
ISR_NOERRCODE 205
ISR_NOERRCODE 206
ISR_NOERRCODE 207
ISR_NOERRCODE 208
ISR_NOERRCODE 209
ISR_NOERRCODE 210
ISR_NOERRCODE 211
ISR_NOERRCODE 212
ISR_NOERRCODE 213
ISR_NOERRCODE 214
ISR_NOERRCODE 215
ISR_NOERRCODE 216
ISR_NOERRCODE 217
ISR_NOERRCODE 218
ISR_NOERRCODE 219
ISR_NOERRCODE 220
ISR_NOERRCODE 221
ISR_NOERRCODE 222
ISR_NOERRCODE 223
ISR_NOERRCODE 224
ISR_NOERRCODE 225
ISR_NOERRCODE 226
ISR_NOERRCODE 227
ISR_NOERRCODE 228
ISR_NOERRCODE 229
ISR_NOERRCODE 230
ISR_NOERRCODE 231
ISR_NOERRCODE 232
ISR_NOERRCODE 233
ISR_NOERRCODE 234
ISR_NOERRCODE 235
ISR_NOERRCODE 236
ISR_NOERRCODE 237
ISR_NOERRCODE 238
ISR_NOERRCODE 239
ISR_NOERRCODE 240
ISR_NOERRCODE 241
ISR_NOERRCODE 242
ISR_NOERRCODE 243
ISR_NOERRCODE 244
ISR_NOERRCODE 245
ISR_NOERRCODE 246
ISR_NOERRCODE 247
ISR_NOERRCODE 248
ISR_NOERRCODE 249
ISR_NOERRCODE 250
ISR_NOERRCODE 251
ISR_NOERRCODE 252
ISR_NOERRCODE 253
ISR_NOERRCODE 254
ISR_NOERRCODE 255

isr_common_stub:
    pusha            # Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax

    mov ax, ds       # Lower 16-bits of eax = ds.
    push eax         # save the data segment descriptor

    mov ax, 0x10     # load the kernel data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call int_handler
    add esp, 4

    pop ebx          # reload the original data segment descriptor
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx

    popa             # Pops edi,esi,ebp...
    add esp, 8       # Cleans up the pushed error code and pushed ISR number
    sti
    iret             # pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP
