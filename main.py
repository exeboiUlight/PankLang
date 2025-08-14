from compiler import Compiler
import sys
import os

os.system("cls" if os.name == "nt" else "clear")

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

    compiler = Compiler()
    c_code = compiler.compile(code)
    
    c_file = input_file.replace(".pank", ".c")
    with open(c_file, "w") as f:
        f.write(c_code)
    
    exe_file = input_file.replace(".pank", ".exe")
    if compiler.compile_to_exe(c_code, exe_file):
        print(f"Compilation successful. Output: {exe_file}")
    else:
        print("Compilation failed")

if __name__ == "__main__":
    main()