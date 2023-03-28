#pragma once

#include <cstdint>
#include <string>
#include <string_view>

enum Op : uint8_t {
    NOT_AN_INSTRUCTION = 0x00, // is label or invalid instruction

    // without argument
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

    // with string argument
    // NONE

    // with argument
    PUSH,

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
Op op_from_string(const std::string& str);
bool op_requires_i64_argument(Op op);
bool op_requires_str_argument(Op op);
bool op_accepts_label_argument(Op op);

union Instr {
    struct {
        Op op : 8;
        int64_t val : 56;
    } s;
    uint64_t v;
};
static_assert(sizeof(Instr) == sizeof(uint64_t));
