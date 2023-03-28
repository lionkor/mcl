#include "compiler.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <climits>
#include <ranges>
#include <regex>
#include <string>

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
        const std::regex re_hex(R"([-+]?0x[0-9]+)");
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

Result<AbstractInstrStream> translate(const TokenStream& tokens) {

}
