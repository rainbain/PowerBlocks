"""
Assembler Utils

Basic utility like logging and expression evaluation
for the assembler.

Author: Samuel Fitzsimons (rainbain)
File: utils.py
Date: 2025
"""

from .lexer import Token

# A quick helper in case you are having issues
def dump_tokens_to_file(tokens, path):
    with open(path, "w") as f:
        line_number = 0

        for token in tokens:
            # If were on a new line, generate a new line
            if token.line != line_number:
                # Generate new line
                if line_number > 0:
                    f.write("\n")
                    
                line_number = token.line

                f.write(f"{token.file}:{token.line}\t| ")
            
            # Show tokens with a value, otherwise no
            if token.value:
                f.write(f"({token.type}={token.value} col={token.col}) ")
            else:
                f.write(f"({token.type} col={token.col}) ")

def assembly_error(token, error):
    raise SyntaxError(f"{error} at {token.file}:{token.line}")

# Returns a name for a token for error output
def name_token(token):
    if token.value == None:
        return token.type
    
    return token.value

def evaluate_parentheses(tokens):
    output = []

    parentheses = []
    nesting = 0

    for token in tokens:
        if token.type == "LPAREN":
            nesting += 1

            if nesting == 1:
                parentheses = [] # Begin
                continue
        elif token.type == "RPAREN":
            nesting -= 1

            if nesting < 0:
                assembly_error(token, "Expected '(' before ')'")

            if nesting == 0:
                if len(parentheses) == 0:
                    assembly_error(token, "Expected expression")
                output.append(Token("INT", evaluate_expression(parentheses), token.line, token.col, token.file))
                continue

        if nesting > 0:
            parentheses.append(token)
        else:
            output.append(token)
    
    return output

def evaluate_unary(tokens, type, operation):
    output = []

    i = 0
    while i < len(tokens):
        t = tokens[i]
        # Cant have a number before too
        if t.type == type and ((i == 0) or (tokens[i-1].type != "INT")):
            # Next token needs to be the input
            number = None
            if i+1 < len(tokens) and tokens[i+1].type == "INT":
                number = tokens[i+1].value
            
            if number == None:
                assembly_error(t, "Expected a number")
            
            # Override current token
            tokens[i] = Token("INT", operation(number), t.line, t.col, t.file)

            # Remove next token
            del tokens[i + 1]
        
        i += 1

# Unary does them all at once, so it can collect all of them,
# To collapse evil unary expressions like "~!~!~-~~4"
unary_ops = {
    "MINUS": lambda a: -a,
    "INVERT": lambda a: ~a,
    "NOT": lambda a: int(not(a))
}

def evaluate_unary(tokens):
    i = 0
    while i < len(tokens):
        # Must have no number before it too
        if tokens[i].type in unary_ops and ((i == 0) or (tokens[i-1].type != "INT")):
            # Collect all chained unary operators
            ops = []
            start = i
            while i < len(tokens) and tokens[i].type in unary_ops:
                ops.append(tokens[i])
                i += 1

            # Now tokens[i] must be an expression
            if tokens[i].type != "INT":
                # We cannot resolve this yet, skip for next pass
                i += 1
                continue

            # Apply operators right-to-left
            value = tokens[i].value
            for token in ops[::-1]:
                value = unary_ops[token.type](value)

            # Replace the whole chain with the result
            tokens[start:i+1] = [Token("INT", value, tokens[i].line, tokens[i].col, tokens[i].file)]

            # Reset i to continue safely
            i = start
        else:
            i += 1

def evaluate_operator(tokens, type, operation):
    i = 0
    while i < len(tokens):
        t = tokens[i]
        if t.type == type:
            a = None
            if (i-1) >= 0 and tokens[i-1].type == "INT":
                a = tokens[i-1].value

            b = None
            if i+1 < len(tokens) and tokens[i+1].type == "INT":
                b = tokens[i+1].value
            
            if a == None or b == None:
                assembly_error(t, "Expected a number")
            
            # Override previous token
            tokens[i-1] = Token("INT", operation(a, b), t.line, t.col, t.file)

            # Erase next to
            del tokens[i:i+2]

            # Back step i
            i -= 2
        
        i += 1

# If you do i.e. "3-3-3" this looks like 3 numbers next to each other, -3, -3, -3, and
# Not subtraction. This fixes that
def evaluate_subtraction_special_case(tokens):
    i = 1
    while i < len(tokens):
        t = tokens[i]
        p = tokens[i-1]
        if t.type == "INT" and t.value < 0 and p.type == "INT":
            # Detected
            v = t.value + p.value
            tokens[i-1] = Token("INT", v, t.line, t.col, t.file)
            
            del tokens[i]
            i -= 1
        
        i += 1

# Evaluates the ? a : b operator.
def evaluate_ternary(tokens):
    condition = []
    a = []
    b = []

    state = 0
    nests = 0

    i = 0
    while i < len(tokens):
        t = tokens[i]
        if state == 0 and t.type == "QUESTION":
            state += 1
            i = i + 1
            continue
        elif state == 1 and t.type == "QUESTION":
            nests += 1
        elif state == 1 and t.type == "COLON":
            if nests == 0:
                state += 1
                i = i + 1
                continue
            else:
                nests -= 1

        if state == 0:
            condition.append(t)
        elif state == 1:
            a.append(t)
        elif state == 2:
            b.append(t)
        
        i = i + 1
    
    # If we got no ternary operator
    if state == 0:
        return condition
    elif state == 1: # Incomplete ternary
        assembly_error(tokens[-1], "Expected `?`")
    else:
        # They all need data
        if len(condition) == 0 or len(a) == 0 or len(b) == 0:
            assembly_error(tokens[-1], "Expected expression")

        t = condition[0]
        condition = evaluate_expression(condition)
        if condition:
            return [Token(evaluate_expression(a), t.value, t.line, t.col, t.file)]
        else:
            return [Token(evaluate_expression(b), t.value, t.line, t.col, t.file)]

# Evaluates an expression. Must be strictly numerical
def evaluate_expression(tokens):
    # Early exit
    if len(tokens) == 1:
        if tokens[0].type != "INT":
            assembly_error(tokens[0], "Expected a number")
        
        return tokens[0].value

    
    # So we do this in passes, in each pass doing the highest priority operators
    # to the least priority operations to maintain order of operations
    
    tokens = evaluate_parentheses(tokens)
    
    # Unary expressions
    evaluate_unary(tokens)

    # Multiply, divide, modulo
    evaluate_operator(tokens, "MUL", lambda a, b: a * b)
    evaluate_operator(tokens, "DIV", lambda a, b: a // b)
    evaluate_operator(tokens, "MOD", lambda a, b: a % b)

    # Add, subtract
    evaluate_operator(tokens, "PLUS", lambda a, b: a + b)
    evaluate_operator(tokens, "MINUS", lambda a, b: a - b)
    evaluate_subtraction_special_case(tokens)

    # Bit shifts
    evaluate_operator(tokens, "SHL", lambda a, b: int(a) << int(b))
    evaluate_operator(tokens, "SHR", lambda a, b: int(a) >> int(b))

    # Comparisons
    evaluate_operator(tokens, "LT", lambda a, b: int(a < b))
    evaluate_operator(tokens, "LTE", lambda a, b: int(a <= b))
    evaluate_operator(tokens, "GT", lambda a, b: int(a > b))
    evaluate_operator(tokens, "GTE", lambda a, b: int(a >= b))

    # Equality
    evaluate_operator(tokens, "EQ", lambda a, b: int(a == b))
    evaluate_operator(tokens, "NEQ", lambda a, b: int(a != b))

    # Bitwise
    evaluate_operator(tokens, "BITAND", lambda a, b: int(a) & int(b))
    evaluate_operator(tokens, "XOR", lambda a, b: int(a) ^ int(b))
    evaluate_operator(tokens, "BITOR", lambda a, b: int(a) | int(b))

    # Logical bitwise
    evaluate_operator(tokens, "AND", lambda a, b: int(bool(a) and bool(b)))
    evaluate_operator(tokens, "OR", lambda a, b: int(bool(a) or bool(b)))

    tokens = evaluate_ternary(tokens)

    # If were left with multiple tokens, some non int token did not phrase
    if len(tokens) > 1:
        for token in tokens:
            if token.type != "INT":
                assembly_error(token, f"Unexpected \"{token.value}\" in expression")
        
        assembly_error(tokens[0], "Malformed expression")

    return tokens[0].value

class TokenConsumer:
    def __init__(self, tokens):
        self.tokens = tokens
        self.current_token = 0
    
    def consume(self, type=None, expected=None):
        # If end of list
        if self.current_token >= len(self.tokens):
            # If you expected something, thats a problem
            if expected:
                previous_token = self.tokens[self.current_token-1]
                assembly_error(previous_token, f"Expected {expected}")
            
            return None

        token = self.tokens[self.current_token]
        self.current_token += 1

        if type:
            if type != token.type:
                assembly_error(token, f"Expected {expected}")
        
        return token
    
    def peak(self, type=None, value=None):
        if self.current_token >= len(self.tokens):
            return False
        
        token = self.tokens[self.current_token]

        passed = True

        if type != None and type != token.type:
            passed = False
        
        if value != None and value != token.value:
            passed = False
        
        return passed
    
    # Returns true if the current token is after a white space
    def after_whitespace(self):
        if self.current_token < 2:
            return False
        
        next = self.tokens[self.current_token - 1]
        prev = self.tokens[self.current_token - 2]

        return next.col != prev.col + len(prev.value)
    
    # Starting after the `(` gather a comma separated list of list of tokens.
    def consume_list(self, end_value):
        arguments = []

        # If a list is inside a list, we need to know that
        nests = 0

        # Gather the arguments
        field = []
        while True:
            value = self.consume(expected="expression")

            # Nested?
            if value.type == "LPAREN":
                nests += 1
            
            # Unnested
            if nests > 0:
                field.append(value)

                if value.type == end_value:
                    nests -= 1
                
                continue

            # End of arguments
            if value.type == end_value:
                # If were multiple arguments in, and yet field is empty, that means
                # there is a rouge coma
                if len(arguments) > 0 and len(field) == 0:
                    assembly_error(value, "Expected expression after ','")
                
                if len(field) > 0:
                    arguments.append(field)
                
                return arguments

            # Comma is end of field
            if value.type == "COMMA":
                # Nothing before coma, thats weird
                if len(field) == 0:
                    assembly_error(value, "Expected expression before ','")
                else:
                    arguments.append(field)
                    field = []
                
                continue

            field.append(value)
    
    # Gathers until a line ending
    def consume_line(self):
        field = []

        while True:
            value = self.consume(expected="value")

            if value.type == "NEWLINE":
                return field

            field.append(value)