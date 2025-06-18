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
mov byte [mincol],0
mov byte [maxcol],79
mov byte [minrow],0
mov byte [maxrow],24
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

    ;border check
    push ax
    mov al,[mincol]
    cmp al,[maxcol]
    jg done

    mov al,[minrow]
    cmp al,[maxrow]
    jg done
    pop ax

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
    mov cx,0
    mov cl,[maxcol]
    cmp si,cx
    jbe updatechar

    mov cx,0
    mov cl,[maxcol]
    mov si,cx
    inc byte [minrow]
    mov bx,1
    inc di
    jmp updatechar

down:
    inc di
    mov cx,0
    mov cl,[maxrow]
    cmp di,cx
    jbe updatechar

    mov cx,0
    mov cl,[maxrow]
    mov di,cx
    dec byte [maxcol]
    mov bx,2
    dec si
    jmp updatechar

left:
    dec si
    mov cx,0
    mov cl,[mincol]
    cmp si,cx
    jge updatechar

    mov cx,0
    mov cl,[mincol]
    mov si,cx
    dec byte [maxrow]
    mov bx,3
    dec di
    jmp updatechar

up:
    dec di
    mov cx,0
    mov cl,[minrow]
    cmp di,cx
    jge updatechar

    mov cx,0
    mov cl,[minrow]
    mov di,cx
    mov bx,0
    inc byte [mincol]
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
    pop ax
jmp $ ; 死循环

color db 1
mincol db 0
maxcol db 79
minrow db 0
maxrow db 24
times 510 - ($ - $$) db 0
db 0x55, 0xaa
