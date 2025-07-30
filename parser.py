# parser.py
def parse_variable_declaration(tokens):
    if tokens[0] != "KEYWORD_VAR":
        return None
    
    name = tokens[1]
    if tokens[2] != '=':
        return None  # Нет оператора присваивания
    
    value = tokens[3]
    remaining_tokens = tokens[4:]
    
    return {
        "tag": "VAR_DECLARATION",
        "name": name,
        "value": value,
        "remaining_tokens": remaining_tokens
    }

def parse_loops(tokens):
    if tokens[0] not in ["KEYWORD_WHILE", "KEYWORD_FOR"]:
        return None

    loop_type = tokens[0].split('_')[1].lower()
    condition_start = 1
    condition_end = tokens.index('{')
    condition = tokens[condition_start:condition_end]
    
    # Находим закрывающую скобку
    brace_count = 1
    body_end = condition_end + 1
    while brace_count > 0 and body_end < len(tokens):
        if tokens[body_end] == '{':
            brace_count += 1
        elif tokens[body_end] == '}':
            brace_count -= 1
        body_end += 1
    
    body = tokens[condition_end+1:body_end-1]
    remaining_tokens = tokens[body_end:]
    
    return {
        "tag": "LOOP",
        "type": loop_type,
        "condition": condition,
        "body": body,
        "remaining_tokens": remaining_tokens
    }

def parse_function(tokens):
    if tokens[0] != "KEYWORD_DEF":
        return None
    
    name = tokens[1]
    if tokens[2] != '(':
        return None
    
    # Парсим параметры
    params = []
    i = 3
    while i < len(tokens) and tokens[i] != ')':
        if tokens[i] != ',':
            params.append(tokens[i])
        i += 1
    
    if tokens[i] != ')':
        return None
    
    # Парсим тело функции
    if tokens[i+1] != '{':
        return None
    
    brace_count = 1
    body_end = i + 2
    while brace_count > 0 and body_end < len(tokens):
        if tokens[body_end] == '{':
            brace_count += 1
        elif tokens[body_end] == '}':
            brace_count -= 1
        body_end += 1
    
    body = tokens[i+2:body_end-1]
    remaining_tokens = tokens[body_end:]
    
    return {
        "tag": "FUNCTION",
        "name": name,
        "params": params,
        "body": body,
        "remaining_tokens": remaining_tokens
    }

def global_parse(tokens):
    ast = []
    while tokens:
        # Пробуем разные парсеры по очереди
        parsed = (parse_function(tokens) or 
                 parse_variable_declaration(tokens) or 
                 parse_loops(tokens))
        
        if parsed:
            ast.append(parsed)
            tokens = parsed.get("remaining_tokens", [])
        else:
            # Если ни один парсер не сработал, пропускаем токен
            tokens = tokens[1:]
    
    return ast