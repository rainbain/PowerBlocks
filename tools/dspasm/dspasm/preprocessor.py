"""
DSP Microcode Assembler Preprocessor

Takes tokens in, evaluates preprocessor derivatives,
and spits the tokens back out.

This is with the purpose of what you see with the preprocessor
in many compilers.

Author: Samuel Fitzsimons (rainbain)
File: preprocessor.py
Date: 2025
"""

from .lexer import lex
from .utils import evaluate_expression, assembly_error

import os

class Macro:
    def __init__(self, name_token, arguments, value):
        # Each argument should just be a single name. if not thats an issue
        self.name_token = name_token

        # If it has arguments, its function like
        if arguments:
            self.arguments = []
            for argument in arguments:
                if len(argument) > 1 or argument[0].type != "IDENT":
                    assembly_error(argument[0], "Invalid definition arguments")
            
                self.arguments.append(argument[0].value)
        else:
            self.arguments = None
        
        self.value = value
    
    # Returns true if its invoked like a function
    def functionlike(self):
        return self.arguments != None
    
    def flatten(self, inputs):
        # If its not function like, then its just its value
        if not self.functionlike():
            return self.value

        output = []

        # Argument count must match
        if len(inputs) != len(self.arguments):
            assembly_error(self.name_token, f"Invalid number of input arguments. Expected {len(self.arguments)}, got {len(inputs)}")
        
        # Ok now fill in each argument
        for token in self.value:
            # See if we can fill in an argument
            if token.type == "IDENT" and token.value in self.arguments:
                argument_index = self.arguments.index(token.value)

                output += inputs[argument_index]
            else:
                output.append(token)
        
        return output

class Preprocessor:
    def __init__(self):
        self.macros = {}

        # Track includes for output
        # And prevent circular includes
        self.include_stack = []

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
                assembly_error(previous_token, f"Expected {expected}")
            
            return None

        token = self.tokens[self.current_token]
        self.current_token += 1

        if type:
            if type != token.type:
                assembly_error(token, f"Expected {expected}")
        
        return token
    
    # Returns true if the current token is after a white space
    def after_whitespace(self):
        if self.current_token < 2:
            return False
        
        next = self.tokens[self.current_token - 1]
        prev = self.tokens[self.current_token - 2]

        return next.col != prev.col + len(prev.value)

    
    # This will go through and father all macros defined in the code
    def gather(self, tokens):
        self.set_state_machine(tokens)

        output = []

        # Consume until none is left
        while True:
            token = self.consume()

            if token == None:
                break

            # Only handle #something
            elif token.type == "DIRECTIVE" and token.value == "#define":
                self.gather_macro()
            else:
                output.append(token)
        
        return output
                
    
    def gather_macro(self):
        name = self.consume("IDENT", "definition name")

        value = self.consume()

        # Macro arguments
        arguments = None
        values = []

        # Gather arguments if the macro has no whitespace between this "value" and the actual value
        if value.type == "LPAREN" and not self.after_whitespace():
            arguments = self.gather_list()

            # Grab the actual value now
            value = self.consume()
        
        # If there is something in value
        if value.type != "NEWLINE":
            # It must be after a white space
            if not self.after_whitespace():
                assembly_error(value, "Expected white space")
            
            # Collect values
            values.append(value)
            values += self.gather_line()

        # Hopefully no duplicate macros
        if name.value in self.macros:
            assembly_error(name, "Redefinition of macro")
        
        self.macros[name.value] = Macro(name, arguments, values)

    # Starting after the `(` gather a comma separated list of list of tokens.
    def gather_list(self):
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

                if value.type == "RPAREN":
                    nests -= 1
                
                continue

            # End of arguments
            if value.type == "RPAREN":
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
    def gather_line(self):
        field = []

        while True:
            value = self.consume(expected="value")

            if value.type == "NEWLINE":
                return field

            field.append(value)
    
    def check_circular_macro(self, start_name, values, visited):
        for token in values:
            if token.type == "IDENT" and token.value in self.macros:
                macro_name = token.value

                # If we've seen this macro already while resolving from start_name → cycle
                if macro_name in visited:
                    assembly_error(token, f"Circular definition detected: "
                                          f"{' -> '.join(list(visited) + [macro_name])}")
            
                # If it directly refers back to the starting macro
                if macro_name == start_name:
                    assembly_error(token, f"Circular definition detected: "
                                          f"{start_name} refers to itself.")

                # Continue recursion
                visited.add(macro_name)
                self.check_circular_macro(start_name, self.macros[macro_name].value, visited)
                visited.remove(macro_name)  # backtrack


    def check_circular_definitions(self):
        for macro_name, macro in self.macros.items():
            self.check_circular_macro(macro_name, macro.value, visited={macro_name})
    
    # Evaluates an expression that can contain macros
    def evaluate_expression(self, input):
        preprocessor = Preprocessor()
        preprocessor.macros = self.macros

        return evaluate_expression(preprocessor.run(input))

    # Runs the preprocessor
    def run(self, tokens):
        output = []

        self.set_state_machine(tokens)

        # Conditional directives, i.e. ifdef
        condition_stack = [True]
        condition_stack_owners = []

        # Consume until none is left
        while True:
            token = self.consume()

            if token == None:
                break

            code_disable = not condition_stack[-1]

            # Always process these, even if ignored
            if  token.type == "DIRECTIVE" and (token.value == "#ifdef" or token.value == "#ifndef"):
                # Ignore if in ignored code, just push continued falses
                if code_disable:
                    condition_stack.append(False)
                    condition_stack_owners.append(False)
                    continue
                
                inverted = token.value == "#ifndef"
                
                # Macro name
                name = self.consume("IDENT", "Expected definition")

                # Consume rest of life
                self.gather_line()

                condition_stack_owners.append(token)
                if name.value in self.macros:
                    condition_stack.append(not inverted)
                else:
                    condition_stack.append(inverted)
                continue
            elif  token.type == "DIRECTIVE" and token.value == "#if":
                # Ignore if in ignored code, just push continued falses
                if code_disable:
                    condition_stack_owners.append(token)
                    condition_stack.append(False)
                    continue

                expression = self.gather_line()
                if len(expression) == 0:
                    assembly_error(token, "Expected expression")
                
                # Does it evaluate to true
                result = self.evaluate_expression(expression)

                condition_stack.append(bool(result))
                condition_stack_owners.append(token)
                continue
            elif  token.type == "DIRECTIVE" and token.value == "#else":
                if len(condition_stack) <= 1:
                    assembly_error(token, "#else unexpected at this time")
                
                # If the one before it is false, this deadshorts to false
                if not condition_stack[-2]:
                    continue

                # Swap the condition
                condition_stack[-1] = not condition_stack[-1]
                condition_stack_owners[-1] = token
                continue
            elif  token.type == "DIRECTIVE" and token.value == "#elif":
                # Its like if, but it only runs if the previous one was false
                if len(condition_stack) <= 1:
                    assembly_error(token, "#elif unexpected at this time")
                
                # If the one before it is false, this deadshorts to false
                if not condition_stack[-2]:
                    continue
                
                condition_stack_owners[-1] = token
                if condition_stack[-1]:
                    condition_stack[-1] = False
                    continue

                expression = self.gather_line()
                if len(expression) == 0:
                    assembly_error(token, "Expected expression")
                
                # Does it evaluate to true
                result = self.evaluate_expression(expression)

                condition_stack[-1] = bool(result)
                continue
            elif token.type == "DIRECTIVE" and token.value == "#endif":
                if len(condition_stack) <= 1:
                    assembly_error(token, "#endif unexpected at this time")
                
                # Consume rest of life
                self.gather_line()
                
                condition_stack.pop()
                condition_stack_owners.pop()
                continue

            # If in ignored part of the code
            if code_disable:
                continue

            if token.type == "IDENT": # Something we will need to try evaluate
                output += self.flatten_macro(token)
            else:
                output.append(token)
        
        # If one has not been closed, thats an issue
        if len(condition_stack) > 1:
            assembly_error(condition_stack_owners[-1], "Unclosed preprocessor conditional")
        
        return output

    
    # Evaluates a sub expression
    def flatten_macro(self, name):
        # Is this a no a macro, then skip it
        if not name.value in self.macros:
            return [name]
        
        macro = self.macros[name.value]

        arguments = None

        # If its functionlike, it must have an argument list
        # Even if empty.
        if macro.functionlike():
            # Make sure it begins with an open parentheses
            self.consume("LPAREN", "'('")

            arguments = self.gather_list()
        
        flattened = macro.flatten(arguments)

        # Things get recursive here,
        # We will resolve any macros invoked by the macro
        # Using the preprocessor
        preprocessor = Preprocessor()
        preprocessor.macros = self.macros

        return preprocessor.run(flattened)

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
                    assembly_error(t, "Expected file name")

                next_t = tokens[i]
                if next_t.type != "STRING":
                    assembly_error(t, "Expected string")

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

