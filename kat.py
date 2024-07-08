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

# TODO: Check that variables have been declared
# TODO: Check that functions have been declared
# TODO: Else
# TODO: Else if
# TODO: Local Variables
# TODO: Break
# TODO: Continue
# TODO: Function Declarations
# TODO: Function Calls

from __future__ import annotations
from typing import Dict, List, Set, Tuple, Any, Optional
from enum import Enum, auto
from narivm import nari_run


OPERATOR_PRESEDENCE = ("*", "^", "/", "%", "//", "+", "&", "-", "!", "<", ">", "<=", ">=", "<>", "=")


class CompilerState:
    def __init__(self):
        self.block_count: int = 0
        self.__block_end_code_stack: List[Tuple[str, Token]] = []

    def add_block_end_code(self, code: str, reference_token: Token):
        """Sets the next block end code to be used when an 'ok;' is found.
        The reference token is there so that if that 'ok;' is missing, we
        can reference which block is missing it.
        """
        self.__block_end_code_stack.append((code, reference_token))
    
    def get_block_end_code(self, caller_command: Token, is_continue: bool = False) -> str:
        """Gets the next block end code to be used when an 'ok;' is found.
        """
        if not self.__block_end_code_stack:
            parse_error("Unexpected 'ok' command.", caller_command.line, caller_command.file)
        if is_continue:
            reference_token = self.__block_end_code_stack[-1][1]
            if reference_token.value != "while":
                parse_error("Continue used outside of loop.", )
        return self.__block_end_code_stack.pop()[0]
    
    def check_for_errors(self):
        """Checks that the state was left in a valid state after compiling.
        """
        if self.__block_end_code_stack:
            parse_error("Missing 'ok' command.", self.__block_end_code_stack[0][1].line, self.__block_end_code_stack[0][1].file)


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
    
    def get_nambly_string(self) -> str:
        """Returns a nambly formatted string to be able to push string values to the nvm.
        """
        if self.type == LexType.TABLE:
            return '"TABLE"'
        else:
            return str(self.value).replace('"', '\\"').replace('\n', '\\n')
        
    def get_var_name(self) -> str:
        """"Returns a nambly formated variable name.
        """
        if self.type == LexType.VARIABLE:
            return self.value[1:]
        else:
            parse_error(f"{self} is not a variable.", self.line, self.file)


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


def parse_error(message: str, line: int, filename: str):
    """Prints an expression error and exits.
    """
    error_title = "Katayln Parse Error"
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


def compile_expression(expr_tokens: List[Token], discard_return_value: bool = False) -> str:
    """Takes an expression and turns it into Nambly code.
    Terminators are not compiled by this function.
    This is the flojest part of the code and should probably
    be turned into an AST first.
    """
    compiled_code: str = ""
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
                if OPERATOR_PRESEDENCE.index(operator.value) < OPERATOR_PRESEDENCE.index(token.value):
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
    if False:
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
        compiled_code += "\n" + compile_expression(expr_tokens[1:-1])
    else:
        if operator is not None and not right_side_tokens:
            expression_error(
                f"Expecting expression after operator {operator.value}",
                operator.line,
                operator.file
            )
        elif operator is None or not left_side_tokens:
            compiled_code += "\n" + compile_terminator(left_side_tokens, discard_return_value)
        else:
            compiled_code += "\n" + compile_expression(left_side_tokens)
            compiled_code += "\n" + compile_expression(right_side_tokens)
    if operator:
        if operator.value == "*":
            compiled_code += "\nMULT"
        elif operator.value == "^":
            compiled_code += "\nPOWR"
        elif operator.value == "/":
            compiled_code += "\nFDIV"
        elif operator.value == "//":
            compiled_code += "\nIDIV"
        elif operator.value == "-":
            compiled_code += "\nSUBT"
        elif operator.value == "+":
            compiled_code += "\nADDV"
        elif operator.value == "&":
            compiled_code += "\nJOIN"
        elif operator.value == "%":
            compiled_code += "\nMODL"
        elif operator.value == "=":
            compiled_code += "\nISEQ"
        elif operator.value == "<>":
            compiled_code += "\nISNE"
        elif operator.value == "<":
            compiled_code += "\nISLT"
        elif operator.value == ">":
            compiled_code += "\nISGT"
        elif operator.value == "<=":
            compiled_code += "\nISLE"
        elif operator.value == ">=":
            compiled_code += "\nISGE"
        else:
            expression_error(
                f"The operator {operator.value} cannot be used as an infix operator.",
                operator.line,
                operator.file
            )
    return compiled_code



def compile_terminator(expr_tokens: List[Token], discard_return_value: bool = False) -> str:
    """Generates Nambly for an expression terminator.
    This is the second flojest part of the code and should
    probably be turned into an AST first.
    """
    compiled_code: str = ""
    add_minus_code: bool = False
    add_negation_code: bool = False
    access_depth: int = 0
    access_tokens: List[Token] = []
    terminator_type: LexType = LexType.UNKNOWN
    function_call_token: Optional[Token] = None
    while expr_tokens:
        token = expr_tokens.pop(0)
        if access_depth > 0:
            if token.type == LexType.ACCESS_OPEN:
                access_depth += 1
                access_tokens.append(token)
            elif token.type == LexType.ACCESS_CLOSE:
                access_depth -= 1
                if access_depth == 0:
                    compiled_code += "\n" + compile_expression(access_tokens)
                    compiled_code += "\nPGET"
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
                    if terminator_type != LexType.UNKNOWN:
                        expression_error(f"Unexpected token '{token.value}'.", token.line, token.file)
                    compiled_code += f"\nPUSH -{next_token.value}"
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
                if terminator_type != LexType.UNKNOWN:
                    expression_error(f"Unexpected token '{token.value}'.", token.line, token.file)
                compiled_code += f'\nVGET "{token.get_var_name()}"'
                terminator_type = token.type
            elif token.type == LexType.STRING:
                if terminator_type != LexType.UNKNOWN:
                    expression_error(f"Unexpected token '{token.value}'.", token.line, token.file)
                compiled_code += f'\nPUSH "{token.get_nambly_string()}"'
                terminator_type = token.type
            elif token.type == LexType.ACCESS_OPEN:
                if terminator_type == LexType.UNKNOWN:
                    expression_error(
                        "Found a table access without a variable or a function.",
                        token.line,
                        token.file
                    )
                if terminator_type not in [LexType.VARIABLE, LexType.WORD]:
                    expression_error(
                        "Attempting to access something that cannot be a table.",
                        token.line,
                        token.file
                    )
                access_tokens = []
                access_depth += 1
            elif token.type in [LexType.INTEGER, LexType.FLOAT]:
                if terminator_type != LexType.UNKNOWN:
                    expression_error(f"Unexpected token '{token.value}'.", token.line, token.file)
                compiled_code += f"\nPUSH {token.value}"
                terminator_type = token.type
            elif token.type == LexType.WORD:
                if terminator_type != LexType.UNKNOWN:
                    expression_error(f"Unexpected token '{token.value}'.", token.line, token.file)
                terminator_type = token.type
                function_call_token = token
                if next_token is None or next_token.type != LexType.PAR_OPEN:
                    expression_error(
                        "Expecting argument list after function call.",
                        token.line,
                        token.file
                    )
            elif token.type == LexType.PAR_OPEN:
                if terminator_type not in [LexType.VARIABLE, LexType.WORD]:
                    expression_error(
                        "Calling non-functional value.",
                        token.line,
                        token.file
                    )
                arguments_list: List[List[Token]] = []
                arguments: List[Token] = []
                open_pars: int = 1
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
                                    arguments_list.append(arguments)
                                arguments = []
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
                            if arguments:
                                arguments_list.append(arguments)
                            arguments = []
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
                # Call the function
                if terminator_type == LexType.WORD:
                    compiled_code += "\n" + compile_function_call(function_call_token, arguments_list, discard_return_value)
                # TODO $a(2, 3, 4)
            else:
                expression_error(
                    f"Unexpected operator: '{token.value}'",
                    token.line,
                    token.file
                )
    if add_minus_code:
        compiled_code += "\nPUSH -1"
        compiled_code += "\nMULT"
    if add_negation_code:
        compiled_code += "\nLNOT"
    return compiled_code


def stylize_namby(code: str) -> str:
    """Stilizes the nambly code by removing unnecessary breaks.
    """
    new_lines = ""
    for line in code.split("\n"):
        if line:
            new_lines += line + "\n"
    return new_lines


def compile_lines(tokenized_lines: List[List[Token]]) -> str:
    """Takes a list of list of lexed tokens and compiles them into Nambly code.
    """
    compiled_code: str = ""
    declared_variables = set()
    for line in tokenized_lines:
        # Check first token in the line, this is our command
        command = line[0]
        args = []
        if len(line) > 1:
            args = line[1:]
        if command.type == LexType.WORD:
            # --- in command ---
            if command.value == "in":
                compiled_code += "\n" + parse_command_in(command, args)
            elif command.value == "while":
                compiled_code += "\n" + parse_command_while(command, args)
            elif command.value == "if":
                compiled_code += "\n" + parse_command_if(command, args)
            elif command.value == "ok":
                compiled_code += "\n" + parse_command_ok(command, args)
            elif command.value == "continue":
                compiled_code += "\n" + parse_command_continue(command, args)
            else:
                # Commands that are "function-like" such as print
                compiled_code += "\n" + compile_expression(line, True)
    return compiled_code


def compile_function_call(command: Token, args_list: List[List[Token]], discard_return_value: bool = False):
    if command.value == "print":
        return parse_command_print(command, args_list, discard_return_value)
    elif command.value == "printc":
        return parse_command_printc(command, args_list, discard_return_value)
    elif command.value == "accept":
        return parse_command_accept(command, args_list, discard_return_value)
    elif command.value == "is":
        return parse_command_is(command, args_list, discard_return_value)
    elif command.value == "exit":
        return parse_command_exit(command, args_list, discard_return_value)
    else:
        # TODO: Ver si la función existe y llamarla y sino tirar:
        parse_error(f"Undeclared function '{command.value}'", command.line, command.file)
    


def parse_command_in(command_token: Token, args: List[Token]) -> str:
    access_compiled_code: str = ""
    set_compiled_code: str = ""
    value_compiled_code: str = ""
    left_side: List[Token] = []
    right_side: List[Token] = []
    found_colon: bool = False
    for token in args:
        if token.type == LexType.DECORATION and token.value == ":":
            found_colon = True
        elif not found_colon:
            left_side.append(token)
        else:
            right_side.append(token)

    # Compile righthand side
    if not right_side:
        parse_error("Empty right side for 'in' statement.", command_token.line, command_token.file)
    else:
        if right_side[0].type == LexType.TABLE:
            if len(right_side) > 1:
                parse_error(
                    f"Unexpected token: {right_side[1]}",
                    right_side[1].line,
                    right_side[1].file
                )
            else:
                value_compiled_code += "\nTABL"
        else:                
            value_compiled_code += "\n" + compile_expression(right_side)

    # Compile lefthand side
    if not left_side:
        parse_error("Empty left side for 'in' statement.", command_token.line, command_token.file)
    else:
        var = left_side[0]
        has_accesses: bool = False
        for token in left_side:
            if token.type == LexType.ACCESS_OPEN:
                has_accesses = True
                break
        if var.type == LexType.WORD:
            parse_error("Function calls not yet supported in the left side of an assignment.", var.line, var.file)
            # TODO: support them!
        elif var.type != LexType.VARIABLE:
            parse_error(f"Variable expected ({var} found).", var.line, var.file)
        else:
            if not has_accesses:
                if len(left_side) > 1:
                    parse_error(
                        f"Unexpected token: '{left_side[1]}'",
                        left_side[1].line,
                        left_side[1].file
                    )
                else:
                    set_compiled_code += f'\nVSET "{var.get_var_name()}"'
            else:
                access_compiled_code += f'\nVGET "{var.get_var_name()}"'
                access_tokens: List[Token] = []
                access_depth: int = 0
                for token in left_side[1:]:
                    if token.type == LexType.ACCESS_OPEN:
                        access_depth += 1
                    if token.type == LexType.ACCESS_CLOSE:
                        access_depth -= 1
                    access_tokens.append(token)
                    if token.type == LexType.ACCESS_CLOSE and access_depth == 0:
                        if access_tokens[0].type != LexType.ACCESS_OPEN or len(access_tokens) == 2:
                            parse_error(
                                "Malformed table access.",
                                access_tokens[0].line,
                                access_tokens[0].file
                            )
                        else:
                            access_compiled_code += "\n" + compile_expression(access_tokens[1:-1])
                            access_tokens = []
                if access_depth > 0:
                    parse_error(
                        "Missing ']'.",
                        access_tokens[0].line,
                        access_tokens[0].file
                    )
                if access_tokens:
                    parse_error(
                        f"Unexpected tokens after table access.",
                        access_tokens[0].line,
                        access_tokens[0].file
                    )
                set_compiled_code += '\nPSET'
    return access_compiled_code + value_compiled_code + set_compiled_code


def parse_command_print(command_token: Token, args_list: List[List[Token]], discard_return_value: bool = False) -> str:
    compiled_code: str = ""
    if discard_return_value:
        for args in args_list:
            compiled_code += "\n" + compile_expression(args)
            compiled_code += "\nDISP"
        compiled_code += '\nPUSH "\\n"'
        compiled_code += "\nDISP"
        return compiled_code
    else:
        compiled_code += '\nPUSH ""'
        for args in args_list:
            compiled_code += "\n" + compile_expression(args)
            compiled_code += "\nDUPL"
            compiled_code += "\nVSET \"$swap\""  # Only system vars start with $ in nambly
            compiled_code += "\nDISP"
            compiled_code += "\nVGET \"$swap\""  # Only system vars start with $ in nambly
            compiled_code += "\nJOIN"
        compiled_code += '\nPUSH "\\n"'
        compiled_code += "\nDISP"
        return compiled_code
    

def parse_command_accept(command_token: Token, args_list: List[List[Token]], discard_return_value: bool = False) -> str:
    compiled_code: str = ""
    if args_list:
        compiled_code += "\n" + parse_command_printc(command_token, args_list, True)
    compiled_code += "\nACCP"
    if discard_return_value:
        compiled_code += "\nPOPV" 
    return compiled_code


def parse_command_printc(command_token: Token, args_list: List[List[Token]], discard_return_value: bool = False) -> str:
    compiled_code: str = ""
    if discard_return_value:
        for args in args_list:
            compiled_code += "\n" + compile_expression(args)
            compiled_code += "\nDISP"
        return compiled_code
    else:
        compiled_code += '\nPUSH ""'
        for args in args_list:
            compiled_code += "\n" + compile_expression(args)
            compiled_code += "\nDUPL"
            compiled_code += "\nVSET \"$swap\""  # Only system vars start with $ in nambly
            compiled_code += "\nDISP"
            compiled_code += "\nVGET \"$swap\""  # Only system vars start with $ in nambly
            compiled_code += "\nJOIN"
        return compiled_code
    

def parse_command_is(command_token: Token, args_list: List[List[Token]], discard_return_value: bool = False) -> str:
    compiled_code: str = ""
    if len(args_list) != 2:
        parse_error("Wrong number of arguments for function is (expected 2).", command_token.line, command_token.file)
    compiled_code += "\n" + compile_expression(args_list[0])
    compiled_code += "\n" + compile_expression(args_list[1])
    compiled_code += "\nPIST"
    if discard_return_value:
        compiled_code += "\nPOPV"
        return compiled_code
    return compiled_code


def parse_command_exit(command_token: Token, args_list: List[List[Token]], discard_return_value: bool = False) -> str:
    compiled_code: str = ""
    if len(args_list) != 1:
        parse_error("Wrong number of arguments for function exit (expected 1).", command_token.line, command_token.file)
    compiled_code += "\n" + compile_expression(args_list[0])
    compiled_code += "\nEXIT"
    return compiled_code


def parse_command_while(command_token: Token, args: List[Token]) -> str:
    global global_compiler_state
    block_number: int = global_compiler_state.block_count
    global_compiler_state.block_count += 1
    start_tag: str = f"LOOP_{block_number}_START"
    end_tag: str = f"LOOP_{block_number}_END"
    compiled_code: str = ""
    block_end_code: str = ""
    compiled_code += f"\n@{start_tag}"
    compiled_code += "\n" + compile_expression(args)
    compiled_code += f"\nJPIF {end_tag}"
    # Push end code to state for it to be used on next ok;
    block_end_code += f"\nJUMP {start_tag}"
    block_end_code += f"\n@{end_tag}"
    global_compiler_state.add_block_end_code(block_end_code, command_token)
    return compiled_code


def parse_command_if(command_token: Token, args: List[Token]) -> str:
    global global_compiler_state
    block_number: int = global_compiler_state.block_count
    global_compiler_state.block_count += 1
    start_tag: str = f"IF_{block_number}_START"
    end_tag: str = f"IF_{block_number}_END"
    compiled_code: str = ""
    block_end_code: str = ""
    compiled_code += f"\n@{start_tag}"
    compiled_code += "\n" + compile_expression(args)
    compiled_code += f"\nJPIF {end_tag}"
    # Push end code to state for it to be used on next ok;
    block_end_code += f"\n@{end_tag}"
    global_compiler_state.add_block_end_code(block_end_code, command_token)
    return compiled_code


def parse_command_ok(command_token: Token, args: List[Token]) -> str:
    global global_compiler_state
    if args:
        parse_error("Unexpected arguments for command 'ok'.", command_token.line, command_token.file)
    return global_compiler_state.get_block_end_code(command_token)

def parse_command_continue(command_token: Token, args: List[Token]) -> str:
    global global_compiler_state
    if args:
        parse_error("Unexpected arguments for command 'continue'.", command_token.line, command_token.file)
    return global_compiler_state.get_block_end_code(command_token, True)


global_compiler_state = CompilerState()


if __name__ == "__main__":
    code: str = ""
    filename: str = "test.fs"
    with open(filename) as f:
        code = f.read()
    tokenized_lines: List[List[Token]] = tokenize_source(code, filename)
    if tokenized_lines:
        # print_tokens(tokenized_lines, filename, "Tokenization")
        lex_tokens(tokenized_lines)
        print_tokens(tokenized_lines, filename, "Lexing")
        nambly = compile_lines(tokenized_lines)
        global_compiler_state.check_for_errors()
        nambly = stylize_namby(nambly)
        print(nambly)
        nari_run(nambly)

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