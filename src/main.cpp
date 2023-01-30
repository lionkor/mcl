#include <fmt/core.h>

int main() {
    fmt::print("Hello from project version {}.{}.{}-{}\n", PRJ_VERSION_MAJOR, PRJ_VERSION_MINOR, PRJ_VERSION_PATCH, PRJ_GIT_HASH);
}
