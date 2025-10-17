section .data
    message db "Hello, World!", 0

section .text
global main
main:
    mov eax, 0
    ret