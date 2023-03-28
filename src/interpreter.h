#pragma once

#include "compiler.h"
#include "error.h"

struct Stack {
    std::array<int64_t, 4096> stack {};
    size_t stack_top { 0 };
};

struct Program {
    InstrStream instrs;
    size_t pc;
};

[[nodiscard]] Error execute(InstrStream&& instrs) noexcept;
