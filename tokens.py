def tokenize(text):
    tokens = []
    keywords = {'if', 'while', 'for', 'else', 'return', 'def', 'var', 'dll', 'use'}
    symbols = {'=', '+', '-', '*', '/', '[', ']', '{', '}', '(', ')', ',', ';'}
    
    current_token = ''
    i = 0
    n = len(text)
    
    while i < n:
        char = text[i]
        
        # Обработка точки
        if char == ".":
            if current_token:  # Добавляем текущий токен перед точкой
                tokens.append(current_token)
                current_token = ''
            tokens.append("END")  # Добавляем END вместо точки
            i += 1  # Не забываем увеличить индекс
            continue

        # Пропускаем пробелы
        if char.isspace():
            if current_token:
                tokens.append(current_token)
                current_token = ''
            i += 1
            continue
        
        # Обработка символов-разделителей
        if char in symbols:
            if current_token:
                tokens.append(current_token)
                current_token = ''
            tokens.append(char)
            i += 1
            continue
        
        # Обработка чисел (простая версия)
        if char.isdigit():
            current_token += char
            i += 1
            continue
        
        # Обработка букв и идентификаторов
        if char.isalpha() or char == '_':
            current_token += char
            i += 1
            continue
        
        # Для других символов просто добавляем в токен
        current_token += char
        i += 1
    
    # Добавляем последний токен
    if current_token:
        tokens.append(current_token)
    
    # Проверяем ключевые слова и числа
    for idx, token in enumerate(tokens):
        if token in keywords:
            tokens[idx] = f'KEYWORD_{token.upper()}'
        elif token.isdigit():
            tokens[idx] = [f'NUMBER', token]
        else:
            tokens[idx] = token.upper()
    
    return tokens