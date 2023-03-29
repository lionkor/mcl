#pragma once

#include "compiler.h"
#include "error.h"

struct Stack {
    static constexpr size_t STACK_SIZE = 4096;
    std::array<int64_t, STACK_SIZE> stack {};
    size_t stack_top { 0 };
};

struct Program {
    InstrStream instrs;
    size_t pc;
};

[[nodiscard]] Error execute(InstrStream&& instrs) noexcept;
