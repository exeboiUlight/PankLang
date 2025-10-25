import os

class Init:
    def __init__(self, project_name, main_file, output, author, version):
        os.system(f"mkdir {project_name}")
        os.mkdir(f"{project_name}/__include")
        os.mkdir(f"{project_name}/__build")

        with open(f"{project_name}/{project_name}.pack", "w") as i:
            i.writelines(f"""
file: {main_file}
output: {output}
author: {author}
version: {version}
""")

class CompileWin64:
    def __init__(self, project_name):
        with open(f"{project_name}/{project_name}.pack", "r") as i:
            a = i.read()
            print(a.strip()[0])
