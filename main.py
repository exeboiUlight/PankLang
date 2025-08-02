# main.py
from compiler import Compiler
import sys
import os

def main():
    if len(sys.argv) < 2:
        print("Usage: python main.py <input_file>")
        return
    
    input_file = sys.argv[1]
    try:
        with open(input_file, "r") as f:
            code = f.read()
    except FileNotFoundError:
        print(f"Error: File '{input_file}' not found")
        return

    # Компиляция
    compiler = Compiler()
    asm_code = compiler.compile(code)
    
    # Сохранение результата
    output_file = input_file.replace(".pank", ".asm")
    with open(output_file, "w") as f:


        f.write(asm_code)

    try:
        exe = input_file.replace(".pank", ".exe")
        os.system(f"nasm {output_file} -o {exe}")
    except Exception as e:
        print(f"error nasm {e}")
    
    print(f"Compilation successful. Output: {output_file} and {exe}")

if __name__ == "__main__":
    main()
