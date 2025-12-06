"""
DSP Microcode Assembler Parser

Takes tokens in, and converts them to a program.

The program will still have to be linked, but contains the actual program data.

Author: Samuel Fitzsimons (rainbain)
File: parser.py
Date: 2025
"""

from .utils import TokenConsumer, assembly_error, evaluate_expression, name_token
from .lexer import Token

class DataDirective:
    def __init__(self, program, word_size, endian="big"):
        self.program = program
        self.word_size = word_size
        self.endian = endian

        self.values = []
    
    def consume(self, consumer: TokenConsumer):
        # Consume a coma separated list.
        self.values = consumer.consume_list("NEWLINE")
    
    def serialize(self, _):
        output = bytearray()

        max_val = (1 << (8 * self.word_size)) - 1
        min_val = -(1 << (8 * self.word_size - 1))

        for value in self.values:
            # Get an actual value, integer
            phrased_value = self.program.evaluate_expression(value)

            if not (min_val <= phrased_value <= max_val):
                assembly_error(phrased_value, f"Value {value} does not fit in {self.word_size} bytes")
            
            value_bytes = phrased_value.to_bytes(
                self.word_size,
                byteorder=self.endian
            )

            output.extend(value_bytes)
        
        return output
    
    def length(self, _):
        return len(self.values) * self.word_size

class StringDirective:
    def __init__(self, program, null_terminator):
        self.program = program
        self.null_terminator = null_terminator
    
    def consume(self, consumer: TokenConsumer):
        self.string = consumer.consume("STRING", "string").value[1:-1]
    
    def serialize(self, _):
        data = self.string.encode('ascii')

        if self.null_terminator:
            data += b'\x00'
        
        return data
    
    def length(self, _):
        return len(self.string) + int(self.null_terminator)

class SpaceDirective:
    def __init__(self, program):
        self.program = program
    
    def consume(self, consumer: TokenConsumer):
        expression = consumer.consume_line()

        # Check for a symbol in the length expression, we cant do that
        for token in expression:
            if token.type == "IDENT":
                assembly_error(token, f"Space/Skip directives must have a constant fill size. Found unevaluated symbol \"{token.value}\"")
        
        self.space = evaluate_expression(expression)
    
    def serialize(self, _):
        data = bytearray()
        data.extend([self.program.fill_pattern] * self.space)
        return data
    
    def length(self, _):
        return self.space

class Label:
    def __init__(self, name):
        self.name = name
    
    def serialize(self, _):
        return bytearray()
    
    def length(self, _):
        return 0

class AlignmentDirective:
    def __init__(self, program):
        self.program = program
    
    def consume(self, consumer: TokenConsumer):
        expression = consumer.consume_line()

        # Check for a symbol in the length expression, we cant do that
        for token in expression:
            if token.type == "IDENT":
                assembly_error(token, f"Alignment directives must have a constant fill size. Found unevaluated symbol \"{token.value}\"")
        
        self.alignment = evaluate_expression(expression)
    
    def serialize(self, pc):
        data = bytearray()
        data.extend([self.program.fill_pattern] * self.length(pc))
        return data
    
    def length(self, pc):
        return (self.alignment - pc % self.alignment) % self.alignment

class OrgDirective:
    def __init__(self, program):
        self.program = program
    
    def consume(self, consumer: TokenConsumer):
        expression = consumer.consume_line()

        # Check for a symbol in the length expression, we cant do that
        for token in expression:
            if token.type == "IDENT":
                assembly_error(token, f"ORG directives must have a constant fill size. Found unevaluated symbol \"{token.value}\"")
        
        self.address = evaluate_expression(expression)
    
    def serialize(self, _):
        return bytearray()
    
    def length(self, _):
        return 0


class Program:
    def __init__(self):
        self.statements = []
        self.symbols = {}

        self.fill_pattern = 0

        self.clear()
    
    def clear(self):
        self.written = bytearray()
        self.data = bytearray()
    
    # Evaluates an expression by first filling in all symbols.
    def evaluate_expression(self, expression):
        for i in range(len(expression)):
            t = expression[i]
            if t.type == "IDENT":
                if not t.value in self.symbols:
                    assembly_error(t, f"Undefined symbol \"{t}\"")

                expression[i] = Token("INT", self.symbols[t.value], t.line, t.col, t.file)
        
        return evaluate_expression(expression)
    
    # Assigns an address to each symbol
    def evaluate_symbols(self):
        pc = 0
        for stmt in self.statements:
            if isinstance(stmt, Label):
                self.symbols[stmt.name] = pc
            elif isinstance(stmt, OrgDirective):
                pc = stmt.address
            else:
                pc += stmt.length(pc)
    
    def write(self, address, data: bytes):
        end = address + len(data)

        if end > len(self.data):
            grow = end - len(self.data)
            self.data.extend([self.fill_pattern] * grow)
            self.written.extend([0] * grow)
        
        for i in range(address, end):
            if self.written[i]:
                msg = f"Memory overlap at address 0x{i:X}"
                raise RuntimeError(msg)
        
        for i in range(address, end):
            self.written[i] = 1

        self.data[address:end] = data
    
    def push(self, value):
        self.statements.append(value)
    
    # Creates bytecode
    def generate_bytecode(self):
        self.clear()

        # Figure out where each symbol is
        self.evaluate_symbols()

        pc = 0
        for stmt in self.statements:
            if isinstance(stmt, OrgDirective):
                pc = stmt.address

            data = stmt.serialize(pc)

            self.write(pc, data)
            pc += stmt.length(pc)
        
        return self.data

class Parser:
    def __init__(self):
        pass

    def run(self, tokens) -> Program:
        self.consumer = TokenConsumer(tokens)
        self.program = Program()

        # Consume until none is left
        while True:
            token = self.consumer.consume()

            if token == None:
                break

            # We can ignore newlines
            if token.type == "NEWLINE":
                continue

            if token.type == "ASMDIRECTIVE":
                self.asm_directive(token)
            elif token.type == "IDENT" and self.consumer.peak(type="COLON"):
                self.label(token)
            else:
                assembly_error(token, f"Was not expecting \"{name_token(token)}\"")
        
        return self.program

    def label(self, token):
        label = Label(token.value)

        self.program.push(label)

        # Consume rest of line
        self.consumer.consume_line()
    
    def asm_directive(self, token):
        # Get the directive
        name = token.value

        directive = None
        if name == ".byte":
            directive = DataDirective(self.program, 1)
        elif name == ".half" or name == ".short":
            directive = DataDirective(self.program, 2)
        elif name == ".word" or name == ".long":
            directive = DataDirective(self.program, 4)
        elif name == ".quad":
            directive = DataDirective(self.program, 8)
        elif name == ".asci":
            directive = StringDirective(self.program, False)
        elif name == ".asciz" or name == ".string":
            directive = StringDirective(self.program, True)
        elif name == ".space" or name == ".skip":
            directive = SpaceDirective(self.program)
        elif name == ".align":
            directive = AlignmentDirective(self.program)
        elif name == ".org":
            directive = OrgDirective(self.program)
        else:
            assembly_error(token, f"Unknown directive \"{name}\"")
        
        directive.consume(self.consumer)

        self.program.push(directive)