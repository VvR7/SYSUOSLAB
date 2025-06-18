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


mov ah, 0x74 ; red
mov al, '2'
mov [gs:2580], ax

mov al, '3'
mov [gs:2582], ax

mov al, '3'
mov [gs:2584], ax

mov al, '3'
mov [gs:2586], ax

mov al, '6'
mov [gs:2588], ax

mov al, '3'
mov [gs:2590], ax

mov al, '2'
mov [gs:2592], ax

mov al, '6'
mov [gs:2594], ax

jmp $ ; 死循环

times 510 - ($ - $$) db 0
db 0x55, 0xaa
