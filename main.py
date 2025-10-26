from generate import Init, UpWin64, Path

if __name__ == "__main__":
    # Создание проекта
    init = Init(
        project_name="my_project", 
        main_file="main.up",
        output="program.exe",
        author="Your Name",
        version="1.0.0"
    )
    
    # Создаем папку __include если её нет
    include_dir = Path("my_project/__include")
    include_dir.mkdir(exist_ok=True)
    
    # Создаем модуль io.up
    io_module = '''# Модуль ввода-вывода для Up языка
# Внешние функции, реализованные в .o файлах

extern print_str(str: ptr) -> void;
extern print_int(num: int) -> void;
extern print_float(num: float) -> void;
extern print_bool(val: bool) -> void;

extern input_str() -> str;
extern input_int() -> int;
extern input_float() -> float;

extern strlen(str: ptr) -> int;
extern strcmp(str1: ptr, str2: ptr) -> int;
extern strcpy(dest: ptr, src: ptr) -> ptr;

extern malloc(size: int) -> ptr;
extern free(ptr: ptr) -> void;

extern exit(code: int) -> void;

# Вспомогательные функции
def println_str(str: ptr) -> void {
    print_str(str);
    print_str("\\n");
}

def println_int(num: int) -> void {
    print_int(num);
    print_str("\\n");
}

def print_bool_str(val: bool) -> void {
    if val {
        print_str("true");
    } else {
        print_str("false");
    }
}

def println_bool(val: bool) -> void {
    print_bool_str(val);
    print_str("\\n");
}'''
    
    with open(include_dir / "io.up", "w", encoding="utf-8") as f:
        f.write(io_module)
    
    # Создаем основной файл
    main_code = '''import "io.up";

global _start

def _start() -> int {
    print_str("Hello, World!\\n");
    print_str("Type 'quit' to exit\\n");
    
    let running : bool = true;
    
    while running {
        print_str("> ");
        let a : str = input_str();
        
        if a == "quit" {
            running = false;
        } else {
            print_str("You typed: ");
            print_str(a);
            print_str("\\n");
        }
    }
    
    print_str("Goodbye!\\n");
    return 0;
}'''
    
    with open("my_project/main.up", "w", encoding="utf-8") as f:
        f.write(main_code)
    
    # Компиляция
    compiler = UpWin64("my_project")
    compiler.lexer()
    print(compiler.functions)