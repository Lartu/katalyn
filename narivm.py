from __future__ import annotations
from io import TextIOWrapper
from typing import Dict, List, Set, Tuple, Any, Optional, Union
from enum import Enum
import math
import sys

variable_table: Dict[str, Value] = {}
label_to_pc: Dict[str, int] = {}
pc_to_label: Dict[int, str] = {}
execution_stack: List[Any] = []
open_files: Dict[str, TextIOWrapper] = {}
return_stack: List[int] = []

class Types(Enum):
    INT = 1
    FLO = 2
    TXT = 3
    TAB = 4
    NIL = 5

code = """
PUSH 0
VSET "loops"
@loop
PUSH 1
PUSH 2
ADDV
VSET "a"
PUSH "Hello there!"
PUSH "I love quotes! \\"\\"\\"\\\\!!!"
JOIN
VSET "b"
VGET "loops"
PUSH 4
JPEQ @end
VGET "loops"
PUSH 1
ADDV
VSET "loops"
@jmpline
JUMP @loop
@end
"""

code = """
TABL
VSET "ttt"
VGET "ttt"
PUSH "x"
PUSH -128.9
PSET
VGET "ttt"
PUSH "x"
PGET
DISP
PUSH "\\n"
DISP
"""

# Para setear tabla: primero tabla, despues campo, despues valor, después escritura (arriba)
# Para leer tabla: primero tabla, después campo, después lectura (arriba)


class Command:
    def __init__(self):
        self.command = ""
        self.arguments: List[Value]= []

    def __repr__(self) -> str:
        rep: str = f"{self.command}"
        if self.arguments:
            rep += " "
        for arg in self.arguments:
            rep += f"{arg}"
        return rep
    

class Value:
    def __init__(self) -> None:
        self.value = ""
        self.type = Types.NIL

    def __repr__(self) -> str:
        type_name = str(self.type).split('.')[1]
        return f"{self.value}:{type_name}"
    
    def __str__(self) -> str:
        return str(self.value)
    
    def get_as_string(self) -> str:
        """Returns a string representation of the value.
        TODO: Optimizar esto para que no haya que convertir cada vez.
        """
        if self.type == Types.NIL:
            return "nil"
        elif self.type == Types.TAB:
            table_values: List[str] = []
            for key in self.value:
                table_values.append(f"{key}: '{self.value[key].get_as_string()}'")
            return "[" + ", ".join(table_values) + "]"
        return str(self.value)
    
    def get_as_number(self) -> Union[float|int]:
        """Returns an int or a float depending on the string.
        TODO: Optimizar esto para que no haya que convertir cada vez.
        """
        if self.type == Types.NIL:
            nambly_error("Runtime error: a NIL value cannot be turned into a number.")
        begin: int = 0
        found_period: bool = False
        if self.type == Types.INT:
            return int(self.value)
        if self.type == Types.FLO:
            return float(self.value)
        if self.type == Types.TXT:
            if self.value[0] in "+-":
                begin = 1
            for char in self.value[begin:]:
                if char not in "0123456789.":
                    nambly_error(f"Invalid number: {self}")
                if char == ".":
                    if found_period:
                        nambly_error(f"Invalid number: {self}")
                    else:
                        found_period = True
            if not found_period:
                return int(self.value)
            else:
                return float(self.value)
    

def nambly_error(message: str):
    print("Runtime Error!")
    print(message)
    exit(1)


def generate_label_map(code_listing: List[Command]) -> List[Command]:
    """Checks a code listing for labels and fills the label map with their PCs. Then
    returns the code without those labels.
    """
    pc: int = 0
    new_listing: List[Command] = []
    for command in code_listing:
        if command.command[0] == "@":
            jmp_pc_value: int = pc
            label_to_pc[command.command[1:]] = jmp_pc_value
            pc_to_label[jmp_pc_value] = command.command
        else:
            new_listing.append(command)
            pc += 1
    return new_listing


def split_lines(code: str) -> List[Command]:
    """Splits a code into multiple lines, skipping empty lines.
    This generates what we call a code listing: a list of assembly lines, trimmed, with
    proper spacing and no empty lines
    """
    lines = code.split("\n")
    code_listing = []
    for line in lines:
        line = line.strip()
        if len(line):
            code_listing.append(split_command_arguments(line))
    return code_listing


def split_command_arguments(line: str) -> Command:
    """Takes a line of code and splits it into a command (or label) and its arguments.
    """
    tokens: List[str] = []
    current_token: str = ""
    in_string: bool = False
    next_char_escaped: bool = False
    uppercase_on_append = False
    for char in line:
        if not in_string and char.isspace():
            if len(current_token):
                if uppercase_on_append:
                    uppercase_on_append = False
                    current_token = current_token.upper()
                tokens.append(current_token)
            current_token = ""
        elif in_string and next_char_escaped:
            next_char_escaped = False
            if char == 'n':
                current_token += '\n'
            elif char == 't':
                current_token += '\t'
            else:
                current_token += char
        elif in_string and char == '\\':
            next_char_escaped = True
        elif char == '"':
            in_string = not in_string
            current_token += char
        else:
            if len(current_token) == 0 and char == "@":
                uppercase_on_append = True
            current_token += char
    if len(current_token):
        if uppercase_on_append:
            uppercase_on_append = False
            current_token = current_token.upper()
        tokens.append(current_token)
    if in_string:
        nambly_error(f"Nambly parsing error: open string for line '{line}'")
    new_command: Command = Command()
    if len(tokens):
        new_command.command = tokens[0].upper()
    for token in tokens[1:]:
        value = Value()
        value.value = token
        value.type = get_token_type(value.value)
        if value.type == Types.TXT:
            value.value = value.value[1:-1]
        if value.type == Types.FLO:
            value.value = float(value.value)
        if value.type == Types.INT:
            value.value = int(value.value)
        new_command.arguments.append(value)
    return new_command


def pad_string(text: str, padding_size: int) -> str:
    """Pads a string to a given number of chars (left side)
    """
    if len(text) > padding_size:
        return text
    else:
        return text + " " * (padding_size - len(text))


def print_code_listing(code_listing: List[Command]):
    """Prints a list of commands.
    """
    pc: int = 0
    for command in code_listing:
        print(pad_string(str(pc), 4), end="")
        print(pad_string(str(command), 40), end="")
        if pc in pc_to_label:
            print(f"<-- {pc_to_label[pc]}")
        else:
            print("")
        pc += 1


def print_stack():
    """Prints the values in the current execution stack.
    """
    print("")
    print("--- EXECUTION STACK ---")
    print("(BOTTOM)")
    for value in execution_stack:
        print(value)
    print("(TOP)")


def print_return_stack():
    """Prints the values in the current return stack.
    """
    print("")
    print("--- RETURN STACK ---")
    print("(BOTTOM)")
    for value in return_stack:
        print(value)
    print("(TOP)")


def print_variable_table(table: Dict[Value] = variable_table, prefix: str = "- "):
    """Prints all values in the variable_table"""
    if table == variable_table:
        print("")
        print("--- VARIABLE TABLE ---")
    for key in table:
        if isinstance(table[key].value, dict):
            print(f"{prefix}{key}: (table)")
            print_variable_table(table[key].value, "  " + prefix)
        else:
            print(f"{prefix}{key}: {table[key]}")


def get_token_type(text: str) -> Types:
    """Calculates the type of a token.
    """
    begin: int = 0
    found_period: bool = False
    if text[0] == '"':
        return Types.TXT
    if text[0] in "+-":
        begin = 1
    for char in text[begin:]:
        if char not in "0123456789.":
            return Types.NIL
        if char == ".":
            if found_period:
                return Types.NIL
            else:
                found_period = True
    if not found_period:
        return Types.INT
    else:
        return Types.FLO
    

def pop(command: Command) -> Value:
    """Pops and returns a value from the execution stack.
    """
    if len(execution_stack):
        return execution_stack.pop()
    else:
        nambly_error(f"Execution stack empty for command: {command}")


def push(v: Value):
    execution_stack.append(v)


def display(v: Value):
    print(v.get_as_string(), end="")


def execute_code_listing(code_listing: List[Command]):
    """Inefficiently executes a code listing.
    """
    pc: int = 0
    while pc < len(code_listing):
        command: Command = code_listing[pc]
        if "PUSH" == command.command:
            push(command.arguments[0])
        elif "PNIL" == command.command:
            result_value = Value()
            result_value.value = None
            result_value.type = Types.NIL
            push(result_value)
        elif "ADDV" == command.command:
            com_2: Value = pop(command)
            com_1: Value = pop(command)
            value_2: Union[float|int] = com_2.get_as_number()
            value_1: Union[float|int] = com_1.get_as_number()
            result_value = Value()
            result_value.value = value_1 + value_2
            if com_1.type == Types.FLO or com_2.type == Types.FLO:
                result_value.type = Types.FLO
            else:
                result_value.type = Types.INT 
            push(result_value)
        elif "SUBT" == command.command:
            com_2: Value = pop(command)
            com_1: Value = pop(command)
            value_2: Union[float|int] = com_2.get_as_number()
            value_1: Union[float|int] = com_1.get_as_number()
            result_value = Value()
            result_value.value = value_1 - value_2
            if com_1.type == Types.FLO or com_2.type == Types.FLO:
                result_value.type = Types.FLO
            else:
                result_value.type = Types.INT 
            push(result_value)
        elif "MULT" == command.command:
            com_2: Value = pop(command)
            com_1: Value = pop(command)
            value_2: Union[float|int] = com_2.get_as_number()
            value_1: Union[float|int] = com_1.get_as_number()
            result_value = Value()
            result_value.value = value_1 * value_2
            if com_1.type == Types.FLO or com_2.type == Types.FLO:
                result_value.type = Types.FLO
            else:
                result_value.type = Types.INT 
            push(result_value)
        elif "FDIV" == command.command:
            value_2: Union[float|int] = pop(command).get_as_number()
            value_1: Union[float|int] = pop(command).get_as_number()
            result_value = Value()
            result_value.value = float(value_1) / float(value_2)
            result_value.type = Types.FLO
            push(result_value)
        elif "IDIV" == command.command:
            value_2: Union[float|int] = pop(command).get_as_number()
            value_1: Union[float|int] = pop(command).get_as_number()
            result_value = Value()
            result_value.value = int(value_1) // int(value_2)
            result_value.type = Types.INT
            push(result_value)
        elif "POWR" == command.command:
            com_2: Value = pop(command)
            com_1: Value = pop(command)
            value_2: Union[float|int] = com_2.get_as_number()
            value_1: Union[float|int] = com_1.get_as_number()
            result_value = Value()
            result_value.value = value_1 ** value_2
            if com_1.type == Types.FLO or com_2.type == Types.FLO:
                result_value.type = Types.FLO
            else:
                result_value.type = Types.INT 
            push(result_value)
        elif "MODL" == command.command:
            value_2: Union[float|int] = pop(command).get_as_number()
            value_1: Union[float|int] = pop(command).get_as_number()
            result_value = Value()
            result_value.value = int(value_1) % int(value_2)
            result_value.type = Types.INT
            push(result_value)
        elif "ISGT" == command.command:
            com_2: Value = pop(command)
            com_1: Value = pop(command)
            value_2: Union[float|int] = com_2.get_as_number()
            value_1: Union[float|int] = com_1.get_as_number()
            result_value = Value()
            result_value.type = Types.INT
            result_value.value = 0
            if value_1 > value_2:
                result_value.value = 1
            push(result_value)
        elif "ISLT" == command.command:
            com_2: Value = pop(command)
            com_1: Value = pop(command)
            value_2: Union[float|int] = com_2.get_as_number()
            value_1: Union[float|int] = com_1.get_as_number()
            result_value = Value()
            result_value.type = Types.INT
            result_value.value = 0
            if value_1 < value_2:
                result_value.value = 1
            push(result_value)
        elif "ISGE" == command.command:
            com_2: Value = pop(command)
            com_1: Value = pop(command)
            value_2: Union[float|int] = com_2.get_as_number()
            value_1: Union[float|int] = com_1.get_as_number()
            result_value = Value()
            result_value.type = Types.INT
            result_value.value = 0
            if value_1 >= value_2:
                result_value.value = 1
            push(result_value)
        elif "ISLE" == command.command:
            com_2: Value = pop(command)
            com_1: Value = pop(command)
            value_2: Union[float|int] = com_2.get_as_number()
            value_1: Union[float|int] = com_1.get_as_number()
            result_value = Value()
            result_value.type = Types.INT
            result_value.value = 0
            if value_1 <= value_2:
                result_value.value = 1
            push(result_value)
        elif "ISEQ" == command.command:
            com_2: Value = pop(command)
            com_1: Value = pop(command)
            result_value = Value()
            result_value.type = Types.INT
            result_value.value = 0
            if com_1.type == Types.NIL or com_2.type == Types.NIL:
                result_value.value = 0
            elif com_1.type == Types.TAB and com_2.type == Types.TAB:
                if com_1.value == com_2.value: # By reference
                    result_value.value = 1
            elif com_1.type == Types.TXT and com_2.type == Types.TXT:
                if com_1.value == com_2.value:
                    result_value.value = 1
            elif com_1.type == Types.INT and com_2.type == Types.INT:
                if com_1.value == com_2.value:
                    result_value.value = 1
            elif com_1.type == Types.FLO and com_2.type == Types.FLO:
                if math.isclose(com_1.value, com_2.value):
                    result_value.value = 1
            else:
                # Default to numeric comparison
                value_2: Union[float|int] = com_2.get_as_number()
                value_1: Union[float|int] = com_1.get_as_number()
                if isinstance(value_1, int) and isinstance(value_2, int):
                    if value_1 == value_2:
                        result_value.value = 1
                    elif math.isclose(value_1, value_2):
                        result_value.value = 1
            push(result_value)
        elif "ISNE" == command.command:
            com_2: Value = pop(command)
            com_1: Value = pop(command)
            result_value = Value()
            result_value.type = Types.INT
            result_value.value = 0
            if com_1.type == Types.NIL or com_2.type == Types.NIL:
                result_value.value = 0
            elif com_1.type == Types.TAB and com_2.type == Types.TAB:
                if com_1.value == com_2.value: # By reference
                    result_value.value = 1
            elif com_1.type == Types.TXT and com_2.type == Types.TXT:
                if com_1.value == com_2.value:
                    result_value.value = 1
            elif com_1.type == Types.INT and com_2.type == Types.INT:
                if com_1.value == com_2.value:
                    result_value.value = 1
            elif com_1.type == Types.FLO and com_2.type == Types.FLO:
                if math.isclose(com_1.value, com_2.value):
                    result_value.value = 1
            else:
                # Default to numeric comparison
                value_2: Union[float|int] = com_2.get_as_number()
                value_1: Union[float|int] = com_1.get_as_number()
                if isinstance(value_1, int) and isinstance(value_2, int):
                    if value_1 == value_2:
                        result_value.value = 1
                    elif math.isclose(value_1, value_2):
                        result_value.value = 1
            if result_value.value == 1:
                result_value.value = 0
            else:
                result_value.value = 1
            push(result_value)
        elif "VSET" == command.command:
            variable_table[command.arguments[0].value] = pop(command)
        elif "VGET" == command.command:
            if command.arguments[0].value in variable_table:
                push(variable_table[command.arguments[0].value])
            else:
                result_value = Value()
                result_value.value = None
                result_value.type = Types.NIL
                push(result_value)
        elif "JOIN" == command.command:
            value_2: str = pop(command).get_as_string()
            value_1: str = pop(command).get_as_string()
            print(f"Joining: '{value_1}' to '{value_2}'")
            result_value = Value()
            result_value.value = value_1 + value_2
            result_value.type = Types.TXT
            push(result_value)
        elif "SSTR" == command.command:  # SubSTRing
            idx_count: str = int(pop(command).get_as_number())
            idx_from: str = int(pop(command).get_as_number())
            val_str: str = pop(command).get_as_string()
            result_value = Value()
            result_value.type = Types.TXT
            if idx_from > 0:
                idx_from -= 1
            if idx_from >= len(val_str) or idx_count == 0:
                result_value.value = ""
                push(result_value)
            else:
                if idx_from < 0:
                    idx_from = len(val_str) + idx_from
                    if idx_from < 0:
                        idx_from = 0
                if idx_from + idx_count >= len(val_str):
                    idx_count = len(val_str) - idx_from
                result_value.value = val_str[idx_from:idx_from+idx_count]
                push(result_value)
        elif "JUMP" == command.command:
            pc = label_to_pc[command.arguments[0].value] - 1
        elif "CALL" == command.command:
            return_stack.append(pc)
            pc = label_to_pc[command.arguments[0].value] - 1
        elif "RTRN" == command.command:
            if not return_stack:
                nambly_error("Empty return stack.")
            pc = return_stack.pop()
        elif "JPIF" == command.command:  #JumP If False
            value_1 = pop(command)
            if value_1.type in (Types.INT, Types.FLO):
                if value_1.value == 0:
                    pc = label_to_pc[command.arguments[0].value] - 1
            elif value_1.type == Types.NIL:
                pc = label_to_pc[command.arguments[0].value] - 1
            elif value_1.type == Types.TAB:
                if len(value_1.value) == 0:
                    pc = label_to_pc[command.arguments[0].value] - 1
            elif value_1.type == Types.TXT:
                if len(value_1.value) == 0:
                    pc = label_to_pc[command.arguments[0].value] - 1
            else:
                nambly_error(f"Cannot check if value {value_1} of type {value_1.type} is false.")
        elif "TABL" == command.command:
            result_value = Value()
            result_value.value = {}
            result_value.type = Types.TAB
            push(result_value)
        elif "PSET" == command.command:
            value: Value = pop(command)
            index: Value = pop(command)
            table: Value = pop(command)
            table.value[index.get_as_string()] = value
        elif "ARRR" == command.command:  # ARRRay, like the pirates - Create array until NIL value is found
            array_name: str = command.arguments[0].value
            # Create array table
            result_value = Value()
            result_value.value = {}
            result_value.type = Types.TAB
            variable_table[array_name] = result_value
            array_values: List[Value] = []
            # Pop values until we find a nil
            while True:
                value: Value = pop(command)
                if value.type == Types.NIL:
                    break
                else:
                    array_values.append(value)
            # Add the values to the array in reverse order
            arr_index: int = 0
            for value in array_values:
                result_value.value[str(len(array_values) - arr_index)] = value
                arr_index += 1
            # Add size to array
            size_value = Value()
            size_value.value = arr_index
            size_value.type = Types.INT
            result_value.value["size"] = size_value
        elif "DUPL" == command.command:
            push(execution_stack[-1])
        elif "PGET" == command.command:
            index = pop(command)
            table = pop(command)
            index_value: str = index.get_as_string()
            if index_value in table.value:
                push(table.value[index_value])
            else:
                result_value = Value()
                result_value.value = None
                result_value.type = Types.NIL
                push(result_value)
        elif "PIST" == command.command:  # check if Position Is SeT
            index = pop(command)
            table = pop(command)
            result_value = Value()
            result_value.value = 0
            result_value.type = Types.INT
            if index.value in table.value:
                result_value.value = 1
            push(result_value)
        elif "DISP" == command.command:
            display(pop(command))
        elif "ACCP" == command.command:
            result_value = Value()
            result_value.value = input()
            result_value.type = Types.TXT
            push(result_value)
        elif "POPV" == command.command:
            pop(command)
        elif "EXIT" == command.command:
            exit(int(pop(command).get_as_number()))
        elif "UNST" == command.command:  #UNSeT
            del variable_table[command.arguments[0].value]
        elif "PUST" == command.command: #Position UnSeT
            index = pop(command)
            table = pop(command)
            if index.value in table.value:
                del table.value[index.value]
        elif "RFIL" == command.command: #Read FILe
            filename = pop(command)
            with open(filename.get_as_string(), "r") as file:
                result_value = Value()
                result_value.value = file.read()
                result_value.type = Types.TXT
                push(result_value)
        elif "FORW" == command.command: #File Open Read Write
            filename = pop(command)
            str_filename = filename.get_as_string()
            if str_filename in open_files:
                open_files[str_filename].close()
            file = open(str_filename, "r+")
            open_files[str_filename] = file
        elif "FORA" == command.command: #File Open Read Append
            filename = pop(command)
            str_filename = filename.get_as_string()
            if str_filename in open_files:
                open_files[str_filename].close()
            file = open(str_filename, "a+")
            open_files[str_filename] = file
        elif "FCLS" == command.command: #File CLoSe
            filename = pop(command)
            str_filename = filename.get_as_string()
            if str_filename in open_files:
                open_files[str_filename].close()
        elif "RLNE" == command.command: #Read LiNE
            filename = pop(command)
            str_filename = filename.get_as_string()
            if str_filename not in open_files:
                nambly_error(f"File '{str_filename}' is not open.")
            line: str = open_files[str_filename].readline()
            result_value = Value()
            result_value.value = line
            result_value.type = Types.TXT
            push(result_value)
        elif "LNOT" == command.command:  # Logic NOT
            com_1: Value = pop(command)
            result_value = Value()
            result_value.type = Types.INT
            result_value.value = 0
            if com_1.type == Types.NIL:
                result_value.value = 1
            elif com_1.type == Types.TAB:
                if len(com_1.value) > 0:
                    result_value.value = 0
                else:
                    result_value.value = 1
            elif com_1.type == Types.TXT:
                if len(com_1.value) > 0:
                    result_value.value = 0
                else:
                    result_value.value = 1
            elif com_1.type == Types.INT:
                if com_1.value == 0:
                    result_value.value = 1
                else:
                    result_value.value = 0
            elif com_1.type == Types.FLO:
                if math.isclose(com_1.value, 0):
                    result_value.value = 1
                else:
                    result_value.value = 0
            else:
                nambly_error(f"Unknown type: {com_1.type}")
            push(result_value)
        elif "TRIM" == command.command:
            string = pop(command)
            result_value = Value()
            result_value.value = string.get_as_string().strip()
            result_value.type = Types.TXT
            push(result_value)
        elif "SLEN" == command.command:  # String Length
            string = pop(command)
            result_value = Value()
            result_value.value = len(string.get_as_string())
            result_value.type = Types.INT
            push(result_value)
        elif "SWAP" == command.command:
            v2 = pop(command)
            v1 = pop(command)
            push(v2)
            push(v1)
        elif "LAND" == command.command:
            com_2: Value = pop(command)
            com_1: Value = pop(command)
            result_value = Value()
            result_value.type = Types.INT
            result_value.value = 0
            if is_true(com_1) and is_true(com_2):
                result_value.value = 1
            push(result_value)
        elif "ISNE" == command.command:
            com_2: Value = pop(command)
            com_1: Value = pop(command)
            result_value = Value()
            result_value.type = Types.INT
            result_value.value = 0
            if is_true(com_1) or is_true(com_2):
                result_value.value = 1
            push(result_value)
        elif "ISIN" == command.command:
            container: Value = pop(command)
            value: Value = pop(command)
            result_value = Value()
            result_value.type = Types.INT
            result_value.value = 0
            if container.type == Types.TAB:
                if value.get_as_string() in container.value:
                    result_value.value = 1
            else:
                if value.get_as_string() in container.get_as_string():
                    result_value.value = 1
            push(result_value)
        else:
            nambly_error(f"Unknown Nambly command: {command}")
        pc += 1

    
def is_true(value: Value) -> bool:
    if value.type == Types.TAB:
        if len(value.value) > 0: # By reference
            return True
    elif value.type == Types.TXT:
        if len(value.value) > 0:
            return True
    elif value.type == Types.INT:
        if value.value != 0:
            return True
    elif value.type == Types.FLO:
        if not math.isclose(value.value, 0):
            return True
    else:
        return False


def nari_run(code: str) -> None:
    """Executes a NariVM code.
    """
    debug: bool = not False
    sys.set_int_max_str_digits(1000000000)
    code_listing: List[Command] = split_lines(code)
    code_listing = generate_label_map(code_listing)
    execute_code_listing(code_listing)
    if debug:
        print_variable_table(variable_table)
        print_stack()
        print_return_stack()


if __name__ == "__main__":
    nari_run(code)