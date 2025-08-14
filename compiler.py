import os
import platform
import subprocess
from parser import global_parse
from tokens import tokenize

class Compiler:
    def __init__(self):
        self.imported_dlls = []
        self.variables = {}
        self.functions = {}
        self.includes = {
            '<stdio.h>', 
            '<stdlib.h>',
            '<string.h>'
        }
        self.platform = platform.system()
        self.setup_platform_specific_includes()
        self.current_function_return_type = "void"

    def setup_platform_specific_includes(self):
        if self.platform == 'Windows':
            self.includes.add('<windows.h>')
        else:
            self.includes.add('<dlfcn.h>')

    def compile(self, code):
        tokens = tokenize(code)
        ast = global_parse(tokens)
        c_code = self.generate_c(ast)
        return c_code

    def generate_c(self, ast):
        c_code = []
        
        for include in sorted(self.includes):
            c_code.append(f"#include {include}")
        c_code.append("")

        if self.imported_dlls:
            c_code.append(self.generate_dll_loader())
            c_code.append("")

        for node in ast:
            if node["tag"] == "FUNCTION":
                c_code.extend(self.compile_function(node))
            elif node["tag"] == "VAR_DECLARATION":
                c_code.extend(self.compile_variable(node))
            elif node["tag"] == "LOOP":
                c_code.extend(self.compile_loop(node))
            elif node["tag"] == "PRINT":
                c_code.extend(self.compile_print(node))
            elif node["tag"] == "INPUT":
                c_code.extend(self.compile_input(node))
            elif node["tag"] == "USE":
                self.handle_dll_import(node)
            elif node["tag"] == "DLL_CALL":
                c_code.extend(self.compile_dll_call(node))

        if not any(node.get("tag") == "FUNCTION" and node["name"] == "hoi" for node in ast):
            c_code.extend([
                "int main() {",
                "    return 0;",
                "}"
            ])
        else:
            c_code.extend([
                "int main() {",
                "    hoi();",
                "    return 0;",
                "}"
            ])

        return "\n".join(c_code)

    def generate_dll_loader(self):
        if self.platform == 'Windows':
            return """
typedef void (*DLL_Func)();
void* load_dll(const char* dll_name) {
    return (void*)LoadLibraryA(dll_name);
}
DLL_Func get_dll_func(void* dll, const char* func_name) {
    return (DLL_Func)GetProcAddress((HMODULE)dll, func_name);
}
void close_dll(void* dll) {
    FreeLibrary((HMODULE)dll);
}"""
        else:
            return """
typedef void (*DLL_Func)();
void* load_dll(const char* dll_name) {
    return dlopen(dll_name, RTLD_LAZY);
}
DLL_Func get_dll_func(void* dll, const char* func_name) {
    return (DLL_Func)dlsym(dll, func_name);
}
void close_dll(void* dll) {
    dlclose(dll);
}"""

    def handle_dll_import(self, node):
        dll_name = node["name"]
        if not dll_name.endswith((".dll", ".so")):
            dll_name += ".dll" if self.platform == 'Windows' else ".so"
        self.imported_dlls.append(dll_name)

    def compile_function(self, node):
        c_code = []
        
        return_type = node.get("_ret", "void")
        self.current_function_return_type = return_type
        
        params = ", ".join(["int " + p for p in node["params"]])
        
        c_code.append(f"{return_type} {node['name']}({params}) {{")
        
        for body_node in global_parse(node["body"]):
            if body_node["tag"] == "VAR_DECLARATION":
                c_code.extend(self.compile_variable(body_node))
            elif body_node["tag"] == "LOOP":
                c_code.extend(self.compile_loop(body_node))
            elif body_node["tag"] == "PRINT":
                c_code.extend(self.compile_print(body_node))
            elif body_node["tag"] == "INPUT":
                c_code.extend(self.compile_input(body_node))
            elif body_node["tag"] == "DLL_CALL":
                c_code.extend(self.compile_dll_call(body_node))
        
        if return_type != "void":
            c_code.append(f"    return 0;")
        
        c_code.append("}")
        c_code.append("")
        return c_code

    def compile_variable(self, node):
        var_name = node["name"]
        var_value = node["value"][1] if isinstance(node["value"], list) else node["value"]
        self.variables[var_name] = var_value
        return [f"    int {var_name} = {var_value};"]

    def compile_loop(self, node):
        c_code = []
        condition = " ".join(
            token[1] if isinstance(token, list) else token 
            for token in node["condition"]
        )
        
        if node["type"] == "while":
            c_code.append(f"    while ({condition}) {{")
        elif node["type"] == "for":
            parts = condition.split(";")
            if len(parts) >= 3:
                c_code.append(f"    for ({parts[0]}; {parts[1]}; {parts[2]}) {{")
        
        for body_node in global_parse(node["body"]):
            if body_node["tag"] == "VAR_DECLARATION":
                c_code.extend(self.compile_variable(body_node))
        
        c_code.append("    }")
        return c_code

    def compile_print(self, node):
        arg = node["arg"][1] if isinstance(node["arg"], list) else node["arg"]
        if arg.isdigit() or (arg[0] == '-' and arg[1:].isdigit()):
            return [f"    printf(\"%d\", {arg});"]
        return [f"    printf(\"{arg}\");"]

    def compile_input(self, node):
        var_name = node["var_name"]
        return [
            f"    char {var_name}_input[256];",
            f"    fgets({var_name}_input, sizeof({var_name}_input), stdin);",
            f"    {var_name}_input[strcspn({var_name}_input, \"\\n\")] = 0;",
            f"    {var_name} = atoi({var_name}_input);"
        ]

    def compile_dll_call(self, node):
        dll_name = node["dll"]
        func_name = node["func"]
        
        # Генерируем уникальные имена для переменных
        dll_var = f"{dll_name}_dll"
        func_var = f"{dll_name}_{func_name}_func"
        
        return [
            f"    void* {dll_var} = load_dll(\"{dll_name}.dll\");",
            f"    if ({dll_var}) {{",
            f"        DLL_Func {func_var} = get_dll_func({dll_var}, \"{func_name}\");",
            f"        if ({func_var}) {func_var}();",
            f"        close_dll({dll_var});",
            f"    }}"
        ]

    def compile_input(self, node):
        var_name = node["var_name"]
        return [
            f"    {var_name} = input_int();"
        ]

    def generate_c(self, ast):
        c_code = []

        # Добавляем заголовочные файлы
        for include in sorted(self.includes):
            c_code.append(f"#include {include}")
        c_code.append("")   
        # Добавляем объявления функций
        c_code.extend([
            "typedef void (*DLL_Func)();",
            "void* load_dll(const char* dll_name);",
            "DLL_Func get_dll_func(void* dll, const char* func_name);",
            "void close_dll(void* dll);",
            "void print(const char* str);",
            "int input_int();",
            ""
        ])

        # Генерация кода для загрузки DLL
        c_code.append(self.generate_dll_loader())
        c_code.append("")   
        # Генерация AST
        for node in ast:
            if node["tag"] == "FUNCTION":
                c_code.extend(self.compile_function(node))
            elif node["tag"] == "VAR_DECLARATION":
                c_code.extend(self.compile_variable(node))
            elif node["tag"] == "LOOP":
                c_code.extend(self.compile_loop(node))
            elif node["tag"] == "PRINT":
                c_code.extend(self.compile_print(node))
            elif node["tag"] == "INPUT":
                c_code.extend(self.compile_input(node))
            elif node["tag"] == "USE":
                self.handle_dll_import(node)
            elif node["tag"] == "DLL_CALL":
                c_code.extend(self.compile_dll_call(node))  
        # Добавляем main()
        if not any(node.get("tag") == "FUNCTION" and node["name"] == "hoi" for node in ast):
            c_code.extend([
                "int main() {",
                "    return 0;",
                "}"
            ])
        else:
            c_code.extend([
                "int main() {",
                "    hoi();",
                "    return 0;",
                "}"
            ])  
        return "\n".join(c_code)

    def compile_to_exe(self, c_code, output_file):
        c_file = output_file.replace(".exe", ".c")
        
        with open(c_file, "w") as f:
            f.write(c_code)

        try:
            cmd = ["tcc", "-o", output_file, c_file]
            
            if self.platform != 'Windows':
                cmd.extend(["-ldl"])
            
            for dll in self.imported_dlls:
                if self.platform == 'Windows':
                    cmd.extend(["-L", os.path.dirname(dll)])
            
            result = subprocess.run(cmd, check=True)
            return result.returncode == 0
        except subprocess.CalledProcessError as e:
            print(f"Compilation error: {e}")
            return False
        except FileNotFoundError:
            print("Error: tcc (Tiny C Compiler) not found. Install tcc or add it to PATH.")
            return False