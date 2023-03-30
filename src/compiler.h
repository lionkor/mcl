#pragma once

#include "abstract_instruction.h"
#include "error.h"
#include <span>
#include <vector>

struct Token {
    int64_t i64;
    std::string str;
    bool is_str;
    bool is_i64;
    SourceLocation loc;

    Token(int64_t i, SourceLocation loc_)
        : i64(i)
        , str()
        , is_str(false)
        , is_i64(true)
        , loc(std::move(loc_)) { }

    Token(std::string s, SourceLocation loc_)
        : i64()
        , str(std::move(s))
        , is_str(true)
        , is_i64(false)
        , loc(std::move(loc_)) { }
};

using TokenStream = std::vector<Token>;
using AbstractInstrStream = std::vector<AbstractInstr>;
using InstrStream = std::vector<Instr>;

Result<TokenStream> parse(const std::span<std::string>& lines, const std::string& filename);
Result<AbstractInstrStream> translate(const TokenStream& tokens);

// do optimization steps between translate() and finalize()

Error optimize_substitute(AbstractInstrStream& abstracts);
Error optimize_fold(AbstractInstrStream& abstracts);

Result<InstrStream> finalize(AbstractInstrStream&& abstracts);
