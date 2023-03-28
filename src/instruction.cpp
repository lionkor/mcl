#include "instruction.h"

std::string_view to_string(Op op) {
    switch (op) {
    case INVALID:
        return "invalid";
    case PUSH:
        return "push";
    case POP:
        return "pop";
    case ADD:
        return "add";
    case SUB:
        return "sub";
    case MUL:
        return "mul";
    case DIV:
        return "div";
    case MOD:
        return "mod";
    case PRINT:
        return "print";
    case HALT:
        return "halt";
    case DUP:
        return "dup";
    case SWAP:
        return "swap";
    case OVER:
        return "over";
    case JE:
        return "je";
    case JN:
        return "jn";
    case JG:
        return "jg";
    case JL:
        return "jl";
    case JGE:
        return "jge";
    case JLE:
        return "jle";
    case JMP:
        return "jmp";
    }
}
