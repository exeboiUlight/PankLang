import generate

project = generate.Init("Test", "main.up", "main.exe", "ExeBoi", "0.0.0")

generate.CompileWin64("Test")