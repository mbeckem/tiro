#include "compiler/syntax/grammar/literals.hpp"

#include "common/safe_int.hpp"
#include "common/text/code_point_range.hpp"

#include <algorithm>

namespace tiro {

namespace {

struct IntegerInfo {
    i64 value;
    bool explicit_base; // True if number starts with 0b or 0o or 0x
    bool leading_zero;  // True if additonal leading zeroes at the start, i.e. "01" but not "0"
};

} // namespace

static std::tuple<int, bool> read_base(std::string_view& source);
static bool has_prefix(std::string_view str, std::string_view prefix);

static std::optional<IntegerInfo>
parse_integer_impl(std::string_view integer_source, FunctionRef<void(std::string_view)> error_sink);

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
    auto result = parse_integer_impl(integer_source, error_sink);
    if (result)
        return result->value;
    return {};
}

// TODO: Algorithm is not very correct nor fast
std::optional<f64>
parse_float_value(std::string_view float_source, FunctionRef<void(std::string_view)> error_sink) {
    const auto [base, has_explicit_base] = read_base(float_source);
    const f64 base_inv = 1.0 / base;
    (void) has_explicit_base;

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

std::optional<u32>
parse_tuple_field(std::string_view source, FunctionRef<void(std::string_view)> error_sink) {
    auto result = parse_integer_impl(source, error_sink);
    if (!result)
        return {};

    if (result->explicit_base) {
        error_sink("tuple fields must use base 10 digits");
        return {};
    }
    if (result->leading_zero) {
        error_sink("tuple fields must not use leading zeroes");
        return {};
    }

    auto value = result->value;
    if (value < 0) {
        error_sink("tuple fields must not be negative");
        return {};
    }

    if (value > std::numeric_limits<u32>::max()) {
        error_sink("tuple field is too large");
        return {};
    }

    return static_cast<u32>(value);
}

bool parse_string_literal(std::string_view string_source, std::string& output,
    FunctionRef<void(std::string_view)> error_sink) {
    bool success = true;

    CodePointRange range(string_source);
    while (!range.at_end()) {
        const auto current = range.get();
        range.advance();

        if (current != '\\') {
            append_utf8(output, current);
            continue;
        }

        if (range.at_end()) {
            error_sink("incomplete escape sequence at the end of the string");
            return false;
        }

        const auto escape_char = range.get();
        range.advance();

        // TODO: Multi char escape sequences, e.g. \xAB or \u{...}
        switch (escape_char) {
        case 'n':
            output += '\n';
            break;
        case 'r':
            output += '\r';
            break;
        case 't':
            output += '\t';
            break;

        case '"':
        case '\'':
        case '\\':
        case '$':
            append_utf8(output, escape_char);
            break;

        default: {
            error_sink(fmt::format("invalid escape character '{}'", to_string_utf8(escape_char)));
            success = false;
            break;
        }
        }
    }
    return success;
}

std::optional<IntegerInfo> parse_integer_impl(
    std::string_view integer_source, FunctionRef<void(std::string_view)> error_sink) {
    const auto [base, has_explicit_base] = read_base(integer_source);

    SafeInt<i64> value = 0;
    int digits = 0;
    int leading_zeroes = 0;
    for (auto c : integer_source) {
        if (c == '_')
            continue;

        auto digit = to_digit(c, base);
        if (TIRO_UNLIKELY(!digit)) {
            error_sink("invalid digit for this base");
            return {};
        }
        ++digits;

        if (value == 0 && *digit == 0)
            ++leading_zeroes;

        if (TIRO_UNLIKELY(!value.try_mul(base) || !value.try_add(*digit))) {
            error_sink("number is too large (integer overflow)");
            return {};
        }
    }

    if (digits == 0) {
        error_sink("expected at least one digit");
        return {};
    }
    bool has_leading_zero = value == 0 ? leading_zeroes > 1 : leading_zeroes > 0;
    return IntegerInfo{static_cast<i64>(value), has_explicit_base, has_leading_zero};
}

std::tuple<int, bool> read_base(std::string_view& source) {
    int base = 10;
    bool explicit_base = false;
    if (has_prefix(source, "0x")) {
        base = 16;
        source.remove_prefix(2);
        explicit_base = true;
    } else if (has_prefix(source, "0o")) {
        base = 8;
        source.remove_prefix(2);
        explicit_base = true;
    } else if (has_prefix(source, "0b")) {
        base = 2;
        source.remove_prefix(2);
        explicit_base = true;
    }
    return std::tuple(base, explicit_base);
}

bool has_prefix(std::string_view str, std::string_view prefix) {
    return str.size() >= prefix.size()
           && std::equal(str.begin(), str.begin() + prefix.size(), prefix.begin());
}

} // namespace tiro
