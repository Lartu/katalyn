from __future__ import annotations
from typing import Dict, List, Set, Tuple, Any, Optional
from enum import Enum, auto

class LexType(Enum):
    WORD = auto()
    INTEGER = auto()
    FLOAT = auto()
    STRING = auto()
    OPERATOR = auto()
    VARIABLE = auto()
    TABLE = auto()
    ACCESS_OPEN = auto()
    ACCESS_CLOSE = auto()
    PAR_OPEN = auto()
    PAR_CLOSE = auto()
    DECORATION = auto()
    UNKNOWN = auto()

class Token:
    def __init__(self, value: str, line: int, file: str) -> None:
        self.value: str = value
        self.line: int = line
        self.file: str = file
        self.type: LexType = LexType.UNKNOWN

    def set_type(self, type: LexType):
        self.type = type

    def __repr__(self):
        return f"{self.value} ({self.file}:{self.line})"
    
    def __str__(self):
        color: str = "\033[31;40m"
        if self.type == LexType.WORD:
            color = "\033[37;44m"
        elif self.type == LexType.STRING:
            color = "\033[33;40m"
        elif self.type == LexType.INTEGER:
            color = "\033[37;45m"
        elif self.type == LexType.FLOAT:
            color = "\033[30;43m"
        elif self.type == LexType.OPERATOR:
            color = "\033[36;40m"
        elif self.type == LexType.VARIABLE:
            color = "\x1b[30;46m"
        elif self.type == LexType.TABLE:
            color = "\033[31;47m"
        elif self.type == LexType.ACCESS_OPEN or self.type == LexType.ACCESS_CLOSE:
            color = "\033[32;47m"
        elif self.type == LexType.PAR_OPEN or self.type == LexType.PAR_CLOSE:
            color = "\033[35;47m"
        elif self.type == LexType.DECORATION:
            color = "\033[30;47m"
        return f"{color} {self.value} \033[0m"


def katalyn_error(title: str, lines: List[str]):
    """Prints a Katalyn standard error with the passed lines and exits.
    """
    print("")
    print(f"=== {title} ===")
    for line in lines:
        print(line)
    print("")
    exit(1)


def tokenization_error(message: str, line: int, filename: str):
    """Prints a tokenization error and exits.
    """
    error_title = "Katayln Tokenization Error"
    error_lines = [
        f"- Where? In file '{filename}', on line {line}.",
        f"- Error Message: {message}",
    ]
    katalyn_error(error_title, error_lines)


def lexing_error(message: str, line: int, filename: str):
    """Prints a lexing error and exits.
    """
    error_title = "Katayln Lexing Error"
    error_lines = [
        f"- Where? In file '{filename}', on line {line}.",
        f"- Error Message: {message}",
    ]
    katalyn_error(error_title, error_lines)


def tokenize_source(code: str, filename: str) -> List[List[Token]]:
    """Takes Katalyn source code and splits it into tokens.
    """
    last_string_open_line = 1  # For error reporting purposes
    line_num = 1
    lines: List[List[Token]] = []
    current_line: List[Token] = []
    code += " "  # To simplify tokenization
    current_token = ""
    in_string = False
    in_access_string = False
    in_comment = False
    i = 0
    while i < len(code) - 1:
        current_char = code[i]
        next_char = code[i + 1]
        if not in_comment and not in_string and not in_access_string and current_char == "(" and next_char == "*":
            in_comment = True
            i += 1
        elif in_comment and not in_string and not in_access_string and current_char == "*" and next_char == ")":
            in_comment = False
            i += 1
        elif not in_comment and (in_string or in_access_string) and current_char == '\\':
            # This block should always go before any that match "" or {}
            if next_char == "n":
                current_token += "\n"
            elif next_char == "t":
                current_token += "\v"
            elif next_char == '"':
                current_token += '"'
            else:
                current_token += next_char
            i += 1
        elif not in_comment and not in_string and not in_access_string and current_char == '"':
            if len(current_token):
                current_line.append(Token(current_token, line_num, filename))
            current_token = ""
            current_token += current_char
            in_string = True
            last_string_open_line = line_num
        elif not in_comment and in_string and not in_access_string and current_char == '"':
            current_token += current_char
            in_string = False
            if len(current_token):
                current_line.append(Token(current_token, last_string_open_line, filename))
            current_token = ""
        elif not in_comment and not in_string and not in_access_string and current_char == '{':
            if len(current_token):
                current_line.append(Token(current_token, line_num, filename))
            current_token = ""
            current_line.append(Token("[", line_num, filename))
            current_token += '"'
            in_access_string = True
            last_string_open_line = line_num
        elif not in_comment and not in_string and in_access_string and current_char == '}':
            current_token += '"'
            in_access_string = False
            if len(current_token):
                current_line.append(Token(current_token, last_string_open_line, filename))
            current_token = ""
            current_line.append(Token("]", line_num, filename))
        elif not in_comment and not in_string and not in_access_string and current_char == ";":
            if len(current_token):
                current_line.append(Token(current_token, line_num, filename))
            current_token = ""
            if len(current_line):
                lines.append(current_line)
            current_line = []
        elif not in_comment and not in_string and not in_access_string and current_char + next_char in (">=", "<=", "!=", "//"):
            # Biglyphs
            if len(current_token):
                current_line.append(Token(current_token, line_num, filename))
            current_token = ""
            current_line.append(Token(current_char + next_char, line_num, filename))
            i += 1
        elif not in_comment and not in_string and not in_access_string and current_char in "(){}[]=<>!+-/&%^*:#":
            # Single glyphs
            if len(current_token):
                current_line.append(Token(current_token, line_num, filename))
            current_token = ""
            current_line.append(Token(current_char, line_num, filename))
        elif not in_comment and not in_string and not in_access_string and current_char.isspace():
            if len(current_token):
                current_line.append(Token(current_token, line_num, filename))
            current_token = ""
            if current_char == "\n":
                line_num += 1
        elif not in_comment:
            current_token += current_char
        i += 1
    # Check for consistency
    # I want to be able to leave comments open til the end of the file
    if in_string:
        tokenization_error("Open string, missing '\"'", last_string_open_line, filename)
    if in_access_string:
        tokenization_error("Open access string, missing '}'", last_string_open_line, filename)
    if len(current_line):
        tokenization_error("Missing ';'", line_num, filename)
    if len(current_token):
        tokenization_error("Missing ';'", line_num, filename)
    return lines


def pad_string(text: str, padding_size: int) -> str:
    """Pads a string to a given number of chars (left side)
    """
    if len(text) > padding_size:
        return text
    else:
        return text + " " * (padding_size - len(text))


def print_tokens(tokenized_lines: List[List[Token]], filename: str, step: str):
    """Prints the result of a tokenization.
    """
    print("")
    print(f"=== {step} Result for File '{filename}' ===")
    for line in tokenized_lines:
        print(f"Line {pad_string(str(line[0].line) + ':', 4)} ", end="")
        tokens_str: List[str] = []
        for token in line:
            tokens_str.append(str(token))
        print(", ".join(tokens_str))
    print("")


def is_integer(text: str) -> bool:
    """Returns true if the string represents an integer number.
    """
    if not len(text):
        return False
    for char in text:
        if char not in "0987654321":
            return False
    return True
        

def is_float(text: str) -> bool:
    """Returns true if the string represents an floating point number.
    """
    if not len(text):
        return False
    found_point = False
    for char in text:
        if char == ".":
            if found_point:
                return False
            else:
                found_point = True
                continue
        elif char not in "0987654321":
            return False
    if text[0] == "." or text[-1] == ".":
        return False
    return found_point


def is_valid_variable(text: str) -> bool:
    """Returns true if the string represents a valid variable name.
    """
    if len(text) < 2:
        return False
    if text[0] != "$":
        return False
    for char in text[1:]:
        if char not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789":
            return False
    return True


def is_valid_identifier(text: str) -> bool:
    """Returns true if the string represents a valid identifier name.
    """
    if not len(text):
        return False
    if text[0] not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_":
        return False
    for char in text[1:]:
        if char not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789":
            return False
    return True


def is_almost_number(text: str) -> bool:
    """Returns true if the string almost represents a valid number.
    """
    if not len(text):
        return False
    for char in text:
        if char not in "0123456789.":
            return False
    return True


def lex_tokens(tokenized_lines: List[List[Token]]) -> List[List[Token]]:
    """Given a list of tokenized lines, lexes each token attatching extra information
    to make the parsing simpler. It modifies the list in place.
    """
    for line in tokenized_lines:
        for token in line:
            if token.value[0] == '"' and token.value[-1] == '"':
                token.value = token.value[1:-1]
                token.type = LexType.STRING
            elif is_valid_variable(token.value):
                token.type = LexType.VARIABLE
            elif token.value == "(":
                token.type = LexType.PAR_OPEN
            elif token.value == ")":
                token.type = LexType.PAR_CLOSE
            elif token.value == "[":
                token.type = LexType.ACCESS_OPEN
            elif token.value == "]":
                token.type = LexType.ACCESS_CLOSE
            elif token.value == ":":
                token.type = LexType.DECORATION
            elif token.value == "#":
                token.type = LexType.TABLE
            elif is_integer(token.value):
                token.type = LexType.INTEGER
            elif is_float(token.value):
                token.type = LexType.FLOAT
            elif token.value in ("<", "<=", ">", ">=", "!", "!=", "=", "+", "-", "*", "/", "%", "//", "^", "&"):
                token.type = LexType.OPERATOR
            elif is_valid_identifier(token.value):
                token.type = LexType.WORD
            else:
                # If we reached this point, there's an error
                if token.value[0] == "$":
                    lexing_error(f"The string '{token.value}' is not a valid variable name.", token.line, token.file)
                elif is_almost_number(token.value):
                    lexing_error(f"The string '{token.value}' is not a valid number.", token.line, token.file)
                else:                   
                    lexing_error(f"The string '{token.value}' is not a valid identifier.", token.line, token.file)

    # Join minus operators to the following numbers in valid positions
    for line in tokenized_lines:
        i = 2
        while i < len(line):
            token: Token = line[i]
            if token.type == LexType.INTEGER or token.type == LexType.FLOAT:
                previous_token: Token = line[i - 1]
                if previous_token.type == LexType.OPERATOR and previous_token.value == "-":
                    preprevious_token: Token = line[i - 2]
                    if preprevious_token.type == LexType.OPERATOR:
                        line.pop(i - 1)
                        token.value = f"-{token.value}"
                        continue
            i += 1


    return tokenized_lines



if __name__ == "__main__":
    code: str = ""
    filename: str = "test.fs"
    with open("test.fs") as f:
        code = f.read()
    tokenized_lines: List[List[Token]] = tokenize_source(code, filename)
    print_tokens(tokenized_lines, filename, "Tokenization")
    lex_tokens(tokenized_lines)
    print_tokens(tokenized_lines, filename, "Lexing")