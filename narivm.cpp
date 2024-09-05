// -- NariVM for Katalyn in C++ by Lartu (25G24 00:17) --
// This is a naïve NariVM implementation that not only does not
// compile comands (it instead parses them every time), but it also
// does not efficiently handle variables or anything. The point of this
// NariVM version is to have a faster implementation than the Python
// one, where other smaller optimizations can be implemented until we
// can migrate NariVM to a better architecture.

#include <iostream>
#include <map>
#include <unordered_map>
#include <stack>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <thread>
#include <chrono>
#include <queue>
#include <fstream>
#include <set>
#include <sys/stat.h>
#include "tiny-process-library/process.hpp"
#include "linenoise.hpp"

using namespace std;
using namespace TinyProcessLib;

#define NUMB 1      // Numeric Value
#define TEXT 2      // String Value
#define TABLE 3     // Table Value
#define NIL 5       // Null Value
#define ITER 6      // Iterator Value
#define LISTLIMIT 7 // List Limit Value

size_t pc = 0; // Program Counter
void raise_nvm_error(string error_message);

extern const char __attribute__((weak)) user_code[] = "";

string get_type_name(char type)
{
    switch (type)
    {
    case NUMB:
        return "NUMBER";
    case TEXT:
        return "TEXT";
    case TABLE:
        return "TABLE";
    case NIL:
        return "NIL";
    case ITER:
        return "ITERATOR";
    case LISTLIMIT:
        return "LISTLIMIT";
    }
    return "???";
}

enum Opcode : uint8_t
{
    PUSH,
    PNIL,
    ADDV,
    SUBT,
    MULT,
    FDIV,
    IDIV,
    POWR,
    MODL,
    ISGT,
    ISLT,
    ISGE,
    ISLE,
    ISEQ, // Is Equal
    ISNE, // Is Not Equal
    VSET,
    GSET,
    VGET,
    JOIN,
    SSTR,
    REPL,
    JUMP,
    CALL,
    RTRN,
    JPIF, // Jump If False
    TABL,
    PSET,
    PGET,
    ARRR,
    DUPL,
    DISP,
    ACCP,
    POPV,
    EXIT,
    UNST,
    PUST,
    FORW, // File Open for Read and Write
    FORA, // File Open for Read and Append
    RFIL, // Read File
    FCLS, // File Close
    RLNE, // File Read Line
    FWRT, // File Write
    LNOT,
    LAND,
    LGOR,
    TRIM,
    SLEN, // String or Table Length
    SWAP,
    ISIN,
    FLOR,
    ADSC,
    DLSC,
    EXEC,
    WAIT,
    KEYS,
    GITR,
    NEXT,
    PLIM,
    EXPL, // Explode
    MXPL, // Multi eXPLode
    FORE, // File Open for REad
    ISOP, // IS file OPen?
    ISNIL,
};

Opcode opcode_from_string(string_view str)
{
    static unordered_map<string_view, Opcode> str_to_opcode = {
        {"PUSH", Opcode::PUSH},
        {"PNIL", Opcode::PNIL},
        {"ADDV", Opcode::ADDV},
        {"SUBT", Opcode::SUBT},
        {"MULT", Opcode::MULT},
        {"FDIV", Opcode::FDIV},
        {"IDIV", Opcode::IDIV},
        {"POWR", Opcode::POWR},
        {"MODL", Opcode::MODL},
        {"ISGT", Opcode::ISGT},
        {"ISLT", Opcode::ISLT},
        {"ISGE", Opcode::ISGE},
        {"ISLE", Opcode::ISLE},
        {"ISEQ", Opcode::ISEQ},
        {"ISNE", Opcode::ISNE},
        {"VSET", Opcode::VSET},
        {"GSET", Opcode::GSET},
        {"VGET", Opcode::VGET},
        {"JOIN", Opcode::JOIN},
        {"SSTR", Opcode::SSTR},
        {"REPL", Opcode::REPL},
        {"JUMP", Opcode::JUMP},
        {"CALL", Opcode::CALL},
        {"RTRN", Opcode::RTRN},
        {"JPIF", Opcode::JPIF},
        {"TABL", Opcode::TABL},
        {"PSET", Opcode::PSET},
        {"PGET", Opcode::PGET},
        {"ARRR", Opcode::ARRR},
        {"DUPL", Opcode::DUPL},
        {"DISP", Opcode::DISP},
        {"ACCP", Opcode::ACCP},
        {"POPV", Opcode::POPV},
        {"EXIT", Opcode::EXIT},
        {"UNST", Opcode::UNST},
        {"PUST", Opcode::PUST},
        {"FORW", Opcode::FORW},
        {"FORA", Opcode::FORA},
        {"RFIL", Opcode::RFIL},
        {"FCLS", Opcode::FCLS},
        {"RLNE", Opcode::RLNE},
        {"FWRT", Opcode::FWRT},
        {"LNOT", Opcode::LNOT},
        {"LAND", Opcode::LAND},
        {"LGOR", Opcode::LGOR},
        {"TRIM", Opcode::TRIM},
        {"SLEN", Opcode::SLEN},
        {"SWAP", Opcode::SWAP},
        {"ISIN", Opcode::ISIN},
        {"FLOR", Opcode::FLOR},
        {"ADSC", Opcode::ADSC},
        {"DLSC", Opcode::DLSC},
        {"EXEC", Opcode::EXEC},
        {"WAIT", Opcode::WAIT},
        {"KEYS", Opcode::KEYS},
        {"GITR", Opcode::GITR},
        {"NEXT", Opcode::NEXT},
        {"PLIM", Opcode::PLIM},
        {"EXPL", Opcode::EXPL},
        {"MXPL", Opcode::MXPL},
        {"FORE", Opcode::FORE},
        {"ISOP", Opcode::ISOP},
        {"NIL?", Opcode::ISNIL},
    };
    return str_to_opcode[str];
}

string wrap_text(const string &text, size_t maxLineLength)
{
    string result = "";
    size_t current_line_len = 0;

    for (size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] == ' ')
        {
            if (current_line_len >= maxLineLength)
            {
                result += "\n";
                current_line_len = 0;
            }
            else if (current_line_len > 0)
            {
                result += " ";
                current_line_len += 1;
            }
        }
        else
        {
            result += string() + text[i];
            current_line_len += 1;
            if (text[i] == '\n')
            {
                current_line_len = 0;
            }
        }
    }

    return result;
}

string_view opcode_as_string(Opcode opcode)
{
    switch (opcode)
    {
    case Opcode::PUSH:
        return "PUSH";
    case Opcode::PNIL:
        return "PNIL";
    case Opcode::ADDV:
        return "ADDV";
    case Opcode::SUBT:
        return "SUBT";
    case Opcode::MULT:
        return "MULT";
    case Opcode::FDIV:
        return "FDIV";
    case Opcode::IDIV:
        return "IDIV";
    case Opcode::POWR:
        return "POWR";
    case Opcode::MODL:
        return "MODL";
    case Opcode::ISGT:
        return "ISGT";
    case Opcode::ISLT:
        return "ISLT";
    case Opcode::ISGE:
        return "ISGE";
    case Opcode::ISLE:
        return "ISLE";
    case Opcode::ISEQ:
        return "ISEQ"; // Is Equal
    case Opcode::ISNE:
        return "ISNE"; // Is Not Equal
    case Opcode::VSET:
        return "VSET";
    case Opcode::GSET:
        return "GSET";
    case Opcode::VGET:
        return "VGET";
    case Opcode::JOIN:
        return "JOIN";
    case Opcode::SSTR:
        return "SSTR";
    case Opcode::REPL:
        return "REPL";
    case Opcode::JUMP:
        return "JUMP";
    case Opcode::CALL:
        return "CALL";
    case Opcode::RTRN:
        return "RTRN";
    case Opcode::JPIF:
        return "JPIF"; // Jump If False
    case Opcode::TABL:
        return "TABL";
    case Opcode::PSET:
        return "PSET";
    case Opcode::PGET:
        return "PGET";
    case Opcode::ARRR:
        return "ARRR";
    case Opcode::DUPL:
        return "DUPL";
    case Opcode::DISP:
        return "DISP";
    case Opcode::ACCP:
        return "ACCP";
    case Opcode::POPV:
        return "POPV";
    case Opcode::EXIT:
        return "EXIT";
    case Opcode::UNST:
        return "UNST";
    case Opcode::PUST:
        return "PUST";
    case Opcode::FORW:
        return "FORW"; // File Open for Read and Write
    case Opcode::FORA:
        return "FORA"; // File Open for Read and Append
    case Opcode::RFIL:
        return "RFIL"; // Read File
    case Opcode::FCLS:
        return "FCLS"; // File Close
    case Opcode::RLNE:
        return "RLNE"; // File Read Line
    case Opcode::FWRT:
        return "FWRT"; // File Write
    case Opcode::LNOT:
        return "LNOT";
    case Opcode::LAND:
        return "LAND";
    case Opcode::LGOR:
        return "LGOR";
    case Opcode::TRIM:
        return "TRIM";
    case Opcode::SLEN:
        return "SLEN"; // String or Table Length
    case Opcode::SWAP:
        return "SWAP";
    case Opcode::ISIN:
        return "ISIN";
    case Opcode::FLOR:
        return "FLOR";
    case Opcode::ADSC:
        return "ADSC";
    case Opcode::DLSC:
        return "DLSC";
    case Opcode::EXEC:
        return "EXEC";
    case Opcode::WAIT:
        return "WAIT";
    case Opcode::KEYS:
        return "KEYS";
    case Opcode::GITR:
        return "GITR";
    case Opcode::NEXT:
        return "NEXT";
    case Opcode::PLIM:
        return "PLIM";
    case Opcode::EXPL:
        return "EXPL";
    case Opcode::MXPL:
        return "MXPL";
    case Opcode::FORE:
        return "FORE";
    case Opcode::ISOP:
        return "ISOP";
    case Opcode::ISNIL:
        return "NIL?";
    default:
        raise_nvm_error("Unknown Nambly opcode");
    }
}

#define EPSILON 0.000000
bool num_eq(double a, double b)
{
    return fabs(a - b) < numeric_limits<double>::epsilon();
}

string double_to_string(double value)
{
    // Check if the value is effectively an integer
    if (num_eq(value, floor(value)))
    {
        return to_string(static_cast<long long>(value)); // Convert to integer string
    }
    else
    {
        // Otherwise, keep the precision for non-integers
        string str_rep = to_string(value);
        while (str_rep[str_rep.size() - 1] == '0' || str_rep[str_rep.size() - 1] == '.')
        {
            str_rep = str_rep.substr(0, str_rep.size() - 1);
        }
        return str_rep;
    }
}

class Value
{
private:
    char type;
    bool has_num_rep;
    bool has_str_rep;
    string str_rep;
    double num_rep;
    shared_ptr<map<string, Value>> table_rep;
    shared_ptr<queue<string> /**/> iterator_elements;

    void reset_values()
    {
        has_num_rep = false;
        has_str_rep = false;
        table_rep = nullptr;
    }

public:
    void set_string_value(const string &value)
    {
        reset_values();
        this->str_rep = value;
        this->type = TEXT;
        has_str_rep = true;
    }

    void set_number_value(double value)
    {
        reset_values();
        this->num_rep = value;
        this->type = NUMB;
        has_num_rep = true;
    }

    void set_table_value()
    {
        reset_values();
        this->table_rep = std::make_shared<map<string, Value>>();
        this->type = TABLE;
    }

    void set_nil_value()
    {
        reset_values();
        this->type = NIL;
    }

    void set_listlimit_value()
    {
        reset_values();
        this->type = LISTLIMIT;
    }

    void set_iterator_value()
    {
        reset_values();
        this->iterator_elements = std::make_shared<queue<string>>();
        this->type = ITER;
    }

    char get_type()
    {
        return type;
    }

    map<string, Value> *get_table()
    {
        return table_rep.get();
    }

    queue<string> *get_iterator_queue()
    {
        return iterator_elements.get();
    }

    const string &get_raw_string_value() const
    {
        // Gets the string value of the value, even if it wasn't set, used for arguments.
        return str_rep;
    }

    const string &get_as_string()
    {
        if (has_str_rep)
        {
            return str_rep;
        }
        else
        {
            if (type == NIL)
            {
                raise_nvm_error("Can't convert NIL value to string.");
            }
            else if (type == LISTLIMIT)
            {
                raise_nvm_error("Can't convert LISTLIMIT value to string.");
            }
            else if (type == ITER)
            {
                raise_nvm_error("Can't convert iterator value to string.");
            }
            else if (type == TABLE)
            {
                queue<string> table_values;
                for (auto it = get_table()->begin(); it != get_table()->end(); ++it)
                {
                    string table_string = "'" + it->first + "':";
                    if (it->second.get_type() == TEXT)
                    {
                        table_string += "'" + it->second.get_as_string() + "'";
                    }
                    else
                    {
                        table_string += it->second.get_as_string();
                    }
                    table_values.push(table_string);
                }
                string return_value = "[";
                while (!table_values.empty())
                {
                    return_value += table_values.front();
                    if (table_values.size() > 1)
                    {
                        return_value += ", ";
                    }
                    table_values.pop();
                }
                str_rep = return_value + "]";
            }
            else if (type == NUMB)
            {
                str_rep = double_to_string(get_as_number());
            }
            return str_rep;
        }
    }

    double get_as_number()
    {
        if (has_num_rep)
        {
            return num_rep;
        }
        else
        {
            if (type == NIL)
            {
                raise_nvm_error("Can't convert NIL value to number.");
            }
            else if (type == LISTLIMIT)
            {
                raise_nvm_error("Can't convert LISTLIMIT value to number.");
            }
            else if (type == ITER)
            {
                raise_nvm_error("Can't convert iterator value to number.");
            }
            else if (type == TABLE)
            {
                num_rep = table_rep != nullptr ? num_rep = table_rep->size() : 0;
            }
            else if (type == TEXT)
            {
                try
                {
                    num_rep = stod(str_rep);
                }
                catch (const invalid_argument &ia)
                {
                    raise_nvm_error("Can't convert value " + str_rep + " to number.");
                }
            }
            return num_rep;
        }
    }
};

class Command
{
private:
    Opcode opcode;
    vector<Value> arguments;
    const size_t line_number;
    const string filename;
    // Only for branch instructions
    size_t branch_target;

public:
    Command(const string command, const size_t line_number, const string filename) : opcode(opcode_from_string(command)), line_number(line_number), filename(filename) {};
    Opcode get_opcode() const { return opcode; }
    string_view get_command() const { return opcode_as_string(opcode); }
    const vector<Value> &get_arguments() const { return arguments; }
    void add_argument(const Value &value)
    {
        arguments.push_back(value);
    }
    const string get_debug_string() const
    {
        string debug_string = string(get_command());
        if (!arguments.empty())
        {
            debug_string += " ";
            for (size_t i = 0; i < arguments.size(); ++i)
            {
                debug_string += "[" + arguments[i].get_raw_string_value() + "]";
            }
        }
        return debug_string;
    }

    const size_t get_line_number() const
    {
        return line_number;
    }

    const string get_file() const
    {
        return filename;
    }

    // Use only for branch instructions
    size_t get_branch_target() const
    {
        return branch_target;
    }

    void set_branch_target(size_t pc)
    {
        branch_target = pc;
    }
};

Command *last_command = nullptr;

void raise_nvm_error(string error_message)
{
    cerr << endl
         << "====== Oh no! Runtime Error! ======" << endl;
    cerr << wrap_text(error_message, 70) << endl;
    if (last_command != nullptr)
    {
        cerr << endl
             << "--- Source File Information --- " << endl;
        cerr << "- Source File: " << last_command->get_file() << endl;
        cerr << "- Source Line: " << last_command->get_line_number() << endl;
    }
    cerr << endl
         << "--- NariVM State Information --- " << endl;
    cerr << "- PC: " << pc + 1 << endl;
    exit(1);
}

vector<map<string, Value> /**/> variable_tables;
map<string, size_t> label_to_pc;
map<size_t, string> pc_to_label;
stack<Value> execution_stack;
map<string, fstream *> open_files;
set<string> untruncated_files; // Garbage
set<string> read_only_files;   // Garbage
stack<size_t> return_stack;

Value get_nil_value()
{
    Value nil_value;
    nil_value.set_nil_value();
    return nil_value;
}

Value get_listlimit_value()
{
    Value ll_value;
    ll_value.set_listlimit_value();
    return ll_value;
}

void print_command_listing(vector<Command> &code_listing)
{
    // Prints a code listing to the console.
    for (size_t i = 0; i < code_listing.size(); ++i)
    {
        if (pc_to_label.count(i) > 0)
        {
            cout << "[" << pc_to_label[i] << "] => ";
        }
        cout << "(" << i + 1 << ") " << code_listing[i].get_debug_string() << endl;
    }
}

string trim(const string &str)
{
    const auto strBegin = str.find_first_not_of(" \t");
    if (strBegin == string::npos)
        return "";
    const auto strEnd = str.find_last_not_of(" \t");
    const auto strRange = strEnd - strBegin + 1;
    return str.substr(strBegin, strRange);
}

char get_token_type(const string &text)
{
    int begin = 0;
    bool found_period = false;

    if (text[0] == '"')
    {
        return TEXT; // Text type if the string starts with a quote
    }

    if (text[0] == '+' || text[0] == '-')
    {
        begin = 1; // Skip the sign if it's there
    }

    for (size_t i = begin; i < text.size(); ++i)
    {
        char ch = text[i];
        if (ch != '.' && !isdigit(ch))
        {
            return NIL; // Not a number if it contains invalid characters
        }

        if (ch == '.')
        {
            if (found_period)
            {
                return NIL; // Invalid if more than one period is found
            }
            found_period = true;
        }
    }
    return NUMB;
}

Command split_command_arguments(const string &line, const size_t full_line_number, const string &full_file_name)
{
    vector<string> tokens;
    string current_token;
    bool in_string = false;
    bool next_char_escaped = false;
    bool uppercase_on_append = false;
    for (char ch : line)
    {
        if (!in_string && isspace(ch))
        {
            if (!current_token.empty())
            {
                if (uppercase_on_append)
                {
                    uppercase_on_append = false;
                    transform(current_token.begin(), current_token.end(), current_token.begin(), ::toupper);
                }
                tokens.push_back(current_token);
            }
            current_token.clear();
        }
        else if (in_string && next_char_escaped)
        {
            next_char_escaped = false;
            if (ch == 'n')
            {
                current_token += '\n';
            }
            else if (ch == 't')
            {
                current_token += '\t';
            }
            else if (ch == 'r')
            {
                current_token += '\r';
            }
            else if (ch == 'b')
            {
                current_token += '\b';
            }
            else if (ch == 'f')
            {
                current_token += '\f';
            }
            else if (ch == 'v')
            {
                current_token += '\v';
            }
            else
            {
                current_token += ch;
            }
        }
        else if (in_string && ch == '\\')
        {
            next_char_escaped = true;
        }
        else if (ch == '"')
        {
            in_string = !in_string;
            current_token += ch;
        }
        else
        {
            if (current_token.empty() && ch == '@')
            {
                uppercase_on_append = true;
            }
            current_token += ch;
        }
    }

    if (!current_token.empty())
    {
        if (uppercase_on_append)
        {
            uppercase_on_append = false;
            transform(current_token.begin(), current_token.end(), current_token.begin(), ::toupper);
        }
        tokens.push_back(current_token);
    }

    if (in_string)
    {
        raise_nvm_error("Nambly parsing error: open string for line '" + line + "'");
    }

    Command new_command(tokens.empty() ? "" : tokens[0], full_line_number, full_file_name);

    for (size_t i = 1; i < tokens.size(); ++i)
    {
        Value value;
        value.set_string_value(tokens[i]);

        char token_type = get_token_type(value.get_as_string());

        if (token_type == TEXT)
        {
            value.set_string_value(tokens[i].substr(1, tokens[i].size() - 2)); // Remove surrounding quotes
        }
        else if (token_type == NUMB)
        {
            value.set_number_value(stod(tokens[i]));
        }
        new_command.add_argument(value);
    }

    return new_command;
}

vector<Command> generate_label_map_and_code_listing(const string &code)
{
    // Checks a code listing for labels and fills the label map with their PCs.
    // Then returns the code without those labels.
    size_t pc = 0;
    vector<Command> code_listing;
    stringstream ss(code);
    string line;
    size_t full_source_line_number = 0;
    string full_source_filename = "";
    while (getline(ss, line, '\n'))
    {
        line = trim(line);
        if (line.empty())
        {
            continue;
        }
        if (line[0] == '@')
        {
            int jmp_pc_value = pc;
            string label_name = line.substr(1);
            label_to_pc[label_name] = jmp_pc_value;
            pc_to_label[pc] = label_name;
            continue;
        }
        string sub = line.substr(0, 5);
        if (sub == ";line")
        {
            full_source_line_number = stol(trim(line.substr(5)));
            continue;
        }
        if (sub == ";file")
        {
            full_source_filename = trim(line.substr(5));
            continue;
        }
        if (line[0] != ';')
        {
            code_listing.push_back(split_command_arguments(line, full_source_line_number, full_source_filename));
            ++pc;
        }
    }
    for (auto &command: code_listing)
    {
        switch (command.get_opcode())
        {
        case Opcode::JUMP:
        case Opcode::JPIF:
        case Opcode::CALL:
            pc = label_to_pc[command.get_arguments()[0].get_raw_string_value()] - 1;
            command.set_branch_target(pc);
        default: break;
        }
    }
    return code_listing;
}

void push(Value v)
{
    execution_stack.push(std::move(v));
}

Value pop(Command &command)
{
    if (execution_stack.empty())
    {
        raise_nvm_error("Execution stack empty for command: " + command.get_debug_string());
    }
    auto v = std::move(execution_stack.top());
    execution_stack.pop();
    return v;
}

void add_scope()
{
    variable_tables.push_back(map<string, Value>());
}

void set_variable(const string &var_name, Value value)
{
    if (variable_tables.empty())
    {
        add_scope();
    }
    variable_tables[variable_tables.size() - 1][var_name] = std::move(value);
}

void delete_variable(const string &name)
{
    if (variable_tables.empty())
    {
        return; // If there are no scopes, nothing to delete
    }

    // Try to delete from the current (last) scope
    auto &current_scope = variable_tables.back(); // Access the last scope
    if (current_scope.find(name) != current_scope.end())
    {
        current_scope.erase(name); // Delete the variable from the current scope
        return;
    }

    // Try to delete from the global (first) scope
    auto &global_scope = variable_tables.front(); // Access the first scope
    if (global_scope.find(name) != global_scope.end())
    {
        global_scope.erase(name); // Delete the variable from the global scope
        return;
    }
}

void set_global_variable(const string &var_name, Value value)
{
    if (variable_tables.empty())
    {
        add_scope();
    }
    variable_tables[0][var_name] = value;
}

const Value &get_variable(const string &var_name)
{
    if (!variable_tables.empty())
    {
        // TODO esta busqueda es ineficiente para variables en el scope global porque busca dos veces
        if (variable_tables[variable_tables.size() - 1].count(var_name) > 0)
        {
            return variable_tables[variable_tables.size() - 1][var_name];
        }
        else if (variable_tables[0].count(var_name) > 0)
        {
            return variable_tables[0][var_name];
        }
    }
    static Value nil_value;
    nil_value.set_nil_value(); // Refresh the nil just in case
    return nil_value;
}

string substring(const string &s, long long from, long long count)
{
    long long len = s.size();

    // Handle negative indices
    if (from < 0)
    {
        from += len; // Negative index counts from the end
    }

    // Ensure `from` is within bounds
    if (from < 0)
    {
        from = 0;
    }
    else if (from >= len)
    {
        return ""; // If `from` is out of bounds, return an empty string
    }

    // Calculate the effective count
    if (count < 0)
    {
        count = 0; // Negative count is treated as zero (like Python does)
    }

    // Adjust `count` so that it doesn't go out of bounds
    count = min(count, len - from);

    // Return the substring
    return s.substr(from, count);
}

queue<string> split(const string &haystack, const string &delimiter, long long max_splits, bool add_empty)
{
    queue<string> result;
    size_t start = 0;
    size_t end = haystack.find(delimiter);
    long long splits_done = 0;

    while (end != string::npos && (max_splits == -1 || splits_done < max_splits))
    {
        string token = haystack.substr(start, end - start);

        if (!token.empty() || add_empty)
        {
            result.push(token);
        }

        start = end + delimiter.length();
        end = haystack.find(delimiter, start);
        ++splits_done;
    }

    // Add the final segment (or the whole string if no delimiter was found)
    string token = haystack.substr(start);
    if (!token.empty() || add_empty)
    {
        result.push(token);
    }

    return result;
}

// Helper function to find the earliest delimiter in the haystack
pair<size_t, string> find_next_delimiter(const string &haystack, const vector<string> &delimiters, size_t start_pos)
{
    size_t earliest_pos = string::npos;
    string found_delimiter;

    for (const auto &delimiter : delimiters)
    {
        size_t pos = haystack.find(delimiter, start_pos);
        if (pos < earliest_pos)
        {
            earliest_pos = pos;
            found_delimiter = delimiter;
        }
    }

    return make_pair(earliest_pos, found_delimiter);
}

queue<string> multisplit(const string &haystack, const vector<string> &delimiters, long long max_splits, bool add_empty)
{
    queue<string> result;
    size_t start = 0;
    long long splits_done = 0;

    // Main loop to split the string by any of the delimiters
    while (splits_done != max_splits)
    {
        pair<size_t, string> delimiter_info = find_next_delimiter(haystack, delimiters, start);
        size_t end = delimiter_info.first;
        string found_delimiter = delimiter_info.second;

        if (end == string::npos)
        {
            break; // No more delimiters found
        }

        string token = haystack.substr(start, end - start);

        if (!token.empty() || add_empty)
        {
            result.push(token);
        }

        start = end + found_delimiter.length();
        ++splits_done;
    }

    // Add the final segment (or the whole string if no delimiter was found)
    string token = haystack.substr(start);
    if (!token.empty() || add_empty)
    {
        result.push(token);
    }

    return result;
}

string input(const string& prompt)
{
    string user_input = "";
    //getline(cin, user_input);
    linenoise::SetHistoryMaxLen(20);
    // Constant Prompt
    linenoise::Readline(prompt.c_str(), user_input);
    linenoise::AddHistory(user_input.c_str());
    return user_input;
}

bool is_true(Value &value)
{
    if (value.get_type() == TABLE)
    {
        return value.get_table()->size() > 0;
    }
    else if (value.get_type() == TEXT)
    {
        return value.get_as_string().size() > 0;
    }
    else if (value.get_type() == NUMB)
    {
        return !num_eq(value.get_as_number(), 0);
    }
    return false;
}

void run_command(const string &command, string &stdout_str, string &stderr_str, int &return_code)
{
    /*namespace bp = boost::process;
    // Capture output and errors
    bp::ipstream stdout_stream;
    bp::ipstream stderr_stream;
    bp::child process(command, bp::std_out > stdout_stream, bp::std_err > stderr_stream, bp::shell);
    string line;
    // Read stdout
    while (getline(stdout_stream, line))
    {
        stdout_str += line + "\n";
    }
    // Read stderr
    while (getline(stderr_stream, line))
    {
        stderr_str += line + "\n";
    }
    // Wait for the process to finish and get the return code
    process.wait();
    return_code = process.exit_code();*/
    stdout_str = "";
    stderr_str = "";
    Process subprocess(
        command, "",
        [&stdout_str](const char *bytes, size_t n)
        {
            stdout_str += string(bytes, n);
        },
        [&stderr_str](const char *bytes, size_t n)
        {
            stderr_str += string(bytes, n);
        });
    return_code = subprocess.get_exit_status();
}

bool file_exists(const string &filename)
{
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
}

void create_empty_file(const string &filename)
{
    // Create an empty file
    ofstream file(filename.c_str());
    if (!file)
    {
        raise_nvm_error("Failed to create file " + filename + ".");
    }
}

// Helper function to check if a string is a valid number (integer or floating-point)
bool is_numeric(const string &s)
{
    istringstream iss(s);
    double d;
    char c;
    // Try to parse a double, and ensure that there is no remaining character after the number
    return iss >> d && !(iss >> c);
}

// Custom comparator function
bool sort_iterator_elements(const string &a, const string &b)
{
    bool a_is_numeric = is_numeric(a);
    bool b_is_numeric = is_numeric(b);

    if (a_is_numeric && b_is_numeric)
    {
        // If both are numeric, compare their double values
        return stod(a) < stod(b);
    }
    else if (a_is_numeric)
    {
        // If only a is numeric, it should come first
        return true;
    }
    else if (b_is_numeric)
    {
        // If only b is numeric, it should come first
        return false;
    }
    else
    {
        // If neither is numeric, compare lexicographically
        return a < b;
    }
}

void replace_all(string &str, const string &from, const string &to)
{
    if (from.empty())
    {
        return; // Avoid infinite loop if 'from' is an empty string
    }

    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Move past the replaced substring
    }
}

void execute_code_listing(vector<Command> &code_listing)
{
    pc = 0;
    while (pc < code_listing.size())
    {
        Command &command = code_listing[pc];
        last_command = &command;
        switch (command.get_opcode())
        {
        case Opcode::PUSH:
        {
            push(command.get_arguments()[0]);
            break;
        }
        case Opcode::PNIL:
        {
            push(get_nil_value());
            break;
        }
        case Opcode::PLIM:
        {
            push(get_listlimit_value());
            break;
        }
        case Opcode::ADDV:
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            Value result;
            result.set_number_value(v1.get_as_number() + v2.get_as_number());
            push(std::move(result));
            break;
        }
        case Opcode::SUBT:
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            Value result;
            result.set_number_value(v1.get_as_number() - v2.get_as_number());
            push(std::move(result));
            break;
        }
        case Opcode::MULT:
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            Value result;
            result.set_number_value(v1.get_as_number() * v2.get_as_number());
            push(std::move(result));
            break;
        }
        case Opcode::FDIV:
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            Value result;
            result.set_number_value(v1.get_as_number() / v2.get_as_number());
            push(std::move(result));
            break;
        }
        case Opcode::IDIV:
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            Value result;
            result.set_number_value(floor(v1.get_as_number() / v2.get_as_number()));
            push(std::move(result));
            break;
        }
        case Opcode::POWR:
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            Value result;
            result.set_number_value(pow(v1.get_as_number(), v2.get_as_number()));
            push(std::move(result));
            break;
        }
        case Opcode::MODL:
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            Value result;
            result.set_number_value((int)floor(v1.get_as_number()) % (int)floor(v2.get_as_number()));
            push(std::move(result));
            break;
        }
        case Opcode::ISGT:
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            Value result;
            result.set_number_value(v1.get_as_number() > v2.get_as_number() ? 1 : 0);
            push(std::move(result));
            break;
        }
        case Opcode::ISLT:
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            Value result;
            result.set_number_value(v1.get_as_number() < v2.get_as_number() ? 1 : 0);
            push(std::move(result));
            break;
        }
        case Opcode::ISGE:
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            Value result;
            result.set_number_value(v1.get_as_number() >= v2.get_as_number() ? 1 : 0);
            push(std::move(result));
            break;
        }
        case Opcode::ISLE:
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            Value result;
            result.set_number_value(v1.get_as_number() <= v2.get_as_number() ? 1 : 0);
            push(std::move(result));
            break;
        }
        case Opcode::ISEQ: // Is Equal
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            Value result;
            if (v1.get_type() == NIL || v2.get_type() == NIL)
            {
                result.set_number_value(0);
            }
            else if (v1.get_type() == TABLE && v2.get_type() == TABLE)
            {
                result.set_number_value(v1.get_table() == v2.get_table() ? 1 : 0); // Comparing pointers!
            }
            else if (v1.get_type() == TEXT && v2.get_type() == TEXT)
            {
                result.set_number_value(v1.get_as_string() == v2.get_as_string() ? 1 : 0);
            }
            else
            {
                result.set_number_value(v1.get_as_number() == v2.get_as_number() ? 1 : 0);
            }
            push(std::move(result));
            break;
        }
        case Opcode::ISNE: // Is Not Equal
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            Value result;
            if (v1.get_type() == NIL || v2.get_type() == NIL)
            {
                result.set_number_value(1);
            }
            else if (v1.get_type() == TABLE && v2.get_type() == TABLE)
            {
                result.set_number_value(v1.get_table() == v2.get_table() ? 0 : 1); // Comparing pointers!
            }
            else if (v1.get_type() == TEXT && v2.get_type() == TEXT)
            {
                result.set_number_value(v1.get_as_string() == v2.get_as_string() ? 0 : 1);
            }
            else
            {
                result.set_number_value(v1.get_as_number() == v2.get_as_number() ? 0 : 1);
            }
            push(std::move(result));
            break;
        }
        case Opcode::VSET:
        {
            Value value = pop(command);
            set_variable(command.get_arguments()[0].get_raw_string_value(), value);
            break;
        }
        case Opcode::GSET:
        {
            set_global_variable(command.get_arguments()[0].get_raw_string_value(), pop(command));
            break;
        }
        case Opcode::VGET:
        {
            push(get_variable(command.get_arguments()[0].get_raw_string_value()));
            break;
        }
        case Opcode::JOIN:
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            Value result;
            string join_result = v1.get_as_string() + v2.get_as_string();
            result.set_string_value(join_result);
            push(std::move(result));
            break;
        }
        case Opcode::SSTR:
        {
            long long idx_count = pop(command).get_as_number();
            long long idx_from = pop(command).get_as_number();
            string val_str = pop(command).get_as_string();
            Value result;
            if (idx_from > 0)
            {
                idx_from -= 1;
            }
            result.set_string_value(substring(val_str, idx_from, idx_count));
            push(std::move(result));
            break;
        }
        case Opcode::REPL: // Replace all instances
        {
            string replacement = pop(command).get_as_string();
            string needle = pop(command).get_as_string();
            string haystack = pop(command).get_as_string();
            Value result;
            replace_all(haystack, needle, replacement);
            result.set_string_value(haystack);
            push(std::move(result));
            break;
        }
        case Opcode::EXPL: // Explode
        {
            Value haystack = pop(command);
            Value delimiter = pop(command);
            Value max_splits = pop(command);
            Value add_empties = pop(command);
            queue<string> expl_results = split(haystack.get_as_string(), delimiter.get_as_string(), max_splits.get_as_number(), num_eq(add_empties.get_as_number(), 1));
            Value result;
            result.set_table_value();
            size_t index = 1;
            while (!expl_results.empty())
            {
                Value element;
                element.set_string_value(expl_results.front());
                expl_results.pop();
                (*result.get_table())[double_to_string(index)] = element;
                ++index;
            }
            push(std::move(result));
            break;
        }
        case Opcode::MXPL: // Multi eXPLode
        {
            Value haystack = pop(command);
            Value delimiters = pop(command);
            Value max_splits = pop(command);
            Value add_empties = pop(command);
            if (delimiters.get_type() != TABLE)
            {
                raise_nvm_error("Delimiters for a multiexplode must be a table.");
            }
            vector<string> delimiters_vector;
            for (auto it = delimiters.get_table()->begin(); it != delimiters.get_table()->end(); ++it)
            {
                delimiters_vector.push_back(it->second.get_as_string());
            }
            queue<string> expl_results = multisplit(haystack.get_as_string(), delimiters_vector, max_splits.get_as_number(), num_eq(add_empties.get_as_number(), 1));
            Value result;
            result.set_table_value();
            size_t index = 1;
            while (!expl_results.empty())
            {
                Value element;
                element.set_string_value(expl_results.front());
                expl_results.pop();
                (*result.get_table())[double_to_string(index)] = element;
                ++index;
            }
            push(std::move(result));
            break;
        }
        case Opcode::JUMP:
        {
            pc = command.get_branch_target();
            break;
        }
        case Opcode::CALL:
        {
            return_stack.push(pc);
            pc = command.get_branch_target();
            break;
        }
        case Opcode::RTRN:
        {
            if (return_stack.empty())
            {
                raise_nvm_error("Empty return stack");
            }
            else
            {
                pc = return_stack.top();
                return_stack.pop();
            }
            break;
        }
        case Opcode::JPIF: // Jump If False
        {
            Value value = pop(command);
            if (value.get_type() == NUMB)
            {
                if (num_eq(value.get_as_number(), 0))
                {
                    pc = command.get_branch_target();
                }
            }
            else if (value.get_type() == NIL)
            {
                pc = command.get_branch_target();
            }
            else if (value.get_type() == TABLE)
            {
                if ((*value.get_table()).size() == 0)
                {
                    pc = command.get_branch_target();
                }
            }
            else if (value.get_type() == TEXT)
            {
                if (value.get_as_string().empty())
                {
                    pc = command.get_branch_target();
                }
            }
            else
            {
                raise_nvm_error("Values of type " + get_type_name(value.get_type()) + " are not logical.");
            }
            break;
        }
        case Opcode::TABL:
        {
            Value value;
            value.set_table_value();
            push(value);
            break;
        }
        case Opcode::PSET:
        {
            Value value = pop(command);
            Value index = pop(command);
            Value table = pop(command);
            (*table.get_table())[index.get_as_string()] = value;
            break;
        }
        case Opcode::PGET:
        {
            Value index = pop(command);
            Value table = pop(command);
            string index_string = index.get_as_string();
            if (table.get_type() == TABLE)
            {
                auto it = table.get_table()->find(index_string);
                if (it != table.get_table()->end())
                {
                    push(it->second);
                }
                else
                {
                    push(get_nil_value());
                }
            }
            else if (table.get_type() == TEXT || table.get_type() == NUMB)
            {
                if (!num_eq(index.get_as_number(), floor(index.get_as_number())))
                {
                    raise_nvm_error("Cannot index scalars with non-integer indexes.");
                }
                else
                {
                    Value result;
                    size_t idx = floor(index.get_as_number());
                    if (idx > 0)
                    {
                        idx -= 1;
                    }
                    if (idx >= table.get_as_string().size())
                    {
                        result.set_string_value("");
                        push(std::move(result));
                    }
                    if (idx < 0)
                    {
                        idx = table.get_as_string().size() + idx;
                    }
                    if (idx < 0)
                    {
                        result.set_string_value("");
                        push(std::move(result));
                    }
                    result.set_string_value(table.get_as_string().substr(idx, 1));
                    push(std::move(result));
                }
            }
            else
            {
                raise_nvm_error("Cannot index a " + get_type_name(table.get_type()) + " value.");
            }
            break;
        }
        case Opcode::ARRR:
        {
            Value result;
            result.set_table_value();
            stack<Value> values;
            // Pop values until we find a nil
            while (true)
            {
                Value v = pop(command);
                if (v.get_type() == LISTLIMIT)
                {
                    break;
                }
                else if (v.get_type() == NIL || v.get_type() == ITER)
                {
                    raise_nvm_error("Cannot add a " + get_type_name(v.get_type()) + " value to a table. This error may trigger if you are passing a nil value to a function or a table constructor.");
                }
                else
                {
                    values.push(v);
                }
            }
            // Add the values to the array in reverse order
            size_t array_index = 1;
            while (!values.empty())
            {
                (*result.get_table())[double_to_string(array_index)] = values.top();
                values.pop();
                array_index += 1;
            }
            push(std::move(result));
            break;
        }
        case Opcode::DUPL:
        {
            push(execution_stack.top());
            break;
        }
        case Opcode::ISNIL:
        {
            Value v1 = pop(command);
            Value result;
            result.set_number_value(v1.get_type() == NIL ? 1 : 0);
            push(std::move(result));
            break;
        }
        case Opcode::DISP:
        {
            cout << pop(command).get_as_string() << flush;
            break;
        }
        case Opcode::ACCP:
        {
            Value prompt = pop(command);
            Value result;
            result.set_string_value(input(prompt.get_as_string()));
            push(std::move(result));
            break;
        }
        case Opcode::POPV:
        {
            if (!execution_stack.empty())
            {
                pop(command);
            }
            break;
        }
        case Opcode::EXIT:
        {
            exit((int)floor(pop(command).get_as_number()));
            break;
        }
        case Opcode::UNST:
        {
            delete_variable(command.get_arguments()[0].get_raw_string_value());
            break;
        }
        case Opcode::PUST:
        {
            string index = pop(command).get_as_string();
            Value table = pop(command);
            if (table.get_type() != TABLE)
            {
                raise_nvm_error("Trying to PUST from a non-table.");
            }
            else
            {
                if (table.get_table()->count(index) > 0)
                {
                    table.get_table()->erase(index);
                }
            }
            break;
        }
        case Opcode::FORW: // File Open for Read and Write
        {
            Value filename = pop(command);
            string str_filename = filename.get_as_string();
            // Close the file if it was already open
            if (open_files.count(str_filename) > 0)
            {
                open_files[str_filename]->close();
                delete (open_files[str_filename]);
                open_files.erase(str_filename);
                if (untruncated_files.count(str_filename) > 0)
                    untruncated_files.erase(str_filename);
                if (read_only_files.count(str_filename) > 0)
                    read_only_files.erase(str_filename);
            }
            // Open the file
            fstream *new_file;
            bool requires_truncating = false;
            if (file_exists(str_filename))
            {
                new_file = new fstream(str_filename, ios::in | ios::out);
                requires_truncating = true;
            }
            else
            {
                new_file = new fstream(str_filename, ios::in | ios::out | ios::trunc);
            }
            if (*new_file)
            {
                open_files[str_filename] = new_file;
                if (requires_truncating)
                    untruncated_files.insert(str_filename);
            }
            // The name of the file is returned, wether it's been opened correctly or not,
            // as the file might not already exist.
            break;
        }
        case Opcode::FORA: // File Open for Read and Append
        {
            Value filename = pop(command);
            string str_filename = filename.get_as_string();
            // Close the file if it was already open
            if (open_files.count(str_filename) > 0)
            {
                open_files[str_filename]->close();
                delete (open_files[str_filename]);
                open_files.erase(str_filename);
                if (untruncated_files.count(str_filename) > 0)
                    untruncated_files.erase(str_filename);
                if (read_only_files.count(str_filename) > 0)
                    read_only_files.erase(str_filename);
            }
            // Open the file
            fstream *new_file;
            if (file_exists(str_filename))
            {
                new_file = new fstream(str_filename, ios::in | ios::out | ios::app);
            }
            else
            {
                new_file = new fstream(str_filename, ios::in | ios::out | ios::trunc);
            }
            if (*new_file)
            {
                open_files[str_filename] = new_file;
            }
            // The name of the file is returned, wether it's been opened correctly or not,
            // as the file might not already exist.
            break;
        }
        case Opcode::FORE: // File Open for REad
        {
            Value filename = pop(command);
            string str_filename = filename.get_as_string();
            // Close the file if it was already open
            if (open_files.count(str_filename) > 0)
            {
                open_files[str_filename]->close();
                delete (open_files[str_filename]);
                open_files.erase(str_filename);
                if (untruncated_files.count(str_filename) > 0)
                    untruncated_files.erase(str_filename);
                if (read_only_files.count(str_filename) > 0)
                    read_only_files.erase(str_filename);
            }
            // Open the file
            fstream *new_file = new fstream(str_filename, ios::in);
            if (*new_file)
            {
                open_files[str_filename] = new_file;
                read_only_files.insert(str_filename);
            }
            // The name of the file is returned, wether it's been opened correctly or not,
            // as the file might not already exist.
            break;
        }
        case Opcode::RFIL: // Read File
        {
            Value filename = pop(command);
            string str_filename = filename.get_as_string();
            if (open_files.count(str_filename) == 0 || !open_files[str_filename]->is_open())
            {
                push(get_nil_value());
            }
            else
            {
                string file_contents = "";
                string line;
                fstream *file = open_files[str_filename];
                if (!file->is_open())
                {
                    raise_nvm_error("The file " + str_filename + " has been closed.");
                }
                open_files[str_filename]->clear(); // Clear EOF flag before reading
                file->seekg(0);                    // I always want to read from the start
                while (getline(*file, line))
                {
                    if (!file_contents.empty())
                    {
                        file_contents += "\n";
                    }
                    file_contents += line;
                }
                Value result;
                result.set_string_value(file_contents);
                push(std::move(result));
            }
            break;
        }
        case Opcode::FCLS: // File Close
        {
            Value filename = pop(command);
            string str_filename = filename.get_as_string();
            if (open_files.count(str_filename) > 0)
            {
                open_files[str_filename]->close();
                delete (open_files[str_filename]);
                open_files.erase(str_filename);
                if (untruncated_files.count(str_filename) > 0)
                    untruncated_files.erase(str_filename);
                if (read_only_files.count(str_filename) > 0)
                    read_only_files.erase(str_filename);
            }
            break;
        }
        case Opcode::ISOP: // IS file OPen?
        {
            Value filename = pop(command);
            string str_filename = filename.get_as_string();
            Value result;
            result.set_number_value(open_files.count(str_filename) == 0 || !open_files[str_filename]->is_open() ? 0 : 1);
            push(result);
            break;
        }
        case Opcode::RLNE: // File Read Line
        {
            Value filename = pop(command);
            string str_filename = filename.get_as_string();
            if (open_files.count(str_filename) == 0 || !open_files[str_filename]->is_open())
            {
                push(get_nil_value());
            }
            else
            {
                string line;
                fstream *file = open_files[str_filename];
                if (!file->is_open())
                {
                    raise_nvm_error("The file " + str_filename + " has been closed.");
                }
                if (getline(*file, line))
                {
                    Value result;
                    result.set_string_value(line);
                    push(std::move(result));
                }
                else
                {
                    push(get_nil_value());
                }
            }
            break;
        }
        case Opcode::FWRT: // File Write
        {
            Value filename = pop(command);
            string str_filename = filename.get_as_string();
            if (open_files.count(str_filename) == 0 || read_only_files.count(str_filename) > 0)
            {
                raise_nvm_error("The file " + str_filename + " is not open for writing.");
            }
            else
            {
                if (untruncated_files.count(str_filename) > 0)
                {
                    // Truncate file first
                    open_files[str_filename]->close();
                    delete (open_files[str_filename]);
                    open_files.erase(str_filename);
                    if (untruncated_files.count(str_filename) > 0)
                        untruncated_files.erase(str_filename);
                    if (read_only_files.count(str_filename) > 0)
                        read_only_files.erase(str_filename);
                    open_files[str_filename] = new fstream(str_filename, ios::in | ios::out | ios::trunc);
                }
                Value contents = pop(command);
                open_files[str_filename]->clear();                 // Clear EOF flag before writing
                open_files[str_filename]->seekp(0, std::ios::end); // I always want to write from the end
                (*open_files[str_filename]) << contents.get_as_string() << flush;
            }
            break;
        }
        case Opcode::LNOT:
        {
            Value value = pop(command);
            Value result;
            if (value.get_type() == TABLE)
            {
                result.set_number_value((*value.get_table()).size() > 0 ? 0 : 1);
            }
            else if (value.get_type() == TEXT)
            {
                result.set_number_value(value.get_as_string().size() > 0 ? 0 : 1);
            }
            else if (value.get_type() == NUMB)
            {
                result.set_number_value(num_eq(value.get_as_number(), 0) ? 1 : 0);
            }
            else
            {
                raise_nvm_error("Values of type " + get_type_name(value.get_type()) + " are not logical.");
            }
            push(std::move(result));
            break;
        }
        case Opcode::LAND:
        {
            Value com_2 = pop(command);
            Value com_1 = pop(command);
            Value result;
            result.set_number_value(is_true(com_1) && is_true(com_2) ? 1 : 0);
            push(std::move(result));
            break;
        }
        case Opcode::LGOR:
        {
            Value com_2 = pop(command);
            Value com_1 = pop(command);
            Value result;
            result.set_number_value(is_true(com_1) || is_true(com_2) ? 1 : 0);
            push(std::move(result));
            break;
        }
        case Opcode::TRIM:
        {
            Value result;
            result.set_string_value(trim(pop(command).get_as_string()));
            push(std::move(result));
            break;
        }
        case Opcode::SLEN: // String or Table Length
        {
            Value value = pop(command);
            Value result;
            if (value.get_type() == TABLE)
            {
                result.set_number_value((*value.get_table()).size());
            }
            else if (value.get_type() == TEXT || value.get_type() == NUMB)
            {
                result.set_number_value(value.get_as_string().size()); // No es Unicode friendly esto! TODO
            }
            else
            {
                raise_nvm_error("Values of type " + get_type_name(value.get_type()) + " don't have a size.");
            }
            push(std::move(result));
            break;
        }
        case Opcode::SWAP:
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            push(v2);
            push(v1);
            break;
        }
        case Opcode::ISIN:
        {
            Value container = pop(command);
            Value value = pop(command);
            Value result;
            if (container.get_type() == TABLE)
            {
                result.set_number_value((*container.get_table()).count(value.get_as_string()) > 0 ? 1 : 0);
            }
            else
            {
                size_t pos = container.get_as_string().find(value.get_as_string());
                if (pos != string::npos)
                {
                    result.set_number_value(1);
                }
                else
                {
                    result.set_number_value(0);
                }
            }
            push(std::move(result));
            break;
        }
        case Opcode::FLOR:
        {
            Value result;
            result.set_number_value(floor(pop(command).get_as_number()));
            push(std::move(result));
            break;
        }
        case Opcode::ADSC:
        {
            add_scope();
            break;
        }
        case Opcode::DLSC:
        {
            if (variable_tables.empty())
            {
                raise_nvm_error("No scopes left.");
            }
            else
            {
                variable_tables.pop_back();
            }
            break;
        }
        case Opcode::EXEC:
        {
            Value exec_command = pop(command);
            string stdout_str;
            string stderr_str;
            int return_code;
            run_command(exec_command.get_as_string(), stdout_str, stderr_str, return_code);
            Value exit_code_value;
            exit_code_value.set_number_value(return_code);
            Value stderr_value;
            stderr_value.set_string_value(stderr_str);
            Value stdout_value;
            stdout_value.set_string_value(stdout_str);
            push(exit_code_value);
            push(stderr_value);
            push(stdout_value);
            break;
        }
        case Opcode::WAIT:
        {
            this_thread::sleep_for(chrono::microseconds((int)floor(pop(command).get_as_number() * 1000000)));
            break;
        }
        case Opcode::KEYS:
        {
            Value value = pop(command);
            if (value.get_type() != TABLE)
            {
                raise_nvm_error("Cannot get keys from a non-table value.");
            }
            else
            {
                Value result;
                result.set_table_value();
                size_t index = 1;
                for (auto it = value.get_table()->begin(); it != value.get_table()->end(); ++it)
                {
                    // Idea: I can use it->second here to add the values as well to the table maybe
                    Value key;
                    key.set_string_value(it->first);
                    (*value.get_table())[double_to_string(index)] = key;
                    ++index;
                }
                push(std::move(result));
            }
            break;
        }
        case Opcode::GITR:
        {
            Value container = pop(command);
            Value result;
            result.set_iterator_value();
            if (container.get_type() == TABLE)
            {
                vector<string> dict_keys;
                for (auto it = container.get_table()->begin(); it != container.get_table()->end(); ++it)
                {
                    dict_keys.push_back(it->first);
                }
                // Sort keys by numeric value first and then by lexicographical order
                sort(dict_keys.begin(), dict_keys.end(), sort_iterator_elements);
                // Add keys to queue
                for (auto it = dict_keys.begin(); it != dict_keys.end(); ++it)
                {
                    result.get_iterator_queue()->push(*it);
                }
            }
            else if (container.get_type() == TEXT || container.get_type() == NUMB)
            {
                for (size_t i = 0; i < container.get_as_string().size(); ++i)
                {
                    string character = double_to_string(i + 1);
                    result.get_iterator_queue()->push(character);
                }
            }
            else
            {
                raise_nvm_error("Cannot iterate over non-iterable value.");
            }
            push(std::move(result));
            break;
        }
        case Opcode::NEXT:
        {
            string iterator_name = command.get_arguments()[0].get_raw_string_value();
            Value iterator_variable = get_variable(iterator_name);
            if (iterator_variable.get_type() == NIL)
            {
                raise_nvm_error("Iterator " + iterator_name + " doesn't exist.");
            }
            else if (iterator_variable.get_type() != ITER)
            {
                raise_nvm_error("Cannot NEXT a non-interator.");
            }
            else
            {
                Value result;
                if (!iterator_variable.get_iterator_queue()->empty())
                {
                    result.set_string_value(iterator_variable.get_iterator_queue()->front());
                    iterator_variable.get_iterator_queue()->pop();
                }
                else
                {
                    result.set_nil_value();
                }
                push(std::move(result));
            }
            break;
        }
        default:
            raise_nvm_error("Unknown Nambly command: " + command.get_debug_string());
        }
        ++pc;
    }
}

void read_source(string filename, string &output)
{
    fstream file(filename, ios::in);
    if (!file.is_open())
    {
        cerr << "File not found: " << filename << endl;
        exit(1);
    }
    output = "";
    string line;
    while (getline(file, line))
    {
        if (!output.empty())
        {
            output += "\n";
        }
        output += line;
    }
}

int main(int argc, char *argv[])
{
    string code = user_code;
    if (code.empty())
    {
        vector<string> args(argv, argv + argc);
        if (args.size() != 2)
        {
            cerr << "Usage: narivm <nambly_file>" << endl;
            exit(1);
        }
        read_source(argv[1], code);
    }
    // cout << code << endl;
    vector<Command> code_listing = generate_label_map_and_code_listing(code);
    // print_command_listing(code_listing);
    execute_code_listing(code_listing);
    return 0;
}
