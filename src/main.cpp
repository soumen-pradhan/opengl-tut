#include <spdlog/spdlog.h>

int main()
{
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern(
        "%Y-%m-%d %H:%M:%S.%e "
        "%^%5l%$ | %s:%# | %v");

    SPDLOG_INFO("Hello {}", 42);
    SPDLOG_ERROR("Something failed: {}", -1);
}
