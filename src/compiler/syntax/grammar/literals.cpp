#include "compiler/syntax/grammar/literals.hpp"

namespace tiro::next {

std::optional<int> to_digit(CodePoint c, int base) {
    switch (base) {
    case 2: {
        if (c >= '0' && c <= '1')
            return c - '0';
        return {};
    }
    case 8: {
        if (c >= '0' && c <= '7') {
            return c - '0';
        }
        return {};
    }
    case 10: {
        if (c >= '0' && c <= '9') {
            return c - '0';
        }
        return {};
    }
    case 16: {
        if (c >= '0' && c <= '9') {
            return c - '0';
        }
        if (c >= 'a' && c <= 'f') {
            return c - 'a' + 10;
        }
        if (c >= 'A' && c <= 'F') {
            return c - 'A' + 10;
        }
        return {};
    }
    }
    TIRO_UNREACHABLE("Invalid base.");
}

} // namespace tiro::next
