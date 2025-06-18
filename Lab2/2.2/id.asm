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


mov si,msg
mov ah,0x13
mov al,0x01
mov bh,0x00
mov bl,0x74
mov dh,5
mov dl,10
mov bp,si
mov cx,8
int 0x10

jmp $ ; 死循环

msg db '23336326',0
times 510 - ($ - $$) db 0
db 0x55, 0xaa
