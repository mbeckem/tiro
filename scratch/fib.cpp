#include <cstdint>

#include <iostream>
#include <sstream>
#include <string>

using i64 = std::int64_t;

i64 fib(i64 i) {
    if (i <= 1)
        return i;
    return fib(i - 1) + fib(i - 2);
}

int main(int argc, char** argv) {
    i64 value;
    
    std::stringstream str_value{std::string(argv[1])};
    str_value >> value;

    std::cout << "Computing fib " << value << std::endl;
    std::cout << "Result: " << fib(value) << std::endl;
}
