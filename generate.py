import os
from pathlib import Path

class Token:
    def FUNCTION(_name: str, _type: str):
        return [_name, _type]
    def VARIABLE(_name: str, _type: str, _bytes: str):
        return [_name, _type, _bytes]

class Init:
    def __init__(self, project_name, main_file, output, author, version):
        try:
            # Создаем объект Path для кроссплатформенности
            project_path = Path(project_name)
            
            # Проверяем, существует ли проект
            if project_path.exists():
                raise FileExistsError(f"Project '{project_name}' already exists")
            
            # Создаем директории
            project_path.mkdir()
            (project_path / "__include").mkdir()
            (project_path / "__build").mkdir()

            # Создаем файл конфигурации
            pack_file = project_path / f"{project_name}.pack"
            with open(pack_file, "w", encoding="utf-8") as f:
                f.write(f"""file: {main_file}
output: {output}
author: {author}
version: {version}
""")
                
            print(f"Project '{project_name}' created successfully!")
            
        except FileExistsError as e:
            print(f"Error: {e}")
        except Exception as e:
            print(f"Unexpected error: {e}")

class UpWin64:
    def __init__(self, project_name):
        self.project_name = project_name
        self.pack_file_path = Path(project_name) / f"{project_name}.pack"
        self.file = None
        self.variables = []
        self.functions = []
        self.globals = []
        
        self._load_config()
    
    def _load_config(self):
        """Загружает конфигурацию из .pack файла"""
        try:
            if not self.pack_file_path.exists():
                raise FileNotFoundError(f"Config file {self.pack_file_path} not found")
            
            with open(self.pack_file_path, "r", encoding="utf-8") as f:
                content = f.read()
                
            # Парсим конфигурацию
            for line in content.splitlines():
                if line.startswith("file:"):
                    self.file = line.split(":", 1)[1].strip()
                    break
                    
            if not self.file:
                raise ValueError("Main file not specified in config")
                
        except Exception as e:
            print(f"Error loading config: {e}")
            raise
    
    def lexer(self):
        """Лексический анализатор"""
        try:
            if not self.file or not Path(self.file).exists():
                raise FileNotFoundError(f"Source file '{self.file}' not found")

            with open(self.file, "r", encoding="utf-8") as f:
                lines = f.readlines()

            in_comment = False
            for line_num, line in enumerate(lines, 1):
                line = line.rstrip()  # Убираем правые пробелы
                i = 0

                while i < len(line):
                    # Обработка комментариев
                    if in_comment:
                        if "*/" in line[i:]:
                            i = line.index("*/", i) + 2
                            in_comment = False
                            continue
                        else:
                            break
                        
                    # Однострочные комментарии
                    if i + 1 < len(line) and line[i] == '/' and line[i + 1] == '/':
                        break  # Пропускаем остаток строки
                    
                    # Многострочные комментарии
                    if i + 1 < len(line) and line[i] == '/' and line[i + 1] == '*':
                        in_comment = True
                        i += 2
                        continue
                    
                    # Обработка объявления функций
                    if i + 2 < len(line) and line[i:i+3] == "def":
                        # Пропускаем "def" и пробелы
                        j = i + 3
                        while j < len(line) and line[j].isspace():
                            j += 1

                        # Ищем имя функции (до пробела или открывающей скобки)
                        func_start = j
                        while j < len(line) and line[j] not in (' ', '('):
                            j += 1
                        func_name = line[func_start:j]

                        # Ищем тип возвращаемого значения (после "->")
                        return_type = None
                        if "->" in line[j:]:
                            arrow_pos = line.index("->", j)
                            # Пропускаем "->" и пробелы
                            type_start = arrow_pos + 2
                            while type_start < len(line) and line[type_start].isspace():
                                type_start += 1

                            # Ищем конец типа (до пробела или открывающей фигурной скобки)
                            type_end = type_start
                            while type_end < len(line) and line[type_end] not in (' ', '{'):
                                type_end += 1
                            return_type = line[type_start:type_end]

                        if return_type:
                            self.functions.append(Token.FUNCTION(func_name, return_type))
                            print(f"Found function: {func_name} -> {return_type}")

                        break  # Переходим к следующей строке после нахождения функции
                    
                    i += 1

        except Exception as e:
            print(f"Lexer error at line {line_num}: {e}")
