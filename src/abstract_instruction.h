#pragma once

#include "instruction.h"
#include "source_location.h"
#include <optional>
#include <string>

struct AbstractInstr {
    /// possibly incomplete instruction
    Instr instr;
    /// location this instruction was compiled from
    SourceLocation location;
    /// unresolved symbol, if exists
    std::optional<std::string> unresolved_symbol;
    /// unresolved label, if exists
    std::optional<std::string> unresolved_label;
};
