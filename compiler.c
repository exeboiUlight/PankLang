#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#define MAX_LABELS 100
#define CODE_SIZE 4096
#define DATA_SIZE 4096

typedef struct {
    char name[50];
    uint32_t address;
} Label;

typedef struct {
    FILE* input;
    uint8_t code[CODE_SIZE];
    uint8_t data[DATA_SIZE];
    uint32_t code_size;
    uint32_t data_size;
    uint32_t code_addr;
    uint32_t data_addr;
    Label labels[MAX_LABELS];
    int label_count;
    int is_64bit;
} CompilerState;

void init_compiler(CompilerState* state, const char* filename, int is_64bit) {
    state->input = fopen(filename, "r");
    if (!state->input) {
        printf("Error: Cannot open file %s\n", filename);
        exit(1);
    }
    
    state->code_size = 0;
    state->data_size = 0;
    state->code_addr = 0x1000;
    state->data_addr = 0x2000;
    state->label_count = 0;
    state->is_64bit = is_64bit;
    
    memset(state->code, 0, CODE_SIZE);
    memset(state->data, 0, DATA_SIZE);
}

void emit_byte(CompilerState* state, uint8_t b) {
    if (state->code_size < CODE_SIZE) {
        state->code[state->code_size++] = b;
    }
}

void emit_dword(CompilerState* state, uint32_t d) {
    emit_byte(state, d & 0xFF);
    emit_byte(state, (d >> 8) & 0xFF);
    emit_byte(state, (d >> 16) & 0xFF);
    emit_byte(state, (d >> 24) & 0xFF);
}

void add_label(CompilerState* state, const char* name, uint32_t addr) {
    if (state->label_count < MAX_LABELS) {
        strcpy(state->labels[state->label_count].name, name);
        state->labels[state->label_count].address = addr;
        state->label_count++;
    }
}

int find_label(CompilerState* state, const char* name) {
    for (int i = 0; i < state->label_count; i++) {
        if (strcmp(state->labels[i].name, name) == 0) {
            return state->labels[i].address;
        }
    }
    return -1;
}

int parse_number(const char* str) {
    if (str[0] == '0' && str[1] == 'x') {
        return (int)strtol(str + 2, NULL, 16);
    }
    return atoi(str);
}

void skip_whitespace_and_comments(char* line) {
    // Remove comments
    char* comment = strchr(line, ';');
    if (comment) *comment = '\0';
    
    // Trim whitespace
    int len = strlen(line);
    while (len > 0 && isspace(line[len-1])) {
        line[--len] = '\0';
    }
}

int is_register(const char* token) {
    const char* regs[] = {"eax", "ebx", "ecx", "edx", "esi", "edi", "esp", "ebp", 
                         "ax", "bx", "cx", "dx", "al", "bl", "cl", "dl", NULL};
    for (int i = 0; regs[i] != NULL; i++) {
        if (strcasecmp(token, regs[i]) == 0) return 1;
    }
    return 0;
}

int get_reg_code(const char* reg) {
    if (strcasecmp(reg, "eax") == 0 || strcasecmp(reg, "ax") == 0) return 0;
    if (strcasecmp(reg, "ecx") == 0 || strcasecmp(reg, "cx") == 0) return 1;
    if (strcasecmp(reg, "edx") == 0 || strcasecmp(reg, "dx") == 0) return 2;
    if (strcasecmp(reg, "ebx") == 0 || strcasecmp(reg, "bx") == 0) return 3;
    if (strcasecmp(reg, "esp") == 0 || strcasecmp(reg, "sp") == 0) return 4;
    if (strcasecmp(reg, "ebp") == 0 || strcasecmp(reg, "bp") == 0) return 5;
    if (strcasecmp(reg, "esi") == 0 || strcasecmp(reg, "si") == 0) return 6;
    if (strcasecmp(reg, "edi") == 0 || strcasecmp(reg, "di") == 0) return 7;
    return -1;
}

void parse_line(CompilerState* state, char* line) {
    skip_whitespace_and_comments(line);
    if (strlen(line) == 0) return;
    
    char* token = strtok(line, " \t,");
    if (!token) return;
    
    // Check for label
    if (token[strlen(token)-1] == ':') {
        token[strlen(token)-1] = '\0';
        add_label(state, token, state->code_addr + state->code_size);
        return;
    }
    
    // Handle directives
    if (strcasecmp(token, "db") == 0) {
        token = strtok(NULL, " \t,");
        if (token) {
            if (token[0] == '"') {
                // String
                char* str = token + 1;
                if (str[strlen(str)-1] == '"') {
                    str[strlen(str)-1] = '\0';
                }
                for (int i = 0; str[i] != '\0'; i++) {
                    state->data[state->data_size++] = str[i];
                }
                state->data[state->data_size++] = 0; // Null terminator
            } else {
                // Number
                int value = parse_number(token);
                state->data[state->data_size++] = value & 0xFF;
            }
        }
        return;
    }
    
    // Handle instructions
    if (strcasecmp(token, "mov") == 0) {
        char* dest = strtok(NULL, " \t,");
        char* src = strtok(NULL, " \t,");
        
        if (dest && src) {
            int dest_reg = get_reg_code(dest);
            int src_reg = get_reg_code(src);
            
            if (dest_reg != -1 && src_reg != -1) {
                // mov reg, reg
                emit_byte(state, 0x89);
                emit_byte(state, 0xC0 | (src_reg << 3) | dest_reg);
            } else if (dest_reg != -1 && isdigit(src[0])) {
                // mov reg, immediate
                int value = parse_number(src);
                emit_byte(state, 0xB8 + dest_reg);
                emit_dword(state, value);
            }
        }
    }
    else if (strcasecmp(token, "push") == 0) {
        char* reg = strtok(NULL, " \t,");
        if (reg) {
            int reg_code = get_reg_code(reg);
            if (reg_code != -1) {
                emit_byte(state, 0x50 + reg_code);
            }
        }
    }
    else if (strcasecmp(token, "pop") == 0) {
        char* reg = strtok(NULL, " \t,");
        if (reg) {
            int reg_code = get_reg_code(reg);
            if (reg_code != -1) {
                emit_byte(state, 0x58 + reg_code);
            }
        }
    }
    else if (strcasecmp(token, "ret") == 0) {
        emit_byte(state, 0xC3);
    }
    else if (strcasecmp(token, "nop") == 0) {
        emit_byte(state, 0x90);
    }
    else if (strcasecmp(token, "xor") == 0) {
        char* dest = strtok(NULL, " \t,");
        char* src = strtok(NULL, " \t,");
        if (dest && src) {
            int dest_reg = get_reg_code(dest);
            int src_reg = get_reg_code(src);
            if (dest_reg != -1 && src_reg != -1 && dest_reg == src_reg) {
                // xor reg, reg
                emit_byte(state, 0x31);
                emit_byte(state, 0xC0 | (src_reg << 3) | dest_reg);
            }
        }
    }
}

// Simple PE header generation
void write_pe_header(FILE* exe, CompilerState* state) {
    // DOS stub
    uint8_t dos_stub[] = {
        0x4D, 0x5A, 0x90, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
        0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
        0x0E, 0x1F, 0xBA, 0x0E, 0x00, 0xB4, 0x09, 0xCD, 0x21, 0xB8, 0x01, 0x4C, 0xCD, 0x21, 0x54, 0x68,
        0x69, 0x73, 0x20, 0x70, 0x72, 0x6F, 0x67, 0x72, 0x61, 0x6D, 0x20, 0x63, 0x61, 0x6E, 0x6E, 0x6F,
        0x74, 0x20, 0x62, 0x65, 0x20, 0x72, 0x75, 0x6E, 0x20, 0x69, 0x6E, 0x20, 0x44, 0x4F, 0x53, 0x20,
        0x6D, 0x6F, 0x64, 0x65, 0x2E, 0x0D, 0x0D, 0x0A, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    fwrite(dos_stub, sizeof(dos_stub), 1, exe);
    
    // PE signature
    uint32_t pe_sig = 0x00004550;
    fwrite(&pe_sig, 4, 1, exe);
    
    // COFF header
    uint16_t machine = 0x014C; // i386
    uint16_t sections = 2;
    uint32_t timestamp = 0;
    uint32_t symtable = 0;
    uint32_t symcount = 0;
    uint16_t opt_size = 0xE0;
    uint16_t characteristics = 0x0102;
    
    fwrite(&machine, 2, 1, exe);
    fwrite(&sections, 2, 1, exe);
    fwrite(&timestamp, 4, 1, exe);
    fwrite(&symtable, 4, 1, exe);
    fwrite(&symcount, 4, 1, exe);
    fwrite(&opt_size, 2, 1, exe);
    fwrite(&characteristics, 2, 1, exe);
    
    // Optional header
    uint16_t magic = 0x010B;
    uint8_t linker_ver = 6;
    uint8_t linker_ver_minor = 0;
    uint32_t code_size = (state->code_size + 0x1FF) & ~0x1FF;
    uint32_t data_size = (state->data_size + 0x1FF) & ~0x1FF;
    uint32_t bss_size = 0;
    uint32_t entry_point = 0x1000;
    uint32_t code_base = 0x1000;
    uint32_t data_base = 0x2000;
    
    uint8_t opt_header[0xE0] = {0};
    memcpy(opt_header, &magic, 2);
    opt_header[2] = linker_ver;
    opt_header[3] = linker_ver_minor;
    memcpy(opt_header + 4, &code_size, 4);
    memcpy(opt_header + 8, &data_size, 4);
    memcpy(opt_header + 12, &bss_size, 4);
    memcpy(opt_header + 16, &entry_point, 4);
    memcpy(opt_header + 20, &code_base, 4);
    memcpy(opt_header + 24, &data_base, 4);
    
    uint32_t image_base = 0x400000;
    uint32_t section_align = 0x1000;
    uint32_t file_align = 0x200;
    memcpy(opt_header + 28, &image_base, 4);
    memcpy(opt_header + 32, &section_align, 4);
    memcpy(opt_header + 36, &file_align, 4);
    
    uint32_t image_size = 0x3000;
    uint32_t header_size = 0x200;
    memcpy(opt_header + 56, &image_size, 4);
    memcpy(opt_header + 60, &header_size, 4);
    
    uint16_t subsystem = 3; // Console
    memcpy(opt_header + 68, &subsystem, 2);
    
    fwrite(opt_header, 0xE0, 1, exe);
    
    // Section headers
    // .text section
    char text_name[8] = ".text";
    uint32_t text_vsize = state->code_size;
    uint32_t text_vaddr = 0x1000;
    uint32_t text_rawsize = code_size;
    uint32_t text_rawptr = 0x200;
    uint32_t text_relocs = 0;
    uint32_t text_lines = 0;
    uint16_t text_nrelocs = 0;
    uint16_t text_nlines = 0;
    uint32_t text_flags = 0x60000020;
    
    fwrite(text_name, 8, 1, exe);
    fwrite(&text_vsize, 4, 1, exe);
    fwrite(&text_vaddr, 4, 1, exe);
    fwrite(&text_rawsize, 4, 1, exe);
    fwrite(&text_rawptr, 4, 1, exe);
    fwrite(&text_relocs, 4, 1, exe);
    fwrite(&text_lines, 4, 1, exe);
    fwrite(&text_nrelocs, 2, 1, exe);
    fwrite(&text_nlines, 2, 1, exe);
    fwrite(&text_flags, 4, 1, exe);
    
    // .data section
    char data_name[8] = ".data";
    uint32_t data_vsize = state->data_size;
    uint32_t data_vaddr = 0x2000;
    uint32_t data_rawsize = data_size;
    uint32_t data_rawptr = 0x400;
    uint32_t data_relocs = 0;
    uint32_t data_lines = 0;
    uint16_t data_nrelocs = 0;
    uint16_t data_nlines = 0;
    uint32_t data_flags = 0xC0000040;
    
    fwrite(data_name, 8, 1, exe);
    fwrite(&data_vsize, 4, 1, exe);
    fwrite(&data_vaddr, 4, 1, exe);
    fwrite(&data_rawsize, 4, 1, exe);
    fwrite(&data_rawptr, 4, 1, exe);
    fwrite(&data_relocs, 4, 1, exe);
    fwrite(&data_lines, 4, 1, exe);
    fwrite(&data_nrelocs, 2, 1, exe);
    fwrite(&data_nlines, 2, 1, exe);
    fwrite(&data_flags, 4, 1, exe);
    
    // Pad to 0x200
    uint8_t padding[0x200] = {0};
    fseek(exe, 0x200, SEEK_SET);
}

void compile_to_exe(CompilerState* state, const char* outfile) {
    FILE* exe = fopen(outfile, "wb");
    if (!exe) {
        printf("Error: Cannot create %s\n", outfile);
        return;
    }
    
    write_pe_header(exe, state);
    
    // Write .text section
    fwrite(state->code, state->code_size, 1, exe);
    
    // Pad to 0x400
    long pos = ftell(exe);
    if (pos < 0x400) {
        uint8_t pad[0x400] = {0};
        fwrite(pad, 0x400 - pos, 1, exe);
    }
    
    // Write .data section
    fwrite(state->data, state->data_size, 1, exe);
    
    fclose(exe);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Simple ASM to EXE Compiler\n");
        printf("Usage: %s input.asm output.exe\n", argv[0]);
        return 1;
    }
    
    CompilerState state;
    init_compiler(&state, argv[1], 0); // 32-bit only for simplicity
    
    char line[256];
    while (fgets(line, sizeof(line), state.input)) {
        parse_line(&state, line);
    }
    
    compile_to_exe(&state, argv[2]);
    fclose(state.input);
    
    printf("Compilation successful!\n");
    printf("Code: %d bytes, Data: %d bytes\n", state.code_size, state.data_size);
    printf("Output: %s\n", argv[2]);
    
    return 0;
}