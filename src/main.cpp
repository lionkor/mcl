#include <fmt/core.h>

int main() {
    fmt::print("v{}.{}.{}-{}\n", PRJ_VERSION_MAJOR, PRJ_VERSION_MINOR, PRJ_VERSION_PATCH, PRJ_GIT_HASH);
}
