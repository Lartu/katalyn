#include "compiler.hpp"
#include "stdlib.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace katalyn
{
    namespace
    {

        constexpr std::string_view operators[] = {
            "!", "^", "*", "/", "%", "//", "+", "-", "&", "::", "<", ">", "<=", ">=",
            "<>", "!=", "=", "&&", "||"};
        constexpr std::string_view loop_tags[] = {"while", "until", "for", "whileis"};

        enum class Kind
        {
            word,
            integer,
            floating,
            string,
            oper,
            variable,
            table,
            access_open,
            access_close,
            par_open,
            par_close,
            decoration,
            unknown
        };

        struct Token
        {
            std::string value;
            int line = 1;
            std::string file;
            Kind kind = Kind::unknown;

            std::string variable_name() const
            {
                if (kind != Kind::variable)
                    throw std::logic_error("not a variable");
                return value.substr(1);
            }
        };

        [[noreturn]] void fail(std::string_view phase, const Token &token, const std::string &message)
        {
            std::ostringstream out;
            out << "\n=== Katalyn " << phase << " Error ===\n"
                << "- Where? In file '" << token.file << "', on line " << token.line << ".\n"
                << "- Error Message: " << message << "\n";
            throw std::runtime_error(out.str());
        }

        bool all_of_chars(std::string_view text, std::string_view chars)
        {
            return !text.empty() && text.find_first_not_of(chars) == std::string_view::npos;
        }

        bool is_integer(std::string_view text) { return all_of_chars(text, "0123456789"); }

        bool is_float(std::string_view text)
        {
            if (text.size() < 3 || text.front() == '.' || text.back() == '.')
                return false;
            return std::count(text.begin(), text.end(), '.') == 1 && all_of_chars(text, "0123456789.");
        }

        bool is_variable(std::string_view text)
        {
            return text.size() >= 2 && text.front() == '$' &&
                   text.substr(1).find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789") == std::string_view::npos;
        }

        bool is_identifier(std::string_view text)
        {
            if (text.empty() || std::string_view("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_").find(text.front()) == std::string_view::npos)
                return false;
            return text.substr(1).find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789") == std::string_view::npos;
        }

        bool is_operator(std::string_view text)
        {
            return std::find(std::begin(operators), std::end(operators), text) != std::end(operators);
        }

        int precedence(std::string_view text, bool prefix = false)
        {
            // Lower numbers bind more tightly. Power is tighter than prefix negation,
            // matching mathematical/Python convention; all other binary levels follow
            // the familiar arithmetic, comparison, AND, OR hierarchy.
            if (text == "^")
                return 0;
            if (prefix)
                return 1;
            if (text == "*" || text == "/" || text == "//" || text == "%")
                return 2;
            if (text == "+" || text == "-")
                return 3;
            if (text == "&")
                return 4;
            if (text == "::" || text == "<" || text == ">" || text == "<=" || text == ">=")
                return 5;
            if (text == "=" || text == "!=" || text == "<>")
                return 6;
            if (text == "&&")
                return 7;
            if (text == "||")
                return 8;
            return 9;
        }

        std::string quote(std::string_view text)
        {
            std::string out;
            out.reserve(text.size() + 2);
            out += '"';
            for (char c : text)
            {
                if (c == '\\' || c == '"')
                    out += '\\';
                if (c == '\n')
                    out += "\\n";
                else
                    out += c;
            }
            out += '"';
            return out;
        }

        std::vector<std::vector<Token>> tokenize(std::string source, const std::string &filename)
        {
            source += ' ';
            std::vector<std::vector<Token>> lines;
            std::vector<Token> line;
            std::string current;
            int line_number = 1;
            int string_line = 1;
            int comment_depth = 0;
            bool inline_comment = false;
            bool in_string = false;
            bool in_brace = false;

            auto push = [&](std::string value, int at = -1)
            {
                if (!value.empty())
                    line.push_back({std::move(value), at < 0 ? line_number : at, filename});
            };
            auto flush = [&]
            {
                if (!current.empty())
                    push(std::exchange(current, ""));
            };

            for (std::size_t i = 0; i + 1 < source.size(); ++i)
            {
                char c = source[i], next = source[i + 1];
                if (!inline_comment && !in_string && !in_brace && comment_depth == 0 && c == '#')
                {
                    inline_comment = true;
                }
                else if (!in_string && !in_brace && c == '(' && next == '*')
                {
                    ++comment_depth;
                    ++i;
                }
                else if (comment_depth > 0 && !in_string && !in_brace && c == '*' && next == ')')
                {
                    --comment_depth;
                    ++i;
                }
                else if (!inline_comment && comment_depth == 0 && (in_string || in_brace) && c == '\\')
                {
                    if (next == 'n')
                        current += '\n';
                    else if (next == 't')
                        current += '\t';
                    else if (std::isspace(static_cast<unsigned char>(next)))
                    {
                        while (i + 1 < source.size() && std::isspace(static_cast<unsigned char>(source[i + 1])))
                            ++i;
                        --i;
                    }
                    else
                        current += next;
                    ++i;
                }
                else if (!inline_comment && comment_depth == 0 && !in_string && !in_brace && c == '"')
                {
                    flush();
                    current = "\"";
                    in_string = true;
                    string_line = line_number;
                }
                else if (!inline_comment && comment_depth == 0 && in_string && c == '"')
                {
                    current += '"';
                    in_string = false;
                    push(std::exchange(current, ""), string_line);
                }
                else if (!inline_comment && comment_depth == 0 && !in_string && !in_brace && c == '{')
                {
                    flush();
                    push("[");
                    current = "\"";
                    in_brace = true;
                    string_line = line_number;
                }
                else if (!inline_comment && comment_depth == 0 && in_brace && c == '}')
                {
                    current += '"';
                    in_brace = false;
                    push(std::exchange(current, ""), string_line);
                    push("]");
                }
                else if (!inline_comment && comment_depth == 0 && !in_string && !in_brace && c == ';')
                {
                    flush();
                    if (!line.empty())
                        lines.push_back(std::exchange(line, {}));
                }
                else if (!inline_comment && comment_depth == 0 && !in_string && !in_brace &&
                         is_operator(std::string{c, next}))
                {
                    flush();
                    push(std::string{c, next});
                    ++i;
                }
                else if (!inline_comment && comment_depth == 0 && !in_string && !in_brace &&
                         std::string_view("()[]=<>!+-/&%^*:#,").find(c) != std::string_view::npos)
                {
                    flush();
                    push(std::string(1, c));
                }
                else if (!inline_comment && comment_depth == 0 && !in_string && !in_brace &&
                         std::isspace(static_cast<unsigned char>(c)))
                {
                    flush();
                }
                else if (!inline_comment && comment_depth == 0)
                {
                    current += c;
                }
                if (c == '\n')
                {
                    ++line_number;
                    inline_comment = false;
                }
            }
            Token where{"", string_line, filename};
            if (in_string)
                fail("Tokenization", where, "Open string, missing '\"'");
            if (in_brace)
                fail("Tokenization", where, "Open access string, missing '}'");
            if (!line.empty() || !current.empty())
            {
                where.line = line.empty() ? line_number : line.back().line;
                fail("Tokenization", where, "Missing ';'");
            }
            return lines;
        }

        void lex(std::vector<std::vector<Token>> &lines)
        {
            for (auto &line : lines)
                for (auto &token : line)
                {
                    auto &v = token.value;
                    if (v.size() >= 2 && v.front() == '"' && v.back() == '"')
                    {
                        v = v.substr(1, v.size() - 2);
                        token.kind = Kind::string;
                    }
                    else if (is_variable(v))
                        token.kind = Kind::variable;
                    else if (v == "(")
                        token.kind = Kind::par_open;
                    else if (v == ")")
                        token.kind = Kind::par_close;
                    else if (v == "[")
                        token.kind = Kind::access_open;
                    else if (v == "]")
                        token.kind = Kind::access_close;
                    else if (v == ":" || v == ",")
                        token.kind = Kind::decoration;
                    else if (v == "table")
                        token.kind = Kind::table;
                    else if (is_integer(v))
                        token.kind = Kind::integer;
                    else if (is_float(v))
                        token.kind = Kind::floating;
                    else if (is_operator(v))
                        token.kind = Kind::oper;
                    else if (is_identifier(v))
                        token.kind = Kind::word;
                    else
                        fail("Lexing", token, "The string '" + v + "' is not a valid token.");
                }
        }

        struct Labels
        {
            std::string start, end, post;
        };
        struct Block
        {
            std::string code;
            Token token;
            int group = -1;
        };

    } // namespace

    class Compiler::Impl
    {
    public:
        int block_count = 0;
        std::vector<Block> blocks;
        std::vector<std::unordered_set<std::string>> scopes{{}};
        std::vector<std::pair<std::string, std::string>> loops;
        std::unordered_map<std::string, Labels> functions;
        std::unordered_map<std::string, std::pair<Token, Labels>> expected;

        void reset()
        {
            block_count = 0;
            blocks.clear();
            scopes = {{}};
            loops.clear();
            functions.clear();
            expected.clear();
        }

        std::optional<std::string> variable(const Token &token, bool only_local = false,
                                            bool only_global = false) const
        {
            const auto name = token.variable_name();
            if (!only_global && scopes.back().count(name))
                return name;
            if (!only_local && scopes.front().count(name))
                return name;
            return std::nullopt;
        }

        std::string require_variable(const Token &token, bool unsafe = false) const
        {
            if (auto found = variable(token))
                return *found;
            if (unsafe)
                return token.variable_name();
            fail("Expression", token, "Variable " + token.value + " read before assignment.");
        }

        std::string declare(const Token &token, bool global = false)
        {
            auto &scope = global ? scopes.front() : scopes.back();
            scope.insert(token.variable_name());
            return token.variable_name();
        }

        Labels function_labels(const Token &token, bool force_new = false)
        {
            if (!force_new)
            {
                if (auto it = functions.find(token.value); it != functions.end())
                    return it->second;
                if (auto it = expected.find(token.value); it != expected.end())
                    return it->second.second;
            }
            int n = block_count++;
            Labels labels{"FUN_" + std::to_string(n) + "_START",
                          "FUN_" + std::to_string(n) + "_END",
                          "FUN_" + std::to_string(n) + "_POST"};
            expected[token.value] = {token, labels};
            return labels;
        }

        std::string expression(std::vector<Token> tokens, bool discard = false, bool unsafe = false)
        {
            if (tokens.empty())
                return discard ? "\nPOPV" : "";
            std::vector<Token> left, right;
            std::optional<Token> op;
            bool op_is_prefix = false;
            bool expecting_operand = true;
            int pars = 0, accesses = 0, depth_zero_count = 0;
            for (const auto &token : tokens)
            {
                int initial = pars + accesses;
                if (token.kind == Kind::par_open)
                    ++pars;
                else if (token.kind == Kind::par_close && --pars < 0)
                    fail("Expression", token, "')' before '('");
                else if (token.kind == Kind::access_open)
                    ++accesses;
                else if (token.kind == Kind::access_close && --accesses < 0)
                    fail("Expression", token, "']' before '['");

                if (pars + accesses == 0 && token.kind == Kind::oper)
                {
                    bool prefix = token.value == "!" || (token.value == "-" && expecting_operand);
                    if (prefix && op)
                    {
                        // A prefix operator on a binary operator's right side is
                        // part of that operand, not a competing binary split.
                        right.push_back(token);
                    }
                    else if (!op)
                    {
                        op = token;
                        op_is_prefix = prefix;
                    }
                    else
                    {
                        int current = precedence(op->value, op_is_prefix);
                        int incoming = precedence(token.value, prefix);
                        bool right_associative_power =
                            current == incoming && op->value == "^" && token.value == "^";
                        if (current < incoming ||
                            (current == incoming && !right_associative_power))
                        {
                            left.push_back(*op);
                            left.insert(left.end(), right.begin(), right.end());
                            right.clear();
                            op = token;
                            op_is_prefix = prefix;
                        }
                        else
                        {
                            right.push_back(token);
                        }
                    }
                    expecting_operand = true;
                }
                else if (!op)
                    left.push_back(token);
                else
                    right.push_back(token);
                if (pars + accesses == 0 && token.kind != Kind::oper &&
                    token.kind != Kind::par_open && token.kind != Kind::access_open)
                    expecting_operand = false;
                if (initial == 0 || pars + accesses == 0)
                    ++depth_zero_count;
            }
            if (pars > 0)
                fail("Expression", tokens.back(), "Missing ')'");
            if (accesses > 0)
                fail("Expression", tokens.back(), "Missing ']'");

            std::string code;
            if (depth_zero_count == 2 && tokens.front().kind == Kind::par_open &&
                tokens.back().kind == Kind::par_close)
            {
                code += expression({tokens.begin() + 1, tokens.end() - 1});
            }
            else if (op && right.empty())
            {
                fail("Expression", *op, "Expecting expression after operator " + op->value);
            }
            else if (!op)
            {
                code += terminator(std::move(left), unsafe);
            }
            else if (op_is_prefix && left.empty())
            {
                code += expression(std::move(right));
                if (op->value == "-")
                    code += "\nPUSH -1\nMULT";
                else
                    code += "\nLNOT";
                op.reset();
            }
            else if (op->value == "&&" || op->value == "||")
            {
                if (left.empty())
                    fail("Expression", *op, "Expecting expression before operator " + op->value);

                int n = block_count++;
                std::string prefix = "LOGIC_" + std::to_string(n);
                std::string end = prefix + "_END";

                if (op->value == "&&")
                {
                    std::string false_label = prefix + "_FALSE";
                    code += expression(std::move(left));
                    code += "\nJPIF " + false_label;
                    code += expression(std::move(right));
                    code += "\nJPIF " + false_label;
                    code += "\nPUSH 1\nJUMP " + end;
                    code += "\n@" + false_label + "\nPUSH 0";
                }
                else
                {
                    std::string right_label = prefix + "_RIGHT";
                    std::string false_label = prefix + "_FALSE";
                    code += expression(std::move(left));
                    code += "\nJPIF " + right_label;
                    code += "\nPUSH 1\nJUMP " + end;
                    code += "\n@" + right_label;
                    code += expression(std::move(right));
                    code += "\nJPIF " + false_label;
                    code += "\nPUSH 1\nJUMP " + end;
                    code += "\n@" + false_label + "\nPUSH 0";
                }
                code += "\n@" + end;
                op.reset();
            }
            else
            {
                code += expression(std::move(left));
                code += expression(std::move(right));
            }
            if (op)
            {
                static const std::unordered_map<std::string, std::string> operations = {
                    {"*", "MULT"}, {"^", "POWR"}, {"/", "FDIV"}, {"//", "IDIV"}, {"-", "SUBT"}, {"+", "ADDV"}, {"&", "JOIN"}, {"%", "MODL"}, {"=", "ISEQ"}, {"<>", "ISNE"}, {"!=", "ISNE"}, {"<", "ISLT"}, {">", "ISGT"}, {"<=", "ISLE"}, {">=", "ISGE"}, {"&&", "LAND"}, {"||", "LGOR"}, {"::", "ISIN"}, {"!", "LNOT"}};
                auto it = operations.find(op->value);
                if (it == operations.end())
                    fail("Expression", *op, "Unsupported operator.");
                if (op->value == "!" && !left.empty())
                    fail("Expression", *op, "The operator ! can only be used as a prefix operator.");
                code += "\n" + it->second;
            }
            if (discard)
                code += "\nPOPV";
            return code;
        }

        std::string terminator(std::vector<Token> tokens, bool unsafe)
        {
            std::string code;
            bool negate = false;
            int access_depth = 0;
            std::vector<Token> access_tokens;
            Kind terminal = Kind::unknown;
            std::optional<Token> function;

            while (!tokens.empty())
            {
                Token token = tokens.front();
                tokens.erase(tokens.begin());
                if (access_depth > 0)
                {
                    if (token.kind == Kind::access_open)
                    {
                        ++access_depth;
                        access_tokens.push_back(token);
                    }
                    else if (token.kind == Kind::access_close)
                    {
                        if (--access_depth == 0)
                        {
                            code += expression(std::move(access_tokens)) + "\nPGET";
                            access_tokens.clear();
                        }
                        else
                            access_tokens.push_back(token);
                    }
                    else
                        access_tokens.push_back(token);
                    continue;
                }
                const Token *next = tokens.empty() ? nullptr : &tokens.front();
                if (token.kind == Kind::oper && token.value == "-")
                {
                    if (!next)
                        fail("Expression", token, "Missing value after '-'");
                    if (next->kind == Kind::integer || next->kind == Kind::floating)
                    {
                        if (terminal != Kind::unknown)
                            fail("Expression", token, "Unexpected '-'.");
                        code += "\nPUSH -" + next->value;
                        terminal = next->kind;
                        tokens.erase(tokens.begin());
                    }
                    else
                        negate = true;
                }
                else if (token.kind == Kind::table)
                {
                    if (terminal != Kind::unknown)
                        fail("Expression", token, "Unexpected table.");
                    code += "\nTABL";
                    terminal = token.kind;
                }
                else if (token.kind == Kind::variable)
                {
                    if (terminal != Kind::unknown)
                        fail("Expression", token, "Unexpected variable.");
                    code += "\nVGET " + quote(require_variable(token, unsafe));
                    terminal = token.kind;
                }
                else if (token.kind == Kind::string)
                {
                    if (terminal != Kind::unknown)
                        fail("Expression", token, "Unexpected string.");
                    code += "\nPUSH " + quote(token.value);
                    terminal = token.kind;
                }
                else if (token.kind == Kind::integer || token.kind == Kind::floating)
                {
                    if (terminal != Kind::unknown)
                        fail("Expression", token, "Unexpected number.");
                    code += "\nPUSH " + token.value;
                    terminal = token.kind;
                }
                else if (token.kind == Kind::access_open)
                {
                    if (terminal == Kind::unknown)
                        fail("Expression", token, "Table access without a value.");
                    access_depth = 1;
                }
                else if (token.kind == Kind::word)
                {
                    if (terminal != Kind::unknown)
                        fail("Expression", token, "Unexpected identifier.");
                    if (!next || next->kind != Kind::par_open)
                        fail("Expression", token, "Expecting argument list after function call.");
                    terminal = token.kind;
                    function = token;
                }
                else if (token.kind == Kind::par_open)
                {
                    if (terminal != Kind::word)
                        fail("Expression", token, "Calling non-functional value.");
                    std::vector<std::vector<Token>> args;
                    std::vector<Token> current;
                    int depth = 1;
                    bool closed = false;
                    while (!tokens.empty())
                    {
                        Token part = tokens.front();
                        tokens.erase(tokens.begin());
                        if (part.kind == Kind::par_open)
                        {
                            ++depth;
                            current.push_back(part);
                        }
                        else if (part.kind == Kind::par_close)
                        {
                            if (--depth == 0)
                            {
                                if (!current.empty())
                                    args.push_back(std::move(current));
                                closed = true;
                                break;
                            }
                            current.push_back(part);
                        }
                        else if (depth == 1 && part.kind == Kind::decoration && part.value == ",")
                        {
                            if (current.empty())
                                fail("Expression", part, "Empty function argument.");
                            args.push_back(std::move(current));
                            current.clear();
                        }
                        else
                            current.push_back(part);
                    }
                    if (!closed)
                        fail("Expression", token, "Missing ')'.");
                    code += function_call(*function, args);
                }
                else
                    fail("Expression", token, "Unexpected token '" + token.value + "'.");
            }
            if (access_depth)
                fail("Expression", access_tokens.front(), "Missing ']'.");
            if (negate)
                code += "\nPUSH -1\nMULT";
            return code;
        }

        void arity(const Token &command, std::size_t got, std::size_t min, std::size_t max)
        {
            if (got < min || got > max)
                fail("Parse", command, "Wrong number of arguments for '" + command.value + "'.");
        }

        std::string function_call(const Token &command, const std::vector<std::vector<Token>> &args)
        {
            const auto &name = command.value;
            auto arg = [&](std::size_t i)
            { return expression(args.at(i)); };
            if (name == "print" || name == "printc")
            {
                std::string code = "\nPUSH \"\"";
                for (const auto &item : args)
                    code += expression(item) + "\nDUPL\nVSET \"$swap\"\nDISP\nVGET \"$swap\"\nJOIN";
                if (name == "print")
                    code += "\nPUSH \"\\n\"\nDISP";
                return code;
            }
            if (name == "accept")
            {
                arity(command, args.size(), 0, 1);
                return (args.empty() ? "\nPUSH \"\"" : arg(0)) + "\nACCP";
            }
            if (name == "is")
            {
                arity(command, args.size(), 1, 1);
                return arg(0) + "\nNIL?\nLNOT";
            }
            if (name == "unsafe")
            {
                arity(command, args.size(), 1, 1);
                return expression(args[0], false, true);
            }
            if (name == "exit")
            {
                arity(command, args.size(), 1, 1);
                return arg(0) + "\nEXIT";
            }
            if (name == "len")
            {
                arity(command, args.size(), 1, 1);
                return arg(0) + "\nSLEN";
            }
            if (name == "env")
            {
                arity(command, args.size(), 0, 2);
                std::string code = args.empty() ? "\nPNIL" : arg(0);
                code += args.size() < 2 ? "\nPNIL" : arg(1);
                return code + "\nENVV";
            }
            if (name == "read_stdin")
            {
                arity(command, args.size(), 0, 1);
                return (args.empty() ? "\nPUSH -1" : arg(0)) + "\nRSTD";
            }
            if (name == "url_decode")
            {
                arity(command, args.size(), 1, 2);
                return arg(0) + (args.size() == 2 ? arg(1) : "\nPUSH 0") + "\nURLD";
            }
            if (name == "parse_query")
            {
                arity(command, args.size(), 1, 1);
                return arg(0) + "\nQPRS";
            }
            if (name == "json_decode")
            {
                arity(command, args.size(), 1, 1);
                return arg(0) + "\nJDEC";
            }
            if (name == "json_encode")
            {
                arity(command, args.size(), 1, 1);
                return arg(0) + "\nJENC";
            }
            if (name == "cgi_request")
            {
                arity(command, args.size(), 0, 1);
                return (args.empty() ? "\nPUSH 16777216" : arg(0)) + "\nCGIR";
            }
            if (name == "cgi_response")
            {
                arity(command, args.size(), 3, 4);
                std::string code = arg(0) + arg(1) + arg(2);
                code += args.size() == 4 ? arg(3) : "\nTABL";
                return code + "\nCGIO";
            }
            if (name == "floor")
            {
                arity(command, args.size(), 1, 1);
                return arg(0) + "\nFLOR";
            }
            if (name == "keys")
            {
                arity(command, args.size(), 1, 1);
                return arg(0) + "\nKEYS";
            }
            if (name == "del")
            {
                arity(command, args.size(), 2, 2);
                return arg(0) + arg(1) + "\nPUST";
            }
            if (name == "set")
            {
                arity(command, args.size(), 2, 2);
                std::vector<Token> assignment = args[0];
                assignment.push_back({":", command.line, command.file, Kind::decoration});
                assignment.insert(assignment.end(), args[1].begin(), args[1].end());
                return assign(command, assignment, false) + expression(args[0]);
            }
            if (name == "unset")
            {
                std::string code;
                for (const auto &item : args)
                {
                    if (item.size() != 1 || item[0].kind != Kind::variable)
                        fail("Parse", command, "unset expects variables.");
                    code += "\nUNST " + quote(require_variable(item[0]));
                    for (auto &scope : scopes)
                        scope.erase(item[0].variable_name());
                }
                return code;
            }
            if (name == "substr")
            {
                arity(command, args.size(), 2, 3);
                std::string code = arg(0);
                if (args.size() == 2)
                    code += "\nDUPL";
                code += arg(1);
                if (args.size() == 3)
                    code += arg(2);
                else
                    code += "\nSWAP\nSLEN";
                return code + "\nSSTR";
            }
            if (name == "replace")
            {
                arity(command, args.size(), 3, 3);
                return arg(0) + arg(1) + arg(2) + "\nREPL";
            }
            if (name == "split" || name == "explode")
            {
                arity(command, args.size(), 2, 4);
                std::string code = args.size() == 4 ? arg(3) : "\nPUSH 1";
                code += args.size() >= 3 ? arg(2) : "\nPUSH -1";
                return code + arg(1) + arg(0) + (name == "split" ? "\nEXPL" : "\nMXPL");
            }
            static const std::unordered_map<std::string, std::pair<std::string, bool>> files = {
                {"open_rw", {"FORW", true}}, {"open_ra", {"FORA", true}}, {"open_r", {"FORE", true}}, {"is_open", {"ISOP", false}}, {"close", {"FCLS", true}}, {"read", {"RFIL", false}}, {"read_line", {"RLNE", false}}};
            if (auto it = files.find(name); it != files.end())
            {
                arity(command, args.size(), 1, 1);
                return arg(0) + (it->second.second ? "\nDUPL" : "") + "\n" + it->second.first;
            }
            if (name == "write")
            {
                arity(command, args.size(), 2, 2);
                return arg(1) + "\nDUPL" + arg(0) + "\nFWRT";
            }
            if (name == "exec")
            {
                arity(command, args.size(), 1, static_cast<std::size_t>(-1));
                std::string code;
                for (std::size_t i = 0; i < args.size(); ++i)
                {
                    code += arg(i);
                    if (i)
                        code += "\nJOIN";
                }
                Token out{"$_stdout", command.line, command.file, Kind::variable};
                Token err{"$_stderr", command.line, command.file, Kind::variable};
                Token status{"$_exitcode", command.line, command.file, Kind::variable};
                return code + "\nEXEC\nVSET " + quote(declare(out)) + "\nVSET " + quote(declare(err)) +
                       "\nDUPL\nVSET " + quote(declare(status));
            }

            Token context{"$_context", command.line, command.file, Kind::variable};
            std::string code = "\nVGET " + quote(require_variable(context)) + "\nPLIM";
            for (const auto &item : args)
                code += expression(item);
            code += "\nCALL " + function_labels(command).start;
            return code;
        }

        std::string assign(const Token &command, const std::vector<Token> &args, bool global)
        {
            auto colon = std::find_if(args.begin(), args.end(), [](const Token &t)
                                      { return t.kind == Kind::decoration && t.value == ":"; });
            if (colon == args.end())
                fail("Parse", command, "Assignment is missing ':'.");
            std::vector<Token> left(args.begin(), colon), right(colon + 1, args.end());
            if (left.empty() || right.empty())
                fail("Parse", command, "Empty side in assignment.");
            if (left[0].kind != Kind::variable)
                fail("Parse", left[0], "Variable expected.");

            const bool accesses = std::any_of(left.begin(), left.end(), [](const Token &t)
                                              { return t.kind == Kind::access_open; });
            std::string before, after;
            auto id = declare(left[0], global);
            if (!accesses)
            {
                if (left.size() != 1)
                    fail("Parse", left[1], "Unexpected token in assignment.");
                after = std::string("\n") + (global ? "GSET " : "VSET ") + quote(id);
            }
            else
            {
                before = "\nVGET " + quote(id);
                int depth = 0, count = 0;
                std::vector<Token> access;
                for (auto it = left.begin() + 1; it != left.end(); ++it)
                {
                    if (it->kind == Kind::access_open)
                        ++depth;
                    if (it->kind == Kind::access_close)
                        --depth;
                    access.push_back(*it);
                    if (it->kind == Kind::access_close && depth == 0)
                    {
                        if (access.size() <= 2)
                            fail("Parse", access.front(), "Malformed table access.");
                        if (count++)
                            before += "\nPGET";
                        before += expression({access.begin() + 1, access.end() - 1});
                        access.clear();
                    }
                }
                if (depth || !access.empty())
                    fail("Parse", left.back(), "Malformed table access.");
                after = "\nPSET";
            }
            return before + expression(std::move(right)) + after;
        }

        std::string flow(const Token &command, const std::vector<Token> &args)
        {
            if (args.empty())
                fail("Parse", command, "Expected an expression.");
            int n = block_count++;
            std::string start = "LOOP_" + std::to_string(n) + "_START";
            std::string end = "LOOP_" + std::to_string(n) + "_END";
            Token result{"$_r", command.line, command.file, Kind::variable};
            std::string code;
            if (command.value == "for")
            {
                Token iterator{"$_itr" + std::to_string(n), command.line, command.file, Kind::variable};
                auto iid = declare(iterator, true);
                code = expression(args) + "\nGITR\nVSET " + quote(iid) + "\n@" + start +
                       "\nNEXT " + quote(iid) + "\nDUPL\nVSET " + quote(declare(result, true)) +
                       "\nJPIF " + end;
                blocks.push_back({"\nJUMP " + start + "\n@" + end + "\nUNST " + quote(iid), command});
            }
            else
            {
                code = "\n@" + start + expression(args) + "\nDUPL";
                if (command.value == "whileis")
                    code += "\nNIL?\nLNOT\nSWAP";
                code += "\nVSET " + quote(declare(result, true));
                if (command.value == "until")
                    code += "\nLNOT";
                code += "\nJPIF " + end;
                blocks.push_back({"\nJUMP " + start + "\n@" + end, command});
            }
            loops.push_back({start, end});
            return code;
        }

        std::string conditional(const Token &command, const std::vector<Token> &args)
        {
            if (command.value == "else")
            {
                if (!args.empty() || blocks.empty() ||
                    (blocks.back().token.value != "if" && blocks.back().token.value != "elif"))
                    fail("Parse", command, "Unexpected else.");
                int group = blocks.back().group;
                std::string code = blocks.back().code;
                blocks.pop_back();
                blocks.push_back({"", command, group});
                return code;
            }
            if (args.empty())
                fail("Parse", command, "Expected an expression.");
            int n = block_count++;
            int group = n;
            std::string code;
            if (command.value == "elif")
            {
                if (blocks.empty() ||
                    (blocks.back().token.value != "if" && blocks.back().token.value != "elif"))
                    fail("Parse", command, "Unexpected elif.");
                group = blocks.back().group;
                code = blocks.back().code;
                blocks.pop_back();
            }
            std::string end = "COND_" + std::to_string(n) + "_END";
            Token result{"$_r", command.line, command.file, Kind::variable};
            if (command.value == "unless")
                code += "\n@COND_" + std::to_string(n) + "_START";
            code += expression(args) + "\nDUPL\nVSET " + quote(declare(result, true));
            if (command.value == "unless")
                code += "\nLNOT";
            code += "\nJPIF " + end;
            std::string tail = command.value == "unless" ? "\n@" + end : "\nJUMP EXIT_IF_" + std::to_string(group) + "\n@" + end;
            blocks.push_back({tail, command, command.value == "unless" ? -1 : group});
            return code;
        }

        std::string definition(const Token &command, const std::vector<Token> &args)
        {
            if (args.size() != 1 || args[0].kind != Kind::word)
                fail("Parse", command, "def expects one function name.");
            scopes.push_back({});
            Labels labels = function_labels(args[0], true);
            functions[args[0].value] = labels;
            Token av{"$_", command.line, command.file, Kind::variable};
            Token caller{"$_caller", command.line, command.file, Kind::variable};
            Token context{"$_context", command.line, command.file, Kind::variable};
            std::string code = "\nJUMP " + labels.post + "\n@" + labels.start + "\nADSC\nARRR\nVSET " +
                               quote(declare(av)) + "\nVSET " + quote(declare(caller)) + "\nPUSH " +
                               quote(args[0].value) + "\nVSET " + quote(declare(context)) + "\nPNIL";
            blocks.push_back({"\n@" + labels.end + "\nDLSC\nRTRN\n@" + labels.post, args[0]});
            return code;
        }

        std::string compile_lines(std::vector<std::vector<Token>> &lines)
        {
            std::string code;
            for (const auto &line : lines)
            {
                const Token &command = line.front();
                std::vector<Token> args(line.begin() + 1, line.end());
                code += "\n;line " + std::to_string(command.line) + "\n;file " + command.file;
                if (command.kind == Kind::variable)
                {
                    std::vector<Token> all{command};
                    all.insert(all.end(), args.begin(), args.end());
                    code += assign(command, all, false);
                }
                else if (command.kind != Kind::word)
                {
                    fail("Parse", command, "Unexpected command '" + command.value + "'.");
                }
                else if (command.value == "in" || command.value == "global")
                {
                    code += assign(command, args, command.value == "global");
                }
                else if (command.value == "while" || command.value == "whileis" ||
                         command.value == "until" || command.value == "for")
                {
                    code += flow(command, args);
                }
                else if (command.value == "if" || command.value == "elif" ||
                         command.value == "else" || command.value == "unless")
                {
                    code += conditional(command, args);
                }
                else if (command.value == "ok")
                {
                    if (!args.empty() || blocks.empty())
                        fail("Parse", command, "Unexpected ok.");
                    Block block = blocks.back();
                    blocks.pop_back();
                    if (std::find(std::begin(loop_tags), std::end(loop_tags), block.token.value) != std::end(loop_tags))
                        loops.pop_back();
                    if (functions.count(block.token.value))
                        scopes.pop_back();
                    code += block.code;
                    if (block.token.value == "if" || block.token.value == "elif" || block.token.value == "else")
                        code += "\n@EXIT_IF_" + std::to_string(block.group);
                }
                else if (command.value == "continue")
                {
                    if (!args.empty() || loops.empty())
                        fail("Parse", command, "continue outside loop.");
                    code += "\nJUMP " + loops.back().first;
                }
                else if (command.value == "break")
                {
                    std::size_t depth = 0;
                    if (!args.empty())
                    {
                        if (args.size() != 1 || !is_integer(args[0].value))
                            fail("Parse", command, "Invalid break depth.");
                        depth = static_cast<std::size_t>(std::stoul(args[0].value) - 1);
                    }
                    if (depth >= loops.size())
                        fail("Parse", command, "break outside loop.");
                    code += "\nJUMP " + loops[loops.size() - 1 - depth].second;
                }
                else if (command.value == "def")
                {
                    code += definition(command, args);
                }
                else if (command.value == "return")
                {
                    if (scopes.size() == 1)
                        fail("Parse", command, "return outside function.");
                    if (!args.empty())
                        code += "\nPOPV" + expression(args);
                    code += "\nDLSC\nRTRN";
                }
                else if (command.value == "sleep")
                {
                    if (!args.empty())
                        code += expression(args);
                    code += "\nWAIT";
                }
                else if (command.value == "import")
                {
                    if (args.size() != 1 || args[0].kind != Kind::string)
                        fail("Parse", command, "import expects one static string.");
                    auto path = (std::filesystem::path(command.file).parent_path() / args[0].value).lexically_normal();
                    code += compile_file_body(path.string(), command);
                }
                else
                {
                    code += expression(line, true);
                }
            }
            return code;
        }

        std::string compile_source_body(const std::string &source, const std::string &filename)
        {
            auto lines = tokenize(source, filename);
            lex(lines);
            return compile_lines(lines);
        }

        std::string compile_file_body(const std::string &filename, const Token &from = {})
        {
            auto absolute = std::filesystem::absolute(filename).lexically_normal().string();
            std::ifstream file(absolute);
            if (!file)
            {
                Token token = from;
                if (token.file.empty())
                    token = {filename, 0, filename};
                fail("Parse", token, "File '" + filename + "' not found.");
            }
            std::ostringstream source;
            source << file.rdbuf();
            return compile_source_body(source.str(), absolute);
        }

        std::string finish(std::string code)
        {
            if (!blocks.empty())
                fail("Parse", blocks.front().token, "Missing 'ok' command.");
            for (const auto &[name, item] : expected)
                if (!functions.count(name))
                    fail("Parse", item.first, "Call to nonexistent function '" + name + "'.");
            std::string clean;
            std::istringstream in(code);
            for (std::string line; std::getline(in, line);)
                if (!line.empty())
                    clean += line + '\n';
            return clean;
        }
    };

    Compiler::Compiler() : impl_(std::make_unique<Impl>()) {}
    Compiler::~Compiler() = default;
    Compiler::Compiler(Compiler &&) noexcept = default;
    Compiler &Compiler::operator=(Compiler &&) noexcept = default;

    std::string Compiler::compile_source(const std::string &source, const std::string &filename,
                                         const CompileOptions &options)
    {
        impl_->reset();
        Token context{"$_context", 0, filename, Kind::variable};
        Token flags{"$_args", 0, filename, Kind::variable};
        Token script_path{"$_scriptpath", 0, filename, Kind::variable};
        Token script_directory{"$_scriptdir", 0, filename, Kind::variable};
        Token working_directory{"$_wdir", 0, filename, Kind::variable};
        std::string code = "\nPUSH \"\"\nVSET " + quote(impl_->declare(context, true));
        code += options.script_path.empty() ? "\nPNIL" : "\nPUSH " + quote(options.script_path);
        code += "\nVSET " + quote(impl_->declare(script_path, true));
        code += options.script_path.empty()
                    ? "\nPNIL"
                    : "\nPUSH " + quote(std::filesystem::path(options.script_path).parent_path().string());
        code += "\nVSET " + quote(impl_->declare(script_directory, true));
        code += "\nPUSH " + quote(std::filesystem::current_path().lexically_normal().string());
        code += "\nVSET " + quote(impl_->declare(working_directory, true)) + "\nPLIM";
        for (const auto &arg : options.arguments)
            code += "\nPUSH " + quote(arg);
        code += "\nARRR\nVSET " + quote(impl_->declare(flags, true));
        if (options.include_standard_library)
            code += impl_->compile_source_body(standard_library, "<stdlib>");
        code += impl_->compile_source_body(source, filename);
        return impl_->finish(std::move(code));
    }

    std::string Compiler::compile_file(const std::string &filename, const CompileOptions &options)
    {
        const auto absolute = std::filesystem::absolute(filename).lexically_normal().string();
        std::ifstream file(absolute);
        if (!file)
            throw std::runtime_error("File " + filename + " not found.");
        std::ostringstream source;
        source << file.rdbuf();
        CompileOptions file_options = options;
        file_options.script_path = absolute;
        return compile_source(source.str(), absolute, file_options);
    }

    const char *version() { return "0.1.0"; }

} // namespace katalyn
