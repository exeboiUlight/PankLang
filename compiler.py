import tokens as tokenizer
import parser

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

class Compiler:
    def __init__(self):
        self.variables = set()
        self.label_counter = 0
        self.temp_counter = 0
    
    def generate_label(self, prefix):
        self.label_counter += 1
        return f"{prefix}_{self.label_counter}"
    
    def get_temp_var(self):
        self.temp_counter += 1
        temp_var = f"temp_{self.temp_counter}"
        self.variables.add(temp_var)
        return temp_var
    
    def compile_expression(self, tokens):
        """Компиляция арифметических выражений с учетом приоритетов операций"""
        asm = []
        output = []
        ops = []
        
        precedence = {'*': 3, '/': 3, '+': 2, '-': 2}
        
        for token in tokens:
            if isinstance(token, list) and token[0] == 'NUMBER':
                output.append(('num', token[1]))
            elif token == '(':
                ops.append(token)
            elif token == ')':
                while ops and ops[-1] != '(':
                    output.append(('op', ops.pop()))
                ops.pop()  # Удаляем '('
            elif token in precedence:
                while (ops and ops[-1] != '(' and 
                       precedence[ops[-1]] >= precedence[token]):
                    output.append(('op', ops.pop()))
                ops.append(token)
        
        while ops:
            output.append(('op', ops.pop()))
        
        # Вычисляем выражение в постфиксной записи
        stack = []
        for item in output:
            if item[0] == 'num':
                stack.append(item[1])
                asm.append(f"push {item[1]}")
            elif item[0] == 'op':
                op = item[1]
                if len(stack) < 2:
                    raise ValueError("Invalid expression")
                
                b = stack.pop()
                a = stack.pop()
                
                temp_var = self.get_temp_var()
                asm.append("pop ebx")
                asm.append("pop eax")
                
                if op == '+':
                    asm.append("add eax, ebx")
                elif op == '-':
                    asm.append("sub eax, ebx")
                elif op == '*':
                    asm.append("imul eax, ebx")
                elif op == '/':
                    asm.append("cdq")
                    asm.append("idiv ebx")
                
                asm.append(f"mov [{temp_var}], eax")
                asm.append(f"push {temp_var}")
                stack.append(temp_var)
        
        if stack:
            asm.append("pop eax")  # Результат выражения в eax
        
        return "\n".join(asm)
    
    def compile_variable(self, node):
        var_name = node["name"]
        value = node["value"]
        remaining_tokens = node["remaining_tokens"]
        self.variables.add(var_name)
        
        expr_tokens = [value] + remaining_tokens
        
        asm = []
        asm.append(f"; Объявление переменной {var_name}")
        asm.append(self.compile_expression(expr_tokens))
        asm.append(f"mov [{var_name}], eax")
        
        return "\n".join(asm)
    
    def compile_loop(self, node):
        loop_type = node["type"]
        condition = node["condition"]
        body = node["body"]
        
        start_label = self.generate_label("loop_start")
        end_label = self.generate_label("loop_end")
        
        asm = []
        asm.append(f"{start_label}:")
        
        # Компиляция условия
        cond_asm = self.compile_expression(condition)
        if cond_asm:
            asm.append(cond_asm)
            asm.append("test eax, eax")
            asm.append(f"jz {end_label}")
        
        # Компиляция тела цикла
        body_asm = self.compile_ast(body)
        if body_asm:
            asm.append(body_asm)
        
        asm.append(f"jmp {start_label}")
        asm.append(f"{end_label}:")
        
        return "\n".join(asm)
    
    def compile_function(self, node):
        func_name = node["name"]
        params = node["params"]
        body = node["body"]
        
        asm = []
        asm.append(f"{func_name}:")
        asm.append("push ebp")
        asm.append("mov ebp, esp")
        
        # Выделение места для параметров
        for i, param in enumerate(params):
            asm.append(f"; Параметр {param}")
            offset = 8 + i * 4  # Смещение относительно ebp
            asm.append(f"mov eax, [ebp+{offset}]")
            asm.append(f"mov [{param}], eax")
            self.variables.add(param)
        
        # Компиляция тела функции
        body_asm = self.compile_ast(body)
        if body_asm:
            asm.append(body_asm)
        
        asm.append("mov esp, ebp")
        asm.append("pop ebp")
        asm.append("ret")
        
        return "\n".join(asm)
    
    def compile_ast(self, ast):
        asm = []
        for node in ast:
            if node["tag"] == "VAR_DECLARATION":
                asm.append(self.compile_variable(node))
            elif node["tag"] == "LOOP":
                asm.append(self.compile_loop(node))
            elif node["tag"] == "FUNCTION":
                asm.append(self.compile_function(node))
        
        return "\n".join(asm)
    
    def generate_prologue(self):
        return """;
; Generated by panks compiler
;

.386
.model flat, stdcall
option casemap :none

.data
"""
    
    def generate_data_section(self):
        data = []
        for var in sorted(self.variables):
            data.append(f"{var} dd ?")
        return "\n".join(data)
    
    def generate_epilogue(self):
        return """
.code
start:
    ; Основной код программы
    call main
    
    ; Завершение программы
    invoke ExitProcess, 0
end start
"""
    
    def compile(self, source_code):
        token_list = tokenizer.tokenize(source_code)
        ast = parser.global_parse(token_list)
        
        self.variables = set()
        self.label_counter = 0
        self.temp_counter = 0
        
        main_code = self.compile_ast(ast)
        
        asm = []
        asm.append(self.generate_prologue())
        asm.append(self.generate_data_section())
        asm.append(self.generate_epilogue())
        
        full_asm = "\n".join(asm)
        full_asm = full_asm.replace("; Основной код программы", main_code)

        return full_asm