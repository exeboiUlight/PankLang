section .text
global main
main:
    xor eax, eax    ; Clear eax
    mov eax, 42     ; Set return value to 42
    ret             ; Return