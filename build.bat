@echo off
gcc -o compiler compiler.c

compiler main.asm output32.exe 32

compiler main.asm output64.exe 64

pause