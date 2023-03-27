#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <vector>

enum Op : uint8_t {
    INVALID = 0,
    PUSH = 1,
    POP,
    ADD,
    SUB,
    MULT,
    DIV,
    MOD,
    PRINT,
    HALT,
    DUP,
};

union Instr {
    struct {
        Op op : 8;
        uint64_t val : 56;
    } s;
    uint64_t v;
};
static_assert(sizeof(Instr) == sizeof(uint64_t));

static bool run_bytecode(const std::vector<Instr> bytecode) {
    size_t pc = 0;
    size_t st = 0;
    std::vector<uint64_t> stack;
    stack.resize(4096, 0);

    bool running = true;
    while (running) {
        if (pc >= bytecode.size()) [[unlikely]] {
            fmt::print("program counter ran off the end of the program, forgot to HALT?\n");
            return false;
        }
        switch (bytecode[pc].s.op) {
        case INVALID:
            fmt::print("INVALID encountered, exiting\n");
            return false;
        case PUSH:
            stack[st++] = bytecode[pc].s.val;
            break;
        case POP:
            --st;
            break;
        case ADD: {
            uint64_t a = stack[st - 2];
            uint64_t b = stack[st - 1];
            st -= 2;
            stack[st++] = a + b;
            break;
        }
        case SUB: {
            uint64_t a = stack[st - 2];
            uint64_t b = stack[st - 1];
            st -= 2;
            stack[st++] = a - b;
            break;
        }
        case MULT: {
            uint64_t a = stack[st - 2];
            uint64_t b = stack[st - 1];
            st -= 2;
            stack[st++] = a * b;
            break;
        }
        case DIV: {
            uint64_t a = stack[st - 2];
            uint64_t b = stack[st - 1];
            if (b == 0) [[unlikely]] {
                fmt::print("exception: DIV by 0 at pc={}\n", pc);
                return false;
            }
            st -= 2;
            stack[st++] = a / b;
            break;
        }
        case MOD: {
            uint64_t a = stack[st - 2];
            uint64_t b = stack[st - 1];
            if (b == 0) [[unlikely]] {
                fmt::print("exception: MOD by 0 at pc={}\n", pc);
                return false;
            }
            st -= 2;
            stack[st++] = a % b;
            break;
        }
        case PRINT:
            fmt::print("{}\n", stack[st - 1]);
            break;
        case HALT:
            running = false;
            break;
        case DUP:
            stack[st] = stack[st - 1];
            ++st;
            break;
        }
        ++pc;
    }
    return true;
}

int main(int argc, char** argv) {
    std::vector<std::string_view> files;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);
        if (arg.starts_with("--")) {
            if (arg == "--help") {
                fmt::print("Usage:\n\t{} [OPTION...] <FILE...>\n\nOptions:\n\t--help\t\t Displays help\n\t--version\t Displays version\n\nFiles: Files are executed in the order they are specified.\n", argv[0]);
                return 0;
            } else if (arg == "--version") {
                fmt::print("v{}.{}.{}-{}\n", PRJ_VERSION_MAJOR, PRJ_VERSION_MINOR, PRJ_VERSION_PATCH, PRJ_GIT_HASH);
                return 0;
            } else {
                fmt::print("Unknown argument '{}', run '{} --help' for help.\n", arg, argv[0]);
                return 1;
            }
        } else {
            files.push_back(arg);
        }
    }
    if (files.empty()) {
        fmt::print("No file(s) specified. See '{} --help' for help.\n", argv[0]);
        return 1;
    }

    std::vector<Instr> bytecode;

    for (const auto& filename : files) {
        std::ifstream file((std::string(filename)));
        std::string line;
        while (std::getline(file, line)) {
            Instr instr {
                .s = {
                    .op = INVALID,
                    .val = 0,
                }
            };
            auto space = line.find_first_of(' ');
            auto left = line.substr(0, space);
            if (space != std::string::npos) {
                auto right = line.substr(space, std::string::npos);
                // parse right as integer
                auto val = std::stoull(right, nullptr);
                instr.s.val = val;
            }
            if (left == "push") {
                instr.s.op = PUSH;
            } else if (left == "pop") {
                instr.s.op = POP;
            } else if (left == "add") {
                instr.s.op = ADD;
            } else if (left == "print") {
                instr.s.op = PRINT;
            } else if (left == "halt") {
                instr.s.op = HALT;
            } else if (left == "sub") {
                instr.s.op = SUB;
            } else if (left == "div") {
                instr.s.op = DIV;
            } else if (left == "mul") {
                instr.s.op = MULT;
            } else if (left == "mod") {
                instr.s.op = MOD;
            } else if (left == "dup") {
                instr.s.op = DUP;
            }

            if (instr.s.op == INVALID) {
                fmt::print("invalid operation: '{}'\n", left);
                return 1;
            }
            bytecode.push_back(instr);
        }
    }

    if (!run_bytecode(bytecode)) {
        fmt::print("error while executing bytecode, execution stopped\n");
        return 1;
    }
    return 0;
}
