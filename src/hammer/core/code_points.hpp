#ifndef HAMMER_CORE_CODE_POINTS_HPP
#define HAMMER_CORE_CODE_POINTS_HPP

#include "hammer/core/defs.hpp"
#include "hammer/core/unicode.hpp"

#include <optional>

namespace hammer {

// Helper class for iterating through an utf8 string.
class CodePointRange final {
public:
    struct iterator;
    struct sentinel;

    struct iterator {
        CodePointRange* instance = nullptr;

        iterator() = default;

        iterator(CodePointRange* instance_)
            : instance(instance_) {}

        bool operator==(const sentinel&) const {
            HAMMER_ASSERT(instance, "Invalid iterator.");
            return instance->at_end();
        }

        bool operator!=(const sentinel&) const {
            HAMMER_ASSERT(instance, "Invalid iterator.");
            return !instance->at_end();
        }

        CodePoint operator*() const {
            HAMMER_ASSERT(instance, "Invalid iterator.");
            return instance->get();
        }

        iterator& operator++() {
            HAMMER_ASSERT(instance, "Invalid iterator.");
            instance->advance();
            return *this;
        }
    };

    // The sentinel marks the end of the string during iteration.
    struct sentinel {};

public:
    CodePointRange(std::string_view str)
        : begin_(str.data())
        , current_(str.data())
        , next_(current_)
        , end_(str.data() + str.size()) {
        if (!at_end()) {
            decode();
        }
    }

    // Check whether we can read a code point.
    bool at_end() const { return current_ == end_; }
    explicit operator bool() const { return !at_end(); }

    // Return the current utf8 code point.
    CodePoint get() const {
        HAMMER_ASSERT(!at_end(), "Reached the end of the string.");
        return cp_;
    }
    CodePoint operator*() const { return get(); }

    // Checked access. TODO rename to get.
    std::optional<CodePoint> current() const {
        if (at_end())
            return {};
        return get();
    }

    // Advance to the next utf8 code point.
    void advance() {
        HAMMER_ASSERT(!at_end(), "Reached the end of the string.");
        current_ = next_;
        if (!at_end()) {
            decode();
        }
    }

    // Advances by "n" code points.
    void advance(size_t n) {
        for (size_t i = 0; i < n; ++i)
            advance();
    }

    CodePointRange& operator++() {
        advance();
        return *this;
    }

    // The current (byte) offset into the original string view.
    // This points to the start of the current code point (if any).
    size_t pos() const { return static_cast<size_t>(current_ - begin_); }

    // Index of the next code point (if any).
    size_t next_pos() const { return static_cast<size_t>(next_ - begin_); }

    // The width (in bytes) of the current code point (if any).
    size_t code_point_width() const {
        HAMMER_ASSERT(!at_end(), "Reached the end of the string.");
        return static_cast<size_t>(next_ - current_);
    }

    // Support for range for loop.
    iterator begin() { return iterator(this); }
    sentinel end() { return sentinel(); }

    // Jump to a specific byte offset.
    void seek(size_t pos) {
        HAMMER_ASSERT(pos <= static_cast<size_t>(end_ - begin_),
            "Position out of bounds.");

        const char* new_current = begin_ + pos;
        if (new_current == current_)
            return;

        current_ = new_current;
        if (!at_end()) {
            decode();
        }
    }

    std::optional<CodePoint> peek() const { return peek(1); }

    std::optional<CodePoint> peek(size_t n) const {
        CodePointRange p = *this;
        while (p && n > 0) {
            ++p;
            --n;
        }

        if (n == 0 && p)
            return *p;
        return {};
    }

private:
    void decode() {
        HAMMER_ASSERT(!at_end(), "Reached the end of the string.");
        std::tie(cp_, next_) = decode_utf8(current_, end_);
    }

private:
    const char* begin_ = nullptr;
    const char* current_ = nullptr;
    const char* next_ = nullptr;
    const char* end_ = nullptr;
    CodePoint cp_ = 0;
};

} // namespace hammer

#endif // HAMMER_CORE_CODE_POINTS_HPP
