def tokenize(text):
    tokens = []
    keywords = {'if', 'while', 'for', 'else', 'return', 'def', 'var', 'use', 'dll'}
    symbols = {'=', '+', '-', '*', '/', '[', ']', '{', '}', '(', ')', ',', ';', '.'}
    
    current_token = ''
    i = 0
    n = len(text)
    
    while i < n:
        char = text[i]
        
        if char.isspace():
            if current_token:
                tokens.append(current_token)
                current_token = ''
            i += 1
            continue
        
        if char in symbols:
            if current_token:
                tokens.append(current_token)
                current_token = ''
            tokens.append(char)
            i += 1
            continue
        
        if char.isdigit():
            current_token += char
            i += 1
            continue
        
        if char.isalpha() or char == '_':
            current_token += char
            i += 1
            continue
        
        current_token += char
        i += 1
    
    if current_token:
        tokens.append(current_token)
    
    for idx, token in enumerate(tokens):
        if token in keywords:
            tokens[idx] = f'KEYWORD_{token.upper()}'
        elif token.isdigit():
            tokens[idx] = ['NUMBER', token]
    
    return tokens