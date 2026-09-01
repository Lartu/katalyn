#include "compiler.hpp"
#include "narivm.hpp"

#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
void help() {
    std::cout
        << "\nUsage: lspl [switches] <source file> [arguments...]\n"
        << "  -a <source>             read source from argument\n"
        << "  -h, --help              print this information\n"
        << "  -i                      print internal representation instead of executing\n"
        << "  -n                      do not include standard library\n"
        << "  -s                      read source from standard input\n"
        << "  -v, --version           print version and build information\n\n";
}

void version() {
    std::string build_date = __DATE__;
    if (build_date.size() > 4 && build_date[4] == ' ')
        build_date.erase(4, 1);

    std::cout << R"(
  _      ____  ____  _
 | |    / ___||  _ \| |
 | |    \___ \| |_) | |
 | |___  ___) |  __/| |___
 |_____||____/|_|   |_____|
             Programming Language

)"
              << "This is LSPL version " << lspl::version()
              << ", running on the NariVM.\n"
              << "Built on " << build_date << " at " << __TIME__ << ".\n"
              << "Copyright 2024, Lartu (www.lartu.net).\n\n";
}
}

int main(int argc, char** argv) {
    try {
        bool stdin_source = false;
        bool print_ir = false;
        bool no_stdlib = false;
        bool argument_source = false;
        bool source_selected = false;
        std::string source;
        std::string filename;
        std::vector<std::string> script_arguments;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (!filename.empty() || argument_source || source_selected) {
                if (argument_source && source.empty()) {
                    source = arg;
                    argument_source = false;
                    source_selected = true;
                } else {
                    script_arguments.push_back(arg);
                }
            } else if (arg == "-a") {
                argument_source = true;
            } else if (arg == "-s") {
                stdin_source = true;
                source_selected = true;
            } else if (arg == "-i") {
                print_ir = true;
            } else if (arg == "-n") {
                no_stdlib = true;
            } else if (arg == "-h" || arg == "--help") {
                help();
                return 0;
            } else if (arg == "-v" || arg == "--version") {
                version();
                return 0;
            } else {
                filename = arg;
            }
        }

        if (argument_source) {
            std::cerr << "Missing source after -a.\n";
            return 1;
        }
        if (stdin_source) {
            std::ostringstream input;
            input << std::cin.rdbuf();
            source = input.str();
        }
        if (filename.empty() && source.empty() && !stdin_source) {
            help();
            return 1;
        }

        lspl::Compiler compiler;
        lspl::CompileOptions options;
        options.include_standard_library = !no_stdlib;
        options.arguments = std::move(script_arguments);
        std::string nambly;
        if (!filename.empty()) {
            nambly = compiler.compile_file(filename, options);
        } else {
            nambly = compiler.compile_source(source, stdin_source ? "stdin" : "argument_code", options);
        }
        if (print_ir) {
            std::cout << nambly;
            return 0;
        }
        return execute_nambly(nambly);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
