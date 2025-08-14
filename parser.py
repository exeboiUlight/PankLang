def parse_variable_declaration(tokens):
    if tokens[0] != "KEYWORD_VAR":
        return None
    
    name = tokens[1]
    if tokens[2] != '=':
        return None
    
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
    
    return_type = "void"
    if len(tokens) > 2 and tokens[2] == "_ret":
        return_type = tokens[3]
        tokens = tokens[:2] + tokens[4:]
    
    if tokens[2] != '(':
        return None
    
    params = []
    i = 3
    while i < len(tokens) and tokens[i] != ')':
        if tokens[i] != ',':
            params.append(tokens[i])
        i += 1
    
    if tokens[i] != ')':
        return None
    
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
    
    result = {
        "tag": "FUNCTION",
        "name": name,
        "params": params,
        "body": body,
        "remaining_tokens": remaining_tokens
    }
    
    if return_type != "void":
        result["_ret"] = return_type
    
    return result

def parse_dll_import(tokens):
    if tokens[0] != "KEYWORD_USE":
        return None
    
    dll_name = tokens[1]
    remaining_tokens = tokens[2:]
    
    return {
        "tag": "USE",
        "name": dll_name,
        "remaining_tokens": remaining_tokens
    }

def parse_dll_call(tokens):
    if len(tokens) < 3 or tokens[1] != '.':
        return None
    
    dll_name = tokens[0]
    func_name = tokens[2]
    remaining_tokens = tokens[3:]
    
    return {
        "tag": "DLL_CALL",
        "dll": dll_name,
        "func": func_name,
        "remaining_tokens": remaining_tokens
    }

def parse_print(tokens):
    if tokens[0] != "KEYWORD_PRINT":
        return None
    
    arg = tokens[1]
    remaining_tokens = tokens[2:]
    
    return {
        "tag": "PRINT",
        "arg": arg,
        "remaining_tokens": remaining_tokens
    }

def parse_input(tokens):
    if tokens[0] != "KEYWORD_INPUT":
        return None
    
    var_name = tokens[1]
    remaining_tokens = tokens[2:]
    
    return {
        "tag": "INPUT",
        "var_name": var_name,
        "remaining_tokens": remaining_tokens
    }

def global_parse(tokens):
    ast = []
    while tokens:
        parsed = (parse_function(tokens) or 
                 parse_variable_declaration(tokens) or 
                 parse_loops(tokens) or
                 parse_dll_import(tokens) or
                 parse_dll_call(tokens) or
                 parse_print(tokens) or
                 parse_input(tokens))
        
        if parsed:
            ast.append(parsed)
            tokens = parsed.get("remaining_tokens", [])
        else:
            tokens = tokens[1:]
    
    return ast