%include "boot.inc"

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

mov ax, LOADER_START_SECTOR
mov cx, LOADER_SECTOR_COUNT
mov bx, LOADER_START_ADDRESS   

mov bx, 0x7e00           ; bootloader的加载地址
mov ah, 02h              ;功能:读扇区
mov ch, 00h              ;柱面
mov cl, 02h              ;扇区  (LBA从0开始,CHS的扇区从1开始)
mov dh, 00h              ;磁头
mov al, 05h              ;扇区数
mov dl, 80h              ;驱动器:80h-0FFh是硬盘
int 13h
jmp 0x0000:0x7e00        ; 跳转到bootloader

times 510 - ($ - $$) db 0
db 0x55, 0xaa