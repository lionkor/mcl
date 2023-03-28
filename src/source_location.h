#pragma once

#include "fmt/core.h"
#include <cstddef>
#include <string>

struct SourceLocation final {
    std::string file;
    size_t line;
    size_t col_start;
    size_t col_end;
};

inline std::string to_string(const SourceLocation& loc) { return fmt::format("{}:{}:{}-{}", loc.file, loc.line, loc.col_start, loc.col_end); }
