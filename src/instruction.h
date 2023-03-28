#pragma once

#include <cstdint>
#include <string_view>

enum Op : uint8_t {
    INVALID = 0x00,
    PUSH,
    POP,
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    PRINT,
    HALT,
    DUP,
    SWAP,
    OVER,

    // keep these at the end
    JE,
    JN,
    JG,
    JL,
    JGE,
    JLE,
    JMP,
};

std::string_view to_string(Op op);
Op from_string(const std::string& str);

union Instr {
    struct {
        Op op : 8;
        uint64_t val : 56;
    } s;
    uint64_t v;
};
static_assert(sizeof(Instr) == sizeof(uint64_t));
