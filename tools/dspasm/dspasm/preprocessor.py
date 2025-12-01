from .lexer import lex

import os
import sys

class Preprocessor:
    def __init__(self):
        self.macros = {}

        # Track includes for output
        # And prevent circular includes
        self.include_stack = []

        # All the macros
        self.macros = []
    
    def preprocessor_error(self, token, expected):
        raise SyntaxError(f"Expected {expected} at {token.file}:{token.line}")

    # Sets up the state machine for processing tokens
    def set_state_machine(self, tokens):
        self.tokens = tokens
        self.current_token = 0
    
    def consume(self, type=None, expected=None):
        # If end of list
        if self.current_token >= len(self.tokens):
            # If you expected something, thats a problem
            if expected:
                previous_token = self.tokens[self.current_token-1]
                self.preprocessor_error(previous_token, expected)
            
            return None

        token = self.tokens[self.current_token]
        self.current_token += 1

        if type:
            if type != token.type:
                self.preprocessor_error(token, expected)
        
        return token
    
    # This will go through and father all macros defined in the code
    def gather(self, tokens):
        self.set_state_machine(tokens)

        # Consume until none is left
        while True:
            token = self.consume()

            if token == None:
                break
            
            # Only handle #something
            if token.type != "DIRECTIVE":
                continue

            if token.value == "#define":
                self.gather_macro()
    
    def gather_macro(self):
        name = self.consume("IDENT", "definition name")

        value = self.consume()

        # Macro arguments
        arguments = []

        values = []

        # Gather arguments if the macro has no whitespace between this "value" and the actual value
        if value.type == "LPAREN" and value.col == name.col + len(name.value):
            arguments = self.gather_list()

            # Grab remaining data
            values = self.gather_line()
        else:
            values.append(value)
            values += self.gather_line()

        print(f"{name.value}({arguments}) = {values}")

    # Starting after the `(` gather a comma separated list of list of tokens.
    def gather_list(self):
        arguments = []

        # Gather the arguments
        field = []
        while True:
            value = self.consume(expected="expression")

            # End of arguments
            if value.type == "RPAREN":
                # If were multiple arguments in, and yet field is empty, that means
                # there is a rouge coma
                if len(arguments) > 0 and len(field) == 0:
                    self.preprocessor_error(value, "expression after ','")
                
                if len(field) > 0:
                    arguments.append(field)
                
                return arguments

            # Comma is end of field
            if value.type == "COMMA":
                # Nothing before coma, thats weird
                if len(field) == 0:
                    self.preprocessor_error(value, "expression before ','")
                else:
                    arguments.append(field)
                    field = []
                
                continue

            field.append(value)
    
    # Gathers until a line ending
    def gather_line(self):
        field = []

        while True:
            value = self.consume(expected="value")

            if value.type == "NEWLINE":
                return field

            field.append(value)
        




    # This step just resolves all includes recursively
    def recursive_include(self, path):
        filename = os.path.basename(path)

        # If this is already in the include stack, thats a include error
        for stack in self.include_stack:
            if os.path.samefile(stack, path):
                error = "Circular include:\n"
                for stack in self.include_stack:
                    error += f"\t\tInclude: {stack}\n"
                
                error += f"\t\tInclude: {path}\n"
                raise RuntimeError(error)
        
                # Push onto include stack
        self.include_stack.append(path)
        
        # Attempt to read file
        with open(path, "r") as f:
            input_text = f.read()

            # Remove all `\r`, they are not needed
            input_text = input_text.replace("\r", "")

            # Add line ending if needed
            if not input_text.endswith("\n"):
                input_text += "\n"
        
        # Tokenize
        tokens = lex(input_text, filename)

        i = 0
        output_tokens = []

        while i < len(tokens):
            t = tokens[i]

            if t.type == "DIRECTIVE" and t.value == "#include":
                i += 1
                if i >= len(tokens):
                    self.preprocessor_error(t, "File Name")

                next_t = tokens[i]
                if next_t.type != "STRING":
                    self.preprocessor_error(t, "String")

                # Strip quotes and resolve path relative to current file
                include_path = os.path.join(os.path.dirname(path), next_t.value.strip('"'))

                # Recursively resolve included file
                included_tokens = self.recursive_include(include_path)

                # Insert included tokens into output (flatten)
                output_tokens.extend(included_tokens)
            else:
                # Regular token, just append
                output_tokens.append(t)

            i += 1
    
        # Pop from include stack
        self.include_stack.pop()
        return output_tokens

