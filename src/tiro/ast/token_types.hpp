#ifndef TIRO_AST_TOKEN_TYPES_HPP
#define TIRO_AST_TOKEN_TYPES_HPP

#include "tiro/ast/fwd.hpp"
#include "tiro/ast/token.hpp"
#include "tiro/core/defs.hpp"

namespace tiro {

/// A set of token types, implemented as an efficient bit set.
class TokenTypes final {
public:
    class const_iterator;

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

    /// Returns a set that contains every token type.
    static TokenTypes all() {
        TokenTypes tt;
        tt.set_.set();
        return tt;
    }

    /// Returns an iterator to the first token type. Returns `end()` if this set is empty.
    inline const_iterator begin() const;

    /// Returns the past-the-end iterator.
    inline const_iterator end() const;

    /// Returns true iff `type` is a member of this set.
    bool contains(TokenType type) const { return set_.test(to_underlying(type)); }

    /// Inserts `type` into the set.
    void insert(TokenType type) { set_.set(to_underlying(type)); }

    /// Removes `type` from the set.
    void remove(TokenType type) { set_.reset(to_underlying(type)); }

    /// Returns the number of token types in this set.
    size_t size() const { return set_.count(); }

    /// Returns true iff `size() == 0`.
    bool empty() const { return !set_.any(); }

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

    /// Formats the token set as a string.
    std::string to_string() const;

    bool operator==(const TokenTypes& other) const { return set_ == other.set_; }
    bool operator!=(const TokenTypes& other) const { return set_ != other.set_; }

private:
    static constexpr auto enum_values = to_underlying(TokenType::MaxEnumValue) + 1;

    using bitset_type = std::bitset<enum_values>;

private:
    // Find the index of the first set bit, starting from the given index.
    // Returns enum_values (i.e. set_.size()) if none was found.
    size_t find_first_from(size_t index) const {
        // set_.find_first(...) would be nice to have. An efficient version would
        // use ffs instructions. We would need our own bitset implementation which is
        // not really needed right now.
        TIRO_DEBUG_ASSERT(index <= enum_values, "Invalid index.");
        for (; index < enum_values; ++index) {
            if (set_.test(index))
                break;
        }
        return index;
    }

private:
    bitset_type set_;
};

class TokenTypes::const_iterator {
public:
    using value_type = TokenType;
    using reference = TokenType;
    using pointer = const TokenType*;
    using difference_type = size_t;
    using iterator_category = std::input_iterator_tag;

public:
    const_iterator() = default;

    const_iterator& operator++() {
        TIRO_DEBUG_ASSERT(tts, "Invalid iterator instance.");
        TIRO_DEBUG_ASSERT(index < tts->set_.size(), "Cannot increment the past-the-end iterator.");
        index = tts->find_first_from(index + 1);
        return *this;
    }

    const_iterator operator++(int) {
        const_iterator old(*this);
        operator++();
        return old;
    }

    TokenType operator*() const {
        TIRO_DEBUG_ASSERT(tts, "Invalid iterator instance.");
        TIRO_DEBUG_ASSERT(
            index < tts->set_.size(), "Cannot dereference the past-the-end iterator.");
        return static_cast<TokenType>(index);
    }

    bool operator==(const const_iterator& other) const {
        TIRO_DEBUG_ASSERT(tts == other.tts, "Comparing iterators from different sets.");
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

TokenTypes::const_iterator TokenTypes::begin() const {
    return const_iterator(this, find_first_from(0));
}

/// Returns the past-the-end iterator.
TokenTypes::const_iterator TokenTypes::end() const {
    return const_iterator(this, enum_values);
}

} // namespace tiro

#endif // TIRO_AST_TOKEN_TYPES_HPP
