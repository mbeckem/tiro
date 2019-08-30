#include "hammer/compiler/token.hpp"

#include "hammer/core/defs.hpp"

namespace hammer {

std::string_view to_token_name(TokenType tok) {
    switch (tok) {
#define HAMMER_CASE(x)   \
    case (TokenType::x): \
        return #x;

        HAMMER_CASE(invalid_token)
        HAMMER_CASE(eof)
        HAMMER_CASE(comment)

        HAMMER_CASE(identifier)
        HAMMER_CASE(string_literal)
        HAMMER_CASE(float_literal)
        HAMMER_CASE(integer_literal)

        HAMMER_CASE(kw_func)
        HAMMER_CASE(kw_var)
        HAMMER_CASE(kw_const)
        HAMMER_CASE(kw_if)
        HAMMER_CASE(kw_else)
        HAMMER_CASE(kw_while)
        HAMMER_CASE(kw_for)
        HAMMER_CASE(kw_continue)
        HAMMER_CASE(kw_break)
        HAMMER_CASE(kw_return)
        HAMMER_CASE(kw_switch)
        HAMMER_CASE(kw_class)
        HAMMER_CASE(kw_struct)
        HAMMER_CASE(kw_protocol)
        HAMMER_CASE(kw_true)
        HAMMER_CASE(kw_false)
        HAMMER_CASE(kw_null)
        HAMMER_CASE(kw_import)
        HAMMER_CASE(kw_export)
        HAMMER_CASE(kw_package)

        HAMMER_CASE(kw_yield)
        HAMMER_CASE(kw_async)
        HAMMER_CASE(kw_await)
        HAMMER_CASE(kw_throw)
        HAMMER_CASE(kw_try)
        HAMMER_CASE(kw_catch)
        HAMMER_CASE(kw_scope)

        HAMMER_CASE(lparen)
        HAMMER_CASE(rparen)
        HAMMER_CASE(lbracket)
        HAMMER_CASE(rbracket)
        HAMMER_CASE(lbrace)
        HAMMER_CASE(rbrace)

        HAMMER_CASE(dot)
        HAMMER_CASE(comma)
        HAMMER_CASE(colon)
        HAMMER_CASE(semicolon)
        HAMMER_CASE(question)
        HAMMER_CASE(plus)
        HAMMER_CASE(minus)
        HAMMER_CASE(star)
        HAMMER_CASE(starstar)
        HAMMER_CASE(slash)
        HAMMER_CASE(percent)
        HAMMER_CASE(plusplus)
        HAMMER_CASE(minusminus)
        HAMMER_CASE(bnot)
        HAMMER_CASE(bor)
        HAMMER_CASE(bxor)
        HAMMER_CASE(band)
        HAMMER_CASE(lnot)
        HAMMER_CASE(lor)
        HAMMER_CASE(land)
        HAMMER_CASE(eq)
        HAMMER_CASE(eqeq)
        HAMMER_CASE(neq)
        HAMMER_CASE(less)
        HAMMER_CASE(greater)
        HAMMER_CASE(lesseq)
        HAMMER_CASE(greatereq)

#undef HAMMER_CASE
    }

    HAMMER_UNREACHABLE("Invalid token type");
}

std::string_view to_helpful_string(TokenType tok) {
    switch (tok) {
#define HAMMER_CASE(x, s) \
    case TokenType::x:    \
        return s;

#define HAMMER_CASE_QUOTED(x, s) HAMMER_CASE(x, "'" s "'")

        HAMMER_CASE(invalid_token, "<invalid_token>")
        HAMMER_CASE(eof, "<end of file>")
        HAMMER_CASE(comment, "<comment>")

        HAMMER_CASE(identifier, "<identifier>")
        HAMMER_CASE(string_literal, "<string>")
        HAMMER_CASE(float_literal, "<float>")
        HAMMER_CASE(integer_literal, "<integer>")

        HAMMER_CASE_QUOTED(kw_func, "func")
        HAMMER_CASE_QUOTED(kw_var, "var")
        HAMMER_CASE_QUOTED(kw_const, "const")
        HAMMER_CASE_QUOTED(kw_if, "if")
        HAMMER_CASE_QUOTED(kw_else, "else")
        HAMMER_CASE_QUOTED(kw_while, "while")
        HAMMER_CASE_QUOTED(kw_for, "for")
        HAMMER_CASE_QUOTED(kw_continue, "continue")
        HAMMER_CASE_QUOTED(kw_break, "break")
        HAMMER_CASE_QUOTED(kw_return, "return")
        HAMMER_CASE_QUOTED(kw_switch, "switch")
        HAMMER_CASE_QUOTED(kw_class, "class")
        HAMMER_CASE_QUOTED(kw_struct, "struct")
        HAMMER_CASE_QUOTED(kw_protocol, "protocol")
        HAMMER_CASE_QUOTED(kw_true, "true")
        HAMMER_CASE_QUOTED(kw_false, "false")
        HAMMER_CASE_QUOTED(kw_null, "null")
        HAMMER_CASE_QUOTED(kw_import, "import")
        HAMMER_CASE_QUOTED(kw_export, "export")
        HAMMER_CASE_QUOTED(kw_package, "package")

        HAMMER_CASE_QUOTED(kw_yield, "yield")
        HAMMER_CASE_QUOTED(kw_async, "async")
        HAMMER_CASE_QUOTED(kw_await, "await")
        HAMMER_CASE_QUOTED(kw_throw, "throw")
        HAMMER_CASE_QUOTED(kw_try, "try")
        HAMMER_CASE_QUOTED(kw_catch, "catch")
        HAMMER_CASE_QUOTED(kw_scope, "scope")

        HAMMER_CASE_QUOTED(lparen, "(")
        HAMMER_CASE_QUOTED(rparen, ")")
        HAMMER_CASE_QUOTED(lbracket, "[")
        HAMMER_CASE_QUOTED(rbracket, "]")
        HAMMER_CASE_QUOTED(lbrace, "{")
        HAMMER_CASE_QUOTED(rbrace, "}")

        HAMMER_CASE_QUOTED(dot, ".")
        HAMMER_CASE_QUOTED(comma, ",")
        HAMMER_CASE_QUOTED(colon, ":")
        HAMMER_CASE_QUOTED(semicolon, ";")
        HAMMER_CASE_QUOTED(question, "?")
        HAMMER_CASE_QUOTED(plus, "+")
        HAMMER_CASE_QUOTED(minus, "-")
        HAMMER_CASE_QUOTED(star, "*")
        HAMMER_CASE_QUOTED(starstar, "**")
        HAMMER_CASE_QUOTED(slash, "/")
        HAMMER_CASE_QUOTED(percent, "%")
        HAMMER_CASE_QUOTED(plusplus, "++")
        HAMMER_CASE_QUOTED(minusminus, "--")
        HAMMER_CASE_QUOTED(bnot, "~")
        HAMMER_CASE_QUOTED(bor, "|")
        HAMMER_CASE_QUOTED(bxor, "^")
        HAMMER_CASE_QUOTED(band, "&")
        HAMMER_CASE_QUOTED(lnot, "!")
        HAMMER_CASE_QUOTED(lor, "||")
        HAMMER_CASE_QUOTED(land, "&&")
        HAMMER_CASE_QUOTED(eq, "=")
        HAMMER_CASE_QUOTED(eqeq, "==")
        HAMMER_CASE_QUOTED(neq, "!=")
        HAMMER_CASE_QUOTED(less, "<")
        HAMMER_CASE_QUOTED(greater, ">")
        HAMMER_CASE_QUOTED(lesseq, "<=")
        HAMMER_CASE_QUOTED(greatereq, ">=")

#undef HAMMER_CASE
#undef HAMMER_CASE_QUOTED
    }

    HAMMER_UNREACHABLE("Invalid token type");
}

i64 Token::int_value() const {
    HAMMER_ASSERT(std::holds_alternative<i64>(value_), "Token does not contain an integer value.");
    return std::get<i64>(value_);
}

double Token::float_value() const {
    HAMMER_ASSERT(std::holds_alternative<double>(value_), "Token does not contain a float value.");
    return std::get<double>(value_);
}

InternedString Token::string_value() const {
    HAMMER_ASSERT(std::holds_alternative<InternedString>(value_),
                  "Token does not contain a string value.");
    return std::get<InternedString>(value_);
}

} // namespace hammer
