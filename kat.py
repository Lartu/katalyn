#!/usr/bin/env python3
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
# For 0.1.0:
# TODO: Lógica de cortocircuito para 'and' and 'or' (Es saltar al final del right side si and es false u or es true en el left side)
# TODO: for key in table
# Nice to have:
# TODO: Operador de composición fun1() => fun2() eq a fun2(fun1())

from __future__ import annotations
import sys
from typing import Dict, List, Tuple, Optional, Set
from enum import Enum, auto
from narivm import nari_run
from sys import exit
import os

VERSION = "0.0.1"
OPERATOR_PRESEDENCE = ("*", "^", "/", "%", "//", "+", "&", "-", "::", "!", "<", ">", "<=", ">=", "<>", "!=", "=", "||", "&&")
LOOP_TAGS = ("while", "until")
NON_DEF_BLOCK_TAGS = ("if", "unless", "while", "until")
ARGS_VAR = "$_"
RESULT_VAR = "$_r"
FLAGS_VAR = "$_args"
EXEC_STDOUT_VAR = "$_stdout"
EXEC_STDERR_VAR = "$_stderr"
EXEC_EXITCODE_VAR = "$_exitcode"
STDLIB_LOCATION = "/Users/lartu/No Sync/Katalyn"


class ScopeSearchType(Enum):
    LOCAL_AND_GLOBAL = auto()
    ONLY_GLOBAL = auto()
    ONLY_LOCAL = auto()


class CompilerState:
    def __init__(self):
        self.block_count: int = 0
        self.__block_end_code_stack: List[Tuple[str, Token]] = []
        self.__declared_variables: List[Dict[str, str]] = []
        self.__open_loop_tags: List[Tuple[str, str]] = []
        self.__function_to_labels: Dict[str, Tuple[str, str]] = {}
        self.__expected_functions: Dict[str, Token, Tuple[str, str]] = {}
        self.add_scope()

    def add_open_loop(self, start_label: str, end_label: str) -> None:
        self.__open_loop_tags.append((start_label, end_label))

    def close_open_loop(self) -> None:
        """Closes an open loop only if the current scope is that of a loop.
        """
        if self.__block_end_code_stack:
            if self.__block_end_code_stack[-1][1].value in LOOP_TAGS:
                self.__open_loop_tags.pop()

    def get_open_loop_tags(self, depth: int = 0) -> Optional[Tuple[str, str]]:
        """Returns the start and end tags of a loop if a loops is open, otherwise None.
        """
        if self.__open_loop_tags:
            index: int = 1 + depth
            if index > len(self.__open_loop_tags):
                index = 0
            return self.__open_loop_tags[-index]
        else:
            return None

    def add_scope(self) -> None:
        self.__declared_variables.append({})

    def del_scope(self) -> str:
        """Deletes a variable scope only if the current scope is that of a function
        """
        compiled_code: str = ""
        if self.__block_end_code_stack[-1][1].value in self.__function_to_labels:
            self.__declared_variables.pop()
        return compiled_code

    def get_var_identifier(self, var: Token, fail_if_not_found: bool, scope_type: ScopeSearchType = ScopeSearchType.LOCAL_AND_GLOBAL, unsafe: bool = False) -> Optional[str]:
        """Gets the scope id (the id for the variable in the current context) of
        the variable with name var_name. If the variable hasn't been declared yet,
        returns None. If force_global, the variable is only looked for in the global
        scope. If unsafe, its identifier is returned and doesn't fail if not found.
        """
        scopes = []
        if scope_type in [ScopeSearchType.LOCAL_AND_GLOBAL, ScopeSearchType.ONLY_LOCAL]:
            scopes.append(len(self.__declared_variables) - 1)
        if scope_type in [ScopeSearchType.LOCAL_AND_GLOBAL, ScopeSearchType.ONLY_GLOBAL]:
            scopes.append(0)
        var_name: str = var.get_var_name()
        for scope in scopes:
            if var_name in self.__declared_variables[scope]:
                return self.__declared_variables[scope][var_name]
        if unsafe:
            return var_name
        if fail_if_not_found:
            if not self.get_in_function_declaration():
                expression_error(f"Variable {var.value} read before assignment.", var.line, var.file)
            else:
                expression_error(
                    f"Variable {var.value} read before assignment. If you are sure this variable will "\
                    f"be declared by the point this function is called, use unsafe({var.value}) instead "\
                    f"of just {var.value}. Bear in mind that if the variable hasn't been initialized by this "\
                    f"point, its value will be nil and your program will likely crash."
                    , var.line, var.file)
        return None
    
    def declare_variable(self, var: Token, global_var: bool = False) -> str:
        """Adds a new variable, if it didn't exist, to the context. Then,
        existed or not, returns its scope_id (the id for the variable in
        the current context). If the variable is declared as global_var, the
        variable is declared only if it didn't exist in the global context.
        """
        var_name = var.get_var_name()
        scope_search_type: ScopeSearchType = ScopeSearchType.ONLY_LOCAL
        if global_var:
            scope_search_type = ScopeSearchType.ONLY_GLOBAL
        if self.get_var_identifier(var, False, scope_search_type) is not None:
            return self.get_var_identifier(var, True, scope_search_type)
        self.__declared_variables[-1][var_name] = var_name
        return var_name
    
    def unset_variable(self, var: Token, global_var: bool = False):
        i = len(self.__declared_variables) - 1
        if global_var:
            i = 0
        var_name: str = var.get_var_name()
        while i >= 0:
            if var_name in self.__declared_variables[i]:
                del self.__declared_variables[i][var_name]
            i -= 1

    def add_block_end_code(self, code: str, reference_token: Token, group_id: int = -1):
        """Sets the next block end code to be used when an 'ok;' is found.
        The reference token is there so that if that 'ok;' is missing, we
        can reference which block is missing it.
        """
        self.__block_end_code_stack.append((code, reference_token, group_id))
    
    def get_block_end_data(self, caller_command: Token) -> Tuple[str, Token, int]:
        """Gets the next block end code to be used when an 'ok;' is found.
        """
        if not self.__block_end_code_stack:
            parse_error("Unexpected 'ok' command.", caller_command.line, caller_command.file)
        return self.__block_end_code_stack.pop()
    
    def add_function(self, caller_command: Token, start_label: str, end_label: str, post_label: str):
        """Adds a function and links it to its label
        """
        function_name: str = caller_command.value
        if function_name in self.__function_to_labels:
            parse_error(f"Duplicate function declaration for '{function_name}'.", caller_command.line, caller_command.file)
        self.__function_to_labels[function_name] = (start_label, end_label, post_label)

    def get_function_label(self, caller_command: Token) -> Tuple[str, str]:
        function_name: str = caller_command.value
        if function_name not in self.__function_to_labels:
            if function_name in self.__expected_functions:
                return self.__expected_functions[caller_command.value][1]
            block_number: int = self.block_count
            self.block_count += 1
            start_label: str = f"FUN_{block_number}_START"
            end_label: str = f"FUN_{block_number}_END"
            post_label: str = f"FUN_{block_number}_POST"
            self.__expected_functions[caller_command.value] = [caller_command, (start_label, end_label, post_label)]
            return (start_label, end_label, post_label)
        return self.__function_to_labels[function_name]
    
    def check_for_errors(self):
        """Checks that the state was left in a valid state after compiling.
        """
        if self.__block_end_code_stack:
            parse_error("Missing 'ok' command.", self.__block_end_code_stack[0][1].line, self.__block_end_code_stack[0][1].file)
        for function in self.__expected_functions:
            if function not in self.__function_to_labels:
                parse_error(
                    f"Call to nonexistent function '{function}'.",
                    self.__expected_functions[function][0].line,
                    self.__expected_functions[function][0].file,
                )

    def get_in_function_declaration(self) -> bool:
        """Returns if the current code is inside a function declaration.
        """
        for tuple_code_token in self.__block_end_code_stack:
            if tuple_code_token[1] not in NON_DEF_BLOCK_TAGS:
                return True
        return False

    def get_in_if_chain(self) -> bool:
        """Returns if the current code is inside an if chain. (But not else!)
        """
        token = self.__block_end_code_stack[-1][1]
        return token.type == LexType.WORD and token.value in ("if", "elif")

    def get_group_id(self) -> int:
        """Returns the current group id for the current block.
        """
        return self.__block_end_code_stack[-1][2]


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
        """"Returns a nambly formated variable name (without the $)
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
        total_length = 0
        tokens = line.split()
        for token in tokens:
            token_length = len(token)
            if total_length > 0:
                token_length += 1  # To account for spaces
            if total_length + token_length > 80:
                print(f"")
                total_length = 2
                token_length = len(token)
                print(f"  ", end="")
            total_length += token_length
            print(f"{token} ", end="")
        print("")
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
    last_string_open_line = 1  # For error reporting purposes
    line_num = 1
    lines: List[List[Token]] = []
    current_line: List[Token] = []
    code += " "  # To simplify tokenization
    current_token = ""
    in_string = False
    in_access_string = False
    comment_depth: int = 0
    in_inline_comment: bool = False
    # Katalyn supports nested comments, but doesn't make any distinction between (* ... *) inside strings
    # once that string is inside an already open comment (such as (* "(* *)" *))
    i = 0
    while i < len(code) - 1:
        current_char = code[i]
        next_char = code[i + 1]
        if not in_inline_comment and not in_string and not in_access_string and comment_depth == 0 and current_char == "#":
            # Inline comment
            in_inline_comment = True
        elif not in_string and not in_access_string and current_char == "(" and next_char == "*":
            # (*...*) comments can be opened and closed from within an inline comment
            comment_depth += 1
            i += 1
        elif comment_depth > 0 and not in_string and not in_access_string and current_char == "*" and next_char == ")":
            # (*...*) comments can be opened and closed from within an inline comment
            comment_depth -= 1
            i += 1
        elif not in_inline_comment and comment_depth == 0 and (in_string or in_access_string) and current_char == '\\':
            # This block should always go before any that match "" or {}
            if next_char == "n":
                current_token += "\n"
            elif next_char == "t":
                current_token += "\t"
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
        elif not in_inline_comment and comment_depth == 0 and not in_string and not in_access_string and current_char == '"':
            if len(current_token):
                current_line.append(Token(current_token, line_num, filename))
            current_token = ""
            current_token += current_char
            in_string = True
            last_string_open_line = line_num
        elif not in_inline_comment and comment_depth == 0 and in_string and not in_access_string and current_char == '"':
            current_token += current_char
            in_string = False
            if len(current_token):
                current_line.append(Token(current_token, last_string_open_line, filename))
            current_token = ""
        elif not in_inline_comment and comment_depth == 0 and not in_string and not in_access_string and current_char == '{':
            if len(current_token):
                current_line.append(Token(current_token, line_num, filename))
            current_token = ""
            current_line.append(Token("[", line_num, filename))
            current_token += '"'
            in_access_string = True
            last_string_open_line = line_num
        elif not in_inline_comment and comment_depth == 0 and not in_string and in_access_string and current_char == '}':
            current_token += '"'
            in_access_string = False
            if len(current_token):
                current_line.append(Token(current_token, last_string_open_line, filename))
            current_token = ""
            current_line.append(Token("]", line_num, filename))
        elif not in_inline_comment and comment_depth == 0 and not in_string and not in_access_string and current_char == ";":
            if len(current_token):
                current_line.append(Token(current_token, line_num, filename))
            current_token = ""
            if len(current_line):
                lines.append(current_line)
            current_line = []
        elif not in_inline_comment and comment_depth == 0 and not in_string and not in_access_string and current_char + next_char in (">=", "<=", "::", "<>", "!=", "//", "&&", "||"):
            # Biglyphs
            if len(current_token):
                current_line.append(Token(current_token, line_num, filename))
            current_token = ""
            current_line.append(Token(current_char + next_char, line_num, filename))
            i += 1
        elif not in_inline_comment and comment_depth == 0 and not in_string and not in_access_string and current_char in "(){}[]=<>!+-/&%^*:#,":
            # Single glyphs
            if len(current_token):
                current_line.append(Token(current_token, line_num, filename))
            current_token = ""
            current_line.append(Token(current_char, line_num, filename))
        elif not in_inline_comment and comment_depth == 0 and not in_string and not in_access_string and current_char.isspace():
            if len(current_token):
                current_line.append(Token(current_token, line_num, filename))
            current_token = ""
        elif not in_inline_comment and comment_depth == 0:
            current_token += current_char
        if current_char == "\n":
            line_num += 1
            in_inline_comment = False
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
            elif token.value == "table":
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
    return tokenized_lines


def compile_expression(expr_tokens: List[Token], discard_return_value: bool = False, unsafe: bool = False) -> str:
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
        if par_depth + access_depth == 0 and token.type == LexType.OPERATOR:
            if operator is None:
                operator = token
            else:
                if OPERATOR_PRESEDENCE.index(operator.value) <= OPERATOR_PRESEDENCE.index(token.value):
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
        elif operator is None: # Caso de los function calls
            compiled_code += "\n" + compile_terminator(left_side_tokens, unsafe)
        elif operator.value == "-" and not left_side_tokens:
            compiled_code += "\n" + compile_terminator([operator] + right_side_tokens, unsafe)
            operator = None
        else:
            compiled_code += "\n" + compile_expression(left_side_tokens)
            compiled_code += "\n" + compile_expression(right_side_tokens)
    if operator:
        infix: bool = False
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
        elif operator.value in ("<>", "!="):
            compiled_code += "\nISNE"
        elif operator.value == "<":
            compiled_code += "\nISLT"
        elif operator.value == ">":
            compiled_code += "\nISGT"
        elif operator.value == "<=":
            compiled_code += "\nISLE"
        elif operator.value == ">=":
            compiled_code += "\nISGE"
        elif operator.value == "&&":
            compiled_code += "\nLAND"
        elif operator.value == "||":
            compiled_code += "\nLGOR"
        elif operator.value == "::":
            compiled_code += "\nISIN"
        elif operator.value == "!":
            compiled_code += "\nLNOT"
            infix = True
        else:
            expression_error(
                f"The operator {operator.value} cannot be used as an infix operator.",
                operator.line,
                operator.file
            )
        if infix and left_side_tokens:
            expression_error(
                f"The operator {operator.value} can only be used as an infix operator.",
                operator.line,
                operator.file
            )
    if discard_return_value:
        compiled_code += "\nPOPV"
    return compiled_code



def compile_terminator(expr_tokens: List[Token], unsafe: bool = False) -> str:
    """Generates Nambly for an expression terminator.
    This is the second flojest part of the code and should
    probably be turned into an AST first.
    """
    compiled_code: str = ""
    add_minus_code: bool = False
    
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
                                f"Unexpected expression element: {expr_tokens[0].value} (did you forget a ';'?)",
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
                    expr_tokens.pop(0)
                else:
                    add_minus_code = True
            elif token.type == LexType.TABLE:
                if terminator_type != LexType.UNKNOWN:
                    expression_error(f"Unexpected token '{token.value}'.", token.line, token.file)
                compiled_code += f'\nTABL'
                terminator_type = token.type
            elif token.type == LexType.VARIABLE:
                if terminator_type != LexType.UNKNOWN:
                    expression_error(f"Unexpected token '{token.value}'.", token.line, token.file)
                var_id: Optional[str] = global_compiler_state.get_var_identifier(token, True, unsafe=unsafe)
                compiled_code += f'\nVGET "{var_id}"'
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
                elif terminator_type not in [LexType.VARIABLE, LexType.WORD, LexType.STRING, LexType.FLOAT, LexType.INTEGER, LexType.TABLE]:
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
                            f"Unexpected expression element: {expr_tokens[0].value} (did you forget a ';'?)",
                            token.line,
                            token.file
                        )
                # Call the function
                if terminator_type == LexType.WORD:
                    compiled_code += "\n" + compile_function_call(function_call_token, arguments_list)
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
    for line in tokenized_lines:
        # Check first token in the line, this is our command
        command = line[0]
        args = []
        if len(line) > 1:
            args = line[1:]
        if command.type == LexType.WORD:
            # --- in command ---
            if command.value == "in":
                compiled_code += "\n" + parse_command_in(command, args, False)
            elif command.value == "global":
                compiled_code += "\n" + parse_command_in(command, args, True)
            elif command.value == "while":
                compiled_code += "\n" + parse_command_while(command, args)
            elif command.value == "until":
                compiled_code += "\n" + parse_command_until(command, args)
            elif command.value == "if":
                compiled_code += "\n" + parse_command_if(command, args)
            elif command.value == "elif":
                compiled_code += "\n" + parse_command_elif(command, args)
            elif command.value == "else":
                compiled_code += "\n" + parse_command_else(command, args)
            elif command.value == "unless":
                compiled_code += "\n" + parse_command_unless(command, args)
            elif command.value == "ok":
                compiled_code += "\n" + parse_command_ok(command, args)
            elif command.value == "continue":
                compiled_code += "\n" + parse_command_continue(command, args)
            elif command.value == "break":
                compiled_code += "\n" + parse_command_break(command, args)
            elif command.value == "def":
                compiled_code += "\n" + parse_command_def(command, args)
            elif command.value == "return":
                compiled_code += "\n" + parse_command_return(command, args)
            elif command.value == "sleep":
                compiled_code += "\n" + parse_command_sleep(command, args)
            elif command.value == "import":
                compiled_code += "\n" + parse_command_import(command, args)
            else:
                # Commands that are "function-like" such as print
                compiled_code += "\n" + compile_expression(line, True)
        elif command.type == LexType.VARIABLE:
            compiled_code += "\n" + parse_command_in(command, [command] + args, False)
        else:
            parse_error(f"Unexpected command '{command.value}'.", command.line, command.file)
    return compiled_code


def compile_function_call(command: Token, args_list: List[List[Token]]):
    if command.value == "print":
        return parse_command_print(command, args_list)
    elif command.value == "printc":
        return parse_command_printc(command, args_list)
    elif command.value == "accept":
        return parse_command_accept(command, args_list)
    elif command.value == "is":
        return parse_command_is(command, args_list)
    elif command.value == "del":
        return parse_command_del(command, args_list)
    elif command.value == "unset":
        return parse_command_unset(command, args_list)
    elif command.value == "unsafe":
        return parse_command_unsafe(command, args_list)
    elif command.value == "exit":
        return parse_command_exit(command, args_list)
    elif command.value == "open_rw":
        return parse_command_open_rw(command, args_list)
    elif command.value == "open_ra":
        return parse_command_open_ra(command, args_list)
    elif command.value == "close":
        return parse_command_close(command, args_list)
    elif command.value == "read":
        return parse_command_read(command, args_list)
    elif command.value == "read_line":
        return parse_command_read_line(command, args_list)
    elif command.value == "len":
        return parse_command_len(command, args_list)
    elif command.value == "substr":
        return parse_command_substr(command, args_list)
    elif command.value == "floor":
        return parse_command_floor(command, args_list)
    elif command.value == "exec":
        return parse_command_exec(command, args_list)
    elif command.value == "keys":
        return parse_command_keys(command, args_list)
    else:
        return parse_function_call(command, args_list)
    


def parse_command_in(command_token: Token, args: List[Token], global_var: bool) -> str:
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
            parse_error(f"Variable expected ('{var.value}' found).", var.line, var.file)
        else:
            if not has_accesses:
                if len(left_side) > 1:
                    parse_error(
                        f"Unexpected token: '{left_side[1]}'",
                        left_side[1].line,
                        left_side[1].file
                    )
                else:
                    var_id = global_compiler_state.declare_variable(var, global_var)
                    if global_var:
                        set_compiled_code += f'\nGSET "{var_id}"'
                    else:
                        set_compiled_code += f'\nVSET "{var_id}"'
            else:
                var_id = global_compiler_state.declare_variable(var, global_var)
                access_compiled_code += f'\nVGET "{var_id}"'
                access_tokens: List[Token] = []
                access_depth: int = 0
                access_count: int = 0
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
                            if access_count > 0:
                                access_compiled_code += f'\nPGET'
                            access_compiled_code += "\n" + compile_expression(access_tokens[1:-1])
                            access_count += 1
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


def parse_command_print(command_token: Token, args_list: List[List[Token]]) -> str:
    if not args_list:
        parse_error(f"Wrong number of arguments for function '{command_token.value}' (expected 1+).", command_token.line, command_token.file)
    compiled_code: str = ""
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
    

def parse_command_accept(command_token: Token, args_list: List[List[Token]]) -> str:
    compiled_code: str = ""
    if args_list:
        compiled_code += "\n" + parse_command_printc(command_token, args_list, True)
    compiled_code += "\nACCP"
    return compiled_code


def parse_command_printc(command_token: Token, args_list: List[List[Token]]) -> str:
    if not args_list:
        parse_error(f"Wrong number of arguments for function '{command_token.value}' (expected 1+).", command_token.line, command_token.file)
    compiled_code: str = ""
    compiled_code += '\nPUSH ""'
    for args in args_list:
        compiled_code += "\n" + compile_expression(args)
        compiled_code += "\nDUPL"
        compiled_code += "\nVSET \"$swap\""  # Only system vars start with $ in nambly
        compiled_code += "\nDISP"
        compiled_code += "\nVGET \"$swap\""  # Only system vars start with $ in nambly
        compiled_code += "\nJOIN"
    return compiled_code
    

def parse_command_is(command_token: Token, args_list: List[List[Token]]) -> str:
    compiled_code: str = ""
    if len(args_list) != 1:
        parse_error(f"Wrong number of arguments for function '{command_token.value}' (expected 1).", command_token.line, command_token.file)
    compiled_code += "\n" + compile_expression(args_list[0])
    compiled_code += "\nNIL?"
    compiled_code += "\nLNOT"
    return compiled_code


def parse_command_del(command_token: Token, args_list: List[List[Token]]) -> str:
    compiled_code: str = ""
    if len(args_list) != 2:
        parse_error(f"Wrong number of arguments for function '{command_token.value}' (expected 2: variable and index).", command_token.line, command_token.file)
    compiled_code += "\n" + compile_expression(args_list[0])
    compiled_code += "\n" + compile_expression(args_list[1])
    compiled_code += "\nPUST"
    return compiled_code


def parse_command_unset(command_token: Token, args_list: List[List[Token]]) -> str:
    compiled_code: str = ""
    if len(args_list) == 0:
        parse_error(f"Wrong number of arguments for function '{command_token.value}' (expected 1+).", command_token.line, command_token.file)
    for arg in args_list:
        if len(arg) > 1:
            parse_error(f"Unexpected token {arg[1]}", arg[1].line, arg[1].file)
        if arg[0].type != LexType.VARIABLE:
            parse_error(f"Variable expected, got {arg[0]}.", arg[0].line, arg[0].file)
        compiled_code += f"\n UNST {global_compiler_state.get_var_identifier(arg[0], True)}"
        global_compiler_state.unset_variable(arg[0], False)
    return compiled_code


def parse_command_unsafe(command_token: Token, args_list: List[List[Token]]) -> str:
    compiled_code: str = ""
    if len(args_list) != 1:
        parse_error(f"Wrong number of arguments for function '{command_token.value}' (expected 1).", command_token.line, command_token.file)
    for arg in args_list:
        if arg[0].type != LexType.VARIABLE:
            parse_error(f"Variable expected, got {arg[0]}.", arg[0].line, arg[0].file)
    compiled_code += f"\n" + compile_expression(args_list[0], discard_return_value=False, unsafe=True)
    return compiled_code


def parse_command_exit(command_token: Token, args_list: List[List[Token]]) -> str:
    compiled_code: str = ""
    if len(args_list) != 1:
        parse_error("Wrong number of arguments for function exit (expected 1).", command_token.line, command_token.file)
    compiled_code += "\n" + compile_expression(args_list[0])
    compiled_code += "\nEXIT"
    return compiled_code


def parse_command_open_rw(command_token: Token, args_list: List[List[Token]]) -> str:
    compiled_code: str = ""
    if len(args_list) != 1:
        parse_error(f"Wrong number of arguments for function '{command_token.value}' (expected 1).", command_token.line, command_token.file)
    compiled_code += "\n" + compile_expression(args_list[0])
    compiled_code += f"\nDUPL"
    compiled_code += f"\nFORW"
    return compiled_code


def parse_command_open_ra(command_token: Token, args_list: List[List[Token]]) -> str:
    compiled_code: str = ""
    # TODO solo debería devolver el nombre del último archivo abierto - un solo return value!
    if len(args_list) != 1:
        parse_error(f"Wrong number of arguments for function '{command_token.value}' (expected 1).", command_token.line, command_token.file)
    compiled_code += "\n" + compile_expression(args_list[0])
    compiled_code += f"\nDUPL"
    compiled_code += f"\nFORA"
    return compiled_code


def parse_command_close(command_token: Token, args_list: List[List[Token]]) -> str:
    compiled_code: str = ""
    if len(args_list) != 1:
        parse_error(f"Wrong number of arguments for function '{command_token.value}' (expected 1).", command_token.line, command_token.file)
    compiled_code += "\n" + compile_expression(args_list[0])
    compiled_code += f"\nDUPL"
    compiled_code += f"\nFCLS"
    return compiled_code


def parse_command_read(command_token: Token, args_list: List[List[Token]]) -> str:
    compiled_code: str = ""
    if len(args_list) != 1:
        parse_error(f"Wrong number of arguments for function '{command_token.value}' (expected 1).", command_token.line, command_token.file)
    compiled_code += "\n" + compile_expression(args_list[0])
    compiled_code += f"\nRFIL"
    return compiled_code


def parse_command_read_line(command_token: Token, args_list: List[List[Token]]) -> str:
    compiled_code: str = ""
    if len(args_list) != 1:
        parse_error(f"Wrong number of arguments for function '{command_token.value}' (expected 1).", command_token.line, command_token.file)
    compiled_code += "\n" + compile_expression(args_list[0])
    compiled_code += f"\nRLNE"
    return compiled_code


def parse_command_while(command_token: Token, args: List[Token]) -> str:
    if not args:
        parse_error(f"Command '{command_token.value}' expects an expression to evaluate.", command_token.line, command_token.file)
    block_number: int = global_compiler_state.block_count
    global_compiler_state.block_count += 1
    start_tag: str = f"LOOP_{block_number}_START"
    end_tag: str = f"LOOP_{block_number}_END"
    compiled_code: str = ""
    block_end_code: str = ""
    compiled_code += f"\n@{start_tag}"
    compiled_code += "\n" + compile_expression(args)
    compiled_code += "\nDUPL"
    it_var: Token = Token(RESULT_VAR, command_token.line, command_token.file)
    it_var.type = LexType.VARIABLE
    it_var_id: str = global_compiler_state.declare_variable(it_var, True)
    compiled_code += f'\nVSET "{it_var_id}"'
    compiled_code += f"\nJPIF {end_tag}"
    # Push end code to state for it to be used on next ok;
    block_end_code += f"\nJUMP {start_tag}"
    block_end_code += f"\n@{end_tag}"
    global_compiler_state.add_open_loop(start_tag, end_tag)
    global_compiler_state.add_block_end_code(block_end_code, command_token)
    return compiled_code


def parse_command_until(command_token: Token, args: List[Token]) -> str:
    if not args:
        parse_error(f"Command '{command_token.value}' expects an expression to evaluate.", command_token.line, command_token.file)
    block_number: int = global_compiler_state.block_count
    global_compiler_state.block_count += 1
    start_tag: str = f"LOOP_{block_number}_START"
    end_tag: str = f"LOOP_{block_number}_END"
    compiled_code: str = ""
    block_end_code: str = ""
    compiled_code += f"\n@{start_tag}"
    compiled_code += "\n" + compile_expression(args)
    compiled_code += "\nDUPL"
    it_var: Token = Token(RESULT_VAR, command_token.line, command_token.file)
    it_var.type = LexType.VARIABLE
    it_var_id: str = global_compiler_state.declare_variable(it_var, True)
    compiled_code += f'\nVSET "{it_var_id}"'
    compiled_code += f"\nLNOT"
    compiled_code += f"\nJPIF {end_tag}"
    # Push end code to state for it to be used on next ok;
    block_end_code += f"\nJUMP {start_tag}"
    block_end_code += f"\n@{end_tag}"
    global_compiler_state.add_open_loop(start_tag, end_tag)
    global_compiler_state.add_block_end_code(block_end_code, command_token)
    return compiled_code


def parse_command_if(command_token: Token, args: List[Token]) -> str:
    if not args:
        parse_error(f"Command '{command_token.value}' expects an expression to evaluate.", command_token.line, command_token.file)
    block_number: int = global_compiler_state.block_count
    global_compiler_state.block_count += 1
    # start_tag: str = f"COND_{block_number}_START"
    end_tag: str = f"COND_{block_number}_END"
    if_final_tag: str = f"EXIT_IF_{block_number}"
    compiled_code: str = ""
    block_end_code: str = ""
    # compiled_code += f"\n@{start_tag}"
    compiled_code += "\n" + compile_expression(args)
    compiled_code += "\nDUPL"
    it_var: Token = Token(RESULT_VAR, command_token.line, command_token.file)
    it_var.type = LexType.VARIABLE
    it_var_id: str = global_compiler_state.declare_variable(it_var, True)
    compiled_code += f'\nVSET "{it_var_id}"'
    compiled_code += f"\nJPIF {end_tag}"
    # Push end code to state for it to be used on next ok;
    block_end_code += f"\nJUMP {if_final_tag}"
    block_end_code += f"\n@{end_tag}"
    global_compiler_state.add_block_end_code(block_end_code, command_token, block_number)
    return compiled_code


def parse_command_elif(command_token: Token, args: List[Token]) -> str:
    if not args:
        parse_error(f"Command '{command_token.value}' expects an expression to evaluate.", command_token.line, command_token.file)
    if not global_compiler_state.get_in_if_chain():
        parse_error(f"Command '{command_token.value}' can only be used after an if block.", command_token.line, command_token.file)
    block_number: int = global_compiler_state.block_count
    global_compiler_state.block_count += 1
    group_id: int = global_compiler_state.get_group_id()
    end_tag: str = f"COND_{block_number}_END"
    if_final_tag: str = f"EXIT_IF_{group_id}"
    compiled_code: str = ""
    compiled_code += global_compiler_state.get_block_end_data(command_token)[0]
    block_end_code: str = ""
    # compiled_code += f"\n@{start_tag}"
    compiled_code += "\n" + compile_expression(args)
    compiled_code += "\nDUPL"
    it_var: Token = Token(RESULT_VAR, command_token.line, command_token.file)
    it_var.type = LexType.VARIABLE
    it_var_id: str = global_compiler_state.declare_variable(it_var, True)
    compiled_code += f'\nVSET "{it_var_id}"'
    compiled_code += f"\nJPIF {end_tag}"
    # Push end code to state for it to be used on next ok;
    block_end_code += f"\nJUMP {if_final_tag}"
    block_end_code += f"\n@{end_tag}"
    global_compiler_state.add_block_end_code(block_end_code, command_token, group_id)
    return compiled_code


def parse_command_else(command_token: Token, args: List[Token]) -> str:
    if args:
        parse_error(f"Command '{command_token.value}' doesn't expect any arguments.", command_token.line, command_token.file)
    if not global_compiler_state.get_in_if_chain():
        parse_error(f"Command '{command_token.value}' can only be used after an if block.", command_token.line, command_token.file)
    group_id: int = global_compiler_state.get_group_id()
    compiled_code: str = ""
    compiled_code += global_compiler_state.get_block_end_data(command_token)[0]
    block_end_code: str = ""
    # Push end code to state for it to be used on next ok;
    global_compiler_state.add_block_end_code(block_end_code, command_token, group_id)
    return compiled_code


def parse_command_unless(command_token: Token, args: List[Token]) -> str:
    block_number: int = global_compiler_state.block_count
    global_compiler_state.block_count += 1
    start_tag: str = f"COND_{block_number}_START"
    end_tag: str = f"COND_{block_number}_END"
    compiled_code: str = ""
    block_end_code: str = ""
    compiled_code += f"\n@{start_tag}"
    compiled_code += "\n" + compile_expression(args)
    compiled_code += "\nDUPL"
    it_var: Token = Token(RESULT_VAR, command_token.line, command_token.file)
    it_var.type = LexType.VARIABLE
    it_var_id: str = global_compiler_state.declare_variable(it_var, True)
    compiled_code += f'\nVSET "{it_var_id}"'
    compiled_code += f"\nLNOT"
    compiled_code += f"\nJPIF {end_tag}"
    # Push end code to state for it to be used on next ok;
    block_end_code += f"\n@{end_tag}"
    global_compiler_state.add_block_end_code(block_end_code, command_token)
    return compiled_code


def parse_command_ok(command_token: Token, args: List[Token]) -> str:
    if args:
        parse_error(f"Unexpected arguments for command '{command_token.value}'.", command_token.line, command_token.file)
    compiled_code: str = ""
    global_compiler_state.close_open_loop()
    compiled_code += global_compiler_state.del_scope()
    block_end_data = global_compiler_state.get_block_end_data(command_token)
    compiled_code += block_end_data[0]
    if block_end_data[1].type == LexType.WORD and block_end_data[1].value in ("if", "elif", "else"):
        compiled_code += f"\n@EXIT_IF_{block_end_data[2]}"
    return compiled_code


def parse_command_continue(command_token: Token, args: List[Token]) -> str:
    compiled_code: str = ""
    if args:
        parse_error(f"Unexpected arguments for command '{command_token.value}'.", command_token.line, command_token.file)
    if global_compiler_state.get_open_loop_tags() is None:
        parse_error(f"Continue can only be used inside loops.", command_token.line, command_token.file)
    else:
        start_tag: str = global_compiler_state.get_open_loop_tags()[0]
        compiled_code += f"\nJUMP {start_tag}"
    return compiled_code


def parse_command_break(command_token: Token, args: List[Token]) -> str:
    compiled_code: str = ""
    if len(args) > 1:
        parse_error(f"Wrong number of arguments for command '{command_token.value}' (expected 0 or 1).", command_token.line, command_token.file)
    count: int = 1
    if args:
        if not is_integer(args[0].value):
            parse_error(f"Expected zero or one integer arguments for command '{command_token.value}'.", command_token.line, command_token.file)
        count = int(args[0].value)
    if global_compiler_state.get_open_loop_tags(depth=count - 1) is None:
        parse_error(f"Break can only be used inside loops.", command_token.line, command_token.file)
    end_tag: str = global_compiler_state.get_open_loop_tags(depth=count - 1)[1]
    compiled_code += f"\nJUMP {end_tag}"
    return compiled_code


def parse_command_len(command_token: Token, args_list: List[List[Token]]) -> str:
    compiled_code: str = ""
    if len(args_list) != 1:
        parse_error(f"Wrong number of arguments for function '{command_token.value}' (expected 1).", command_token.line, command_token.file)
    compiled_code += "\n" + compile_expression(args_list[0])
    compiled_code += f"\nSLEN"
    return compiled_code


def parse_command_substr(command_token: Token, args_list: List[List[Token]]) -> str:
    compiled_code: str = ""
    if len(args_list) != 2 and len(args_list) != 3:
        parse_error(f"Wrong number of arguments for function '{command_token.value}' (expected 2 or 3).", command_token.line, command_token.file)
    compiled_code += "\n" + compile_expression(args_list[0])
    if len(args_list) == 2:
        compiled_code += f"\nDUPL"
    compiled_code += "\n" + compile_expression(args_list[1])
    if len(args_list) == 3:
        compiled_code += "\n" + compile_expression(args_list[2])
    elif len(args_list) == 2:
        compiled_code += f"\nSWAP"
        compiled_code += f"\nSLEN"
    compiled_code += f"\nSSTR"
    return compiled_code


def parse_command_def(command_token: Token, args: List[Token]) -> str:
    if len(args) != 1:
        parse_error(f"Unexpected token in def line '{args[1].value}'.", args[1].line, args[1].file)
    global_compiler_state.add_scope()
    start_tag, end_tag, post_tag = global_compiler_state.get_function_label(args[0])
    compiled_code: str = ""
    block_end_code: str = ""
    compiled_code += f"\nJUMP {post_tag}"
    compiled_code += f"\n@{start_tag}"
    compiled_code += f"\nADSC"
    args_var: Token = Token(ARGS_VAR, command_token.line, command_token.file)
    args_var.type = LexType.VARIABLE
    compiled_code += f'\nARRR'
    compiled_code += f'\nVSET "{global_compiler_state.declare_variable(args_var, False)}"'
    args_var: Token = Token(ARGS_VAR, command_token.line, command_token.file)
    args_var.type = LexType.VARIABLE
    args_var: str = global_compiler_state.declare_variable(args_var, False)
    compiled_code += f'\nPNIL'  # Default return value
    # Push end code to state for it to be used on next ok;
    block_end_code += f"\n@{end_tag}"
    block_end_code += f"\nDLSC"
    block_end_code += f"\nRTRN"
    block_end_code += f"\n@{post_tag}"
    global_compiler_state.add_function(args[0], start_tag, end_tag, post_tag)
    global_compiler_state.add_block_end_code(block_end_code, args[0])
    return compiled_code


def parse_function_call(command_token: Token, args_list: List[List[Token]]) -> str:
    # All functions are variadic in Katalyn
    compiled_code: str = ""
    compiled_code += f'\nPNIL'  # So the ARRR works.
    for args in args_list:
        compiled_code += "\n" + compile_expression(args)
    function_end_label: str = global_compiler_state.get_function_label(command_token)[0]
    compiled_code += f'\nCALL {function_end_label}'
    return compiled_code


def parse_command_return(command_token: Token, args: List[Token]) -> str:
    compiled_code: str = ""
    if not global_compiler_state.get_in_function_declaration():
        parse_error(f"Return can only be used inside functions.", command_token.line, command_token.file)
    if len(args):
        compiled_code += f"\nPOPV"  # To remove the default nil
        compiled_code += "\n" + compile_expression(args)
    compiled_code += f"\nDLSC"
    compiled_code += f"\nRTRN"
    return compiled_code


def parse_command_floor(command_token: Token, args_list: List[List[Token]]) -> str:
    compiled_code: str = ""
    if len(args_list) != 1:
        parse_error(f"Wrong number of arguments for function '{command_token.value}' (expected 1).", command_token.line, command_token.file)
    compiled_code += "\n" + compile_expression(args_list[0])
    compiled_code += f"\nFLOR"
    return compiled_code


def parse_command_exec(command_token: Token, args_list: List[List[Token]]) -> str:
    if not args_list:
        parse_error(f"Wrong number of arguments for function '{command_token.value}' (expected 1+).", command_token.line, command_token.file)
    compiled_code: str = ""
    pushed_count: int = 0
    for args in args_list:
        compiled_code += "\n" + compile_expression(args)
        if pushed_count > 0:
            compiled_code += "\nJOIN"
        pushed_count += 1
    compiled_code += '\nEXEC'
    stdout_var: Token = Token(EXEC_STDOUT_VAR, command_token.line, command_token.file)
    stdout_var.type = LexType.VARIABLE
    stdout_var_id = global_compiler_state.declare_variable(stdout_var, False)
    stderr_var: Token = Token(EXEC_STDERR_VAR, command_token.line, command_token.file)
    stderr_var.type = LexType.VARIABLE
    stderr_var_id = global_compiler_state.declare_variable(stderr_var, False)
    exitcode_var: Token = Token(EXEC_EXITCODE_VAR, command_token.line, command_token.file)
    exitcode_var.type = LexType.VARIABLE
    exitcode_var_id = global_compiler_state.declare_variable(exitcode_var, False)
    compiled_code += f"\nVSET \"{stdout_var_id}\""
    compiled_code += f"\nVSET \"{stderr_var_id}\""
    compiled_code += f"\nDUPL"
    compiled_code += f"\nVSET \"{exitcode_var_id}\""
    return compiled_code


def parse_command_sleep(command_token: Token, args: List[Token]) -> str:
    compiled_code: str = ""
    if len(args):
        compiled_code += "\n" + compile_expression(args)
    compiled_code += f"\nWAIT"
    return compiled_code


def parse_command_keys(command_token: Token, args_list: List[List[Token]]) -> str:
    compiled_code: str = ""
    if len(args_list) != 1:
        parse_error(f"Wrong number of arguments for function '{command_token.value}' (expected 1).", command_token.line, command_token.file)
    compiled_code += "\n" + compile_expression(args_list[0])
    compiled_code += f"\nKEYS"
    return compiled_code


def get_relative_path(base_path, target_path):
    base_dir = os.path.dirname(os.path.abspath(base_path))
    relative_path = os.path.join(base_dir, target_path)
    absolute_path = os.path.abspath(relative_path)
    return absolute_path


def parse_command_import(command_token: Token, args: List[Token]) -> str:
    compiled_code: str = ""
    if len(args) != 1:
        parse_error(f"Wrong number of arguments for command '{command_token.value}' (expected 1).", command_token.line, command_token.file)
    if args:
        if args[0].type != LexType.STRING:
            parse_error(f"Expected a static string argument for command '{command_token.value}'.", command_token.line, command_token.file)
        else:
            path = get_relative_path(command_token.file, args[0].value.strip())
            compiled_code += "\n" + file_to_nambly(path)
    return compiled_code


global_compiler_state = CompilerState()


def print_help():
    print("Usage: katalyn [switches] <source file>")
    print("  -h                      print this information")
    print("  -n                      do not include standard library")
    print("  -s                      read source from standard input")
    print("  -v                      print version")


def print_version():
    print(f"This is Katalyn version {VERSION}.")
    print("Copyright 2024, Lartu (www.lartu.net).")


def file_to_nambly(filename: str, called_from_file: str = "", called_from_line: int = 0) -> str:
    """Loads, tokenizes, lexes, parses and compiles a whole Katalyn file into Nambly code.
    """
    nambly = ""
    try:
        with open(filename) as f:
            code = f.read()
    except:
        if not called_from_file:
            print(f"File {filename} not found.")
        else:
            parse_error(f"File {filename} not found.", called_from_line, called_from_file)
        exit(1)
    return code_to_nambly(code, filename)


def code_to_nambly(code: str, filename: str) -> str:
    """Tokenizes, lexes, parses and compiles Katalyn code into Nambly code.
    """
    tokenized_lines: List[List[Token]] = tokenize_source(code, filename)
    if tokenized_lines:
        # print_tokens(tokenized_lines, filename, "Tokenization")
        lex_tokens(tokenized_lines)
        #print_tokens(tokenized_lines, filename, "Lexing")
        nambly = compile_lines(tokenized_lines)
        global_compiler_state.check_for_errors()
    return nambly



if __name__ == "__main__":
    flags_var: Token = Token(FLAGS_VAR, 0, "")
    flags_var.type = LexType.VARIABLE
    flags_var_id = global_compiler_state.declare_variable(flags_var, True)
    full_nambly: str = "PNIL"
    filename: str = ""
    include_standard_lib: bool = True
    read_standard_input: bool = False
    for arg in sys.argv[1:]:
        if filename:
            # Pass arguments to Nambly
            full_nambly += f"\nPUSH \"{arg}\""
        else:
            if arg == "-h":
                print_help()
                exit(0)
            elif arg == "-n":
                include_standard_lib = False
            elif arg == "-s":
                read_standard_input = True
            elif arg == "-v":
                print_version()
                exit(0)
            else:
                filename = arg
    full_nambly += f"\nARRR"
    full_nambly += f"\nVSET \"{flags_var_id}\""
    if not filename and not read_standard_input:
        print_help()
        exit(1)
    if include_standard_lib:
        full_nambly += "\n" + file_to_nambly(f"{STDLIB_LOCATION}/stdlib.kat")
    if read_standard_input:
        code: str = ""
        while True:
            try:
                code += input()
            except EOFError:
                break
            except Exception:
                exit(1)
        full_nambly += "\n" + code_to_nambly(code, os.path.join(os.getcwd(), "stdin"))
    else:
        full_nambly += "\n" + file_to_nambly(filename)
    nambly = stylize_namby(full_nambly)
    # print(nambly)
    nari_run(nambly)
