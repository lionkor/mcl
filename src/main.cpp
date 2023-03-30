#include "compiler.h"
#include "interpreter.h"
#include <algorithm>
#include <cerrno>
#include <compare>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <ios>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

struct Config {
    bool compile_only = false;
    bool exec_only = false;
    std::vector<std::string_view> files {};
};

static Result<Config> parse_config_from_argv(int argc, char** argv) {
    Config cfg;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);
        if (arg.starts_with("--")) {
            if (arg == "--help") {
                fmt::print("Usage:\n\t{} [OPTION...] <FILE...>\n\nOptions:\n"
                           "\t--help\t\t Displays help\n"
                           "\t--version\t Displays version\n"
                           "\t--compile\t Enables compiling bytecode and not running the code. First specified file becomes output file ending in .mclb\n"
                           "\t--exec\t\t Expects files to be bytecode executables, and runs them\n",
                    argv[0]);
                std::exit(0);
            } else if (arg == "--version") {
                fmt::print("v{}.{}.{}-{}\n", PRJ_VERSION_MAJOR, PRJ_VERSION_MINOR, PRJ_VERSION_PATCH, PRJ_GIT_HASH);
                std::exit(0);
            } else if (arg == "--compile") {
                cfg.compile_only = true;
            } else if (arg == "--exec") {
                cfg.exec_only = true;
            } else {
                return { "Unknown argument '{}', run '{} --help' for help.", arg, argv[0] };
            }
        } else {
            cfg.files.push_back(arg);
        }
    }
    return cfg;
}

int main(int argc, char** argv) {
    Config cfg;
    auto cfg_res = parse_config_from_argv(argc, argv);
    if (cfg_res) {
        cfg = cfg_res.move();
    } else {
        fmt::print("Error: {}\n", cfg_res.error);
        return 1;
    }
    if (cfg.files.empty()) {
        fmt::print("Error: No file(s) specified. See '{} --help' for help.\n", argv[0]);
        return 1;
    }

    if (cfg.exec_only && cfg.compile_only) {
        fmt::print("Error: `exec` and `compile` not allowed at the same time. Run without arguments either of these arguments to compile and interpret source code in one go.\n");
        return 1;
    }

    bool interpret = !cfg.compile_only && !cfg.exec_only;

    if (cfg.compile_only || interpret) {
        for (const auto& filename : cfg.files) {
            if (filename.ends_with("mclb")) {
                fmt::print("Error: Passed `.mclb` file '{}' to the compiler, but `.mclb` is the extension of files which have already been compiled. Not allowing this.\n", filename);
                return 1;
            }
            std::ifstream file((std::string(filename)));
            std::string line;
            std::vector<std::string> lines;
            while (std::getline(file, line)) {
                lines.push_back(line);
            }
            auto parse_res = parse(lines, filename.data());
            TokenStream tokens;
            if (parse_res) {
                tokens = parse_res.move();
                fmt::print("Parsed {} tokens.\n", tokens.size());
            } else {
                fmt::print("Error while parsing: {}\n", parse_res.error);
                return 1;
            }
            auto translate_res = translate(tokens);
            AbstractInstrStream abstract_instrs;
            if (translate_res) {
                abstract_instrs = translate_res.move();
                fmt::print("Translated into {} abstract instructions.\n", abstract_instrs.size());
            } else {
                fmt::print("Error while translating: {}\n", translate_res.error);
                return 1;
            }

            auto err = optimize_substitute(abstract_instrs);
            if (err) {
                fmt::print("Error while applying substitution optimizations: {}\n", err.error);
                return 1;
            } else {
                fmt::print("Applied substitution optimizations resulting in {} abstract instructions.\n", abstract_instrs.size());
            }

            auto finalize_res = finalize(std::move(abstract_instrs));
            InstrStream instrs;
            if (finalize_res) {
                instrs = finalize_res.move();
                fmt::print("Finalized into {} instructions.\n", instrs.size());
            } else {
                fmt::print("Error while finalizing: {}\n", finalize_res.error);
                return 1;
            }
            // now write to file
            std::ofstream outfile(std::filesystem::path(filename).replace_extension("mclb").string(), std::ios::trunc | std::ios::binary);
            outfile.write(reinterpret_cast<const char*>(instrs.data()), std::streamsize(instrs.size() * sizeof(Instr)));
        }
    }
    if (interpret) {
        for (const auto& filename_mcl : cfg.files) {
            auto filename = std::filesystem::path(filename_mcl).replace_extension("mclb").string();
            std::ifstream file(filename, std::ios::binary);
            InstrStream instrs;
            instrs.resize(std::filesystem::file_size(filename));
            file.read(reinterpret_cast<char*>(instrs.data()), std::streamsize(instrs.size() * sizeof(Instr)));
            auto err = execute(std::move(instrs));
            if (err) {
                fmt::print("Error executing '{}': {}\n", filename, err.error);
                return 1;
            }
        }
    } else if (cfg.exec_only) {
        for (const auto& filename : cfg.files) {
            if (filename.ends_with(".mcl")) {
                fmt::print("Error: '{}' ends in '.mcl', which indicates it's a source file. For `--exec` mode, you must only pass compiled binary objects.", filename);
                return 1;
            }
            std::ifstream file(filename.data(), std::ios::binary);
            InstrStream instrs;
            instrs.resize(std::filesystem::file_size(filename.data()));
            file.read(reinterpret_cast<char*>(instrs.data()), std::streamsize(instrs.size() * sizeof(Instr)));
            auto err = execute(std::move(instrs));
            if (err) {
                fmt::print("Error executing '{}': {}\n", filename, err.error);
                return 1;
            }
        }
    }
    return 0;
}
