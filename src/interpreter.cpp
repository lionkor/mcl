#include "interpreter.h"
#include "instruction.h"

static inline void push(Stack& stack, int64_t value) {
#ifdef _DEBUG
    if (stack.stack_top + 1 >= stack.stack.size()) {
        fmt::print("FATAL: Stack check failed, tried to push onto full stack.\n");
        std::abort();
    }
#endif
    stack.stack[stack.stack_top] = value;
    ++stack.stack_top;
}

static inline int64_t pop(Stack& stack) {
#ifdef _DEBUG
    if (stack.stack_top == 0) {
        fmt::print("FATAL: Stack check failed, tried to pop from empty stack.\n");
        std::abort();
    }
#endif
    return stack.stack[--stack.stack_top];
}

static inline void pop_ignore(Stack& stack) {
#ifdef _DEBUG
    if (stack.stack_top == 0) {
        fmt::print("FATAL: Stack check failed, tried to pop from empty stack.\n");
        std::abort();
    }
#endif
    --stack.stack_top;
}

static inline int64_t at_offset(Stack& stack, int64_t offset) {
#ifdef _DEBUG
    if (int64_t(stack.stack_top) + offset < 0 || offset > 0) {
        fmt::print("FATAL: Stack check failed, tried access invalid stack position '{}'.\n", int64_t(stack.stack_top) + offset);
        std::abort();
    }
#endif
    return stack.stack[size_t(int64_t(stack.stack_top) + offset)];
}

static inline void swap(Stack& stack, int64_t o1, int64_t o2) {
#ifdef _DEBUG
    if (int64_t(stack.stack_top) + o1 < 0 || o1 > 0) {
        fmt::print("FATAL: Stack check failed, tried to swap with invalid stack position '{}'.\n", int64_t(stack.stack_top) + o1);
        std::abort();
    } else if (int64_t(stack.stack_top) + o2 < 0 || o2 > 0) {
        fmt::print("FATAL: Stack check failed, tried to swap with invalid stack position '{}'.\n", int64_t(stack.stack_top) + o2);
        std::abort();
    }
#endif
    std::swap(
        stack.stack[size_t(int64_t(stack.stack_top) + o1)],
        stack.stack[size_t(int64_t(stack.stack_top) + o2)]);
}

Error execute(InstrStream&& instrs) noexcept {
    Stack stack;
    Program prog {
        .instrs = std::move(instrs),
        .pc = 0,
    };

    bool running = true;
    while (running) [[likely]] {
#ifdef _DEBUG
        std::string ins = "";
        if (op_requires_i64_argument(prog.instrs[prog.pc].s.op)) {
            ins = fmt::format("{} {}", to_string(prog.instrs[prog.pc].s.op), int64_t(prog.instrs[prog.pc].s.val));
        } else {
            ins = fmt::format("{}", to_string(prog.instrs[prog.pc].s.op));
        }
        std::string stack_fmt = "";
        for (size_t i = 0; i < stack.stack_top; ++i) {
            stack_fmt += fmt::format("{} ", stack.stack.at(i));
        }
        fmt::print("dbg: {:<7} | {}\n", ins, stack_fmt);
#endif
        size_t next_pc = size_t(-1);
        switch (prog.instrs[prog.pc].s.op) {
        [[unlikely]] case NOT_AN_INSTRUCTION:
            return Error("Invalid instruction. pc={}, stack_top={}", prog.pc, stack.stack_top);
        case POP:
            pop_ignore(stack);
            break;
        case ADD: {
            const auto b = pop(stack);
            const auto a = pop(stack);
            push(stack, a + b);
            break;
        }
        case SUB: {
            const auto b = pop(stack);
            const auto a = pop(stack);
            push(stack, a - b);
            break;
        }
        case MUL: {
            const auto b = pop(stack);
            const auto a = pop(stack);
            push(stack, a * b);
            break;
        }
        case DIV: {
            const auto b = pop(stack);
            const auto a = pop(stack);
            if (b == 0) [[unlikely]] {
                return Error("Division by zero: {}/{}. pc={}", a, b, prog.pc);
            }
            push(stack, a / b);
            break;
        }
        case MOD: {
            const auto b = pop(stack);
            const auto a = pop(stack);
            if (b == 0) [[unlikely]] {
                return Error("Modulo division by zero: {}/{}. pc={}", a, b, prog.pc);
            }
            push(stack, a % b);
            break;
        }
        case PRINT:
            fmt::print("{}\n", pop(stack));
            break;
        case HALT:
            running = false;
            break;
        case DUP:
            push(stack, at_offset(stack, -1));
            break;
        case SWAP:
            swap(stack, -1, -2);
            break;
        case OVER:
            push(stack, at_offset(stack, -2));
            break;
        case PUSH:
            push(stack, prog.instrs[prog.pc].s.val);
            break;
        case JE: {
            const auto b = pop(stack);
            const auto a = pop(stack);
            if (a == b) {
                next_pc = size_t(prog.instrs[prog.pc].s.val);
            }
            break;
        }
        case JN: {
            const auto b = pop(stack);
            const auto a = pop(stack);
            if (a != b) {
                next_pc = size_t(prog.instrs[prog.pc].s.val);
            }
            break;
        }
        case JG: {
            const auto b = pop(stack);
            const auto a = pop(stack);
            if (a > b) {
                next_pc = size_t(prog.instrs[prog.pc].s.val);
            }
            break;
        }
        case JL: {
            const auto b = pop(stack);
            const auto a = pop(stack);
            if (a < b) {
                next_pc = size_t(prog.instrs[prog.pc].s.val);
            }
            break;
        }
        case JGE: {
            const auto b = pop(stack);
            const auto a = pop(stack);
            if (a >= b) {
                next_pc = size_t(prog.instrs[prog.pc].s.val);
            }
            break;
        }
        case JLE: {
            const auto b = pop(stack);
            const auto a = pop(stack);
            if (a <= b) {
                next_pc = size_t(prog.instrs[prog.pc].s.val);
            }
            break;
        }
        case JMP:
            next_pc = size_t(prog.instrs[prog.pc].s.val);
            break;
        }
        if (next_pc == size_t(-1)) [[likely]] {
            ++prog.pc;
        } else {
            prog.pc = next_pc;
        }
    }
    return {};
}
