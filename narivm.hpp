#pragma once

#include <string>

// Execute one Nambly program in a fresh, isolated runtime. All VM stacks,
// scopes, labels, handlers, and file handles belong to this invocation.
int execute_nambly(const std::string &code);
