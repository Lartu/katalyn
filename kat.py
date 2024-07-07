# 
# The__  __.       __         .__                
# |    |/ _|____ _/  |______  |  | ___.__. ____  
# |      < \__  \\   __\__  \ |  |<   |  |/    \ 
# |    |  \ / __ \|  |  / __ \|  |_\___  |   |  \
# |____|__ (____  /__| (____  /____/ ____|___|  /
#        \/    \/          \/ Programming Language
# 
# Started by Lartu on July 3, 2024 (01:13 AM).
# 

from __future__ import annotations
from typing import Dict, List, Set, Tuple, Any, Optional
from enum import Enum, auto


OPERATOR_PRESEDENCE = ("<", ">", "<=", ">=", "=", "<>", "!", "&", "*", "^", "/", "%", "//", "+", "-")


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


def expression_error(message: str, line: int, filename: str):
    """Prints an expression error and exits.
    """
    error_title = "Katayln Expression Error"
    error_lines = [
        f"- Where? In file '{filename}', on line {line}.",
        f"- Error Message: {message}",
    ]
    katalyn_error(error_title, error_lines)


def tokenize_source(code: str, filename: str) -> List[List[Token]]:
    """Takes Katalyn source code and splits it into tokens.
    """
    last_line_with_tokens = 1  # For error reporting purposes
    last_string_open_line = 1  # For error reporting purposes
    line_num = 1
    lines: List[List[Token]] = []
    current_line: List[Token] = []
    code += " "  # To simplify tokenization
    current_token = ""
    in_string = False
    in_access_string = False
    comment_depth: int = 0
    # Katalyn supports nested comments, but doesn't make any distinction between (* ... *) inside strings
    # once that string is inside an already open comment (such as (* "(* *)" *))
    i = 0
    while i < len(code) - 1:
        current_char = code[i]
        next_char = code[i + 1]
        if not in_string and not in_access_string and current_char == "(" and next_char == "*":
            comment_depth += 1
            i += 1
        elif comment_depth > 0 and not in_string and not in_access_string and current_char == "*" and next_char == ")":
            comment_depth -= 1
            i += 1
        elif comment_depth == 0 and (in_string or in_access_string) and current_char == '\\':
            # This block should always go before any that match "" or {}
            if next_char == "n":
                current_token += "\n"
            elif next_char == "t":
                current_token += "\v"
            elif next_char == '"':
                current_token += '"'
            elif next_char.isspace():
                idx: int = i + 1
                while code[idx].isspace():
                    idx += 1
                    i += 1
                i -= 1
            else:
                current_token += next_char
            i += 1
        elif comment_depth == 0 and not in_string and not in_access_string and current_char == '"':
            if len(current_token):
                current_line.append(Token(current_token, line_num, filename))
            current_token = ""
            current_token += current_char
            in_string = True
            last_string_open_line = line_num
        elif comment_depth == 0 and in_string and not in_access_string and current_char == '"':
            current_token += current_char
            in_string = False
            if len(current_token):
                current_line.append(Token(current_token, last_string_open_line, filename))
            current_token = ""
        elif comment_depth == 0 and not in_string and not in_access_string and current_char == '{':
            if len(current_token):
                current_line.append(Token(current_token, line_num, filename))
            current_token = ""
            current_line.append(Token("[", line_num, filename))
            current_token += '"'
            in_access_string = True
            last_string_open_line = line_num
        elif comment_depth == 0 and not in_string and in_access_string and current_char == '}':
            current_token += '"'
            in_access_string = False
            if len(current_token):
                current_line.append(Token(current_token, last_string_open_line, filename))
            current_token = ""
            current_line.append(Token("]", line_num, filename))
        elif comment_depth == 0 and not in_string and not in_access_string and current_char == ";":
            if len(current_token):
                current_line.append(Token(current_token, line_num, filename))
            current_token = ""
            if len(current_line):
                lines.append(current_line)
            current_line = []
        elif comment_depth == 0 and not in_string and not in_access_string and current_char + next_char in (">=", "<=", "<>", "//"):
            # Biglyphs
            if len(current_token):
                current_line.append(Token(current_token, line_num, filename))
            current_token = ""
            current_line.append(Token(current_char + next_char, line_num, filename))
            i += 1
        elif comment_depth == 0 and not in_string and not in_access_string and current_char in "(){}[]=<>!+-/&%^*:#,":
            # Single glyphs
            if len(current_token):
                current_line.append(Token(current_token, line_num, filename))
            current_token = ""
            current_line.append(Token(current_char, line_num, filename))
        elif comment_depth == 0 and not in_string and not in_access_string and current_char.isspace():
            if len(current_token):
                current_line.append(Token(current_token, line_num, filename))
            current_token = ""
        elif comment_depth == 0:
            current_token += current_char
        if current_char == "\n":
            line_num += 1
        i += 1
    # Check for consistency
    # I want to be able to leave comments open til the end of the file
    if in_string:
        tokenization_error("Open string, missing '\"'", last_string_open_line, filename)
    if in_access_string:
        tokenization_error("Open access string, missing '}'", last_string_open_line, filename)
    if len(current_line):
        tokenization_error("Missing ';'", current_line[-1].line, filename)
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
    padding_size: int = len(str(tokenized_lines[-1][0].line)) + 1  # This +1 is to account for the ':'.
    print("")
    print(f"=== {step} Result for File '{filename}' ===")
    for line in tokenized_lines:
        print(f"Line {pad_string(str(line[0].line) + ':', padding_size)} ", end="")
        tokens_str: List[str] = []
        for token in line:
            tokens_str.append(str(token).replace("\n", "␤"))
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
            elif token.value in (":", ","):
                token.type = LexType.DECORATION
            elif token.value == "#":
                token.type = LexType.TABLE
            elif is_integer(token.value):
                token.type = LexType.INTEGER
            elif is_float(token.value):
                token.type = LexType.FLOAT
            elif token.value in OPERATOR_PRESEDENCE:
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
    """for line in tokenized_lines:
        i = 2
        while i < len(line):
            token: Token = line[i]
            if token.type == LexType.INTEGER or token.type == LexType.FLOAT:
                previous_token: Token = line[i - 1]
                if previous_token.type in (LexType.OPERATOR, LexType.PAR_CLOSE, LexType.ACCESS_CLOSE) and previous_token.value == "-":
                    preprevious_token: Token = line[i - 2]
                    if preprevious_token.type == LexType.OPERATOR:
                        line.pop(i - 1)
                        token.value = f"-{token.value}"
                        continue
            i += 1"""
    return tokenized_lines


def compile_expression(expr_tokens: List[Token]) -> None:
    """Takes an expression and turns it into Nambly code.
    Terminators are not compiled by this function.
    This is the flojest part of the code and should probably
    be turned into an AST first.
    """
    depth_0_token_count: int = 0
    left_side_tokens: List[Token] = []
    right_side_tokens: List[Token] = []
    operator: Optional[Token] = None
    par_depth: int = 0
    access_depth: int = 0
    last_line: int = 0
    last_file: str = ""
    for token in expr_tokens:
        last_line = token.line
        last_file = token.file
        initial_depth: int = par_depth + access_depth
        if token.type == LexType.PAR_OPEN:
            par_depth += 1
        elif token.type == LexType.PAR_CLOSE:
            par_depth -= 1
            if par_depth < 0:
                expression_error(
                    "')' before '('",
                    token.line,
                    token.file
                )
        elif token.type == LexType.ACCESS_OPEN:
            access_depth += 1
        elif token.type == LexType.ACCESS_CLOSE:
            access_depth -= 1
            if access_depth < 0:
                expression_error(
                    "']' before '['",
                    token.line,
                    token.file
                )
        if par_depth + access_depth == 0 and token.type == LexType.OPERATOR and left_side_tokens:
            if operator is None:
                operator = token
            else:
                if OPERATOR_PRESEDENCE.index(operator.value) > OPERATOR_PRESEDENCE.index(token.value):
                    left_side_tokens.append(operator)
                    left_side_tokens += right_side_tokens
                    right_side_tokens = []
                    operator = token
                else:
                    right_side_tokens.append(token)
        else:
            if operator is None:
                left_side_tokens.append(token)
            else:
                right_side_tokens.append(token)
        if initial_depth == 0 or par_depth + access_depth == 0:
            depth_0_token_count += 1
    if par_depth > 0:
        expression_error(
            "Missing ')'",
            last_line,
            last_file
        )
    if access_depth > 0:
        expression_error(
            "Missing ']'",
            last_line,
            last_file
        )
    if False or True:
        # print(depth_0_token_count)
        print("-------------")
        print("OPERATOR:", operator)
        if left_side_tokens:
            print("LSIDE: ", end = "")
            for token in left_side_tokens:
                print(token, end="")
            print("")
        if right_side_tokens:
            print("RSIDE: ", end = "")
            for token in right_side_tokens:
                print(token, end="")
            print("")
    # Reevaluate expressions completely enclosed by parenthesis
    if depth_0_token_count == 2 and expr_tokens[0].type == LexType.PAR_OPEN and expr_tokens[-1].type == LexType.PAR_CLOSE:
        compile_expression(expr_tokens[1:-1])
    else:
        # In order to be a terminator, it must be:
        # - A value
        # - A minus and a value
        # - A function call (with n >= 0 accessess)
        # - A variable (with n >= 0 accessess)
        if operator is not None and not right_side_tokens:
            expression_error(
                f"Expecting expression after operator {operator.value}",
                operator.line,
                operator.file
            )
        elif operator is None or not left_side_tokens:
            compile_terminator(left_side_tokens)
            pass
        else:
            compile_expression(left_side_tokens)
            compile_expression(right_side_tokens)
    if operator:
        if operator.value == "*":
            print("MULT")
        elif operator.value == "^":
            print("POWR")
        elif operator.value == "/":
            print("FDIV")
        elif operator.value == "//":
            print("IDIV")
        elif operator.value == "-":
            print("SUBT")
        elif operator.value == "+":
            print("ADDV")
        elif operator.value == "&":
            print("JOIN")
        elif operator.value == "%":
            print("MODL")
        elif operator.value == "=":
            print("ISEQ")
        elif operator.value == "<>":
            print("ISNE")
        elif operator.value == "<":
            print("ISLT")
        elif operator.value == ">":
            print("ISGT")
        elif operator.value == "<=":
            print("ISLE")
        elif operator.value == ">=":
            print("ISGE")
        else:
            expression_error(
                f"The operator {operator.value} cannot be used as an infix operator.",
                operator.line,
                operator.file
            )



def compile_terminator(expr_tokens: List[Token]) -> None:
    """Generates Nambly for an expression terminator.
    This is the second flojest part of the code and should
    probably be turned into an AST first.
    """
    add_minus_code: bool = False
    add_negation_code: bool = False
    access_depth: int = 0
    access_tokens: List[Token] = []
    terminator_type: LexType = LexType.UNKNOWN
    while expr_tokens:
        token = expr_tokens.pop(0)
        if access_depth > 0:
            if token.type == LexType.ACCESS_OPEN:
                access_depth += 1
                access_tokens.append(token)
            elif token.type == LexType.ACCESS_CLOSE:
                access_depth -= 1
                if access_depth == 0:
                    compile_expression(access_tokens)
                    print("PGET")
                    if expr_tokens:
                        if expr_tokens[0].type not in (LexType.ACCESS_OPEN, LexType.PAR_OPEN):
                            expression_error(
                                f"Unexpected expression element: {expr_tokens[0].value}",
                                token.line,
                                token.file
                            )
                else:
                    access_tokens.append(token)
            else:
                access_tokens.append(token)
        else:
            next_token = None
            if len(expr_tokens) > 0:
                next_token = expr_tokens[0]
            if token.type == LexType.OPERATOR and token.value == "-":
                if next_token is None:
                    expression_error(
                        "Missing value after '-'",
                        token.line,
                        token.file
                    )
                if next_token.type in [LexType.INTEGER, LexType.FLOAT]:
                    print(f"PUSH -{next_token.value}")
                    terminator_type = next_token.type
                else:
                    add_minus_code = True
            elif token.type == LexType.OPERATOR and token.value == "!":
                if next_token is None:
                    expression_error(
                        "Missing value after '!'",
                        token.line,
                        token.file
                    )
                add_negation_code = True
            elif token.type == LexType.VARIABLE:
                print(f'VGET "{token.value[1:]}"')
                terminator_type = token.type
            elif token.type == LexType.ACCESS_OPEN:
                if terminator_type == LexType.UNKNOWN:
                    expression_error(
                        "Found an index access without a variable or a function.",
                        token.line,
                        token.file
                    )
                if terminator_type not in [LexType.VARIABLE, LexType.WORD]:
                    expression_error(
                        "Attempting to index access something that's not a variable nor a function.",
                        token.line,
                        token.file
                    )
                access_tokens = []
                access_depth += 1
            elif token.type in [LexType.INTEGER, LexType.FLOAT]:
                print(f"PUSH {token.value}")
                terminator_type = token.type
            elif token.type == LexType.WORD:
                terminator_type = token.type
                if next_token is None or next_token.type != LexType.PAR_OPEN:
                    expression_error(
                        "Expecting argument list after function call.",
                        token.line,
                        token.file
                    )
                print(f"TODO COMPILAR FUNCTION CALL A {token.value}")
            elif token.type == LexType.PAR_OPEN:
                if terminator_type not in [LexType.VARIABLE, LexType.WORD]:
                    expression_error(
                        "Calling non-functional value.",
                        token.line,
                        token.file
                    )
                print("TABL")
                print('VSET "_"')
                arguments: List[Token] = []
                open_pars: int = 1
                arg_count: int = 0
                prev_token: Optional[Token] = None
                while expr_tokens:
                    token: Token = expr_tokens[0]
                    if token.type == LexType.PAR_OPEN:
                        open_pars += 1
                        if open_pars > 1:
                            arguments.append(token)
                    elif token.type == LexType.PAR_CLOSE:
                        if open_pars > 1:
                            arguments.append(token)
                        open_pars -= 1
                        if open_pars == 0:
                            if not arguments and (prev_token is not None and prev_token.type == LexType.DECORATION and prev_token.value == ","):
                                expression_error(
                                    f"Empty argument for function call",
                                    token.line,
                                    token.file
                                )
                            else:
                                if arguments:
                                    compile_expression(arguments)
                                arguments = []
                                print("CALL")
                                expr_tokens.pop(0)
                                break
                    elif open_pars > 1:
                        arguments.append(token)
                    elif token.type == LexType.DECORATION:
                        if token.value != ",":
                            expression_error(
                                f"Unexpected string '{token.value}'",
                                token.line,
                                token.file
                            )
                        elif not arguments:
                            expression_error(
                                f"Empty argument for function call",
                                token.line,
                                token.file
                            )
                        else:
                            print('VGET "_"')
                            print(f"PUSH {arg_count}")
                            compile_expression(arguments)
                            print(f"PSET")
                            arguments = []
                            arg_count += 1
                    else:
                        arguments.append(token)
                    prev_token = token
                    expr_tokens.pop(0)
                if expr_tokens:
                    if expr_tokens[0].type not in (LexType.ACCESS_OPEN, LexType.PAR_OPEN):
                        expression_error(
                            f"Unexpected expression element: {expr_tokens[0].value}",
                            token.line,
                            token.file
                        )
            else:
                expression_error(
                    f"Unexpected operator: '{token.value}'",
                    token.line,
                    token.file
                )
    if add_minus_code:
        print("PUSH -1")
        print("MULT")
    if add_negation_code:
        print("LNOT")



if __name__ == "__main__":
    code: str = ""
    filename: str = "test.fs"
    with open(filename) as f:
        code = f.read()
    tokenized_lines: List[List[Token]] = tokenize_source(code, filename)
    # print_tokens(tokenized_lines, filename, "Tokenization")
    lex_tokens(tokenized_lines)
    print_tokens(tokenized_lines, filename, "Lexing")
    for line in tokenized_lines:
        compile_expression(line)


# Next Steps:
"""
El parser debería ser relativamente simple por cómo diseñé el lenguaje
Todas las lineas empiezan con un comando y tienen cosas en posiciones fijas,
con lo único variable siendo todo lo que sean expresiones. Como las variables
son $... y tengo palabras reservadas, puedo encontrar fácilmente dónde terminan
y empiezan las expresiones (incluyendo los llamados a funciones).
Con eso construyo una lista de secuencias comando-expresión.
Y después una vez que esté eso puedo iterar por eso generando Nambly.
"""