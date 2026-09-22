#include <iostream>
#include <string_view>

int main() {
    constexpr std::string_view msg = "Kindle Native Daemon initialized";
    std::cout << msg << std::endl;
    return 0;
}
