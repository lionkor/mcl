#include "instruction.h"

#include <unordered_map>

std::string_view to_string(Op op) {
    switch (op) {
    case NOT_AN_INSTRUCTION:
        return "not_an_instruction";
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

Op op_from_string(const std::string& str) {
    if (str == "push") {
        return PUSH;
    } else if (str == "pop") {
        return POP;
    } else if (str == "add") {
        return ADD;
    } else if (str == "sub") {
        return SUB;
    } else if (str == "mul") {
        return MUL;
    } else if (str == "div") {
        return DIV;
    } else if (str == "mod") {
        return MOD;
    } else if (str == "print") {
        return PRINT;
    } else if (str == "halt") {
        return HALT;
    } else if (str == "dup") {
        return DUP;
    } else if (str == "swap") {
        return SWAP;
    } else if (str == "over") {
        return OVER;
    } else if (str == "je") {
        return JE;
    } else if (str == "jn") {
        return JN;
    } else if (str == "jg") {
        return JG;
    } else if (str == "jl") {
        return JL;
    } else if (str == "jge") {
        return JGE;
    } else if (str == "jle") {
        return JLE;
    } else if (str == "jmp") {
        return JMP;
    } else {
        return NOT_AN_INSTRUCTION;
    }
}

bool op_requires_i64_argument(Op op) {
    if (op < PUSH) {
        return false;
    } else {
        return true;
    }
}

bool op_requires_str_argument(Op op) {
    if (op < PUSH && op > OVER) {
        return true;
    } else {
        return false;
    }
}

bool op_accepts_label_argument(Op op) {
    if (op >= JE) {
        return true;
    } else {
        return false;
    }
}
