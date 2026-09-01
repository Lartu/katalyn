#pragma once

#include <string>
#include <memory>
#include <vector>

namespace lspl {

struct CompileOptions {
    bool include_standard_library = true;
    std::vector<std::string> arguments;
    // Empty for source supplied through -a/-s or another non-file source.
    std::string script_path;
};

class Compiler {
public:
    Compiler();
    ~Compiler();
    Compiler(Compiler&&) noexcept;
    Compiler& operator=(Compiler&&) noexcept;
    Compiler(const Compiler&) = delete;
    Compiler& operator=(const Compiler&) = delete;
    std::string compile_file(const std::string& filename, const CompileOptions& options = {});
    std::string compile_source(const std::string& source, const std::string& filename,
                               const CompileOptions& options = {});

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

const char* version();

} // namespace lspl
