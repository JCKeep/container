        .code16
        .global code16, code16_end
guest16:
        # output a new line
        movb $10, %al
        outb %al, $0xE9
        movw $42, %ax
        movw %ax, 0x400
        hlt
guest16_end:
