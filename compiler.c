#include "compiler.h"
#include <stdarg.h>

// Списки инструкций и директив
const char* instructions[] = {
    "mov", "add", "sub", "imul", "idiv", "inc", "dec",
    "and", "or", "xor", "not", "shl", "shr",
    "cmp", "test", "jmp", "je", "jne", "jg", "jge", "jl", "jle",
    "call", "ret", "push", "pop", "nop", "int",
    NULL
};

const char* directives[] = {
    "section", "global", "extern", "db", "dw", "dd", "dq",
    "times", "equ", "bits",
    NULL
};

// Коды регистров
typedef struct {
    const char* name;
    int code;
} RegisterInfo;

RegisterInfo registers_32[] = {
    {"eax", 0}, {"ecx", 1}, {"edx", 2}, {"ebx", 3},
    {"esp", 4}, {"ebp", 5}, {"esi", 6}, {"edi", 7},
    {"ax", 0}, {"cx", 1}, {"dx", 2}, {"bx", 3},
    {"sp", 4}, {"bp", 5}, {"si", 6}, {"di", 7},
    {"al", 0}, {"cl", 1}, {"dl", 2}, {"bl", 3},
    {"ah", 4}, {"ch", 5}, {"dh", 6}, {"bh", 7},
    {NULL, -1}
};

RegisterInfo registers_64[] = {
    {"rax", 0}, {"rcx", 1}, {"rdx", 2}, {"rbx", 3},
    {"rsp", 4}, {"rbp", 5}, {"rsi", 6}, {"rdi", 7},
    {"r8", 8}, {"r9", 9}, {"r10", 10}, {"r11", 11},
    {"r12", 12}, {"r13", 13}, {"r14", 14}, {"r15", 15},
    {"eax", 0}, {"ecx", 1}, {"edx", 2}, {"ebx", 3},
    {"esp", 4}, {"ebp", 5}, {"esi", 6}, {"edi", 7},
    {"ax", 0}, {"cx", 1}, {"dx", 2}, {"bx", 3},
    {"sp", 4}, {"bp", 5}, {"si", 6}, {"di", 7},
    {"al", 0}, {"cl", 1}, {"dl", 2}, {"bl", 3},
    {NULL, -1}
};

// Глобальная переменная для доступа в is_register
static CompilerState* current_state = NULL;

void init_compiler(CompilerState* state, const char* input_file, int is_64bit) {
    state->input = fopen(input_file, "r");
    if (!state->input) {
        fprintf(stderr, "Error: Cannot open input file %s\n", input_file);
        exit(1);
    }
    
    state->line = 1;
    state->column = 0;
    state->address_counter = 0;
    state->data_address_counter = 0;
    state->code_size = 0;
    state->data_size = 0;
    state->label_count = 0;
    state->symbol_count = 0;
    state->relocation_count = 0;
    state->is_64bit = is_64bit;
    state->has_entry_point = 0;
    state->entry_point[0] = '\0';
    
    memset(state->code_buffer, 0, CODE_BUFFER_SIZE);
    memset(state->data_buffer, 0, CODE_BUFFER_SIZE);
    
    current_state = state;
}

void next_token(CompilerState* state) {
    int c;
    int i = 0;
    
    // Пропускаем пробелы
    while ((c = fgetc(state->input)) != EOF && isspace(c) && c != '\n') {
        state->column++;
    }
    
    if (c == EOF) {
        state->current_token.type = TOKEN_EOF;
        strcpy(state->current_token.value, "");
        return;
    }
    
    state->current_token.line = state->line;
    state->current_token.column = state->column;
    
    // Обработка комментариев
    if (c == ';') {
        while ((c = fgetc(state->input)) != EOF && c != '\n') {
            state->column++;
        }
        if (c == '\n') {
            state->line++;
            state->column = 0;
        }
        next_token(state);
        return;
    }
    
    // Обработка новой строки
    if (c == '\n') {
        state->current_token.type = TOKEN_NEWLINE;
        strcpy(state->current_token.value, "\n");
        state->line++;
        state->column = 0;
        return;
    }
    
    // Обработка чисел
    if (isdigit(c) || c == '-') {
        state->current_token.type = TOKEN_NUMBER;
        state->current_token.value[i++] = c;
        state->column++;
        
        while ((c = fgetc(state->input)) != EOF && (isdigit(c) || isxdigit(c) || c == 'x' || c == 'h')) {
            if (i < MAX_TOKEN_LENGTH - 1) {
                state->current_token.value[i++] = c;
            }
            state->column++;
        }
        ungetc(c, state->input);
        state->current_token.value[i] = '\0';
        return;
    }
    
    // Обработка строк
    if (c == '"') {
        state->current_token.type = TOKEN_STRING;
        state->column++;
        
        while ((c = fgetc(state->input)) != EOF && c != '"') {
            if (i < MAX_TOKEN_LENGTH - 1) {
                state->current_token.value[i++] = c;
            }
            state->column++;
        }
        state->column++; // для закрывающей кавычки
        state->current_token.value[i] = '\0';
        return;
    }
    
    // Обработка идентификаторов и ключевых слов
    if (isalpha(c) || c == '_' || c == '.') {
        state->current_token.value[i++] = c;
        state->column++;
        
        while ((c = fgetc(state->input)) != EOF && (isalnum(c) || c == '_' || c == '.')) {
            if (i < MAX_TOKEN_LENGTH - 1) {
                state->current_token.value[i++] = c;
            }
            state->column++;
        }
        ungetc(c, state->input);
        state->current_token.value[i] = '\0';
        
        // Определяем тип токена
        if (is_instruction(state->current_token.value)) {
            state->current_token.type = TOKEN_INSTRUCTION;
        } else if (is_directive(state->current_token.value)) {
            state->current_token.type = TOKEN_DIRECTIVE;
        } else if (is_register(state->current_token.value)) {
            state->current_token.type = TOKEN_REGISTER;
        } else {
            state->current_token.type = TOKEN_IDENTIFIER;
        }
        return;
    }
    
    // Обработка специальных символов
    switch (c) {
        case ',':
            state->current_token.type = TOKEN_COMMA;
            strcpy(state->current_token.value, ",");
            state->column++;
            break;
        case ':':
            state->current_token.type = TOKEN_COLON;
            strcpy(state->current_token.value, ":");
            state->column++;
            break;
        default:
            state->current_token.type = TOKEN_IDENTIFIER;
            state->current_token.value[0] = c;
            state->current_token.value[1] = '\0';
            state->column++;
            break;
    }
}

int is_instruction(const char* token) {
    for (int i = 0; instructions[i] != NULL; i++) {
        if (strcasecmp(token, instructions[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int is_directive(const char* token) {
    for (int i = 0; directives[i] != NULL; i++) {
        if (strcasecmp(token, directives[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int is_register(const char* token) {
    if (current_state == NULL) return 0;
    
    RegisterInfo* regs = current_state->is_64bit ? registers_64 : registers_32;
    for (int i = 0; regs[i].name != NULL; i++) {
        if (strcasecmp(token, regs[i].name) == 0) {
            return 1;
        }
    }
    return 0;
}

int get_register_code(const char* reg, int is_64bit) {
    RegisterInfo* regs = is_64bit ? registers_64 : registers_32;
    for (int i = 0; regs[i].name != NULL; i++) {
        if (strcasecmp(reg, regs[i].name) == 0) {
            return regs[i].code;
        }
    }
    return -1;
}

void emit_byte(CompilerState* state, uint8_t byte) {
    if (state->code_size < CODE_BUFFER_SIZE) {
        state->code_buffer[state->code_size++] = byte;
    }
    state->address_counter++;
}

void emit_word(CompilerState* state, uint16_t word) {
    emit_byte(state, word & 0xFF);
    emit_byte(state, (word >> 8) & 0xFF);
}

void emit_dword(CompilerState* state, uint32_t dword) {
    emit_byte(state, dword & 0xFF);
    emit_byte(state, (dword >> 8) & 0xFF);
    emit_byte(state, (dword >> 16) & 0xFF);
    emit_byte(state, (dword >> 24) & 0xFF);
}

void emit_qword(CompilerState* state, uint64_t qword) {
    emit_dword(state, qword & 0xFFFFFFFF);
    emit_dword(state, (qword >> 32) & 0xFFFFFFFF);
}

void add_label(CompilerState* state, const char* name, int is_data) {
    if (state->label_count >= MAX_LABELS) {
        fprintf(stderr, "Error: Too many labels\n");
        exit(1);
    }
    
    strcpy(state->labels[state->label_count].name, name);
    if (is_data) {
        state->labels[state->label_count].address = state->data_address_counter;
    } else {
        state->labels[state->label_count].address = state->address_counter;
    }
    state->labels[state->label_count].is_defined = 1;
    state->labels[state->label_count].is_external = 0;
    state->label_count++;
}

int find_label(CompilerState* state, const char* name) {
    for (int i = 0; i < state->label_count; i++) {
        if (strcmp(state->labels[i].name, name) == 0) {
            return state->labels[i].address;
        }
    }
    return -1;
}

void add_relocation(CompilerState* state, const char* label, uint32_t offset, int size, int is_relative) {
    if (state->relocation_count >= MAX_RELOCATIONS) {
        fprintf(stderr, "Error: Too many relocations\n");
        exit(1);
    }
    
    strcpy(state->relocations[state->relocation_count].label_name, label);
    state->relocations[state->relocation_count].offset = offset;
    state->relocations[state->relocation_count].size = size;
    state->relocations[state->relocation_count].is_relative = is_relative;
    state->relocation_count++;
}

// Генерация машинного кода для инструкций
void generate_mov(CompilerState* state) {
    char dest[256], src[256];
    
    strcpy(dest, state->current_token.value);
    next_token(state);
    
    if (state->current_token.type == TOKEN_COMMA) {
        next_token(state);
    }
    
    strcpy(src, state->current_token.value);
    next_token(state);
    
    int dest_reg = get_register_code(dest, state->is_64bit);
    int src_reg = get_register_code(src, state->is_64bit);
    
    if (dest_reg != -1 && src_reg != -1) {
        // mov reg, reg
        if (state->is_64bit) {
            emit_byte(state, 0x48); // REX.W prefix
        }
        emit_byte(state, 0x89);
        emit_byte(state, 0xC0 | (src_reg << 3) | dest_reg);
    } else if (dest_reg != -1 && isdigit(src[0])) {
        // mov reg, immediate
        int value = atoi(src);
        if (state->is_64bit) {
            emit_byte(state, 0x48); // REX.W prefix
            emit_byte(state, 0xB8 + dest_reg);
            emit_qword(state, value);
        } else {
            emit_byte(state, 0xB8 + dest_reg);
            emit_dword(state, value);
        }
    }
}

void generate_push(CompilerState* state) {
    char reg[256];
    strcpy(reg, state->current_token.value);
    next_token(state);
    
    int reg_code = get_register_code(reg, state->is_64bit);
    if (reg_code != -1) {
        emit_byte(state, 0x50 + reg_code);
    }
}

void generate_pop(CompilerState* state) {
    char reg[256];
    strcpy(reg, state->current_token.value);
    next_token(state);
    
    int reg_code = get_register_code(reg, state->is_64bit);
    if (reg_code != -1) {
        emit_byte(state, 0x58 + reg_code);
    }
}

void generate_ret(CompilerState* state) {
    next_token(state);
    emit_byte(state, 0xC3);
}

void generate_nop(CompilerState* state) {
    next_token(state);
    emit_byte(state, 0x90);
}

void parse_instruction(CompilerState* state) {
    char instruction[256];
    strcpy(instruction, state->current_token.value);
    
    if (strcasecmp(instruction, "mov") == 0) {
        next_token(state);
        generate_mov(state);
    } else if (strcasecmp(instruction, "push") == 0) {
        next_token(state);
        generate_push(state);
    } else if (strcasecmp(instruction, "pop") == 0) {
        next_token(state);
        generate_pop(state);
    } else if (strcasecmp(instruction, "ret") == 0) {
        generate_ret(state);
    } else if (strcasecmp(instruction, "nop") == 0) {
        generate_nop(state);
    } else {
        // Пропускаем неизвестные инструкции
        while (state->current_token.type != TOKEN_NEWLINE && 
               state->current_token.type != TOKEN_EOF) {
            next_token(state);
        }
    }
}

void parse_directive(CompilerState* state) {
    char directive[256];
    strcpy(directive, state->current_token.value);
    next_token(state);
    
    if (strcasecmp(directive, "db") == 0) {
        // Define byte
        if (state->current_token.type == TOKEN_STRING) {
            char* str = state->current_token.value;
            for (int i = 0; str[i] != '\0'; i++) {
                state->data_buffer[state->data_size++] = str[i];
            }
            state->data_buffer[state->data_size++] = 0; // null terminator
            state->data_address_counter += strlen(str) + 1;
        } else if (state->current_token.type == TOKEN_NUMBER) {
            int value = atoi(state->current_token.value);
            state->data_buffer[state->data_size++] = value & 0xFF;
            state->data_address_counter++;
        }
        next_token(state);
    } else if (strcasecmp(directive, "global") == 0) {
        if (state->current_token.type == TOKEN_IDENTIFIER) {
            if (!state->has_entry_point) {
                strcpy(state->entry_point, state->current_token.value);
                state->has_entry_point = 1;
            }
        }
        next_token(state);
    } else if (strcasecmp(directive, "section") == 0) {
        // Игнорируем секции для простоты
        while (state->current_token.type != TOKEN_NEWLINE && 
               state->current_token.type != TOKEN_EOF) {
            next_token(state);
        }
    }
}

void parse_line(CompilerState* state) {
    if (state->current_token.type == TOKEN_IDENTIFIER) {
        char identifier[256];
        strcpy(identifier, state->current_token.value);
        next_token(state);
        
        if (state->current_token.type == TOKEN_COLON) {
            add_label(state, identifier, 0);
            next_token(state);
        } else {
            // Возвращаем токен назад
            strcpy(state->current_token.value, identifier);
            state->current_token.type = TOKEN_IDENTIFIER;
        }
    }
    
    if (state->current_token.type == TOKEN_INSTRUCTION) {
        parse_instruction(state);
    } else if (state->current_token.type == TOKEN_DIRECTIVE) {
        parse_directive(state);
    }
    
    while (state->current_token.type != TOKEN_NEWLINE && 
           state->current_token.type != TOKEN_EOF) {
        next_token(state);
    }
    
    if (state->current_token.type == TOKEN_NEWLINE) {
        next_token(state);
    }
}

void parse_program(CompilerState* state) {
    next_token(state);
    while (state->current_token.type != TOKEN_EOF) {
        parse_line(state);
    }
}

// Структуры для PE файла
#pragma pack(push, 1)
typedef struct {
    uint16_t e_magic;
    uint16_t e_cblp;
    uint16_t e_cp;
    uint16_t e_crlc;
    uint16_t e_cparhdr;
    uint16_t e_minalloc;
    uint16_t e_maxalloc;
    uint16_t e_ss;
    uint16_t e_sp;
    uint16_t e_csum;
    uint16_t e_ip;
    uint16_t e_cs;
    uint16_t e_lfarlc;
    uint16_t e_ovno;
    uint16_t e_res[4];
    uint16_t e_oemid;
    uint16_t e_oeminfo;
    uint16_t e_res2[10];
    uint32_t e_lfanew;
} IMAGE_DOS_HEADER;

typedef struct {
    uint32_t Signature;
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
} IMAGE_FILE_HEADER;

typedef struct {
    uint16_t Magic;
    uint8_t  MajorLinkerVersion;
    uint8_t  MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint32_t BaseOfData;
    uint32_t ImageBase;
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint32_t SizeOfStackReserve;
    uint32_t SizeOfStackCommit;
    uint32_t SizeOfHeapReserve;
    uint32_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
} IMAGE_OPTIONAL_HEADER32;

typedef struct {
    uint8_t  Name[8];
    uint32_t VirtualSize;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
} IMAGE_SECTION_HEADER;
#pragma pack(pop)

void generate_pe_file(CompilerState* state, const char* output_file) {
    FILE* output = fopen(output_file, "wb");
    if (!output) {
        fprintf(stderr, "Error: Cannot create output file %s\n", output_file);
        exit(1);
    }
    
    // DOS header
    IMAGE_DOS_HEADER dos_header = {0};
    dos_header.e_magic = 0x5A4D; // "MZ"
    dos_header.e_cblp = 0x90;
    dos_header.e_cp = 3;
    dos_header.e_crlc = 0;
    dos_header.e_cparhdr = 4;
    dos_header.e_minalloc = 0;
    dos_header.e_maxalloc = 0xFFFF;
    dos_header.e_ss = 0;
    dos_header.e_sp = 0xB8;
    dos_header.e_csum = 0;
    dos_header.e_ip = 0;
    dos_header.e_cs = 0;
    dos_header.e_lfarlc = 0x40;
    dos_header.e_ovno = 0;
    dos_header.e_lfanew = 0x80;
    
    fwrite(&dos_header, sizeof(dos_header), 1, output);
    
    // DOS stub
    uint8_t dos_stub[0x40] = {
        0x0E, 0x1F, 0xBA, 0x0E, 0x00, 0xB4, 0x09, 0xCD, 0x21, 0xB8, 0x01, 0x4C, 0xCD, 0x21, 0x54, 0x68,
        0x69, 0x73, 0x20, 0x70, 0x72, 0x6F, 0x67, 0x72, 0x61, 0x6D, 0x20, 0x63, 0x61, 0x6E, 0x6E, 0x6F,
        0x74, 0x20, 0x62, 0x65, 0x20, 0x72, 0x75, 0x6E, 0x20, 0x69, 0x6E, 0x20, 0x44, 0x4F, 0x53, 0x20,
        0x6D, 0x6E, 0x64, 0x65, 0x2E, 0x0D, 0x0D, 0x0A, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    fwrite(dos_stub, sizeof(dos_stub), 1, output);
    
    // PE signature
    uint32_t pe_signature = 0x00004550; // "PE\0\0"
    fwrite(&pe_signature, sizeof(pe_signature), 1, output);
    
    // COFF header
    IMAGE_FILE_HEADER file_header = {0};
    file_header.Machine = state->is_64bit ? 0x8664 : 0x014C;
    file_header.NumberOfSections = 2; // .text and .data
    file_header.TimeDateStamp = 0;
    file_header.PointerToSymbolTable = 0;
    file_header.NumberOfSymbols = 0;
    file_header.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER32);
    file_header.Characteristics = 0x0102; // Executable, 32-bit
    
    fwrite(&file_header, sizeof(file_header), 1, output);
    
    // Optional header
    IMAGE_OPTIONAL_HEADER32 opt_header = {0};
    opt_header.Magic = 0x10B;
    opt_header.MajorLinkerVersion = 6;
    opt_header.MinorLinkerVersion = 0;
    opt_header.SizeOfCode = (state->code_size + 0x1FF) & ~0x1FF;
    opt_header.SizeOfInitializedData = (state->data_size + 0x1FF) & ~0x1FF;
    opt_header.SizeOfUninitializedData = 0;
    opt_header.AddressOfEntryPoint = 0x1000; // .text section RVA
    opt_header.BaseOfCode = 0x1000;
    opt_header.BaseOfData = 0x2000; // .data section RVA
    opt_header.ImageBase = 0x400000;
    opt_header.SectionAlignment = 0x1000;
    opt_header.FileAlignment = 0x200;
    opt_header.MajorOperatingSystemVersion = 4;
    opt_header.MinorOperatingSystemVersion = 0;
    opt_header.MajorImageVersion = 0;
    opt_header.MinorImageVersion = 0;
    opt_header.MajorSubsystemVersion = 4;
    opt_header.MinorSubsystemVersion = 0;
    opt_header.Win32VersionValue = 0;
    opt_header.SizeOfImage = 0x3000;
    opt_header.SizeOfHeaders = 0x200;
    opt_header.CheckSum = 0;
    opt_header.Subsystem = 3; // Console
    opt_header.DllCharacteristics = 0;
    opt_header.SizeOfStackReserve = 0x100000;
    opt_header.SizeOfStackCommit = 0x1000;
    opt_header.SizeOfHeapReserve = 0x100000;
    opt_header.SizeOfHeapCommit = 0x1000;
    opt_header.LoaderFlags = 0;
    opt_header.NumberOfRvaAndSizes = 0;
    
    fwrite(&opt_header, sizeof(opt_header), 1, output);
    
    // Section headers
    IMAGE_SECTION_HEADER text_section = {0};
    memcpy(text_section.Name, ".text", 6);
    text_section.VirtualSize = state->code_size;
    text_section.VirtualAddress = 0x1000;
    text_section.SizeOfRawData = (state->code_size + 0x1FF) & ~0x1FF;
    text_section.PointerToRawData = 0x200;
    text_section.PointerToRelocations = 0;
    text_section.PointerToLinenumbers = 0;
    text_section.NumberOfRelocations = 0;
    text_section.NumberOfLinenumbers = 0;
    text_section.Characteristics = 0x60000020; // Code, executable, readable
    
    IMAGE_SECTION_HEADER data_section = {0};
    memcpy(data_section.Name, ".data", 6);
    data_section.VirtualSize = state->data_size;
    data_section.VirtualAddress = 0x2000;
    data_section.SizeOfRawData = (state->data_size + 0x1FF) & ~0x1FF;
    data_section.PointerToRawData = 0x400;
    data_section.PointerToRelocations = 0;
    data_section.PointerToLinenumbers = 0;
    data_section.NumberOfRelocations = 0;
    data_section.NumberOfLinenumbers = 0;
    data_section.Characteristics = 0xC0000040; // Initialized data, readable, writable
    
    fwrite(&text_section, sizeof(text_section), 1, output);
    fwrite(&data_section, sizeof(data_section), 1, output);
    
    // Выравнивание до 0x200 байт
    uint8_t padding[0x200] = {0};
    fseek(output, 0x200, SEEK_SET);
    
    // .text section
    fwrite(state->code_buffer, state->code_size, 1, output);
    
    // Выравнивание .text section
    long pos = ftell(output);
    if (pos < 0x400) {
        fwrite(padding, 0x400 - pos, 1, output);
    }
    
    // .data section
    fwrite(state->data_buffer, state->data_size, 1, output);
    
    fclose(output);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: %s <input.asm> <output.exe> [architecture]\n", argv[0]);
        printf("Architecture: 32 for x86 (default), 64 for x64\n");
        return 1;
    }
    
    int is_64bit = 0;
    if (argc >= 4) {
        is_64bit = atoi(argv[3]) == 64;
    }
    
    CompilerState state;
    init_compiler(&state, argv[1], is_64bit);
    parse_program(&state);
    generate_pe_file(&state, argv[2]);
    
    fclose(state.input);
    
    printf("Compilation completed successfully!\n");
    printf("Code size: %d bytes\n", state.code_size);
    printf("Data size: %d bytes\n", state.data_size);
    printf("Output: %s\n", argv[2]);
    
    return 0;
}