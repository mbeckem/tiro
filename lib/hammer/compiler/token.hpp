#ifndef HAMMER_COMPILER_TOKEN_HPP
#define HAMMER_COMPILER_TOKEN_HPP

#include "hammer/compiler/source_reference.hpp"
#include "hammer/compiler/string_table.hpp"

#include <bitset>
#include <string_view>
#include <variant>

namespace hammer {

/**
 * List of all known tokens.
 *
 * Note: if you add a new keyword, you will likely want to
 * add the string --> token_type mapping in lexer.cpp (keywords_table) as well.
 */
enum class TokenType : byte {
    invalid_token = 0,
    eof,
    comment,

    // Primitives
    identifier,
    string_literal,
    float_literal,
    integer_literal,

    // Keywords
    kw_func,
    kw_var,
    kw_const,
    kw_if,
    kw_else,
    kw_while,
    kw_for,
    kw_continue,
    kw_break,
    kw_return,
    kw_switch,
    kw_class,
    kw_struct,
    kw_protocol,
    kw_true,
    kw_false,
    kw_null,
    kw_import,
    kw_export,
    kw_package,

    // Reserved
    kw_yield,
    kw_async,
    kw_await,
    kw_throw,
    kw_try,
    kw_catch,
    kw_scope,

    // Braces
    lparen,   // (
    rparen,   // )
    lbracket, // [
    rbracket, // ]
    lbrace,   // {
    rbrace,   // }

    // Operators
    dot,        // .
    comma,      // ,
    colon,      // :
    semicolon,  // ;
    question,   // ?
    plus,       // +
    minus,      // -
    star,       // *
    starstar,   // **
    slash,      // /
    percent,    // %
    plusplus,   // ++
    minusminus, // --
    bnot,       // ~
    bor,        // |
    bxor,       // ^
    band,       // &
    lnot,       // !
    lor,        // ||
    land,       // &&
    eq,         // =
    eqeq,       // ==
    neq,        // !=
    less,       // <
    greater,    // >
    lesseq,     // <=
    greatereq,  // >=

    // Must keep in sync with largest value!
    max_enum_value = greatereq
};

// Returns the name of the enum identifier.
std::string_view to_token_name(TokenType tok);

// Returns a human readable string for the given token.
std::string_view to_helpful_string(TokenType tok);

// Returns the raw numeric value of the given token type.
static constexpr auto to_underlying(TokenType type) {
    return static_cast<std::underlying_type_t<TokenType>>(type);
}

class Token {
public:
    Token() = default;

    Token(TokenType type, const SourceReference& source)
        : type_(type)
        , source_(source) {}

    // Type of the token.
    TokenType type() const { return type_; }
    void type(TokenType t) { type_ = t; }

    // Source code part that contains the token.
    const SourceReference& source() const { return source_; }
    void source(const SourceReference& source) { source_ = source; }

    // True if the Token contains an error (e.g. invalid characters within a
    // number or an identifier).
    bool has_error() const { return has_error_; }
    void has_error(bool has_error) { has_error_ = has_error; }

    // It depends on the token type whether there is a value of the requested type.

    // only integer_literal
    i64 int_value() const;
    void int_value(i64 v) { value_ = v; }

    // only float_literal
    double float_value() const;
    void float_value(double v) { value_ = v; }

    // only string_literal and identifier
    InternedString string_value() const;
    void string_value(InternedString v) { value_ = v; }

private:
    TokenType type_ = TokenType::invalid_token;
    bool has_error_ = false;
    SourceReference source_;
    std::variant<std::monostate, i64, double, InternedString> value_;
};

/**
 * A set of token types, implemented as an efficient bit set.
 */
class TokenTypes {
public:
    class const_iterator {
    public:
        using value_type = TokenType;
        using reference = TokenType;
        using pointer = const TokenType*;
        using difference_type = size_t;
        using iterator_category = std::input_iterator_tag;

    public:
        const_iterator() = default;

        const_iterator& operator++() {
            HAMMER_ASSERT(tts, "Invalid iterator instance.");
            HAMMER_ASSERT(index < tts->set_.size(), "Cannot increment the past-the-end iterator.");
            index = tts->find_first_from(index + 1);
            return *this;
        }

        const_iterator operator++(int) const {
            const_iterator old(*this);
            (*this)++;
            return old;
        }

        TokenType operator*() const {
            HAMMER_ASSERT(tts, "Invalid iterator instance.");
            HAMMER_ASSERT(index < tts->set_.size(),
                          "Cannot dereference the past-the-end iterator.");
            return static_cast<TokenType>(index);
        }

        bool operator==(const const_iterator& other) const {
            HAMMER_ASSERT(tts == other.tts, "Comparing iterators from different sets.");
            return index == other.index;
        }

        bool operator!=(const const_iterator& other) const { return !operator==(other); }

    private:
        friend TokenTypes;

        explicit const_iterator(const TokenTypes* tts_, size_t index_)
            : tts(tts_)
            , index(index_) {}

    private:
        const TokenTypes* tts = nullptr;
        size_t index = 0;
    };

    /// Constructs an empty set.
    TokenTypes() = default;

    /// Constructs a set with a single member.
    TokenTypes(TokenType tok) { insert(tok); }

    /// Constructs a set from the contents of the given initializer list.
    TokenTypes(std::initializer_list<TokenType> toks)
        : TokenTypes(toks.begin(), toks.end()) {}

    /// Constructs a set from the contents of the given range of TokenType values.
    template<typename Iterator>
    TokenTypes(Iterator first, Iterator last) {
        for (; first != last; ++first) {
            TokenType tok = *first;
            insert(tok);
        }
    }

    /// Returns an iterator to the first token type. Returns `end()` if this set is empty.
    const_iterator begin() const { return const_iterator(this, find_first_from(0)); }

    /// Returns the past-the-end iterator.
    const_iterator end() const { return const_iterator(this, enum_values); }

    /// Returns true iff `type` is a member of this set.
    bool contains(TokenType type) const { return set_.test(to_underlying(type)); }

    /// Inserts `type` into the set.
    void insert(TokenType type) { set_.set(to_underlying(type)); }

    /// Removes `type` from the set.
    void remove(TokenType type) { set_.reset(to_underlying(type)); }

    /// Returns the number of token types in this set.
    size_t size() const { return set_.count(); }

    /// Returns true iff `size() == 0`.
    bool empty() const { return size() == 0; }

    /// Returns a new set that is the union of `*this` and `other`.
    TokenTypes union_with(TokenTypes other) const {
        other.set_ |= set_;
        return other;
    }

    /// Returns a new set that is the intersection of `*this` and `other`.
    TokenTypes intersection_with(TokenTypes other) const {
        other.set_ &= set_;
        return other;
    }

    bool operator==(const TokenTypes& other) const { return set_ == other.set_; }
    bool operator!=(const TokenTypes& other) const { return set_ != other.set_; }

private:
    static constexpr auto enum_values = to_underlying(TokenType::max_enum_value) + 1;

    using bitset_type = std::bitset<enum_values>;

private:
    // Find the index of the first set bit, starting from the given index.
    // Returns enum_values (i.e. set_.size()) if none was found.
    size_t find_first_from(size_t index) const {
        // set_.find_first(...) would be nice to have. An efficient version would
        // use ffs instructions. We would need our own bitset implementation which is
        // not really needed right now.
        HAMMER_ASSERT(index <= enum_values, "Invalid index.");
        for (; index < enum_values; ++index) {
            if (set_.test(index))
                break;
        }
        return index;
    }

private:
    bitset_type set_;
};

} // namespace hammer

#endif // HAMMER_COMPILER_TOKEN_HPP