; If you meet compile error, try 'sudo apt install gcc-multilib g++-multilib' first

%include "head.include"
; you code here

your_if:
; put your implementation here
    mov eax,[a1]

    cmp eax,40
    jl branch1

    add eax,3
    cdq   ;extend edx as eax's sign bit
    mov ecx,5
    idiv ecx
    mov [if_flag],eax

    jmp done
branch1:
    cmp eax,18
    jl branch2

    mov ecx,eax
    shl ecx,1
    mov eax,80
    sub eax,ecx
    mov [if_flag],eax

    jmp done
branch2:
    shl eax,5
    mov [if_flag],eax
done:
    
your_while:
; put your implementation here
    mov eax,[a2]
    mov esi,[while_flag]
while:
    cmp eax,25
    jge endwhile
    mov [a2],eax

    call my_random
    mov ecx,[a2]
    shl ecx,1
    mov [esi+ecx],al
    
    mov eax,[a2]
    inc eax
    jmp while
endwhile:
    mov [a2],eax

%include "end.include"

your_function:
; put your implementation here
    mov esi,[your_string]
    xor ecx,ecx

for:
    mov al,[esi+ecx]
    test al,al
    jz end_for

    pushad

    add al,9
    movzx eax,al
    push eax
    call print_a_char
    add esp,4

    popad
    inc ecx
    jmp for
end_for:
    ret

