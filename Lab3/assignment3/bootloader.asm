%include "boot.inc"
 org 0x7e00
[bits 16]
;空描述符
mov dword [GDT_START_ADDRESS+0x00],0x00
mov dword [GDT_START_ADDRESS+0x04],0x00  

;创建描述符，这是一个数据段，对应0~4GB的线性地址空间
mov dword [GDT_START_ADDRESS+0x08],0x0000ffff    ; 基地址为0，段界限为0xFFFFF
mov dword [GDT_START_ADDRESS+0x0c],0x00cf9200    ; 粒度为4KB，存储器段描述符 

;建立保护模式下的堆栈段描述符      
mov dword [GDT_START_ADDRESS+0x10],0x00000000    ; 基地址为0x00000000，界限0x0 
mov dword [GDT_START_ADDRESS+0x14],0x00409600    ; 粒度为1个字节

;建立保护模式下的显存描述符   
mov dword [GDT_START_ADDRESS+0x18],0x80007fff    ; 基地址为0x000B8000，界限0x07FFF 
mov dword [GDT_START_ADDRESS+0x1c],0x0040920b    ; 粒度为字节

;创建保护模式下平坦模式代码段描述符
mov dword [GDT_START_ADDRESS+0x20],0x0000ffff    ; 基地址为0，段界限为0xFFFFF
mov dword [GDT_START_ADDRESS+0x24],0x00cf9800    ; 粒度为4kb，代码段描述符 

pgdt dw 0
     dd GDT_START_ADDRESS
;初始化描述符表寄存器GDTR
mov word [pgdt], 39      ;描述符表的界限   
lgdt [pgdt]
      
in al,0x92                         ;南桥芯片内的端口 
or al,0000_0010B
out 0x92,al                        ;打开A20

cli                                ;中断机制尚未工作
mov eax,cr0
or eax,1
mov cr0,eax                        ;设置PE位
      
;以下进入保护模式
jmp dword CODE_SELECTOR:protect_mode_begin

[bits 32]
protect_mode_begin:
mov eax, DATA_SELECTOR                     ;加载数据段(0..4GB)选择子
mov ds, eax
mov es, eax
mov eax, STACK_SELECTOR
mov ss, eax
mov eax, VIDEO_SELECTOR
mov gs, eax

;清屏:
mov ecx,80*25
mov edi,0
mov ah,0x07
mov al,' '
clear:
    mov [gs:edi],ax
    add edi,2
    loop clear




xor esi, esi        ; col
xor ebp, ebp        ; row
xor ebx, ebx        ; direction
mov al, '1'         ; 
mov dl, 0x01        ; color


main:
    ; 显存偏移地址:(row*80 + col)*2
    mov ecx, ebp
    imul ecx, 80
    add ecx, esi
    shl ecx, 1

    mov ah, dl
    mov word [gs:ecx], ax

    ; 延时
    push eax
    push ebx
    mov ecx, 2000      
delay_outer:
    mov eax, 5000
delay_inner:
    dec eax
    jnz delay_inner
    loop delay_outer
    pop ebx
    pop eax

    ; 更新方向
    cmp ebx, 0
    je right
    cmp ebx, 1
    je down
    cmp ebx, 2
    je left
    cmp ebx, 3
    je up

right:
    inc esi
    cmp esi, 80
    jb update_char
    mov esi, 79
    inc ebp
    mov ebx, 1
    jmp update_char

down:
    inc ebp
    cmp ebp, 25
    jb update_char
    mov ebp, 24
    dec esi
    mov ebx, 2
    jmp update_char

left:
    dec esi
    cmp esi, 0
    jge update_char
    mov esi, 0
    dec ebp
    mov ebx, 3
    jmp update_char

up:
    dec ebp
    cmp ebp, 0
    jge update_char
    mov ebp, 0
    inc esi
    mov ebx, 0

update_char:
    cmp al, '9'
    je reset_char
    inc al
    jmp update_color
reset_char:
    mov al, '0'

update_color:
    inc dl
    cmp dl, 255          ; 限制颜色在1-15之间
    jb main
    mov dl, 1
    jmp main



jmp $ ; 死循环



