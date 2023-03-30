#include "compiler.h"
#include "abstract_instruction.h"
#include "instruction.h"
#include "source_location.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <climits>
#include <ranges>
#include <regex>
#include <span>
#include <string>
#include <unordered_map>

static inline std::string& ltrim_inplace(std::string& str) {
    auto it2 = std::find_if(str.begin(), str.end(), [](char ch) { return !std::isspace<char>(ch, std::locale::classic()); });
    str.erase(str.begin(), it2);
    return str;
}

static inline std::string& rtrim_inplace(std::string& str) {
    auto it1 = std::find_if(str.rbegin(), str.rend(), [](char ch) { return !std::isspace<char>(ch, std::locale::classic()); });
    str.erase(it1.base(), str.end());
    return str;
}

static inline std::string ltrim(const std::string& str_in) {
    auto str = str_in;
    auto it2 = std::find_if(str.begin(), str.end(), [](char ch) { return !std::isspace<char>(ch, std::locale::classic()); });
    str.erase(str.begin(), it2);
    return str;
}

static inline std::string rtrim(const std::string& str_in) {
    auto str = str_in;
    auto it1 = std::find_if(str.rbegin(), str.rend(), [](char ch) { return !std::isspace<char>(ch, std::locale::classic()); });
    str.erase(it1.base(), str.end());
    return str;
}

static inline std::string trim(const std::string& str) {
    auto res = str;
    return ltrim_inplace(rtrim_inplace(res));
}

Result<TokenStream> parse(const std::span<std::string>& lines, const std::string& filename) {
    TokenStream tokens;
    SourceLocation loc {
        .file = filename,
        .line = 0,
        .col_start = 0,
        .col_end = 0,
    };
    const std::string_view valid_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-0123456789:";

    for (const auto& raw_line : lines) {
        // keep track of line count, do first so it starts at 1
        ++loc.line;

        auto line = trim(raw_line);

        std::string::size_type word_start = 0;

        const std::regex re_str(R"(:?[a-zA-Z_][a-zA-Z_0-9]*)");
        const std::regex re_int(R"([-+]?[0-9]+)");
        const std::regex re_hex(R"([-+]?0x[0-9a-f]+)");
        const std::regex re_comment(R"(#.*)");

        line = std::regex_replace(line, re_comment, "");
        if (line.empty()) {
            continue;
        }

        while (true) {
            // isolate a word
            if (word_start >= line.size()) {
                break;
            }
            auto word_end = line.find_first_not_of(valid_chars, word_start);
            if (word_end == word_start) {
                ++word_start;
                continue;
            }
            std::string word_untrimmed = line.substr(word_start, word_end - word_start);
            // trim left, see difference
            auto word = ltrim(word_untrimmed);
            auto left_trimmed_n = (word_untrimmed.size() - word.size());
            loc.col_start = word_start + 1 + left_trimmed_n;
            loc.col_end = loc.col_start + word.size();
            // check if the word is a string, integer, etc.
            if (std::regex_match(word, re_str)) {
                tokens.emplace_back(word, loc);
            } else if (std::regex_match(word, re_int)) {
                char* end;
                int64_t i = std::strtoll(word.c_str(), &end, 10);
                if (i == LLONG_MAX || i == LLONG_MIN) {
                    // out of range for i64
                    return { "{}: Integer too large.", to_string(loc) };
                } else if (i == 0 && end == word.c_str()) {
                    return { "{}: Not an integer.", to_string(loc) };
                }
                tokens.emplace_back(i, loc);
            } else if (std::regex_match(word, re_hex)) {
                char* end;
                int64_t i = std::strtoll(word.c_str(), &end, 16);
                if (i == LLONG_MAX || i == LLONG_MIN) {
                    // out of range for i64
                    return { "{}: (Hexadecimal) integer too large.", to_string(loc) };
                } else if (i == 0 && end == word.c_str()) {
                    return { "{}: Not a valid (hexadecimal) integer.", to_string(loc) };
                }
                tokens.emplace_back(i, loc);
            } else {
                return { "{}: Invalid token '{}'.", to_string(loc), word };
            }
            if (word_end == std::string::npos) {
                break;
            }
            word_start = word_end + 1;
        }
    }
    return tokens;
}

static inline Token consume(std::span<const Token>& span) {
    auto tok = std::move(span.front());
    span = span.subspan(1);
    return tok;
}

static inline bool can_consume(const std::span<const Token>& span) {
    return !span.empty();
}

Result<AbstractInstrStream> translate(const TokenStream& tokens) {
    AbstractInstrStream result;
    std::span<const Token> span(tokens.begin(), tokens.end());
    while (!span.empty()) {
        auto verb = consume(span);
        if (!verb.is_str) {
            return { "{}: Expected instruction, instead got '{}'", to_string(verb.loc), verb.i64 };
        }
        if (verb.str.starts_with(':')) {
            // is label, so create an abstract instruction which simply holds the label
            // value. this is a bit of a hack, but makes the program flow very simple.
            result.push_back(AbstractInstr {
                .instr = {
                    .s = {
                        .op = NOT_AN_INSTRUCTION,
                        .val = 0,
                    },
                },
                .location = verb.loc,
                .unresolved_symbol = std::nullopt,
                .unresolved_label = verb.str.substr(1),
            });
        } else {
            // assume it's an instruction
            auto op = op_from_string(verb.str);
            if (op == NOT_AN_INSTRUCTION) {
                // invalid instr
                return { "{}: Invalid instruction '{}'.", to_string(verb.loc), verb.str };
            }
            if (op_requires_i64_argument(op)) {
                if (!can_consume(span)) {
                    return { "{}: '{}' expects an i64 argument, but no argument was provided.", to_string(verb.loc), verb.str };
                }
                auto arg = consume(span);
                if (!arg.is_i64) {
                    if (!op_accepts_label_argument(op)) {
                        // TODO: check if it's an instruction, give an error about "missing" argument instead
                        return { "{}: '{}' expects an i64 argument, but given argument '{}' has the wrong type.", to_string(verb.loc), verb.str, arg.str };
                    }
                    if (arg.str.starts_with(':')) {
                        // is label
                        result.push_back(AbstractInstr {
                            .instr = {
                                .s = {
                                    .op = op,
                                    .val = 0,
                                },
                            },
                            .location = {
                                .file = verb.loc.file,
                                .line = verb.loc.line,
                                .col_start = verb.loc.col_start,
                                .col_end = arg.loc.col_end,
                            },
                            .unresolved_symbol = std::nullopt,
                            .unresolved_label = arg.str.substr(1),
                        });
                    } else {
                        return { "{}: '{}' expects an i64 argument (or a label), but given argument '{}' has the wrong type.", to_string(verb.loc), verb.str, arg.str };
                    }
                } else {
                    // TODO: If the instruction stretches over two lines, the columns are wrong here.
                    result.push_back(AbstractInstr {
                        .instr = {
                            .s = {
                                .op = op,
                                .val = arg.i64,
                            },
                        },
                        .location = {
                            .file = verb.loc.file,
                            .line = verb.loc.line,
                            .col_start = verb.loc.col_start,
                            .col_end = arg.loc.col_end,
                        },
                        .unresolved_symbol = std::nullopt,
                        .unresolved_label = std::nullopt,
                    });
                }
            } else {
                result.push_back(AbstractInstr {
                    .instr = {
                        .s = {
                            .op = op,
                            .val = 0,
                        },
                    },
                    .location = {
                        .file = verb.loc.file,
                        .line = verb.loc.line,
                        .col_start = verb.loc.col_start,
                        .col_end = verb.loc.col_end,
                    },
                    .unresolved_symbol = std::nullopt,
                    .unresolved_label = std::nullopt,
                });
            }
        }
    }
    return result;
}

static inline void populate_labels(const AbstractInstrStream& abstracts, std::unordered_map<std::string, size_t>& labels) {
    size_t addr_counter = 0;
    for (const auto& abstract : abstracts) {
        if (abstract.instr.s.op == NOT_AN_INSTRUCTION) {
            // set label address to next instruction's address
            // "unresolved" is abused here, and is in fact helping resolve it later
            labels[abstract.unresolved_label.value()] = addr_counter;
        } else {
            ++addr_counter;
        }
    }
}

static inline Error resolve_labels(const std::unordered_map<std::string, size_t>& labels, AbstractInstrStream& abstracts) {
    for (auto& abstract : abstracts) {
        if (abstract.instr.s.op != NOT_AN_INSTRUCTION
            && abstract.unresolved_label.has_value()) {
            // has a label which needs to be resolved
            auto u_label = abstract.unresolved_label.value();
            if (!labels.contains(u_label)) {
                return Error("{}: Could not find label '{}'.", to_string(abstract.location), u_label);
            }
            abstract.instr.s.val = int64_t(labels.at(u_label));
        }
    }
    return {};
}

Result<InstrStream> finalize(AbstractInstrStream&& abstracts) {
    std::unordered_map<std::string, size_t> labels;
    populate_labels(abstracts, labels);
    auto err = resolve_labels(labels, abstracts);
    if (err) {
        return { "Failed to resolve label(s): {}", err.error };
    }

    InstrStream instrs;
    instrs.reserve(abstracts.size() - labels.size());

    for (const auto& abstract : abstracts) {
        if (abstract.instr.s.op != NOT_AN_INSTRUCTION) {
            instrs.push_back(abstract.instr);
        }
    }
    return instrs;
}

/// Replace `push 1; add` with `inc`
static Error optimize_substitute_inc(AbstractInstrStream& abstracts) {
    std::vector<size_t> to_remove {};
    for (size_t i = 0; i < abstracts.size() - 1; ++i) {
        Op& op = abstracts[i].instr.s.op;
        Op& next_op = abstracts[i + 1].instr.s.op;

        int64_t val = abstracts[i].instr.s.val;
        if (op == PUSH && val == 1
            && next_op == ADD) {
            to_remove.push_back(i + 1);
            op = INC;
            val = 0;
        }
        abstracts[i].instr.s.val = val;
    }
    std::reverse(to_remove.begin(), to_remove.end());
    for (size_t i : to_remove) {
        abstracts.erase(abstracts.begin() + long(i));
    }
    return {};
}

/// Replace `push 1; sub` with `dec`
static Error optimize_substitute_dec(AbstractInstrStream& abstracts) {
    std::vector<size_t> to_remove {};
    for (size_t i = 0; i < abstracts.size() - 1; ++i) {
        Op& op = abstracts[i].instr.s.op;
        Op& next_op = abstracts[i + 1].instr.s.op;

        int64_t val = abstracts[i].instr.s.val;
        if (op == PUSH && val == 1
            && next_op == SUB) {
            to_remove.push_back(i + 1);
            op = DEC;
            val = 0;
        }
        abstracts[i].instr.s.val = val;
    }
    std::reverse(to_remove.begin(), to_remove.end());
    for (size_t i : to_remove) {
        abstracts.erase(abstracts.begin() + long(i));
    }
    return {};
}

/// Replace `push 0; je` with `jz`
static Error optimize_substitute_jz(AbstractInstrStream& abstracts) {
    std::vector<size_t> to_remove {};
    for (size_t i = 0; i < abstracts.size() - 1; ++i) {
        Op& op = abstracts[i].instr.s.op;
        Op& next_op = abstracts[i + 1].instr.s.op;

        int64_t val = abstracts[i].instr.s.val;
        if (op == PUSH && val == 0
            && next_op == JE) {
            to_remove.push_back(i);
            next_op = JZ;
        }
    }
    std::reverse(to_remove.begin(), to_remove.end());
    for (size_t i : to_remove) {
        abstracts.erase(abstracts.begin() + long(i));
    }
    return {};
}

/// Replace `push 0; jn` with `jnz`
static Error optimize_substitute_jnz(AbstractInstrStream& abstracts) {
    std::vector<size_t> to_remove {};
    for (size_t i = 0; i < abstracts.size() - 1; ++i) {
        Op& op = abstracts[i].instr.s.op;
        Op& next_op = abstracts[i + 1].instr.s.op;

        int64_t val = abstracts[i].instr.s.val;
        if (op == PUSH && val == 0
            && next_op == JN) {
            to_remove.push_back(i);
            next_op = JNZ;
        }
    }
    std::reverse(to_remove.begin(), to_remove.end());
    for (size_t i : to_remove) {
        abstracts.erase(abstracts.begin() + long(i));
    }
    return {};
}

/// Replace `over; over` with `dup2`
static Error optimize_substitute_dup2(AbstractInstrStream& abstracts) {
    std::vector<size_t> to_remove {};
    for (size_t i = 0; i < abstracts.size() - 1; ++i) {
        Op& op = abstracts[i].instr.s.op;
        Op& next_op = abstracts[i + 1].instr.s.op;
        if (op == OVER && next_op == OVER) {
            to_remove.push_back(i + 1);
            op = DUP2;
        }
    }
    std::reverse(to_remove.begin(), to_remove.end());
    for (size_t i : to_remove) {
        abstracts.erase(abstracts.begin() + long(i));
    }
    return {};
}

Error optimize_substitute(AbstractInstrStream& abstracts) {
    auto err = optimize_substitute_inc(abstracts);
    if (err) {
        return err;
    }
    err = optimize_substitute_dec(abstracts);
    if (err) {
        return err;
    }
    err = optimize_substitute_jz(abstracts);
    if (err) {
        return err;
    }
    err = optimize_substitute_jnz(abstracts);
    if (err) {
        return err;
    }
    err = optimize_substitute_dup2(abstracts);
    if (err) {
        return err;
    }
    return {};
}
