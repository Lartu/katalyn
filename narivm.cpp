// -- NariVM for Katalyn in C++ by Lartu (25G24 00:17) --
// This is a naïve NariVM implementation that not only does not
// compile comands (it instead parses them every time), but it also
// does not efficiently handle variables or anything. The point of this
// NariVM version is to have a faster implementation than the Python
// one, where other smaller optimizations can be implemented until we
// can migrate NariVM to a better architecture.

#include <iostream>
#include <map>
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
#include <sys/stat.h>
#include <boost/process.hpp>

using namespace std;

#define NUMB 1  // Numeric Value
#define TEXT 2  // String Value
#define TABLE 3 // Table Value
#define NIL 5   // Null Value
#define ITER 6  // Iterator Value

size_t pc = 0; // Program Counter

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
    }
    return "???";
}

void raise_nvm_error(string error_message)
{
    cerr << "NariVM Execution Error" << endl;
    cerr << error_message << endl;
    cerr << "PC: " << pc + 1 << endl;
    exit(1);
}

string double_to_string(double value)
{
    // Check if the value is effectively an integer
    if (value == floor(value))
    {
        return to_string(static_cast<long long>(value)); // Convert to integer string
    }
    else
    {
        // Otherwise, keep the precision for non-integers
        return to_string(value);
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
    map<string, Value> *table_rep; // TODO: this is probably a huge memory leak lol
    queue<string> *iterator_elements;

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
        this->table_rep = new map<string, Value>();
        this->type = TABLE;
    }

    void set_nil_value()
    {
        reset_values();
        this->type = NIL;
    }

    void set_iterator_value()
    {
        reset_values();
        this->iterator_elements = new queue<string>();
        this->type = ITER;
    }

    char get_type()
    {
        return type;
    }

    map<string, Value> *get_table()
    {
        return table_rep;
    }

    queue<string> *get_iterator_queue()
    {
        return iterator_elements;
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
            else if (type == ITER)
            {
                raise_nvm_error("Can't convert iterator value to string.");
            }
            else if (type == TABLE)
            {
                queue<string> table_values;
                for (auto it = get_table()->begin(); it != get_table()->end(); ++it)
                {
                    table_values.push("'" + it->first + "': '" + it->second.get_as_string() + "'");
                }
                string return_value = "[";
                while(!table_values.empty())
                {
                    return_value += table_values.front();
                    if(table_values.size() > 1)
                    {
                        return_value += ", ";
                    }
                    table_values.pop();
                }
                str_rep = return_value;
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
    const string command;
    vector<Value> arguments;

public:
    Command(const string command) : command(command) {};
    const string &get_command() const { return command; }
    const vector<Value> &get_arguments() const { return arguments; }
    void add_argument(const Value &value)
    {
        arguments.push_back(value);
    }
    const string get_debug_string() const
    {
        string debug_string = get_command();
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
};

vector<map<string, Value> /**/> variable_tables;
map<string, size_t> label_to_pc;
map<size_t, string> pc_to_label;
stack<Value> execution_stack;
map<string, fstream *> open_files;
stack<size_t> return_stack;

Value get_nil_value()
{
    Value nil_value;
    nil_value.set_nil_value();
    return nil_value;
}

#define EPSILON 0.000000
bool num_eq(double a, double b)
{
    return fabs(a - b) < numeric_limits<double>::epsilon();
}

vector<Command> generate_label_map(const vector<Command> &code_listing)
{
    // Checks a code listing for labels and fills the label map with their PCs.
    // Then returns the code without those labels.
    size_t index = 0;
    size_t pc = 0;
    vector<Command> new_listing;
    while (index < code_listing.size())
    {
        if (code_listing[index].get_command()[0] == '@')
        {
            int jmp_pc_value = pc;
            string label_name = code_listing[index].get_command().substr(1);
            label_to_pc[label_name] = jmp_pc_value;
            pc_to_label[pc] = label_name;
        }
        else
        {
            new_listing.push_back(code_listing[index]);
            ++pc;
        }
        ++index;
    }
    return new_listing;
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

Command split_command_arguments(const string &line)
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

    Command new_command(tokens.empty() ? "" : tokens[0]);

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

vector<Command> split_lines(const string &code)
{
    vector<Command> code_listing;
    stringstream ss(code);
    string line;
    while (getline(ss, line, '\n'))
    {
        line = trim(line);
        if (!line.empty())
        {
            if (line[0] != ';')
            {
                code_listing.push_back(split_command_arguments(line));
            }
        }
    }
    return code_listing;
}

void push(Value v)
{
    execution_stack.push(v);
}

Value pop(Command &command)
{
    if (execution_stack.empty())
    {
        raise_nvm_error("Execution stack empty for command: " + command.get_debug_string());
    }
    Value v = execution_stack.top();
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
    variable_tables[variable_tables.size() - 1][var_name] = value;
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

Value get_variable(const string &var_name)
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
    return get_nil_value();
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

string input()
{
    string user_input;
    getline(cin, user_input);
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
    namespace bp = boost::process;
    cout << "Running command " << command << endl;
    // Capture output and errors
    bp::ipstream stdout_stream;
    bp::ipstream stderr_stream;
    bp::child process(command, bp::std_out > stdout_stream, bp::std_err > stderr_stream, bp::shell);
    std::string line;
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
    return_code = process.exit_code();
}

bool file_exists(const string& filename) {
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
}

void create_empty_file(const string& filename) {
    // Create an empty file
    ofstream file(filename.c_str());
    if (!file) {
        raise_nvm_error("Failed to create file " + filename + ".");
    }
}

void execute_code_listing(vector<Command> &code_listing)
{
    pc = 0;
    while (pc < code_listing.size())
    {
        Command &command = code_listing[pc];
        if (command.get_command() == "PUSH")
        {
            push(command.get_arguments()[0]);
        }
        else if (command.get_command() == "PNIL")
        {
            push(get_nil_value());
        }
        else if (command.get_command() == "ADDV")
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            Value result;
            result.set_number_value(v1.get_as_number() + v2.get_as_number());
            push(result);
        }
        else if (command.get_command() == "SUBT")
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            Value result;
            result.set_number_value(v1.get_as_number() - v2.get_as_number());
            push(result);
        }
        else if (command.get_command() == "MULT")
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            Value result;
            result.set_number_value(v1.get_as_number() * v2.get_as_number());
            push(result);
        }
        else if (command.get_command() == "FDIV")
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            Value result;
            result.set_number_value(v1.get_as_number() / v2.get_as_number());
            push(result);
        }
        else if (command.get_command() == "IDIV")
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            Value result;
            result.set_number_value(floor(v1.get_as_number() / v2.get_as_number()));
            push(result);
        }
        else if (command.get_command() == "POWR")
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            Value result;
            result.set_number_value(pow(v1.get_as_number(), v2.get_as_number()));
            push(result);
        }
        else if (command.get_command() == "MODL")
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            Value result;
            result.set_number_value((int)floor(v1.get_as_number()) % (int)floor(v2.get_as_number()));
            push(result);
        }
        else if (command.get_command() == "ISGT")
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            Value result;
            result.set_number_value(v1.get_as_number() > v2.get_as_number() ? 1 : 0);
            push(result);
        }
        else if (command.get_command() == "ISLT")
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            Value result;
            result.set_number_value(v1.get_as_number() < v2.get_as_number() ? 1 : 0);
            push(result);
        }
        else if (command.get_command() == "ISGE")
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            Value result;
            result.set_number_value(v1.get_as_number() >= v2.get_as_number() ? 1 : 0);
            push(result);
        }
        else if (command.get_command() == "ISLE")
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            Value result;
            result.set_number_value(v1.get_as_number() <= v2.get_as_number() ? 1 : 0);
            push(result);
        }
        else if (command.get_command() == "ISEQ") // Is Equal
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
            push(result);
        }
        else if (command.get_command() == "ISNE") // Is Not Equal
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
            push(result);
        }
        else if (command.get_command() == "VSET")
        {
            Value value = pop(command);
            set_variable(command.get_arguments()[0].get_raw_string_value(), value);
        }
        else if (command.get_command() == "GSET")
        {
            set_global_variable(command.get_arguments()[0].get_raw_string_value(), pop(command));
        }
        else if (command.get_command() == "VGET")
        {
            push(get_variable(command.get_arguments()[0].get_raw_string_value()));
        }
        else if (command.get_command() == "JOIN")
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            Value result;
            string join_result = v1.get_as_string() + v2.get_as_string();
            result.set_string_value(join_result);
            push(result);
        }
        else if (command.get_command() == "SSTR")
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
            push(result);
        }
        else if (command.get_command() == "JUMP")
        {
            pc = label_to_pc[command.get_arguments()[0].get_raw_string_value()] - 1;
        }
        else if (command.get_command() == "CALL")
        {
            return_stack.push(pc);
            pc = label_to_pc[command.get_arguments()[0].get_raw_string_value()] - 1;
        }
        else if (command.get_command() == "RTRN")
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
        }
        else if (command.get_command() == "JPIF") // Jump If False
        {
            Value value = pop(command);
            if (value.get_type() == NUMB)
            {
                if (num_eq(value.get_as_number(), 0))
                {
                    pc = label_to_pc[command.get_arguments()[0].get_raw_string_value()] - 1;
                }
            }
            else if (value.get_type() == NIL)
            {
                pc = label_to_pc[command.get_arguments()[0].get_raw_string_value()] - 1;
            }
            else if (value.get_type() == TABLE)
            {
                if ((*value.get_table()).size() == 0)
                {
                    pc = label_to_pc[command.get_arguments()[0].get_raw_string_value()] - 1;
                }
            }
            else if (value.get_type() == TEXT)
            {
                if (value.get_as_string().empty())
                {
                    pc = label_to_pc[command.get_arguments()[0].get_raw_string_value()] - 1;
                }
            }
            else
            {
                raise_nvm_error("Values of type " + get_type_name(value.get_type()) + " are not logical.");
            }
        }
        else if (command.get_command() == "TABL")
        {
            Value value;
            value.set_table_value();
            push(value);
        }
        else if (command.get_command() == "PSET")
        {
            Value value = pop(command);
            Value index = pop(command);
            Value table = pop(command);
            (*table.get_table())[index.get_as_string()] = value;
        }
        else if (command.get_command() == "PGET")
        {
            Value index = pop(command);
            Value table = pop(command);
            string index_string = index.get_as_string();
            if (table.get_type() == TABLE)
            {
                if ((*table.get_table()).count(index_string)) // TODO usar count no es eficiente at all porque no para al encontrarlo
                {
                    push((*table.get_table())[index_string]);
                }
                else
                {
                    push(get_nil_value());
                }
            }
            else if (table.get_type() == NIL)
            {
                raise_nvm_error("Cannot index a nil value.");
            }
            else
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
                        push(result);
                    }
                    if (idx < 0)
                    {
                        idx = table.get_as_string().size() + idx;
                    }
                    if (idx < 0)
                    {
                        result.set_string_value("");
                        push(result);
                    }
                    result.set_string_value(table.get_as_string().substr(idx, 1));
                    push(result);
                }
            }
        }
        else if (command.get_command() == "ARRR")
        {
            Value result;
            result.set_table_value();
            stack<Value> values;
            // Pop values until we find a nil
            while (true)
            {
                Value v = pop(command);
                if (v.get_type() == NIL)
                {
                    break;
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
            push(result);
        }
        else if (command.get_command() == "DUPL")
        {
            push(execution_stack.top());
        }
        else if (command.get_command() == "NIL?")
        {
            Value v1 = pop(command);
            Value result;
            result.set_number_value(v1.get_type() == NIL ? 1 : 0);
            push(result);
        }
        else if (command.get_command() == "DISP")
        {
            cout << pop(command).get_as_string() << flush;
        }
        else if (command.get_command() == "ACCP")
        {
            Value result;
            result.set_string_value(input());
            push(result);
        }
        else if (command.get_command() == "POPV")
        {
            if (!execution_stack.empty())
            {
                pop(command);
            }
        }
        else if (command.get_command() == "EXIT")
        {
            exit((int)floor(pop(command).get_as_number()));
        }
        else if (command.get_command() == "UNST")
        {
            delete_variable(command.get_arguments()[0].get_raw_string_value());
        }
        else if (command.get_command() == "PUST")
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
        }
        else if (command.get_command() == "FORW") // File Open for Read and Write
        {
            Value filename = pop(command);
            string str_filename = filename.get_as_string();
            // Close the file if it was already open
            if (open_files.count(str_filename) > 0)
            {
                open_files[str_filename]->close();
            }
            // Open the file
            fstream *new_file;
            if(file_exists(str_filename))
            {
                new_file = new fstream(str_filename, ios::in | ios::out);
            }else{
                new_file = new fstream(str_filename, ios::in | ios::out | ios::trunc);
            }
            if(!(*new_file))
            {
                raise_nvm_error("Failed to open file " + str_filename + " for read/write.");
            }
            open_files[str_filename] = new_file;
            // The name of the file is returned, wether it's been opened correctly or not,
            // as the file might not already exist.
        }
        else if (command.get_command() == "FORA") // File Open for Read and Append
        {
            Value filename = pop(command);
            string str_filename = filename.get_as_string();
            // Close the file if it was already open
            if (open_files.count(str_filename) > 0)
            {
                open_files[str_filename]->close();
            }
            // Open the file
            fstream *new_file = new fstream(str_filename, ios::in | ios::out | ios::app);
            if(!(*new_file))
            {
                raise_nvm_error("Failed to open file " + str_filename + " for read/append.");
            }
            open_files[str_filename] = new_file;
            // The name of the file is returned, wether it's been opened correctly or not,
            // as the file might not already exist.
        }
        else if (command.get_command() == "RFIL") // Read File
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
                file->seekg(0);
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
                push(result);
            }
        }
        else if (command.get_command() == "FCLS") // File Close
        {
            Value filename = pop(command);
            string str_filename = filename.get_as_string();
            if (open_files.count(str_filename) > 0)
            {
                open_files[str_filename]->close();
                open_files.erase(str_filename);
            }
        }
        else if (command.get_command() == "RLNE") // File Read Line
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
                    push(result);
                }
                else
                {
                    push(get_nil_value());
                }
            }
        }
        else if (command.get_command() == "FWRT") // File Write
        {
            Value filename = pop(command);
            string str_filename = filename.get_as_string();
            if (open_files.count(str_filename) == 0)
            {
                raise_nvm_error("The file " + str_filename + " is not open for writing.");
            }
            else
            {
                Value contents = pop(command);
                (*open_files[str_filename]) << contents.get_as_string() << flush;
            }
        }
        else if (command.get_command() == "LNOT")
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
            push(result);
        }
        else if (command.get_command() == "LAND")
        {
            Value com_2 = pop(command);
            Value com_1 = pop(command);
            Value result;
            result.set_number_value(is_true(com_1) && is_true(com_2) ? 1 : 0);
            push(result);
        }
        else if (command.get_command() == "LGOR")
        {
            Value com_2 = pop(command);
            Value com_1 = pop(command);
            Value result;
            result.set_number_value(is_true(com_1) || is_true(com_2) ? 1 : 0);
            push(result);
        }
        else if (command.get_command() == "TRIM")
        {
            Value result;
            result.set_string_value(trim(pop(command).get_as_string()));
            push(result);
        }
        else if (command.get_command() == "SLEN") // String or Table Length
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
            push(result);
        }
        else if (command.get_command() == "SWAP")
        {
            Value v2 = pop(command);
            Value v1 = pop(command);
            push(v2);
            push(v1);
        }
        else if (command.get_command() == "ISIN")
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
            push(result);
        }
        else if (command.get_command() == "FLOR")
        {
            Value result;
            result.set_number_value(floor(pop(command).get_as_number()));
            push(result);
        }
        else if (command.get_command() == "ADSC")
        {
            add_scope();
        }
        else if (command.get_command() == "DLSC")
        {
            if (variable_tables.empty())
            {
                raise_nvm_error("No scopes left.");
            }
            else
            {
                variable_tables.pop_back();
            }
        }
        else if (command.get_command() == "EXEC")
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
        }
        else if (command.get_command() == "WAIT")
        {
            this_thread::sleep_for(chrono::microseconds((int)floor(pop(command).get_as_number() * 1000000)));
        }
        else if (command.get_command() == "KEYS")
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
                push(result);
            }
        }
        else if (command.get_command() == "GITR")
        {
            Value table = pop(command);
            Value result;
            result.set_iterator_value();
            if (table.get_type() == TABLE)
            {
                for (auto it = table.get_table()->begin(); it != table.get_table()->end(); ++it)
                {
                    // Idea: I can use it->second here to add the values as well to the table maybe
                    result.get_iterator_queue()->push(it->first);
                }
            }
            else if (table.get_type() == TEXT || table.get_type() == NUMB)
            {
                for (size_t i = 0; i < table.get_as_string().size(); ++i)
                {
                    string character = string(table.get_as_string()[i], 1);
                    result.get_iterator_queue()->push(character);
                }
            }
            else
            {
                raise_nvm_error("Cannot iterate over non-iterable value.");
            }
            push(result);
        }
        else if (command.get_command() == "NEXT")
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
                push(result);
            }
        }
        else
        {
            raise_nvm_error("Unknown Nambly command: " + command.get_debug_string());
        }
        ++pc;
    }
}

int main()
{
    string code = "";
    string input;
    while (getline(cin, input))
    {
        code = code + "\n" + input;
    }
    // cout << code << endl;
    vector<Command> code_listing = split_lines(code);
    code_listing = generate_label_map(code_listing);
    // print_command_listing(code_listing);
    execute_code_listing(code_listing);
    return 0;
}
