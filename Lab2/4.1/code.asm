;org 0x7c00
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

;clear screen
mov ah,06h
mov al,0
mov bh,07h
mov ch,0
mov cl,0
mov dh,24
mov dl,79
int 10h

;reset clip
mov ah,02h
mov bh,0
mov dh,0
mov dl,0
int 10h

;reset register
mov si,0   ;col
mov di,0   ;row
mov bx,0   ;direction
mov al,'1'

print:
    push bx
    mov ah,02h
    mov bh,0
    push ax
    mov ax,di
    mov dh,al
    mov ax,si
    mov dl,al
    pop ax
    int 10h

    mov ah,09h
    mov bh,0
    mov bl,[color]
    mov cx,1
    int 10h

    pop bx
    ;delay time
    mov cx,2000
outer_loop:
    mov dx,5000
inner_loop:
    dec dx
    jnz inner_loop
    loop outer_loop

    cmp bx,0
    je right

    cmp bx,1
    je down

    cmp bx,2
    je left

    cmp bx,3
    je up

    cmp bx,4
    je done

right:
    inc si
    cmp si,80
    jb updatechar

    mov si,79
    mov bx,1
    inc di
    jmp updatechar

down:
    inc di
    cmp di,25
    jb updatechar

    mov di,24
    mov bx,2
    dec si
    jmp updatechar

left:
    dec si
    cmp si,0
    jge updatechar

    mov si,0
    mov bx,3
    dec di
    jmp updatechar

up:
    dec di
    cmp di,0
    jge updatechar

    mov di,0
    mov bx,0
    inc si
    ;mov bx,4
    jmp updatechar

updatechar:
    cmp al,'9'
    je resetchar

    inc al
    jmp updatecolor
resetchar:
    mov al,'0'

updatecolor:
    push ax
    mov al,[color]
    inc al
    cmp al,256
    jne setcolor
    mov al,1
setcolor:
    mov [color],al
    pop ax

    jmp print

done:

jmp $ ; 死循环

color db 1
times 510 - ($ - $$) db 0
db 0x55, 0xaa
