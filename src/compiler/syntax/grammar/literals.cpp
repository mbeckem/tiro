#include "compiler/syntax/grammar/literals.hpp"

#include "common/safe_int.hpp"
#include "common/text/code_point_range.hpp"

#include <algorithm>

namespace tiro::next {

static int read_base(std::string_view& source);
static bool has_prefix(std::string_view str, std::string_view prefix);

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

std::optional<std::string_view>
parse_symbol_name(std::string_view symbol_source, FunctionRef<void(std::string_view)> error_sink) {
    CodePointRange input(symbol_source);
    if (input.at_end() || *input != '#') {
        error_sink("symbols must start with '#'");
        return {};
    }
    input.advance();

    size_t start = input.pos();
    if (input.at_end()) {
        error_sink("symbols must have a name");
        return {};
    }
    if (!is_letter(*input)) {
        error_sink("symbols must start with a letter");
        return {};
    }
    return symbol_source.substr(start);
}

std::optional<i64> parse_integer_value(
    std::string_view integer_source, FunctionRef<void(std::string_view)> error_sink) {
    const int base = read_base(integer_source);

    SafeInt<i64> value = 0;
    bool has_digit = false;
    for (auto c : integer_source) {
        if (c == '_')
            continue;

        auto digit = to_digit(c, base);
        if (TIRO_UNLIKELY(!digit)) {
            error_sink("invalid digit for this base");
            return {};
        }

        has_digit = true;
        if (TIRO_UNLIKELY(!value.try_mul(base) || !value.try_add(*digit))) {
            error_sink("number is too large (integer overflow)");
            return {};
        }
    }

    if (!has_digit) {
        error_sink("expected at least one digit");
        return {};
    }
    return static_cast<i64>(value);
}

// TODO: Algorithm is not very correct nor fast
std::optional<f64>
parse_float_value(std::string_view float_source, FunctionRef<void(std::string_view)> error_sink) {
    const int base = read_base(float_source);
    const f64 base_inv = 1.0 / base;

    f64 int_value = 0;
    f64 frac_value = 0;
    bool has_digit = false;

    auto p = float_source.begin();
    const auto e = float_source.end();

    // Integer part
    for (; p != e; ++p) {
        auto c = *p;
        if (c == '_')
            continue;
        if (c == '.')
            break;

        auto digit = to_digit(c, base);
        if (TIRO_UNLIKELY(!digit)) {
            error_sink("invalid digit for this base");
            return {};
        }

        has_digit = true;
        int_value *= base;
        int_value += *digit;
    }

    // Float part
    if (p != e && *p == '.') {
        ++p;

        f64 factor = base_inv;
        for (; p != e; ++p) {
            auto c = *p;
            if (c == '_')
                continue;

            auto digit = to_digit(c, base);
            if (TIRO_UNLIKELY(!digit)) {
                error_sink("invalid digit for this base");
                return {};
            }

            has_digit = true;
            frac_value += factor * *digit;
            factor *= base_inv;
        }
    }

    if (!has_digit) {
        error_sink("expected at least one digit");
        return {};
    }
    return int_value + frac_value;
}

int read_base(std::string_view& source) {
    int base = 10;
    if (has_prefix(source, "0x")) {
        base = 16;
        source.remove_prefix(2);
    } else if (has_prefix(source, "0o")) {
        base = 8;
        source.remove_prefix(2);
    } else if (has_prefix(source, "0b")) {
        base = 2;
        source.remove_prefix(2);
    }
    return base;
}

bool has_prefix(std::string_view str, std::string_view prefix) {
    return str.size() >= prefix.size()
           && std::equal(str.begin(), str.begin() + prefix.size(), prefix.begin());
}

} // namespace tiro::next
