// -- NariVM for Katalyn in C++ by Lartu (25G24 00:17) --

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
#include <memory>
#include <thread>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <iomanip>
#include <queue>
#include <fstream>
#include <set>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <sys/stat.h>
#include "lib/tiny-process-library/process.hpp"
#include "lib/linenoise.hpp"

using namespace std;
using namespace TinyProcessLib;

#define NUMB 1      // Numeric Value
#define TEXT 2      // String Value
#define TABLE 3     // Table Value
#define BYTES 4     // Immutable byte sequence
#define NIL 5       // Null Value
#define ITER 6      // Iterator Value
#define LISTLIMIT 7 // List Limit Value
#define PID 8       // Actor process identifier

void raise_nvm_error(string error_message);

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
    case BYTES:
        return "BYTES";
    case NIL:
        return "NIL";
    case ITER:
        return "ITERATOR";
    case LISTLIMIT:
        return "LISTLIMIT";
    case PID:
        return "PID";
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
    ENVV, // Environment value/table
    RSTD, // Raw standard input
    URLD, // URL decode
    QPRS, // Query-string parse
    JDEC, // JSON decode
    JENC, // JSON encode
    CGIR, // Build CGI request
    CGIO, // Write CGI response
    PTRY, // Push error handler
    ETRY, // Finish try body
    CERR, // Bind caught error
    NCTH, // Propagate through a try without catch
    ECTH, // Finish catch body
    EFIN, // Finish finally body
    THRW, // Raise a value
    FJMP, // Jump while honoring finally
    FRET, // Return while honoring finally
    BNEW, // Construct bytes
    UENC, // UTF-8 encode
    UDEC, // UTF-8 decode
    BSLC, // Byte slice
    RSTB, // Raw standard input as bytes
    RBIN, // Read open file as bytes
    WBIN, // Write bytes to open file
    HENC, // Hex encode
    HDEC, // Hex decode
    B64E, // Base64 encode
    B64D, // Base64 decode
    PJON, // Join paths
    PPAR, // Path parent
    PNAM, // Path filename
    PEXT, // Path extension
    PABS, // Absolute path
    PNOR, // Normalize path
    PEXS, // Path exists
    PFIL, // Is regular file
    PDIR, // Is directory
    FSIZ, // File size
    LDIR, // List directory
    MDIR, // Make directory
    CPFL, // Copy file
    MVFL, // Move file
    RMFL, // Remove file
    RMDR, // Remove empty directory
    DTIM, // Local date and time
    SPWN, // Spawn worker
    SELF, // Current worker PID
    SEND, // Send worker message
    RECV, // Receive with optional timeout
    RNOW, // Receive without blocking
    WALV, // Worker alive
    WWAIT, // Wait for worker
    DEBUG,
};

Opcode opcode_from_string(string_view str)
{
    static const unordered_map<string_view, Opcode> str_to_opcode = {
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
        {"ENVV", Opcode::ENVV},
        {"RSTD", Opcode::RSTD},
        {"URLD", Opcode::URLD},
        {"QPRS", Opcode::QPRS},
        {"JDEC", Opcode::JDEC},
        {"JENC", Opcode::JENC},
        {"CGIR", Opcode::CGIR},
        {"CGIO", Opcode::CGIO},
        {"PTRY", Opcode::PTRY}, {"ETRY", Opcode::ETRY},
        {"CERR", Opcode::CERR}, {"NCTH", Opcode::NCTH},
        {"ECTH", Opcode::ECTH}, {"EFIN", Opcode::EFIN},
        {"THRW", Opcode::THRW}, {"FJMP", Opcode::FJMP},
        {"FRET", Opcode::FRET}, {"BNEW", Opcode::BNEW},
        {"UENC", Opcode::UENC}, {"UDEC", Opcode::UDEC},
        {"BSLC", Opcode::BSLC}, {"RSTB", Opcode::RSTB},
        {"RBIN", Opcode::RBIN}, {"WBIN", Opcode::WBIN},
        {"HENC", Opcode::HENC}, {"HDEC", Opcode::HDEC},
        {"B64E", Opcode::B64E}, {"B64D", Opcode::B64D},
        {"PJON", Opcode::PJON}, {"PPAR", Opcode::PPAR},
        {"PNAM", Opcode::PNAM}, {"PEXT", Opcode::PEXT},
        {"PABS", Opcode::PABS}, {"PNOR", Opcode::PNOR},
        {"PEXS", Opcode::PEXS}, {"PFIL", Opcode::PFIL},
        {"PDIR", Opcode::PDIR}, {"FSIZ", Opcode::FSIZ},
        {"LDIR", Opcode::LDIR}, {"MDIR", Opcode::MDIR},
        {"CPFL", Opcode::CPFL}, {"MVFL", Opcode::MVFL},
        {"RMFL", Opcode::RMFL}, {"RMDR", Opcode::RMDR},
        {"DTIM", Opcode::DTIM},
        {"SPWN", Opcode::SPWN}, {"SELF", Opcode::SELF},
        {"SEND", Opcode::SEND}, {"RECV", Opcode::RECV},
        {"RNOW", Opcode::RNOW}, {"WALV", Opcode::WALV},
        {"WWAIT", Opcode::WWAIT},
        {"DBUG", Opcode::DEBUG},
    };
    auto found = str_to_opcode.find(str);
    if (found == str_to_opcode.end())
        raise_nvm_error("Unknown Nambly opcode: " + string(str));
    return found->second;
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
    case Opcode::ENVV:
        return "ENVV";
    case Opcode::RSTD:
        return "RSTD";
    case Opcode::URLD:
        return "URLD";
    case Opcode::QPRS:
        return "QPRS";
    case Opcode::JDEC:
        return "JDEC";
    case Opcode::JENC:
        return "JENC";
    case Opcode::CGIR:
        return "CGIR";
    case Opcode::CGIO:
        return "CGIO";
    case Opcode::PTRY: return "PTRY";
    case Opcode::ETRY: return "ETRY";
    case Opcode::CERR: return "CERR";
    case Opcode::NCTH: return "NCTH";
    case Opcode::ECTH: return "ECTH";
    case Opcode::EFIN: return "EFIN";
    case Opcode::THRW: return "THRW";
    case Opcode::FJMP: return "FJMP";
    case Opcode::FRET: return "FRET";
    case Opcode::BNEW: return "BNEW";
    case Opcode::UENC: return "UENC";
    case Opcode::UDEC: return "UDEC";
    case Opcode::BSLC: return "BSLC";
    case Opcode::RSTB: return "RSTB";
    case Opcode::RBIN: return "RBIN";
    case Opcode::WBIN: return "WBIN";
    case Opcode::HENC: return "HENC";
    case Opcode::HDEC: return "HDEC";
    case Opcode::B64E: return "B64E";
    case Opcode::B64D: return "B64D";
    case Opcode::PJON: return "PJON";
    case Opcode::PPAR: return "PPAR";
    case Opcode::PNAM: return "PNAM";
    case Opcode::PEXT: return "PEXT";
    case Opcode::PABS: return "PABS";
    case Opcode::PNOR: return "PNOR";
    case Opcode::PEXS: return "PEXS";
    case Opcode::PFIL: return "PFIL";
    case Opcode::PDIR: return "PDIR";
    case Opcode::FSIZ: return "FSIZ";
    case Opcode::LDIR: return "LDIR";
    case Opcode::MDIR: return "MDIR";
    case Opcode::CPFL: return "CPFL";
    case Opcode::MVFL: return "MVFL";
    case Opcode::RMFL: return "RMFL";
    case Opcode::RMDR: return "RMDR";
    case Opcode::DTIM: return "DTIM";
    case Opcode::SPWN: return "SPWN";
    case Opcode::SELF: return "SELF";
    case Opcode::SEND: return "SEND";
    case Opcode::RECV: return "RECV";
    case Opcode::RNOW: return "RNOW";
    case Opcode::WALV: return "WALV";
    case Opcode::WWAIT: return "WWAIT";
    case Opcode::DEBUG:
        return "DBUG";
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

bool is_utf8_continuation(unsigned char byte)
{
    return byte >= 0x80 && byte <= 0xBF;
}

vector<size_t> build_utf8_offsets(const string &text)
{
    vector<size_t> offsets;
    offsets.reserve(text.size() + 1);
    offsets.push_back(0);

    size_t i = 0;
    while (i < text.size())
    {
        const unsigned char first = static_cast<unsigned char>(text[i]);
        size_t width = 1;

        if (first >= 0xC2 && first <= 0xDF && i + 1 < text.size() &&
            is_utf8_continuation(static_cast<unsigned char>(text[i + 1])))
        {
            width = 2;
        }
        else if (first == 0xE0 && i + 2 < text.size() &&
                 static_cast<unsigned char>(text[i + 1]) >= 0xA0 &&
                 static_cast<unsigned char>(text[i + 1]) <= 0xBF &&
                 is_utf8_continuation(static_cast<unsigned char>(text[i + 2])))
        {
            width = 3;
        }
        else if (((first >= 0xE1 && first <= 0xEC) ||
                  (first >= 0xEE && first <= 0xEF)) &&
                 i + 2 < text.size() &&
                 is_utf8_continuation(static_cast<unsigned char>(text[i + 1])) &&
                 is_utf8_continuation(static_cast<unsigned char>(text[i + 2])))
        {
            width = 3;
        }
        else if (first == 0xED && i + 2 < text.size() &&
                 static_cast<unsigned char>(text[i + 1]) >= 0x80 &&
                 static_cast<unsigned char>(text[i + 1]) <= 0x9F &&
                 is_utf8_continuation(static_cast<unsigned char>(text[i + 2])))
        {
            width = 3;
        }
        else if (first == 0xF0 && i + 3 < text.size() &&
                 static_cast<unsigned char>(text[i + 1]) >= 0x90 &&
                 static_cast<unsigned char>(text[i + 1]) <= 0xBF &&
                 is_utf8_continuation(static_cast<unsigned char>(text[i + 2])) &&
                 is_utf8_continuation(static_cast<unsigned char>(text[i + 3])))
        {
            width = 4;
        }
        else if (first >= 0xF1 && first <= 0xF3 && i + 3 < text.size() &&
                 is_utf8_continuation(static_cast<unsigned char>(text[i + 1])) &&
                 is_utf8_continuation(static_cast<unsigned char>(text[i + 2])) &&
                 is_utf8_continuation(static_cast<unsigned char>(text[i + 3])))
        {
            width = 4;
        }
        else if (first == 0xF4 && i + 3 < text.size() &&
                 static_cast<unsigned char>(text[i + 1]) >= 0x80 &&
                 static_cast<unsigned char>(text[i + 1]) <= 0x8F &&
                 is_utf8_continuation(static_cast<unsigned char>(text[i + 2])) &&
                 is_utf8_continuation(static_cast<unsigned char>(text[i + 3])))
        {
            width = 4;
        }

        // Invalid UTF-8 is deliberately consumed one byte at a time. This
        // keeps arbitrary file and subprocess data indexable without crashes.
        i += width;
        offsets.push_back(i);
    }
    return offsets;
}

bool is_valid_utf8(const string &text)
{
    size_t i = 0;
    while (i < text.size())
    {
        const unsigned char first = static_cast<unsigned char>(text[i]);
        if (first <= 0x7F)
        {
            ++i;
            continue;
        }

        size_t width = 0;
        if (first >= 0xC2 && first <= 0xDF && i + 1 < text.size() &&
            is_utf8_continuation(static_cast<unsigned char>(text[i + 1])))
            width = 2;
        else if (first == 0xE0 && i + 2 < text.size() &&
                 static_cast<unsigned char>(text[i + 1]) >= 0xA0 &&
                 static_cast<unsigned char>(text[i + 1]) <= 0xBF &&
                 is_utf8_continuation(static_cast<unsigned char>(text[i + 2])))
            width = 3;
        else if (((first >= 0xE1 && first <= 0xEC) ||
                  (first >= 0xEE && first <= 0xEF)) &&
                 i + 2 < text.size() &&
                 is_utf8_continuation(static_cast<unsigned char>(text[i + 1])) &&
                 is_utf8_continuation(static_cast<unsigned char>(text[i + 2])))
            width = 3;
        else if (first == 0xED && i + 2 < text.size() &&
                 static_cast<unsigned char>(text[i + 1]) >= 0x80 &&
                 static_cast<unsigned char>(text[i + 1]) <= 0x9F &&
                 is_utf8_continuation(static_cast<unsigned char>(text[i + 2])))
            width = 3;
        else if (first == 0xF0 && i + 3 < text.size() &&
                 static_cast<unsigned char>(text[i + 1]) >= 0x90 &&
                 static_cast<unsigned char>(text[i + 1]) <= 0xBF &&
                 is_utf8_continuation(static_cast<unsigned char>(text[i + 2])) &&
                 is_utf8_continuation(static_cast<unsigned char>(text[i + 3])))
            width = 4;
        else if (first >= 0xF1 && first <= 0xF3 && i + 3 < text.size() &&
                 is_utf8_continuation(static_cast<unsigned char>(text[i + 1])) &&
                 is_utf8_continuation(static_cast<unsigned char>(text[i + 2])) &&
                 is_utf8_continuation(static_cast<unsigned char>(text[i + 3])))
            width = 4;
        else if (first == 0xF4 && i + 3 < text.size() &&
                 static_cast<unsigned char>(text[i + 1]) >= 0x80 &&
                 static_cast<unsigned char>(text[i + 1]) <= 0x8F &&
                 is_utf8_continuation(static_cast<unsigned char>(text[i + 2])) &&
                 is_utf8_continuation(static_cast<unsigned char>(text[i + 3])))
            width = 4;

        if (width == 0)
            return false;
        i += width;
    }
    return true;
}

class Value
{
private:
    char type;
    bool has_num_rep;
    bool has_str_rep;
    string str_rep;
    double num_rep;
    uint64_t pid_rep = 0;
    shared_ptr<map<string, Value>> table_rep;
    shared_ptr<vector<unsigned char>> bytes_rep;
    shared_ptr<queue<string> /**/> iterator_elements;
    shared_ptr<vector<size_t>> utf8_offsets;

    void reset_values()
    {
        has_num_rep = false;
        has_str_rep = false;
        table_rep = nullptr;
        bytes_rep = nullptr;
        utf8_offsets = nullptr;
        pid_rep = 0;
    }

public:
    Value() : type(NIL), has_num_rep(false), has_str_rep(false), num_rep(0) {}

    void set_string_value(const string &value)
    {
        reset_values();
        this->str_rep = value;
        this->type = TEXT;
        has_str_rep = true;
        utf8_offsets = make_shared<vector<size_t>>();
    }

    void set_number_value(double value)
    {
        reset_values();
        this->num_rep = value;
        this->type = NUMB;
        has_num_rep = true;
        utf8_offsets = make_shared<vector<size_t>>();
    }

    void set_table_value()
    {
        reset_values();
        this->table_rep = std::make_shared<map<string, Value>>();
        this->type = TABLE;
    }

    void set_bytes_value(const vector<unsigned char> &value)
    {
        reset_values();
        bytes_rep = make_shared<vector<unsigned char>>(value);
        type = BYTES;
    }

    void set_bytes_value(vector<unsigned char> &&value)
    {
        reset_values();
        bytes_rep = make_shared<vector<unsigned char>>(std::move(value));
        type = BYTES;
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

    void set_pid_value(uint64_t value)
    {
        reset_values();
        pid_rep = value;
        type = PID;
    }

    char get_type() const
    {
        return type;
    }

    uint64_t get_pid() const
    {
        return pid_rep;
    }

    map<string, Value> *get_table()
    {
        return table_rep.get();
    }

    const map<string, Value> *get_table() const
    {
        return table_rep.get();
    }

    vector<unsigned char> *get_bytes()
    {
        return bytes_rep.get();
    }

    const vector<unsigned char> *get_bytes() const
    {
        return bytes_rep.get();
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
            else if (type == BYTES)
            {
                raise_nvm_error("Can't convert BYTES value to string; use utf8_decode, hex_encode, or base64_encode.");
            }
            else if (type == PID)
            {
                str_rep = "<pid:" + to_string(pid_rep) + ">";
            }
            else if (type == NUMB)
            {
                str_rep = double_to_string(get_as_number());
            }
            return str_rep;
        }
    }

    const vector<size_t> &get_utf8_offsets()
    {
        const string &text = get_as_string();
        if (!utf8_offsets)
        {
            utf8_offsets = make_shared<vector<size_t>>();
        }
        if (utf8_offsets->empty())
        {
            *utf8_offsets = build_utf8_offsets(text);
        }
        return *utf8_offsets;
    }

    size_t get_codepoint_count()
    {
        return get_utf8_offsets().size() - 1;
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
            else if (type == BYTES)
            {
                raise_nvm_error("Can't convert BYTES value to number.");
            }
            else if (type == PID)
            {
                raise_nvm_error("Can't convert PID value to number.");
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

Value text_value(const string &text)
{
    Value value;
    value.set_string_value(text);
    return value;
}

Value number_value(double number)
{
    Value value;
    value.set_number_value(number);
    return value;
}

Value table_value()
{
    Value value;
    value.set_table_value();
    return value;
}

Value bytes_value(const string &bytes)
{
    Value value;
    value.set_bytes_value(vector<unsigned char>(bytes.begin(), bytes.end()));
    return value;
}

Value bytes_value(vector<unsigned char> bytes)
{
    Value value;
    value.set_bytes_value(std::move(bytes));
    return value;
}

Value pid_value(uint64_t pid)
{
    Value value;
    value.set_pid_value(pid);
    return value;
}

char **process_environment()
{
#if defined(_WIN32)
    return _environ;
#else
    extern char **environ;
    return environ;
#endif
}

Value environment_table()
{
    Value result = table_value();
    char **entries = process_environment();
    if (!entries)
        return result;
    for (; *entries; ++entries)
    {
        string entry(*entries);
        size_t separator = entry.find('=');
        if (separator != string::npos)
            (*result.get_table())[entry.substr(0, separator)] = text_value(entry.substr(separator + 1));
    }
    return result;
}

string environment_text(const string &name)
{
    const char *value = getenv(name.c_str());
    return value ? string(value) : string();
}

int hex_digit(char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    return -1;
}

string url_decode_text(const string &input, bool plus_as_space)
{
    string result;
    result.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i)
    {
        if (plus_as_space && input[i] == '+')
        {
            result += ' ';
        }
        else if (input[i] == '%' && i + 2 < input.size())
        {
            int high = hex_digit(input[i + 1]);
            int low = hex_digit(input[i + 2]);
            if (high >= 0 && low >= 0)
            {
                result += static_cast<char>((high << 4) | low);
                i += 2;
            }
            else
            {
                result += input[i];
            }
        }
        else
        {
            result += input[i];
        }
    }
    return result;
}

Value parse_query_text(const string &query)
{
    Value result = table_value();
    size_t start = 0;
    while (start < query.size())
    {
        size_t end = query.find('&', start);
        if (end == string::npos)
            end = query.size();
        string field = query.substr(start, end - start);
        size_t equal = field.find('=');
        string key = url_decode_text(field.substr(0, equal), true);
        string value = equal == string::npos ? string() : url_decode_text(field.substr(equal + 1), true);
        (*result.get_table())[key] = text_value(value);
        if (end == query.size())
            break;
        start = end + 1;
    }
    return result;
}

void append_utf8(string &output, unsigned int codepoint)
{
    if (codepoint <= 0x7F)
        output += static_cast<char>(codepoint);
    else if (codepoint <= 0x7FF)
    {
        output += static_cast<char>(0xC0 | (codepoint >> 6));
        output += static_cast<char>(0x80 | (codepoint & 0x3F));
    }
    else if (codepoint <= 0xFFFF)
    {
        output += static_cast<char>(0xE0 | (codepoint >> 12));
        output += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        output += static_cast<char>(0x80 | (codepoint & 0x3F));
    }
    else
    {
        output += static_cast<char>(0xF0 | (codepoint >> 18));
        output += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
        output += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        output += static_cast<char>(0x80 | (codepoint & 0x3F));
    }
}

class JsonParser
{
private:
    const string &input;
    size_t position = 0;

    [[noreturn]] void fail(const string &message)
    {
        raise_nvm_error("Invalid JSON at byte " + to_string(position + 1) + ": " + message);
        std::abort();
    }

    void whitespace()
    {
        while (position < input.size() &&
               (input[position] == ' ' || input[position] == '\t' ||
                input[position] == '\r' || input[position] == '\n'))
            ++position;
    }

    bool take(char expected)
    {
        whitespace();
        if (position < input.size() && input[position] == expected)
        {
            ++position;
            return true;
        }
        return false;
    }

    unsigned int unicode_escape()
    {
        if (position + 4 > input.size())
            fail("incomplete Unicode escape");
        unsigned int codepoint = 0;
        for (int i = 0; i < 4; ++i)
        {
            int digit = hex_digit(input[position++]);
            if (digit < 0)
                fail("invalid Unicode escape");
            codepoint = (codepoint << 4) | static_cast<unsigned int>(digit);
        }
        return codepoint;
    }

    string string_value()
    {
        whitespace();
        if (position >= input.size() || input[position++] != '"')
            fail("expected a string");
        string result;
        while (position < input.size())
        {
            unsigned char ch = static_cast<unsigned char>(input[position++]);
            if (ch == '"')
            {
                if (!is_valid_utf8(result))
                    fail("string is not valid UTF-8");
                return result;
            }
            if (ch < 0x20)
                fail("unescaped control character in string");
            if (ch != '\\')
            {
                result += static_cast<char>(ch);
                continue;
            }
            if (position >= input.size())
                fail("incomplete escape sequence");
            char escaped = input[position++];
            switch (escaped)
            {
            case '"': result += '"'; break;
            case '\\': result += '\\'; break;
            case '/': result += '/'; break;
            case 'b': result += '\b'; break;
            case 'f': result += '\f'; break;
            case 'n': result += '\n'; break;
            case 'r': result += '\r'; break;
            case 't': result += '\t'; break;
            case 'u':
            {
                unsigned int codepoint = unicode_escape();
                if (codepoint >= 0xD800 && codepoint <= 0xDBFF)
                {
                    if (position + 2 > input.size() || input[position] != '\\' || input[position + 1] != 'u')
                        fail("high surrogate without a low surrogate");
                    position += 2;
                    unsigned int low = unicode_escape();
                    if (low < 0xDC00 || low > 0xDFFF)
                        fail("invalid low surrogate");
                    codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (low - 0xDC00);
                }
                else if (codepoint >= 0xDC00 && codepoint <= 0xDFFF)
                {
                    fail("unexpected low surrogate");
                }
                append_utf8(result, codepoint);
                break;
            }
            default:
                fail("unknown escape sequence");
            }
        }
        fail("unterminated string");
    }

    Value number()
    {
        size_t start = position;
        if (position < input.size() && input[position] == '-')
            ++position;
        if (position >= input.size())
            fail("incomplete number");
        if (input[position] == '0')
            ++position;
        else if (input[position] >= '1' && input[position] <= '9')
        {
            while (position < input.size() && isdigit(static_cast<unsigned char>(input[position])))
                ++position;
        }
        else
            fail("invalid number");
        if (position < input.size() && input[position] == '.')
        {
            ++position;
            size_t fraction = position;
            while (position < input.size() && isdigit(static_cast<unsigned char>(input[position])))
                ++position;
            if (fraction == position)
                fail("fraction has no digits");
        }
        if (position < input.size() && (input[position] == 'e' || input[position] == 'E'))
        {
            ++position;
            if (position < input.size() && (input[position] == '+' || input[position] == '-'))
                ++position;
            size_t exponent = position;
            while (position < input.size() && isdigit(static_cast<unsigned char>(input[position])))
                ++position;
            if (exponent == position)
                fail("exponent has no digits");
        }
        double parsed = 0;
        try
        {
            parsed = stod(input.substr(start, position - start));
        }
        catch (...)
        {
            fail("number is outside the supported range");
        }
        if (!isfinite(parsed))
            fail("number is outside the supported range");
        return number_value(parsed);
    }

    Value array()
    {
        Value result = table_value();
        if (take(']'))
            return result;
        size_t index = 1;
        while (true)
        {
            (*result.get_table())[double_to_string(index++)] = value();
            if (take(']'))
                return result;
            if (!take(','))
                fail("expected ',' or ']' in array");
        }
    }

    Value object()
    {
        Value result = table_value();
        if (take('}'))
            return result;
        while (true)
        {
            string key = string_value();
            if (!take(':'))
                fail("expected ':' after object key");
            (*result.get_table())[key] = value();
            if (take('}'))
                return result;
            if (!take(','))
                fail("expected ',' or '}' in object");
        }
    }

    Value value()
    {
        whitespace();
        if (position >= input.size())
            fail("expected a value");
        char ch = input[position];
        if (ch == '"')
            return text_value(string_value());
        if (ch == '{')
        {
            ++position;
            return object();
        }
        if (ch == '[')
        {
            ++position;
            return array();
        }
        if (ch == '-' || isdigit(static_cast<unsigned char>(ch)))
            return number();
        if (input.compare(position, 4, "true") == 0)
        {
            position += 4;
            return number_value(1);
        }
        if (input.compare(position, 5, "false") == 0)
        {
            position += 5;
            return number_value(0);
        }
        if (input.compare(position, 4, "null") == 0)
        {
            position += 4;
            Value nil;
            nil.set_nil_value();
            return nil;
        }
        fail("unexpected token");
    }

public:
    explicit JsonParser(const string &text) : input(text) {}

    Value parse()
    {
        Value result = value();
        whitespace();
        if (position != input.size())
            fail("unexpected text after the value");
        return result;
    }
};

string json_escape(const string &text)
{
    if (!is_valid_utf8(text))
        raise_nvm_error("Cannot encode invalid UTF-8 text as JSON.");
    ostringstream output;
    output << '"';
    for (unsigned char ch : text)
    {
        switch (ch)
        {
        case '"': output << "\\\""; break;
        case '\\': output << "\\\\"; break;
        case '\b': output << "\\b"; break;
        case '\f': output << "\\f"; break;
        case '\n': output << "\\n"; break;
        case '\r': output << "\\r"; break;
        case '\t': output << "\\t"; break;
        default:
            if (ch < 0x20)
                output << "\\u" << hex << setw(4) << setfill('0') << static_cast<int>(ch) << dec;
            else
                output << static_cast<char>(ch);
        }
    }
    output << '"';
    return output.str();
}

string json_encode_value(Value value, set<const map<string, Value> *> &active)
{
    if (value.get_type() == NIL)
        return "null";
    if (value.get_type() == TEXT)
        return json_escape(value.get_as_string());
    if (value.get_type() == NUMB)
    {
        double number = value.get_as_number();
        if (!isfinite(number))
            raise_nvm_error("Cannot encode a non-finite number as JSON.");
        return double_to_string(number);
    }
    if (value.get_type() != TABLE)
        raise_nvm_error("Cannot encode " + get_type_name(value.get_type()) + " as JSON.");

    map<string, Value> *table = value.get_table();
    if (active.count(table))
        raise_nvm_error("Cannot encode a cyclic table as JSON.");
    active.insert(table);

    bool array = !table->empty();
    for (size_t i = 1; array && i <= table->size(); ++i)
        array = table->count(double_to_string(i)) != 0;

    string result = array ? "[" : "{";
    if (array)
    {
        for (size_t i = 1; i <= table->size(); ++i)
        {
            if (i > 1)
                result += ',';
            result += json_encode_value(table->at(double_to_string(i)), active);
        }
        result += ']';
    }
    else
    {
        bool first = true;
        for (auto &entry : *table)
        {
            if (!first)
                result += ',';
            first = false;
            result += json_escape(entry.first) + ':' + json_encode_value(entry.second, active);
        }
        result += '}';
    }
    active.erase(table);
    return result;
}

string read_standard_input(long long requested)
{
    if (requested < 0)
    {
        ostringstream input;
        input << cin.rdbuf();
        return input.str();
    }
    if (static_cast<unsigned long long>(requested) >
        static_cast<unsigned long long>(numeric_limits<streamsize>::max()))
        raise_nvm_error("Requested standard-input length is too large.");
    string result(static_cast<size_t>(requested), '\0');
    cin.read(result.data(), static_cast<streamsize>(requested));
    result.resize(static_cast<size_t>(cin.gcount()));
    return result;
}

Value cgi_request_value(long long maximum_body)
{
    if (maximum_body < 0)
        raise_nvm_error("CGI maximum body size cannot be negative.");

    string length_text = environment_text("CONTENT_LENGTH");
    long long content_length = 0;
    if (!length_text.empty())
    {
        try
        {
            size_t consumed = 0;
            content_length = stoll(length_text, &consumed);
            if (consumed != length_text.size() || content_length < 0)
                throw invalid_argument("content length");
        }
        catch (...)
        {
            raise_nvm_error("Invalid CGI CONTENT_LENGTH value.");
        }
    }
    if (content_length > maximum_body)
        raise_nvm_error("CGI request body exceeds the configured limit of " +
                        to_string(maximum_body) + " bytes.");

    string body = read_standard_input(content_length);
    if (static_cast<long long>(body.size()) != content_length)
        raise_nvm_error("CGI request body ended before CONTENT_LENGTH bytes were read.");

    string query_string = environment_text("QUERY_STRING");
    string content_type = environment_text("CONTENT_TYPE");
    Value environment = environment_table();
    Value headers = table_value();
    for (auto &entry : *environment.get_table())
    {
        string name;
        if (entry.first.rfind("HTTP_", 0) == 0)
            name = entry.first.substr(5);
        else if (entry.first == "CONTENT_TYPE" || entry.first == "CONTENT_LENGTH")
            name = entry.first;
        else
            continue;
        transform(name.begin(), name.end(), name.begin(), [](unsigned char ch) {
            return ch == '_' ? '-' : static_cast<char>(tolower(ch));
        });
        (*headers.get_table())[name] = entry.second;
    }

    Value request = table_value();
    (*request.get_table())["method"] = text_value(environment_text("REQUEST_METHOD"));
    (*request.get_table())["query_string"] = text_value(query_string);
    (*request.get_table())["query"] = parse_query_text(query_string);
    (*request.get_table())["content_type"] = text_value(content_type);
    (*request.get_table())["content_length"] = number_value(content_length);
    (*request.get_table())["body"] = text_value(body);
    (*request.get_table())["body_bytes"] = bytes_value(body);
    (*request.get_table())["path_info"] = text_value(environment_text("PATH_INFO"));
    (*request.get_table())["script_name"] = text_value(environment_text("SCRIPT_NAME"));
    (*request.get_table())["remote_addr"] = text_value(environment_text("REMOTE_ADDR"));
    (*request.get_table())["request_uri"] = text_value(environment_text("REQUEST_URI"));
    (*request.get_table())["headers"] = headers;
    (*request.get_table())["environment"] = environment;

    Value form = table_value();
    string lowered = content_type;
    transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) {
        return static_cast<char>(tolower(ch));
    });
    size_t parameter = lowered.find(';');
    string media_type = lowered.substr(0, parameter);
    size_t media_start = 0;
    while (media_start < media_type.size() &&
           isspace(static_cast<unsigned char>(media_type[media_start])))
        ++media_start;
    size_t media_end = media_type.size();
    while (media_end > media_start &&
           isspace(static_cast<unsigned char>(media_type[media_end - 1])))
        --media_end;
    media_type = media_type.substr(media_start, media_end - media_start);
    if (media_type == "application/x-www-form-urlencoded")
        form = parse_query_text(body);
    (*request.get_table())["form"] = form;
    return request;
}

string lower_ascii(string text)
{
    transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(tolower(ch));
    });
    return text;
}

bool valid_header_name(const string &name)
{
    if (name.empty())
        return false;
    for (unsigned char ch : name)
        if (!(isalnum(ch) || ch == '-'))
            return false;
    return true;
}

string status_reason(int status)
{
    static const map<int, string> reasons = {
        {200, "OK"}, {201, "Created"}, {202, "Accepted"}, {204, "No Content"},
        {301, "Moved Permanently"}, {302, "Found"}, {304, "Not Modified"},
        {400, "Bad Request"}, {401, "Unauthorized"}, {403, "Forbidden"},
        {404, "Not Found"}, {405, "Method Not Allowed"}, {409, "Conflict"},
        {413, "Content Too Large"}, {415, "Unsupported Media Type"},
        {422, "Unprocessable Content"}, {429, "Too Many Requests"},
        {500, "Internal Server Error"}, {501, "Not Implemented"},
        {502, "Bad Gateway"}, {503, "Service Unavailable"}};
    auto found = reasons.find(status);
    return found == reasons.end() ? "Status" : found->second;
}

void write_cgi_response(int status, const string &content_type, Value body, Value headers)
{
    if (status < 100 || status > 999)
        raise_nvm_error("CGI response status must be between 100 and 999.");
    if (content_type.empty() || content_type.find('\r') != string::npos ||
        content_type.find('\n') != string::npos)
        raise_nvm_error("Invalid CGI response content type.");
    if (headers.get_type() != TABLE)
        raise_nvm_error("CGI response headers must be a table.");
    if (body.get_type() != TEXT && body.get_type() != BYTES)
        raise_nvm_error("CGI response body must be TEXT or BYTES.");

    vector<pair<string, string>> validated_headers;
    for (auto &entry : *headers.get_table())
    {
        string lowered = lower_ascii(entry.first);
        string value = entry.second.get_as_string();
        if (!valid_header_name(entry.first) || value.find('\r') != string::npos ||
            value.find('\n') != string::npos)
            raise_nvm_error("Invalid CGI response header.");
        if (lowered == "status" || lowered == "content-type")
            raise_nvm_error("Pass Status and Content-Type through their dedicated CGI response arguments.");
        validated_headers.emplace_back(entry.first, std::move(value));
    }

    cout << "Status: " << status << ' ' << status_reason(status) << "\r\n";
    cout << "Content-Type: " << content_type << "\r\n";
    for (const auto &entry : validated_headers)
        cout << entry.first << ": " << entry.second << "\r\n";
    cout << "\r\n";
    if (body.get_type() == BYTES)
    {
        const auto &data = *body.get_bytes();
        cout.write(reinterpret_cast<const char *>(data.data()), static_cast<streamsize>(data.size()));
    }
    else if (body.get_type() == TEXT)
    {
        const string &text = body.get_as_string();
        cout.write(text.data(), static_cast<streamsize>(text.size()));
    }
    cout.flush();
}

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

    size_t get_line_number() const
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

class VmException : public exception
{
public:
    explicit VmException(Value value) : error_value(std::move(value))
    {
        auto found = error_value.get_table()->find("message");
        message = found == error_value.get_table()->end()
                      ? "Katalyn runtime error"
                      : found->second.get_as_string();
    }

    const char *what() const noexcept override { return message.c_str(); }
    Value error() const { return error_value; }

private:
    Value error_value;
    string message;
};

enum class HandlerPhase { Trying, Catching, Finally };

struct ErrorHandler
{
    size_t start_pc = 0;
    size_t catch_pc = 0;
    size_t finally_pc = 0;
    size_t end_pc = 0;
    size_t stack_depth = 0;
    size_t scope_depth = 0;
    size_t return_depth = 0;
    HandlerPhase phase = HandlerPhase::Trying;
    optional<Value> caught_error;
    optional<Value> pending_error;
};

enum class ControlKind { None, Jump, Return };

class ActorSystem;

struct RuntimeState
{
    size_t pc = 0;
    Command *last_command = nullptr;
    vector<unordered_map<string, Value>> variable_tables;
    map<string, size_t> label_to_pc;
    map<size_t, string> pc_to_label;
    stack<Value> execution_stack;
    map<string, fstream *> open_files;
    set<string> untruncated_files;
    set<string> read_only_files;
    stack<size_t> return_stack;
    vector<ErrorHandler> error_handlers;
    ControlKind pending_control = ControlKind::None;
    size_t pending_jump = 0;
    optional<Value> pending_return;
    Value nil_value;
    shared_ptr<ActorSystem> actor_system;
    uint64_t worker_id = 0;

    map<string, size_t> &labels() { return label_to_pc; }
    map<size_t, string> &reverse_labels() { return pc_to_label; }
    vector<unordered_map<string, Value>> &scopes() { return variable_tables; }

    ~RuntimeState()
    {
        for (auto &entry : open_files)
        {
            entry.second->close();
            delete entry.second;
        }
    }
};

struct ActorEnvelope
{
    uint64_t from = 0;
    Value message;
};

struct WorkerRecord
{
    explicit WorkerRecord(uint64_t worker) : id(worker) {}
    uint64_t id;
    mutex lock;
    condition_variable changed;
    deque<ActorEnvelope> inbox;
    bool finished = false;
    optional<Value> result;
    optional<Value> error;
    thread execution;
};

class ActorSystem : public enable_shared_from_this<ActorSystem>
{
public:
    ActorSystem(shared_ptr<vector<Command>> listing,
                map<string, size_t> labels,
                map<size_t, string> reverse_labels);
    ~ActorSystem();

    uint64_t spawn(size_t entry_pc, vector<Value> arguments,
                   const unordered_map<string, Value> &globals,
                   string caller_context);
    void send(uint64_t from, uint64_t destination, Value message);
    optional<ActorEnvelope> receive(uint64_t worker, optional<double> timeout_seconds);
    bool alive(uint64_t worker);
    Value wait(uint64_t caller, uint64_t worker, optional<double> timeout_seconds);
    void finish(uint64_t worker, optional<Value> result, optional<Value> error);
    void wait_for_all();

    shared_ptr<vector<Command>> code_listing;
    map<string, size_t> program_labels;
    map<size_t, string> program_reverse_labels;

private:
    shared_ptr<WorkerRecord> find(uint64_t worker);
    mutex workers_lock;
    unordered_map<uint64_t, shared_ptr<WorkerRecord>> workers;
    atomic<uint64_t> next_id{2};
};

thread_local RuntimeState *active_runtime = nullptr;

RuntimeState &runtime_state()
{
    if (!active_runtime)
        throw logic_error("NariVM runtime state is unavailable.");
    return *active_runtime;
}

struct RuntimeActivation
{
    explicit RuntimeActivation(RuntimeState &state) : previous(active_runtime)
    {
        active_runtime = &state;
    }
    ~RuntimeActivation() { active_runtime = previous; }
    RuntimeState *previous;
};

Value make_error_value(const string &message, const string &kind = "RuntimeError")
{
    Value error = table_value();
    (*error.get_table())["kind"] = text_value(kind);
    (*error.get_table())["message"] = text_value(message);
    RuntimeState &state = runtime_state();
    if (state.last_command)
    {
        (*error.get_table())["file"] = text_value(state.last_command->get_file());
        (*error.get_table())["line"] = number_value(state.last_command->get_line_number());
    }
    (*error.get_table())["pc"] = number_value(state.pc + 1);
    return error;
}

void raise_nvm_error(string error_message)
{
    throw VmException(make_error_value(error_message));
}

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
        if (runtime_state().pc_to_label.count(i) > 0)
        {
            cout << "[" << runtime_state().pc_to_label[i] << "] => ";
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
            auto &state = runtime_state();
            if (state.label_to_pc.find(label_name) == state.label_to_pc.end())
            {
                state.label_to_pc[label_name] = jmp_pc_value;
                state.pc_to_label[pc] = label_name;
            }
            else
            {
                raise_nvm_error("Duplicate label: " + label_name);
            }
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
    for (auto &command : code_listing)
    {
        switch (command.get_opcode())
        {
        case Opcode::JUMP:
        case Opcode::JPIF:
        case Opcode::CALL:
        case Opcode::FJMP:
        {
            const string label = command.get_arguments()[0].get_raw_string_value();
            auto found = runtime_state().label_to_pc.find(label);
            if (found == runtime_state().label_to_pc.end())
                raise_nvm_error("Unknown label: " + label);
            pc = found->second - 1;
            command.set_branch_target(pc);
            break;
        }
        default:
            break;
        }
    }
    return code_listing;
}

// The VM implementation below intentionally reads its state through the active
// RuntimeState. These aliases keep the instruction implementations compact while
// ensuring separate and nested interpreter instances do not share mutable state.
#define pc (runtime_state().pc)
#define last_command (runtime_state().last_command)
#define variable_tables (runtime_state().variable_tables)
#define label_to_pc (runtime_state().label_to_pc)
#define pc_to_label (runtime_state().pc_to_label)
#define execution_stack (runtime_state().execution_stack)
#define open_files (runtime_state().open_files)
#define untruncated_files (runtime_state().untruncated_files)
#define read_only_files (runtime_state().read_only_files)
#define return_stack (runtime_state().return_stack)

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
    variable_tables.push_back(unordered_map<string, Value>());
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
        auto location = variable_tables[variable_tables.size() - 1].find(var_name);
        if (location != variable_tables[variable_tables.size() - 1].end())
        {
            return location->second;
        }
        location = variable_tables[0].find(var_name);
        if (location != variable_tables[0].end())
        {
            return location->second;
        }
    }
    return runtime_state().nil_value;
}

string substring(Value &value, long long from, long long count)
{
    const string &text = value.get_as_string();
    const vector<size_t> &offsets = value.get_utf8_offsets();
    long long len = static_cast<long long>(value.get_codepoint_count());

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

    const size_t byte_from = offsets[static_cast<size_t>(from)];
    const size_t byte_to = offsets[static_cast<size_t>(from + count)];
    return text.substr(byte_from, byte_to - byte_from);
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

string input(const string &prompt)
{
    string user_input = "";
    // getline(cin, user_input);
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
    else if (value.get_type() == BYTES)
    {
        return !value.get_bytes()->empty();
    }
    else if (value.get_type() == PID)
    {
        return true;
    }
    return false;
}

long long require_integer(Value value, const string &description)
{
    double number = value.get_as_number();
    if (!isfinite(number) || number != floor(number) ||
        number < static_cast<double>(numeric_limits<long long>::min()) ||
        number > static_cast<double>(numeric_limits<long long>::max()))
        raise_nvm_error(description + " must be an integer.");
    return static_cast<long long>(number);
}

string bytes_as_string(const Value &value)
{
    auto *data = value.get_bytes();
    return string(reinterpret_cast<const char *>(data->data()), data->size());
}

string encode_hex(const vector<unsigned char> &data)
{
    static constexpr char digits[] = "0123456789abcdef";
    string result;
    result.reserve(data.size() * 2);
    for (unsigned char byte : data)
    {
        result += digits[byte >> 4];
        result += digits[byte & 15];
    }
    return result;
}

vector<unsigned char> decode_hex(const string &text)
{
    if (text.size() % 2)
        raise_nvm_error("Hex text must contain an even number of digits.");
    vector<unsigned char> result;
    result.reserve(text.size() / 2);
    for (size_t i = 0; i < text.size(); i += 2)
    {
        int high = hex_digit(text[i]), low = hex_digit(text[i + 1]);
        if (high < 0 || low < 0)
            raise_nvm_error("Invalid hexadecimal digit.");
        result.push_back(static_cast<unsigned char>((high << 4) | low));
    }
    return result;
}

string encode_base64(const vector<unsigned char> &data)
{
    static constexpr char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    string result;
    for (size_t i = 0; i < data.size(); i += 3)
    {
        unsigned int chunk = static_cast<unsigned int>(data[i]) << 16;
        if (i + 1 < data.size()) chunk |= static_cast<unsigned int>(data[i + 1]) << 8;
        if (i + 2 < data.size()) chunk |= data[i + 2];
        result += alphabet[(chunk >> 18) & 63];
        result += alphabet[(chunk >> 12) & 63];
        result += i + 1 < data.size() ? alphabet[(chunk >> 6) & 63] : '=';
        result += i + 2 < data.size() ? alphabet[chunk & 63] : '=';
    }
    return result;
}

vector<unsigned char> decode_base64(const string &text)
{
    if (text.size() % 4)
        raise_nvm_error("Base64 text length must be a multiple of four.");
    auto value = [](unsigned char ch) -> int {
        if (ch >= 'A' && ch <= 'Z') return ch - 'A';
        if (ch >= 'a' && ch <= 'z') return ch - 'a' + 26;
        if (ch >= '0' && ch <= '9') return ch - '0' + 52;
        if (ch == '+') return 62;
        if (ch == '/') return 63;
        return -1;
    };
    vector<unsigned char> result;
    for (size_t i = 0; i < text.size(); i += 4)
    {
        const bool pad2 = text[i + 2] == '=', pad3 = text[i + 3] == '=';
        if ((pad2 && !pad3) || (i + 4 != text.size() && (pad2 || pad3)))
            raise_nvm_error("Invalid Base64 padding.");
        int a = value(text[i]), b = value(text[i + 1]);
        int c = pad2 ? 0 : value(text[i + 2]);
        int d = pad3 ? 0 : value(text[i + 3]);
        if (a < 0 || b < 0 || c < 0 || d < 0)
            raise_nvm_error("Invalid Base64 character.");
        unsigned int chunk = static_cast<unsigned int>((a << 18) | (b << 12) | (c << 6) | d);
        result.push_back(static_cast<unsigned char>((chunk >> 16) & 255));
        if (!pad2) result.push_back(static_cast<unsigned char>((chunk >> 8) & 255));
        if (!pad3) result.push_back(static_cast<unsigned char>(chunk & 255));
    }
    return result;
}

[[noreturn]] void raise_filesystem_error(const string &operation, const filesystem::path &path,
                                         const error_code &error)
{
    raise_nvm_error(operation + " '" + path.string() + "': " + error.message());
    std::abort();
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

size_t label_pc(const Value &label)
{
    auto found = label_to_pc.find(label.get_raw_string_value());
    if (found == label_to_pc.end())
        raise_nvm_error("Unknown label: " + label.get_raw_string_value());
    return found->second;
}

void trim_execution_stack(size_t depth)
{
    while (execution_stack.size() > depth)
        execution_stack.pop();
}

void trim_scopes(size_t depth)
{
    while (variable_tables.size() > depth)
        variable_tables.pop_back();
}

void trim_returns(size_t depth)
{
    while (return_stack.size() > depth)
        return_stack.pop();
}

void jump_to(size_t target)
{
    pc = target - 1;
}

bool handler_contains_target(const ErrorHandler &handler, size_t target)
{
    return target > handler.start_pc && target < handler.end_pc;
}

void continue_pending_control()
{
    RuntimeState &state = runtime_state();
    while (!state.error_handlers.empty())
    {
        ErrorHandler &handler = state.error_handlers.back();
        bool same_function = handler.return_depth == return_stack.size();
        bool must_leave = same_function &&
                          (state.pending_control == ControlKind::Return ||
                           !handler_contains_target(handler, state.pending_jump));
        if (!must_leave)
            break;
        if (handler.phase == HandlerPhase::Finally)
        {
            state.error_handlers.pop_back();
            continue;
        }
        handler.phase = HandlerPhase::Finally;
        handler.pending_error.reset();
        jump_to(handler.finally_pc);
        return;
    }

    if (state.pending_control == ControlKind::Jump)
    {
        size_t target = state.pending_jump;
        state.pending_control = ControlKind::None;
        jump_to(target);
        return;
    }

    if (state.pending_control == ControlKind::Return)
    {
        if (return_stack.empty() || !state.pending_return)
            raise_nvm_error("Empty return stack.");
        Value result = std::move(*state.pending_return);
        state.pending_return.reset();
        state.pending_control = ControlKind::None;
        if (execution_stack.empty())
            raise_nvm_error("Missing function return slot.");
        execution_stack.pop();
        if (variable_tables.empty())
            raise_nvm_error("No function scope to return from.");
        variable_tables.pop_back();
        pc = return_stack.top();
        return_stack.pop();
        push(std::move(result));
    }
}

bool dispatch_error(Value error)
{
    RuntimeState &state = runtime_state();
    state.pending_control = ControlKind::None;
    state.pending_return.reset();
    while (!state.error_handlers.empty())
    {
        ErrorHandler &handler = state.error_handlers.back();
        trim_execution_stack(handler.stack_depth);
        trim_scopes(handler.scope_depth);
        trim_returns(handler.return_depth);
        if (handler.phase == HandlerPhase::Trying)
        {
            handler.phase = HandlerPhase::Catching;
            handler.caught_error = std::move(error);
            jump_to(handler.catch_pc);
            return true;
        }
        if (handler.phase == HandlerPhase::Catching)
        {
            handler.phase = HandlerPhase::Finally;
            handler.pending_error = std::move(error);
            jump_to(handler.finally_pc);
            return true;
        }
        state.error_handlers.pop_back();
    }
    return false;
}

Value normalized_raised_value(Value value)
{
    if (value.get_type() == TEXT)
        return make_error_value(value.get_as_string(), "Error");
    if (value.get_type() != TABLE)
        raise_nvm_error("raise expects TEXT or an error TABLE.");
    auto &fields = *value.get_table();
    if (!fields.count("message")) fields["message"] = text_value("Katalyn error");
    if (!fields.count("kind")) fields["kind"] = text_value("Error");
    if (fields["message"].get_type() != TEXT)
        raise_nvm_error("Raised error TABLE field 'message' must be TEXT.");
    if (fields["kind"].get_type() != TEXT)
        raise_nvm_error("Raised error TABLE field 'kind' must be TEXT.");
    if (!fields.count("pc")) fields["pc"] = number_value(pc + 1);
    if (last_command && !fields.count("file"))
    {
        fields["file"] = text_value(last_command->get_file());
        fields["line"] = number_value(last_command->get_line_number());
    }
    return value;
}

Value clone_actor_value(const Value &source, unordered_map<const map<string, Value> *, Value> &seen)
{
    if (source.get_type() == ITER || source.get_type() == LISTLIMIT)
        raise_nvm_error("ITERATOR and internal LISTLIMIT values cannot cross worker boundaries.");
    if (source.get_type() != TABLE)
        return source;

    const auto *original = source.get_table();
    auto existing = seen.find(original);
    if (existing != seen.end())
        return existing->second;

    Value copy = table_value();
    seen[original] = copy;
    for (const auto &entry : *original)
        (*copy.get_table())[entry.first] = clone_actor_value(entry.second, seen);
    return copy;
}

Value clone_actor_value(const Value &source)
{
    unordered_map<const map<string, Value> *, Value> seen;
    return clone_actor_value(source, seen);
}

unordered_map<string, Value> clone_actor_globals(const unordered_map<string, Value> &source)
{
    unordered_map<const map<string, Value> *, Value> seen;
    unordered_map<string, Value> result;
    for (const auto &entry : source)
        result[entry.first] = clone_actor_value(entry.second, seen);
    return result;
}

void execute_code_listing(vector<Command> &code_listing, size_t start_pc = 0);

ActorSystem::ActorSystem(shared_ptr<vector<Command>> listing,
                         map<string, size_t> labels,
                         map<size_t, string> reverse_labels)
    : code_listing(std::move(listing)),
      program_labels(std::move(labels)),
      program_reverse_labels(std::move(reverse_labels))
{
    workers.emplace(1, make_shared<WorkerRecord>(1));
}

ActorSystem::~ActorSystem()
{
    vector<shared_ptr<WorkerRecord>> snapshot;
    {
        lock_guard<mutex> guard(workers_lock);
        for (auto &entry : workers)
            if (entry.first != 1)
                snapshot.push_back(entry.second);
    }
    for (auto &worker : snapshot)
    {
        if (!worker->execution.joinable())
            continue;
        if (worker->execution.get_id() == this_thread::get_id())
            worker->execution.detach();
        else
            worker->execution.join();
    }
}

shared_ptr<WorkerRecord> ActorSystem::find(uint64_t worker)
{
    lock_guard<mutex> guard(workers_lock);
    auto found = workers.find(worker);
    return found == workers.end() ? nullptr : found->second;
}

uint64_t ActorSystem::spawn(size_t entry_pc, vector<Value> arguments,
                            const unordered_map<string, Value> &globals,
                            string caller_context)
{
    uint64_t id = next_id.fetch_add(1);
    auto worker = make_shared<WorkerRecord>(id);
    auto copied_globals = clone_actor_globals(globals);
    for (Value &argument : arguments)
        argument = clone_actor_value(argument);
    {
        lock_guard<mutex> guard(workers_lock);
        workers[id] = worker;
    }

    shared_ptr<ActorSystem> system = shared_from_this();
    worker->execution = thread([system, worker, entry_pc,
                                arguments = std::move(arguments),
                                globals = std::move(copied_globals),
                                caller_context = std::move(caller_context)]() mutable {
        RuntimeState state;
        state.actor_system = system;
        state.worker_id = worker->id;
        state.labels() = system->program_labels;
        state.reverse_labels() = system->program_reverse_labels;
        state.scopes().push_back(std::move(globals));
        RuntimeActivation activation(state);
        optional<Value> result;
        optional<Value> error;
        try
        {
            push(text_value(caller_context));
            push(get_listlimit_value());
            for (auto &argument : arguments)
                push(std::move(argument));
            return_stack.push(system->code_listing->size());
            execute_code_listing(*system->code_listing, entry_pc);
            result = execution_stack.empty() ? get_nil_value() : clone_actor_value(execution_stack.top());
        }
        catch (const VmException &failure)
        {
            error = clone_actor_value(failure.error());
        }
        catch (const exception &failure)
        {
            error = make_error_value(failure.what(), "HostError");
        }
        system->finish(worker->id, std::move(result), std::move(error));
    });
    return id;
}

void ActorSystem::send(uint64_t from, uint64_t destination, Value message)
{
    auto worker = find(destination);
    if (!worker)
        raise_nvm_error("Unknown worker PID.");
    Value copied = clone_actor_value(message);
    {
        lock_guard<mutex> guard(worker->lock);
        if (worker->finished)
            raise_nvm_error("Cannot send a message to a finished worker.");
        worker->inbox.push_back({from, std::move(copied)});
    }
    worker->changed.notify_all();
}

optional<ActorEnvelope> ActorSystem::receive(uint64_t worker_id, optional<double> timeout_seconds)
{
    auto worker = find(worker_id);
    if (!worker)
        raise_nvm_error("Current worker is not registered.");
    unique_lock<mutex> guard(worker->lock);
    auto ready = [&] { return !worker->inbox.empty(); };
    if (!timeout_seconds)
        worker->changed.wait(guard, ready);
    else if (!worker->changed.wait_for(guard, chrono::duration<double>(*timeout_seconds), ready))
        return nullopt;
    ActorEnvelope envelope = std::move(worker->inbox.front());
    worker->inbox.pop_front();
    return envelope;
}

bool ActorSystem::alive(uint64_t worker_id)
{
    auto worker = find(worker_id);
    if (!worker)
        return false;
    lock_guard<mutex> guard(worker->lock);
    return !worker->finished;
}

Value ActorSystem::wait(uint64_t caller, uint64_t worker_id, optional<double> timeout_seconds)
{
    if (caller == worker_id)
        raise_nvm_error("A worker cannot wait for itself.");
    auto worker = find(worker_id);
    if (!worker)
        raise_nvm_error("Unknown worker PID.");
    unique_lock<mutex> guard(worker->lock);
    auto ready = [&] { return worker->finished; };
    bool completed = timeout_seconds
                         ? worker->changed.wait_for(guard, chrono::duration<double>(*timeout_seconds), ready)
                         : (worker->changed.wait(guard, ready), true);
    Value response = table_value();
    if (!completed)
    {
        (*response.get_table())["status"] = text_value("timeout");
        return response;
    }
    optional<Value> result = worker->result;
    optional<Value> error = worker->error;
    guard.unlock();
    if (error)
    {
        (*response.get_table())["status"] = text_value("error");
        (*response.get_table())["error"] = clone_actor_value(*error);
    }
    else
    {
        (*response.get_table())["status"] = text_value("done");
        (*response.get_table())["value"] = result ? clone_actor_value(*result) : get_nil_value();
    }
    return response;
}

void ActorSystem::finish(uint64_t worker_id, optional<Value> result, optional<Value> error)
{
    auto worker = find(worker_id);
    if (!worker)
        return;
    {
        lock_guard<mutex> guard(worker->lock);
        worker->result = std::move(result);
        worker->error = std::move(error);
        worker->finished = true;
    }
    worker->changed.notify_all();
}

void ActorSystem::wait_for_all()
{
    size_t joined = 0;
    while (true)
    {
        vector<shared_ptr<WorkerRecord>> snapshot;
        {
            lock_guard<mutex> guard(workers_lock);
            for (auto &entry : workers)
                if (entry.first != 1)
                    snapshot.push_back(entry.second);
        }
        for (auto &worker : snapshot)
            if (worker->execution.joinable())
                worker->execution.join();
        if (snapshot.size() == joined)
            return;
        joined = snapshot.size();
    }
}

void execute_code_listing(vector<Command> &code_listing, size_t start_pc)
{
    pc = start_pc;
    while (pc < code_listing.size())
    {
        Command &command = code_listing[pc];
        last_command = &command;
        try
        {
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
            else if (v1.get_type() == BYTES && v2.get_type() == BYTES)
            {
                result.set_number_value(*v1.get_bytes() == *v2.get_bytes() ? 1 : 0);
            }
            else if (v1.get_type() == PID && v2.get_type() == PID)
            {
                result.set_number_value(v1.get_pid() == v2.get_pid() ? 1 : 0);
            }
            else if (v1.get_type() == PID || v2.get_type() == PID)
            {
                result.set_number_value(0);
            }
            else if (v1.get_type() == BYTES || v2.get_type() == BYTES)
            {
                result.set_number_value(0);
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
            else if (v1.get_type() == BYTES && v2.get_type() == BYTES)
            {
                result.set_number_value(*v1.get_bytes() == *v2.get_bytes() ? 0 : 1);
            }
            else if (v1.get_type() == PID && v2.get_type() == PID)
            {
                result.set_number_value(v1.get_pid() == v2.get_pid() ? 0 : 1);
            }
            else if (v1.get_type() == PID || v2.get_type() == PID)
            {
                result.set_number_value(1);
            }
            else if (v1.get_type() == BYTES || v2.get_type() == BYTES)
            {
                result.set_number_value(1);
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
            if (v1.get_type() == BYTES || v2.get_type() == BYTES)
            {
                if (v1.get_type() != BYTES || v2.get_type() != BYTES)
                    raise_nvm_error("Cannot concatenate BYTES and non-BYTES values.");
                vector<unsigned char> joined = *v1.get_bytes();
                joined.insert(joined.end(), v2.get_bytes()->begin(), v2.get_bytes()->end());
                push(bytes_value(std::move(joined)));
                break;
            }
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
            Value value = pop(command);
            Value result;
            if (idx_from > 0)
            {
                idx_from -= 1;
            }
            result.set_string_value(substring(value, idx_from, idx_count));
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
        case Opcode::PTRY:
        {
            if (command.get_arguments().size() != 3)
                raise_nvm_error("Malformed PTRY instruction.");
            ErrorHandler handler;
            handler.start_pc = pc;
            handler.catch_pc = label_pc(command.get_arguments()[0]);
            handler.finally_pc = label_pc(command.get_arguments()[1]);
            handler.end_pc = label_pc(command.get_arguments()[2]);
            handler.stack_depth = execution_stack.size();
            handler.scope_depth = variable_tables.size();
            handler.return_depth = return_stack.size();
            runtime_state().error_handlers.push_back(std::move(handler));
            break;
        }
        case Opcode::ETRY:
        case Opcode::ECTH:
        {
            if (runtime_state().error_handlers.empty())
                raise_nvm_error("No active try block.");
            ErrorHandler &handler = runtime_state().error_handlers.back();
            handler.phase = HandlerPhase::Finally;
            handler.pending_error.reset();
            jump_to(handler.finally_pc);
            break;
        }
        case Opcode::CERR:
        {
            if (runtime_state().error_handlers.empty() ||
                !runtime_state().error_handlers.back().caught_error)
                raise_nvm_error("No caught error is available.");
            set_variable(command.get_arguments()[0].get_raw_string_value(),
                         *runtime_state().error_handlers.back().caught_error);
            runtime_state().error_handlers.back().caught_error.reset();
            break;
        }
        case Opcode::NCTH:
        {
            if (runtime_state().error_handlers.empty())
                raise_nvm_error("No active try block.");
            ErrorHandler &handler = runtime_state().error_handlers.back();
            handler.pending_error = std::move(handler.caught_error);
            handler.phase = HandlerPhase::Finally;
            jump_to(handler.finally_pc);
            break;
        }
        case Opcode::EFIN:
        {
            if (runtime_state().error_handlers.empty())
                raise_nvm_error("No active finally block.");
            optional<Value> error = std::move(runtime_state().error_handlers.back().pending_error);
            runtime_state().error_handlers.pop_back();
            if (error)
                throw VmException(std::move(*error));
            if (runtime_state().pending_control != ControlKind::None)
                continue_pending_control();
            break;
        }
        case Opcode::THRW:
            throw VmException(normalized_raised_value(pop(command)));
        case Opcode::FJMP:
            runtime_state().pending_control = ControlKind::Jump;
            runtime_state().pending_jump = command.get_branch_target() + 1;
            runtime_state().pending_return.reset();
            continue_pending_control();
            break;
        case Opcode::FRET:
        {
            runtime_state().pending_control = ControlKind::Return;
            if (command.get_arguments().empty())
                raise_nvm_error("Malformed FRET instruction.");
            if (execution_stack.empty())
                raise_nvm_error("Missing function return value.");
            Value return_kind = command.get_arguments()[0];
            runtime_state().pending_return =
                num_eq(return_kind.get_as_number(), 0)
                    ? execution_stack.top()
                    : pop(command);
            continue_pending_control();
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
            else if (value.get_type() == BYTES)
            {
                if (value.get_bytes()->empty()) pc = command.get_branch_target();
            }
            else if (value.get_type() == PID)
            {
                // PIDs are always truthy.
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
            if (table.get_type() != TABLE)
                raise_nvm_error("Only TABLE values can be assigned through an index.");
            (*table.get_table())[index.get_as_string()] = value;
            break;
        }
        case Opcode::PGET:
        {
            Value index = pop(command);
            Value table = pop(command);
            if (table.get_type() == TABLE)
            {
                string index_string = index.get_as_string();
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
            else if (table.get_type() == BYTES)
            {
                long long idx = require_integer(index, "Byte index");
                const long long length = static_cast<long long>(table.get_bytes()->size());
                if (idx > 0) --idx;
                if (idx < 0) idx += length;
                if (idx < 0 || idx >= length) push(get_nil_value());
                else push(number_value((*table.get_bytes())[static_cast<size_t>(idx)]));
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
                    long long idx = static_cast<long long>(floor(index.get_as_number()));
                    const string &text = table.get_as_string();
                    const vector<size_t> &offsets = table.get_utf8_offsets();
                    const long long length =
                        static_cast<long long>(table.get_codepoint_count());
                    if (idx > 0)
                    {
                        idx -= 1;
                    }
                    if (idx < 0)
                    {
                        idx = length + idx;
                    }
                    if (idx < 0 || idx >= length)
                    {
                        result.set_string_value("");
                    }
                    else
                    {
                        const size_t position = static_cast<size_t>(idx);
                        const size_t byte_from = offsets[position];
                        const size_t byte_to = offsets[position + 1];
                        result.set_string_value(text.substr(byte_from, byte_to - byte_from));
                    }
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
        case Opcode::ENVV:
        {
            Value fallback = pop(command);
            Value name = pop(command);
            if (name.get_type() == NIL)
            {
                push(environment_table());
                break;
            }
            string variable = name.get_as_string();
            const char *found = getenv(variable.c_str());
            if (found)
                push(text_value(found));
            else
                push(std::move(fallback));
            break;
        }
        case Opcode::RSTD:
        {
            double requested_number = pop(command).get_as_number();
            if (!isfinite(requested_number) || requested_number != floor(requested_number) ||
                requested_number < -1 ||
                requested_number > 9007199254740991.0)
                raise_nvm_error("read_stdin length must be -1 or a non-negative integer.");
            push(text_value(read_standard_input(static_cast<long long>(requested_number))));
            break;
        }
        case Opcode::URLD:
        {
            Value plus = pop(command);
            string encoded = pop(command).get_as_string();
            push(text_value(url_decode_text(encoded, is_true(plus))));
            break;
        }
        case Opcode::QPRS:
        {
            push(parse_query_text(pop(command).get_as_string()));
            break;
        }
        case Opcode::JDEC:
        {
            string json = pop(command).get_as_string();
            push(JsonParser(json).parse());
            break;
        }
        case Opcode::JENC:
        {
            Value value = pop(command);
            set<const map<string, Value> *> active;
            push(text_value(json_encode_value(value, active)));
            break;
        }
        case Opcode::CGIR:
        {
            double maximum_number = pop(command).get_as_number();
            if (!isfinite(maximum_number) || maximum_number != floor(maximum_number) ||
                maximum_number < 0 ||
                maximum_number > 9007199254740991.0)
                raise_nvm_error("cgi_request body limit must be a non-negative integer.");
            push(cgi_request_value(static_cast<long long>(maximum_number)));
            break;
        }
        case Opcode::CGIO:
        {
            Value headers = pop(command);
            Value body = pop(command);
            string content_type = pop(command).get_as_string();
            double status_number = pop(command).get_as_number();
            if (!isfinite(status_number) || status_number != floor(status_number) ||
                status_number < 100 || status_number > 999)
                raise_nvm_error("CGI response status must be an integer between 100 and 999.");
            write_cgi_response(static_cast<int>(status_number), content_type, body, headers);
            push(number_value(1));
            break;
        }
        case Opcode::BNEW:
        {
            vector<unsigned char> data;
            while (!execution_stack.empty() && execution_stack.top().get_type() != LISTLIMIT)
            {
                long long byte = require_integer(pop(command), "Byte value");
                if (byte < 0 || byte > 255)
                    raise_nvm_error("Byte value must be between 0 and 255.");
                data.push_back(static_cast<unsigned char>(byte));
            }
            if (execution_stack.empty())
                raise_nvm_error("Missing bytes argument-list marker.");
            pop(command);
            reverse(data.begin(), data.end());
            push(bytes_value(std::move(data)));
            break;
        }
        case Opcode::UENC:
        {
            Value source = pop(command);
            if (source.get_type() != TEXT)
                raise_nvm_error("utf8_encode expects TEXT.");
            string text = source.get_as_string();
            push(bytes_value(text));
            break;
        }
        case Opcode::UDEC:
        {
            Value data = pop(command);
            if (data.get_type() != BYTES)
                raise_nvm_error("utf8_decode expects BYTES.");
            string text = bytes_as_string(data);
            if (!is_valid_utf8(text))
                raise_nvm_error("Byte sequence is not valid UTF-8.");
            push(text_value(text));
            break;
        }
        case Opcode::BSLC:
        {
            long long count = require_integer(pop(command), "Byte slice length");
            long long start = require_integer(pop(command), "Byte slice start");
            Value source = pop(command);
            if (source.get_type() != BYTES)
                raise_nvm_error("bytes_slice expects BYTES.");
            const auto &data = *source.get_bytes();
            long long size = static_cast<long long>(data.size());
            long long offset = start > 0 ? start - 1 : (start < 0 ? size + start : 0);
            offset = max(0LL, min(offset, size));
            if (count < -1)
                raise_nvm_error("Byte slice length must be -1 or a non-negative integer.");
            if (count == -1) count = size - offset;
            long long end = min(size, offset + count);
            push(bytes_value(vector<unsigned char>(data.begin() + offset, data.begin() + end)));
            break;
        }
        case Opcode::RSTB:
        {
            long long length = require_integer(pop(command), "read_stdin_bytes length");
            if (length < -1)
                raise_nvm_error("read_stdin_bytes length must be -1 or a non-negative integer.");
            push(bytes_value(read_standard_input(length)));
            break;
        }
        case Opcode::RBIN:
        {
            filesystem::path path(pop(command).get_as_string());
            ifstream file(path, ios::binary);
            if (!file)
                raise_nvm_error("Could not open '" + path.string() + "' for binary reading.");
            vector<unsigned char> data((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
            push(bytes_value(std::move(data)));
            break;
        }
        case Opcode::WBIN:
        {
            filesystem::path path(pop(command).get_as_string());
            Value data = pop(command);
            if (data.get_type() != BYTES)
                raise_nvm_error("write_bytes expects a BYTES value.");
            ofstream file(path, ios::binary | ios::trunc);
            if (!file)
                raise_nvm_error("Could not open '" + path.string() + "' for binary writing.");
            file.write(reinterpret_cast<const char *>(data.get_bytes()->data()),
                       static_cast<streamsize>(data.get_bytes()->size()));
            if (!file)
                raise_nvm_error("Could not finish writing '" + path.string() + "'.");
            push(number_value(data.get_bytes()->size()));
            break;
        }
        case Opcode::HENC:
        case Opcode::B64E:
        {
            Value data = pop(command);
            if (data.get_type() != BYTES)
                raise_nvm_error(string(command.get_opcode() == Opcode::HENC ? "hex_encode" : "base64_encode") + " expects BYTES.");
            push(text_value(command.get_opcode() == Opcode::HENC
                                ? encode_hex(*data.get_bytes())
                                : encode_base64(*data.get_bytes())));
            break;
        }
        case Opcode::HDEC:
        case Opcode::B64D:
        {
            Value source = pop(command);
            if (source.get_type() != TEXT)
                raise_nvm_error(string(command.get_opcode() == Opcode::HDEC ? "hex_decode" : "base64_decode") + " expects TEXT.");
            string text = source.get_as_string();
            push(bytes_value(command.get_opcode() == Opcode::HDEC ? decode_hex(text) : decode_base64(text)));
            break;
        }
        case Opcode::PJON:
        {
            vector<string> parts;
            while (!execution_stack.empty() && execution_stack.top().get_type() != LISTLIMIT)
                parts.push_back(pop(command).get_as_string());
            if (execution_stack.empty())
                raise_nvm_error("Missing path_join argument-list marker.");
            pop(command);
            reverse(parts.begin(), parts.end());
            filesystem::path result;
            for (const auto &part : parts) result /= filesystem::path(part);
            push(text_value(result.lexically_normal().string()));
            break;
        }
        case Opcode::PPAR:
        case Opcode::PNAM:
        case Opcode::PEXT:
        case Opcode::PABS:
        case Opcode::PNOR:
        {
            filesystem::path path(pop(command).get_as_string());
            error_code error;
            filesystem::path result;
            if (command.get_opcode() == Opcode::PPAR) result = path.parent_path();
            else if (command.get_opcode() == Opcode::PNAM) result = path.filename();
            else if (command.get_opcode() == Opcode::PEXT) result = path.extension();
            else if (command.get_opcode() == Opcode::PNOR) result = path.lexically_normal();
            else result = filesystem::absolute(path, error).lexically_normal();
            if (error) raise_filesystem_error("Could not make path absolute", path, error);
            push(text_value(result.string()));
            break;
        }
        case Opcode::PEXS:
        case Opcode::PFIL:
        case Opcode::PDIR:
        {
            filesystem::path path(pop(command).get_as_string());
            error_code error;
            bool result = command.get_opcode() == Opcode::PEXS ? filesystem::exists(path, error)
                        : command.get_opcode() == Opcode::PFIL ? filesystem::is_regular_file(path, error)
                                                              : filesystem::is_directory(path, error);
            if (error) raise_filesystem_error("Could not inspect path", path, error);
            push(number_value(result ? 1 : 0));
            break;
        }
        case Opcode::FSIZ:
        {
            filesystem::path path(pop(command).get_as_string());
            error_code error;
            auto size = filesystem::file_size(path, error);
            if (error) raise_filesystem_error("Could not read file size", path, error);
            push(number_value(static_cast<double>(size)));
            break;
        }
        case Opcode::LDIR:
        {
            filesystem::path path(pop(command).get_as_string());
            error_code error;
            vector<string> names;
            filesystem::directory_iterator iterator(path, error), end;
            if (error) raise_filesystem_error("Could not list directory", path, error);
            while (iterator != end)
            {
                names.push_back(iterator->path().filename().string());
                iterator.increment(error);
                if (error) raise_filesystem_error("Could not list directory", path, error);
            }
            sort(names.begin(), names.end());
            Value result = table_value();
            for (size_t i = 0; i < names.size(); ++i)
                (*result.get_table())[double_to_string(i + 1)] = text_value(names[i]);
            push(std::move(result));
            break;
        }
        case Opcode::MDIR:
        {
            bool parents = is_true(execution_stack.top());
            pop(command);
            filesystem::path path(pop(command).get_as_string());
            error_code error;
            bool created = parents ? filesystem::create_directories(path, error)
                                   : filesystem::create_directory(path, error);
            if (error) raise_filesystem_error("Could not create directory", path, error);
            push(number_value(created ? 1 : 0));
            break;
        }
        case Opcode::CPFL:
        {
            bool overwrite = is_true(execution_stack.top());
            pop(command);
            filesystem::path destination(pop(command).get_as_string());
            filesystem::path source(pop(command).get_as_string());
            error_code error;
            bool copied = filesystem::copy_file(source, destination,
                overwrite ? filesystem::copy_options::overwrite_existing : filesystem::copy_options::none, error);
            if (error) raise_filesystem_error("Could not copy file", source, error);
            push(number_value(copied ? 1 : 0));
            break;
        }
        case Opcode::MVFL:
        {
            filesystem::path destination(pop(command).get_as_string());
            filesystem::path source(pop(command).get_as_string());
            error_code error;
            filesystem::rename(source, destination, error);
            if (error) raise_filesystem_error("Could not move path", source, error);
            push(number_value(1));
            break;
        }
        case Opcode::RMFL:
        case Opcode::RMDR:
        {
            filesystem::path path(pop(command).get_as_string());
            error_code error;
            if (command.get_opcode() == Opcode::RMFL && filesystem::is_directory(path, error))
                raise_nvm_error("remove_file refuses to remove a directory.");
            if (command.get_opcode() == Opcode::RMDR && !filesystem::is_directory(path, error))
                raise_nvm_error("remove_directory expects a directory.");
            if (error) raise_filesystem_error("Could not inspect path", path, error);
            bool removed = filesystem::remove(path, error);
            if (error) raise_filesystem_error("Could not remove path", path, error);
            push(number_value(removed ? 1 : 0));
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
            if (value.get_type() == NIL)
            {
                result.set_number_value(1);
            }
            else if (value.get_type() == TABLE)
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
            else if (value.get_type() == BYTES)
            {
                result.set_number_value(value.get_bytes()->empty() ? 1 : 0);
            }
            else if (value.get_type() == PID)
            {
                result.set_number_value(0);
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
            else if (value.get_type() == BYTES)
            {
                result.set_number_value(value.get_bytes()->size());
            }
            else if (value.get_type() == TEXT || value.get_type() == NUMB)
            {
                result.set_number_value(value.get_codepoint_count());
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
        case Opcode::DTIM:
        {
            time_t current = time(nullptr);
            if (current == static_cast<time_t>(-1))
                raise_nvm_error("Could not read the system clock.");

            tm local{};
#if defined(_WIN32)
            if (localtime_s(&local, &current) != 0)
                raise_nvm_error("Could not convert the system clock to local time.");
#else
            if (localtime_r(&current, &local) == nullptr)
                raise_nvm_error("Could not convert the system clock to local time.");
#endif

            auto padded = [](int value, int width) {
                ostringstream output;
                output << setw(width) << setfill('0') << value;
                return text_value(output.str());
            };

            Value result = table_value();
            auto &fields = *result.get_table();
            fields["dow"] = padded(local.tm_wday == 0 ? 7 : local.tm_wday, 1);
            fields["date"] = padded(local.tm_mday, 2);
            fields["day"] = fields["date"];
            fields["month"] = padded(local.tm_mon + 1, 2);
            fields["year"] = padded(local.tm_year + 1900, 4);
            fields["hour"] = padded(local.tm_hour, 2);
            fields["h12"] = padded(local.tm_hour % 12 == 0 ? 12 : local.tm_hour % 12, 2);
            fields["ampm"] = text_value(local.tm_hour < 12 ? "AM" : "PM");
            fields["min"] = padded(local.tm_min, 2);
            fields["sec"] = padded(local.tm_sec, 2);
            push(std::move(result));
            break;
        }
        case Opcode::SPWN:
        {
            if (!runtime_state().actor_system || runtime_state().worker_id == 0)
                raise_nvm_error("Worker runtime is unavailable.");
            vector<Value> arguments;
            while (!execution_stack.empty() && execution_stack.top().get_type() != LISTLIMIT)
                arguments.push_back(pop(command));
            if (execution_stack.empty())
                raise_nvm_error("Missing spawn argument-list marker.");
            pop(command);
            reverse(arguments.begin(), arguments.end());
            if (variable_tables.empty())
                raise_nvm_error("No global scope is available for spawning.");
            string caller = get_variable("_context").get_type() == NIL
                                ? string()
                                : Value(get_variable("_context")).get_as_string();
            uint64_t child = runtime_state().actor_system->spawn(
                label_pc(command.get_arguments()[0]), std::move(arguments),
                variable_tables.front(), std::move(caller));
            push(pid_value(child));
            break;
        }
        case Opcode::SELF:
            if (!runtime_state().actor_system || runtime_state().worker_id == 0)
                raise_nvm_error("Worker runtime is unavailable.");
            push(pid_value(runtime_state().worker_id));
            break;
        case Opcode::SEND:
        {
            Value message = pop(command);
            Value destination = pop(command);
            if (destination.get_type() != PID)
                raise_nvm_error("send expects a PID as its first argument.");
            runtime_state().actor_system->send(runtime_state().worker_id,
                                               destination.get_pid(), std::move(message));
            push(number_value(1));
            break;
        }
        case Opcode::RECV:
        case Opcode::RNOW:
        {
            optional<double> timeout;
            if (command.get_opcode() == Opcode::RNOW)
                timeout = 0.0;
            else
            {
                double seconds = pop(command).get_as_number();
                if (!isfinite(seconds) || seconds < -1)
                    raise_nvm_error("receive timeout must be -1 or a non-negative number.");
                if (seconds >= 0) timeout = seconds;
            }
            auto envelope = runtime_state().actor_system->receive(runtime_state().worker_id, timeout);
            if (!envelope)
                push(get_nil_value());
            else
            {
                Value result = table_value();
                (*result.get_table())["from"] = pid_value(envelope->from);
                (*result.get_table())["message"] = std::move(envelope->message);
                push(std::move(result));
            }
            break;
        }
        case Opcode::WALV:
        {
            Value worker = pop(command);
            if (worker.get_type() != PID)
                raise_nvm_error("worker_alive expects a PID.");
            push(number_value(runtime_state().actor_system->alive(worker.get_pid()) ? 1 : 0));
            break;
        }
        case Opcode::WWAIT:
        {
            double seconds = pop(command).get_as_number();
            Value worker = pop(command);
            if (worker.get_type() != PID)
                raise_nvm_error("wait expects a PID as its first argument.");
            if (!isfinite(seconds) || seconds < -1)
                raise_nvm_error("wait timeout must be -1 or a non-negative number.");
            optional<double> timeout;
            if (seconds >= 0) timeout = seconds;
            push(runtime_state().actor_system->wait(runtime_state().worker_id,
                                                    worker.get_pid(), timeout));
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
                    Value key;
                    key.set_string_value(it->first);
                    (*result.get_table())[double_to_string(index)] = key;
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
                for (size_t i = 0; i < container.get_codepoint_count(); ++i)
                {
                    string character = double_to_string(i + 1);
                    result.get_iterator_queue()->push(character);
                }
            }
            else if (container.get_type() == BYTES)
            {
                for (size_t i = 0; i < container.get_bytes()->size(); ++i)
                    result.get_iterator_queue()->push(double_to_string(i + 1));
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
        case Opcode::DEBUG:
        {
            cout << "NariVM Debug Output:" << endl;
            cout << "--- Variables ---" << endl;
            for (size_t scope = 0; scope < variable_tables.size(); ++scope)
            {
                cout << "Variable scope #" << scope << endl;
                for (auto it = variable_tables[scope].begin(); it != variable_tables[scope].end(); ++it)
                {
                    cout << "[" << it->first << "] =>" << it->second.get_as_string();
                }
            }
            cout << "--- Return Stack ---" << endl;
            cout << "Size: " << return_stack.size() << endl;
            cout << "--- Execution Stack ---" << endl;
            cout << "Size: " << execution_stack.size() << endl;
            break;
        }
        default:
            raise_nvm_error("Unknown Nambly command: " + command.get_debug_string());
        }
        }
        catch (const VmException &error)
        {
            if (!dispatch_error(error.error()))
                throw;
        }
        ++pc;
    }
}

int execute_nambly(const string &code)
{
    RuntimeState state;
    RuntimeActivation activation(state);
    shared_ptr<ActorSystem> actors;
    try
    {
        auto code_listing = make_shared<vector<Command>>(generate_label_map_and_code_listing(code));
        actors = make_shared<ActorSystem>(code_listing, state.labels(), state.reverse_labels());
        state.actor_system = actors;
        state.worker_id = 1;
        execute_code_listing(*code_listing);
        actors->wait_for_all();
        actors->finish(1, get_nil_value(), nullopt);
        return 0;
    }
    catch (const VmException &error)
    {
        if (actors)
            actors->finish(1, nullopt, error.error());
        Value detail = error.error();
        auto &fields = *detail.get_table();
        cerr << endl << "====== Oh no! Runtime Error! ======" << endl;
        cerr << wrap_text(fields["message"].get_as_string(), 70) << endl;
        if (fields.count("file"))
        {
            cerr << endl << "--- Source File Information --- " << endl;
            cerr << "- Source File: " << fields["file"].get_as_string() << endl;
            cerr << "- Source Line: " << fields["line"].get_as_number() << endl;
        }
        cerr << endl << "--- NariVM State Information --- " << endl;
        cerr << "- PC: " << fields["pc"].get_as_number() << endl;
        return 1;
    }
}
