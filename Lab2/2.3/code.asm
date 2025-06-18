org 0x7c00
[bits 16]
xor ax, ax ; eax = 0
; 初始化段寄存器, 段地址全部设为0
mov ds, ax
mov ss, ax
mov es, ax
mov fs, ax
mov gs, ax

; 初始化栈指针
mov sp, 0x7c00
mov ax, 0xb800
mov gs, ax


start:
    mov ah,00H
    int 16h

    cmp al,0dH  ;Enter
    je handle_enter

    cmp al,08H  ;backspace
    je handle_backspace

    call print

    cmp al,27
    jne start

    jmp exit
handle_enter:
    mov al,0dH
    call print
    mov al,0aH
    call print
    jmp start

handle_backspace:
    mov ah,03H
    mov bh,00H
    int 10h

    cmp dl,00H   ;handle the front of the row
    je start

    mov al,08H
    call print

    mov al,' '
    call print

    mov al,08H
    call print

    jmp start

print:
    mov ah,0eH
    mov bh,00H
    int 10h
    ret
exit:
jmp $ ; 死循环


times 510 - ($ - $$) db 0
db 0x55, 0xaa
