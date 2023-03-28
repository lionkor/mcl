#include <algorithm>
#include <cerrno>
#include <compare>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unordered_map>
#include <vector>

#ifdef _DEBUG
#define DEBUG_PRINT(pc, stack, st, op, aval)                                                                 \
    do {                                                                                                     \
        fmt::print("STACK: {:4} {} {} | ", pc, to_string(op), aval);                                         \
        for (const auto& val : std::vector<int64_t>(stack.begin(), stack.begin() + static_cast<long>(st))) { \
            fmt::print("{} ", val);                                                                          \
        }                                                                                                    \
        fmt::print("\n");                                                                                    \
    } while (false)
#else
#define DEBUG_PRINT(pc, stack, st, op, aval)
#endif

static inline int64_t get(const std::vector<int64_t>& stack, size_t st, int64_t offset) {
#ifdef _DEBUG
    if ((offset < 0 && size_t(std::abs(offset)) > st) || size_t(int64_t(st) + offset) >= stack.size()) {
        fmt::print("STACK CHECK FAILED in get: {}+({}) is not a valid stack position. Aborting.\n", st, offset);
        std::abort();
    }
#endif
    st = size_t(int64_t(st) + offset);
    return stack[st];
}

static inline void set(std::vector<int64_t>& stack, size_t st, int64_t offset, int64_t value) {
#ifdef _DEBUG
    if ((offset < 0 && size_t(std::abs(offset)) > st) || size_t(int64_t(st) + offset) >= stack.size()) {
        fmt::print("STACK CHECK FAILED in set: {}+({}) is not a valid stack position. Aborting.\n", st, offset);
        std::abort();
    }
#endif
    st = size_t(int64_t(st) + offset);
    stack[st] = value;
}

static inline void move_stack_top(size_t& st, int64_t by) {
#ifdef _DEBUG
    if (by < 0 && size_t(std::abs(by)) > st) {
        fmt::print("STACK CHECK FAILED in move_stack_top: {}+({}) is an invalid stack position. Aborting.\n", st, by);
        std::abort();
    }
#endif
    st = size_t(int64_t(st) + by);
}

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

static std::string_view to_string(Op op) {
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

union Instr {
    struct {
        Op op : 8;
        uint64_t val : 56;
    } s;
    uint64_t v;
};
static_assert(sizeof(Instr) == sizeof(uint64_t));

static bool run_bytecode(std::vector<Instr>&& bytecode) {
    size_t pc = 0;
    size_t st = 0;
    std::vector<int64_t> stack;
    stack.resize(4096);
    // terminate the end to avoid out-of-range pc
    bytecode.push_back(Instr{.s={.op=HALT, .val =0}});

    bool running = true;
    while (running) {
        const auto& op = bytecode[pc].s.op;
#ifdef _DEBUG
        const auto& arg_val = bytecode[pc].s.val;
        fmt::print("---\nPC: {:4}, OP: {:5}, VAL: {:4}\n", pc, to_string(op), arg_val);
        auto old_pc = pc;
#endif
        switch (op) {
        case INVALID:
            fmt::print("INVALID encountered, exiting\n");
            return false;
        case PUSH:
            set(stack, st, 0, bytecode[pc].s.val);
            move_stack_top(st, 1);
            break;
        case POP:
            move_stack_top(st, -1);
            break;
        case ADD: {
            int64_t a = get(stack, st, -2);
            int64_t b = get(stack, st, -1);
            move_stack_top(st, -1);
            set(stack, st, -1, a + b);
            break;
        }
        case SUB: {
            int64_t a = get(stack, st, -2);
            int64_t b = get(stack, st, -1);
            move_stack_top(st, -1);
            set(stack, st, -1, a - b);
            break;
        }
        case MUL: {
            int64_t a = get(stack, st, -2);
            int64_t b = get(stack, st, -1);
            move_stack_top(st, -1);
            set(stack, st, -1, a * b);
            break;
        }
        case DIV: {
            int64_t a = get(stack, st, -2);
            int64_t b = get(stack, st, -1);
            if (b == 0) [[unlikely]] {
                fmt::print("exception: DIV by 0 at pc={}\n", pc);
                return false;
            }
            move_stack_top(st, -1);
            set(stack, st, -1, a / b);
            break;
        }
        case MOD: {
            int64_t a = get(stack, st, -2);
            int64_t b = get(stack, st, -1);
            if (b == 0) [[unlikely]] {
                fmt::print("exception: MOD by 0 at pc={}\n", pc);
                return false;
            }
            move_stack_top(st, -1);
            set(stack, st, -1, a % b);
            break;
        }
        case PRINT:
            fmt::print("{}\n", get(stack, st, -1));
            break;
        case HALT:
            running = false;
            break;
        case DUP: {
            auto value = get(stack, st, -1);
            set(stack, st, 0, value);
            move_stack_top(st, 1);
            break;
        }
        case SWAP:
#ifdef _DEBUG
            if (2 > st) {
                fmt::print("STACK CHECK FAILED: swap called with <2 elements on the stack.\n");
                std::abort();
            }
#endif
            std::swap(stack[st - 2], stack[st - 1]);
            break;
        case OVER: {
            auto value = get(stack, st, -2);
            move_stack_top(st, +1);
            set(stack, st, -1, value);
            break;
        }
        case JE: {
            int64_t a = stack[st - 2];
            int64_t b = stack[st - 1];
            st -= 2;
            if (a == b) {
                pc = bytecode[pc].s.val;
            } else {
                ++pc;
            }
            break;
        }
        case JN: {
            int64_t a = stack[st - 2];
            int64_t b = stack[st - 1];
            st -= 2;
            if (a != b) {
                pc = bytecode[pc].s.val;
            } else {
                ++pc;
            }
            break;
        }
        case JG: {
            int64_t a = stack[st - 2];
            int64_t b = stack[st - 1];
            st -= 2;
            if (a > b) {
                pc = bytecode[pc].s.val;
            } else {
                ++pc;
            }
            break;
        }
        case JL: {
            int64_t a = stack[st - 2];
            int64_t b = stack[st - 1];
            st -= 2;
            if (a < b) {
                pc = bytecode[pc].s.val;
            } else {
                ++pc;
            }
            break;
        }
        case JGE: {
            int64_t a = stack[st - 2];
            int64_t b = stack[st - 1];
            st -= 2;
            if (a >= b) {
                pc = bytecode[pc].s.val;
            } else {
                ++pc;
            }
            break;
        }
        case JLE: {
            int64_t a = stack[st - 2];
            int64_t b = stack[st - 1];
            st -= 2;
            if (a <= b) {
                pc = bytecode[pc].s.val;
            } else {
                ++pc;
            }
            break;
        }
        case JMP: {
            pc = bytecode[pc].s.val;
            break;
        }
        }
        DEBUG_PRINT(old_pc, stack, st, op, arg_val);
        // increment pc if not a JUMP instr
        // unlikely since most instrs are not jumps
        if (op < JE) [[unlikely]] {
            ++pc;
        }
    }
    return true;
}

int main(int argc, char** argv) {
    std::vector<std::string_view> files;
    bool compile_only = false;
    bool exec_only = false;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);
        if (arg.starts_with("--")) {
            if (arg == "--help") {
                fmt::print("Usage:\n\t{} [OPTION...] <FILE...>\n\nOptions:\n"
                           "\t--help\t\t Displays help\n"
                           "\t--version\t Displays version\n"
                           "\t--compile\t Enables compiling bytecode and not running the code. First specified file becomes output file ending in .mclb\n"
                           "\t--exec\t\t Expects files to be bytecode executables, and runs them\n"
                           "\nFiles: Files are executed in the order they are specified.\n",
                    argv[0]);
                return 0;
            } else if (arg == "--version") {
                fmt::print("v{}.{}.{}-{}\n", PRJ_VERSION_MAJOR, PRJ_VERSION_MINOR, PRJ_VERSION_PATCH, PRJ_GIT_HASH);
                return 0;
            } else if (arg == "--compile") {
                compile_only = true;
            } else if (arg == "--exec") {
                exec_only = true;
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

    if (exec_only && compile_only) {
        fmt::print("Error: `exec` and `compile` not allowed at the same time. Run without arguments either of these arguments to compile and interpret source code in one go.\n");
        return 1;
    }

    std::vector<Instr> bytecode;

    if (exec_only) {
        if (files.size() > 1) {
            fmt::print("Error: More than one file specified for `--exec`, this is currently not supported.\n");
            return 1;
        }
        auto filename = files.front();
        if (filename.ends_with("mcl")) {
            fmt::print("Error: Looks like you're passing a source code file (.mcl) to mcl invoked with `--exec`, not allowing this. Compiled mcl files end in `.mclb`, use (for example) `{} --compile {}` to compile the source code first.\n", argv[0], filename);
            return 1;
        }
        auto* file = std::fopen(filename.data(), "rb");
        if (!file) {
            fmt::print("Error: Failed to read bytecode from file '{}': {}\n", filename, std::strerror(errno));
            return 1;
        }
        auto size = std::filesystem::file_size(filename);
        bytecode.resize(size / sizeof(Instr));
        std::fread(bytecode.data(), sizeof(Instr), bytecode.size(), file);
        std::fclose(file);
    } else {
        std::unordered_map<std::string, size_t> labels;
        std::unordered_map<size_t, std::string> unresolved;

        for (const auto& filename : files) {
            if (filename.ends_with("mclb")) {
                fmt::print("Error: Passed `.mclb` file '{}' to the compiler, but `.mclb` is the extension of files which have already been compiled. Not allowing this.\n", filename);
                return 1;
            }
            std::ifstream file((std::string(filename)));
            std::string line;
            while (std::getline(file, line)) {
                if (line.starts_with(":")) {
                    // is jump label
                    auto id = line.substr(1);
                    labels[id] = bytecode.size(); // set jump label to next bytecode
                    continue;
                } else if (line.empty()) {
                    continue;
                }
                Instr instr {
                    .s = {
                        .op = INVALID,
                        .val = 0,
                    }
                };
                auto space = line.find_first_of(' ');
                auto left = line.substr(0, space);
                if (space != std::string::npos) {
                    auto right = line.substr(space + 1, std::string::npos);
                    if (right.starts_with(":")) {
                        // label
                        unresolved[bytecode.size()] = right.substr(1);
                    } else {
                        // parse right as integer
                        auto val = std::stoull(right, nullptr);
                        instr.s.val = val;
                    }
                    if (left == "je") {
                        instr.s.op = JE;
                    } else if (left == "jn") {
                        instr.s.op = JN;
                    } else if (left == "jl") {
                        instr.s.op = JL;
                    } else if (left == "jg") {
                        instr.s.op = JG;
                    } else if (left == "jge") {
                        instr.s.op = JGE;
                    } else if (left == "jle") {
                        instr.s.op = JLE;
                    } else if (left == "jmp") {
                        instr.s.op = JMP;
                    } else if (left == "push") {
                        instr.s.op = PUSH;
                    }
                }
                if (left == "pop") {
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
                    instr.s.op = MUL;
                } else if (left == "mod") {
                    instr.s.op = MOD;
                } else if (left == "dup") {
                    instr.s.op = DUP;
                } else if (left == "over") {
                    instr.s.op = OVER;
                } else if (left == "swap") {
                    instr.s.op = SWAP;
                }

                if (instr.s.op == INVALID) {
                    fmt::print("Error: invalid operation: '{}', is it missing a value/argument?\n", left);
                    return 1;
                }
                bytecode.push_back(instr);
            }
        }
        for (const auto& [pc, id] : unresolved) {
            if (!labels.contains(id)) {
                fmt::print("Error: label '{}' is unknown\n", id);
                return 1;
            }
            if (pc > bytecode.size()) {
                fmt::print("Error: pc > bytecode.size() on label '{}'", id);
                return 1;
            }
            bytecode[pc].s.val = labels[id];
        }
    }

    if (compile_only) {
        auto name = std::filesystem::path(files.front()).replace_extension(".mclb");
        auto* file = std::fopen(name.string().data(), "w+b");
        if (!file) {
            fmt::print("Error: Failed to open output file '{}': {}\n", name.string(), std::strerror(errno));
            return 1;
        }
        for (const auto& instr : bytecode) {
            std::fwrite(&instr.v, 1, sizeof(instr.v), file);
        }
        std::fclose(file);
    }

    if (!compile_only) {
        if (!run_bytecode(std::move(bytecode))) {
            fmt::print("error while executing bytecode, execution stopped\n");
            return 1;
        }
    }
    return 0;
}
