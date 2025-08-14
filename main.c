#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

typedef void (*DLL_Func)();
void* load_dll(const char* dll_name);
DLL_Func get_dll_func(void* dll, const char* func_name);
void close_dll(void* dll);
void print(const char* str);
int input_int();


typedef void (*DLL_Func)();
void* load_dll(const char* dll_name) {
    return (void*)LoadLibraryA(dll_name);
}
DLL_Func get_dll_func(void* dll, const char* func_name) {
    return (DLL_Func)GetProcAddress((HMODULE)dll, func_name);
}
void close_dll(void* dll) {
    FreeLibrary((HMODULE)dll);
}

    void* console_dll = load_dll("console.dll");
    if (console_dll) {
        DLL_Func console_print_func = get_dll_func(console_dll, "print");
        if (console_print_func) console_print_func();
        close_dll(console_dll);
    }
    int x = console;
    void* console_dll = load_dll("console.dll");
    if (console_dll) {
        DLL_Func console_print_func = get_dll_func(console_dll, "print");
        if (console_print_func) console_print_func();
        close_dll(console_dll);
    }
    void* console_dll = load_dll("console.dll");
    if (console_dll) {
        DLL_Func console_print_func = get_dll_func(console_dll, "print");
        if (console_print_func) console_print_func();
        close_dll(console_dll);
    }
int main() {
    return 0;
}