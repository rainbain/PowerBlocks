"""
DSP Microcode Assembler Lexer

Converts text into discretized tokens for easy processing.

Author: Samuel Fitzsimons (rainbain)
File: lexer.py
Date: 2025
"""

import re

# Define what a token is
class Token:
    def __init__(self, type, value, line, col, file):
        self.type = type
        self.value = value
        self.line = line
        self.col = col
        self.file = file
    
    def __repr__(self):
        return f"Token({self.type}, {self.value}, {self.line}:{self.col})"

# regex patterns for each kind of token.
TOKEN_SPEC = [
    # ------------------------------------
    # Spacing / Comments
    # ------------------------------------
    ("COMMENT",      r"//[^\n]*"),
    ("NEWLINE",      r"\n"),
    ("SKIP",         r"[ \t]+"),

    # ------------------------------------
    # Literals (longest/most specific first)
    # ------------------------------------
    ("BINARY",       r"0[bB][01]+"),
    ("HEX",          r"0[xX][0-9A-Fa-f]+"),
    ("INT",          r"-?[0-9]+"),
    ("STRING",       r'"[^"\n]*"'),

    # ------------------------------------
    # Preprocessor
    # ------------------------------------
    ("DIRECTIVE",    r"#[A-Za-z_][A-Za-z0-9_]*"),

    # ------------------------------------
    # Operators: longest FIRST
    # ------------------------------------
    # Comparison
    ("EQ",           r"=="),
    ("NEQ",          r"!="),
    ("LTE",          r"<="),
    ("GTE",          r">="),

    # Bitshift
    ("SHL",          r"<<"),
    ("SHR",          r">>"),

    # Logical Bitwise
    ("AND",          r"&&"),
    ("OR",          r"\|\|"),

    # Single-char operators (order irrelevant now)
    ("LT",           r"<"),
    ("GT",           r">"),
    ("PLUS",         r"\+"),
    ("MINUS",        r"-"),
    ("MUL",          r"\*"),
    ("DIV",          r"/"),
    ("MOD",          r"%"),
    ("BITAND",       r"&"),
    ("BITOR",        r"\|"),
    ("XOR",          r"\^"),
    ("ASSIGN",       r"="),
    ("NOT",          r"!"),
    ("INVERT",       r"~"),

    # ------------------------------------
    # Punctuation
    # ------------------------------------
    ("LPAREN",       r"\("),
    ("RPAREN",       r"\)"),
    ("COMMA",        r","),
    ("COLON",        r":"),
    ("QUESTION",     r"\?"),

    # ------------------------------------
    # Identifiers
    # ------------------------------------
    ("IDENT",        r"[A-Za-z_][A-Za-z0-9_']*"),

    # ------------------------------------
    # Fallback for bad characters
    # ------------------------------------
    ("MISMATCH",     r"."),
]

# Compile the regex pattern
MASTER_RE = re.compile("|".join(f"(?P<{name}>{pattern})" for name,pattern in TOKEN_SPEC))

def strip_multiline_comments(text: str, filename: str):
    out = []
    i = 0
    n = len(text)

    while i < n:
        if i + 1 < n and text[i] == '/' and text[i+1] == '*':
            end = text.find("*/", i+2)
            if end == -1:
                raise SyntaxError(f"{filename}: Unterminated /* comment */")

            # Preserve newlines inside the comment so line numbers remain correct
            for c in text[i+2:end]:
                if c == '\n':
                    out.append('\n')

            i = end + 2
            continue

        out.append(text[i])
        i += 1

    return ''.join(out)


def lex(text: str, filename: str):
    text = strip_multiline_comments(text, filename)

    tokens = []
    line_number = 1
    line_start = 0

    # Go through text with regex patterns
    for m in MASTER_RE.finditer(text):
        kind = m.lastgroup
        value = m.group()
        col = m.start() - line_start + 1

        if kind == "NEWLINE":
            tokens.append(Token(kind, None, line_number, col, filename))

            line_number += 1
            line_start = m.end()
        elif kind == "SKIP" or kind == "COMMENT":
            # Remove these
            continue
        elif kind == "HEX":
            tokens.append(Token("INT", int(value, 16), line_number, col, filename))
        elif kind == "BINARY":
            tokens.append(Token("INT", int(value, 2), line_number, col, filename))
        elif kind == "INT":
            tokens.append(Token("INT", int(value), line_number, col, filename))
        elif kind == "MISMATCH":
            raise SyntaxError(f"Illegal character \"{value!r}\" at line {line_number} column {col}")
        else:
            # All others
            tokens.append(Token(kind, value, line_number, col, filename))
        
    return tokens