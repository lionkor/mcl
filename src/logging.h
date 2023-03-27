#pragma once

#include <spdlog/spdlog.h>
#include <string_view>

namespace l = spdlog;

#define name_of(x) std::string_view(#x)

