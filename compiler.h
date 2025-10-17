#ifndef COMPILER_H
#define COMPILER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#define MAX_TOKEN_LENGTH 256
#define MAX_LABELS 1000
#define MAX_SYMBOLS 1000
#define CODE_BUFFER_SIZE 65536
#define MAX_RELOCATIONS 1000

typedef enum {
    TOKEN_EOF,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_NEWLINE,
    TOKEN_INSTRUCTION,
    TOKEN_DIRECTIVE,
    TOKEN_REGISTER
} TokenType;

typedef struct {
    TokenType type;
    char value[MAX_TOKEN_LENGTH];
    int line;
    int column;
} Token;

typedef struct {
    char name[MAX_TOKEN_LENGTH];
    uint32_t address;
    int is_defined;
    int is_external;
} Label;

typedef struct {
    char name[MAX_TOKEN_LENGTH];
    uint8_t data[16];
    int size;
    int is_initialized;
} Symbol;

typedef struct {
    char label_name[MAX_TOKEN_LENGTH];
    uint32_t offset;
    int size; // 1, 2, 4 bytes
    int is_relative;
} Relocation;

typedef struct {
    FILE* input;
    Token current_token;
    int line;
    int column;
    uint32_t address_counter;
    uint32_t data_address_counter;
    
    Label labels[MAX_LABELS];
    int label_count;
    
    Symbol symbols[MAX_SYMBOLS];
    int symbol_count;
    
    uint8_t code_buffer[CODE_BUFFER_SIZE];
    uint32_t code_size;
    
    uint8_t data_buffer[CODE_BUFFER_SIZE];
    uint32_t data_size;
    
    Relocation relocations[MAX_RELOCATIONS];
    int relocation_count;
    
    int is_64bit;
    int has_entry_point;
    char entry_point[MAX_TOKEN_LENGTH];
} CompilerState;

// Функции лексического анализа
void init_compiler(CompilerState* state, const char* input_file, int is_64bit);
void next_token(CompilerState* state);
void match_token(CompilerState* state, TokenType expected);

// Функции синтаксического анализа
void parse_program(CompilerState* state);
void parse_line(CompilerState* state);
void parse_instruction(CompilerState* state);
void parse_directive(CompilerState* state);

// Функции генерации кода
void emit_byte(CompilerState* state, uint8_t byte);
void emit_word(CompilerState* state, uint16_t word);
void emit_dword(CompilerState* state, uint32_t dword);
void emit_qword(CompilerState* state, uint64_t qword);
void emit_bytes(CompilerState* state, const uint8_t* bytes, int count);

// Генерация машинного кода для инструкций
void generate_mov(CompilerState* state);
void generate_arithmetic(CompilerState* state, int opcode);
void generate_push(CompilerState* state);
void generate_pop(CompilerState* state);
void generate_call(CompilerState* state);
void generate_ret(CompilerState* state);
void generate_jump(CompilerState* state, int opcode);

// Функции для работы с PE файлами
void generate_pe_file(CompilerState* state, const char* output_file);
void write_dos_header(FILE* output);
void write_pe_header(FILE* output, CompilerState* state);
void write_section_table(FILE* output, CompilerState* state, int section_count);

// Вспомогательные функции
int is_instruction(const char* token);
int is_directive(const char* token);
int is_register(const char* token);
void add_label(CompilerState* state, const char* name, int is_data);
int find_label(CompilerState* state, const char* name);
void add_relocation(CompilerState* state, const char* label, uint32_t offset, int size, int is_relative);
int get_register_code(const char* reg, int is_64bit);

#endif