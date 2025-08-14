#include <stdio.h>

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

EXPORT void print(const char* text) {
    printf("%s", text);
}

EXPORT int input(const char* prompt) {
    printf("%s", prompt);
    int value;
    scanf("%d", &value);
    return value;
}