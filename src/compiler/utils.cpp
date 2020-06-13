#include "compiler/utils.hpp"

namespace tiro {

static constexpr char hex_chars[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

static_assert(sizeof(hex_chars) == 16);

static void append_hex(byte c, std::string& output) {
    output += hex_chars[(c >> 4) & 0x0f];
    output += hex_chars[c & 0x0f];
}

std::string escape_string(std::string_view input) {
    std::string output;
    output.reserve(input.size());

    for (char c : input) {
        // Should keep the list of escape characters in sync with the lexer.
        switch (c) {
        case '\n':
            output += "\\n";
            continue;
        case '\r':
            output += "\\r";
            continue;
        case '\t':
            output += "\\t";
            continue;
        case '\"':
            output += "\\\"";
            continue;
        case '\'':
            output += "\\'";
            continue;
        case '\\':
            output += "\\\\";
            continue;
        case '$':
            output += "\\$";
            continue;
        }

        if (c < 32 || c >= 127) {
            output += "\\x";
            append_hex(byte(c), output);
        } else {
            output += c;
        }
    }

    return output;
}

std::string format_tree(const StringTree& tree) {
    struct Printer {
        fmt::memory_buffer buf_;

        // Offsets at which vertical lines are printed.
        std::vector<size_t> seps_;

        void print(const StringTree& tree) {
            print_line(tree.line, 0, false);
            print_children(tree, 1);
        }

        void print_children(const StringTree& tree, size_t depth) {
            const size_t count = tree.children.size();
            if (count == 0)
                return;

            seps_.push_back(depth - 1);
            for (size_t i = 0; i < count - 1; ++i) {
                print_line(tree.children[i].line, depth, false);
                print_children(tree.children[i], depth + 1);
            }

            print_line(tree.children[count - 1].line, depth, true);
            seps_.pop_back();
            print_children(tree.children[count - 1], depth + 1);
        }

        void print_line(std::string_view line, size_t depth, bool last_child) {
            std::string prefix;

            auto next_sep = seps_.begin();
            const auto last_sep = seps_.end();
            for (size_t i = 0; i < depth; ++i) {
                std::string_view branch = " ";
                std::string_view space = " ";

                const bool print_sep = next_sep != last_sep && *next_sep == i;
                if (print_sep) {
                    ++next_sep;

                    if (i == depth - 1) {
                        if (last_child) {
                            branch = u8"└";
                        } else {
                            branch = u8"├";
                        }
                        space = u8"─";
                    } else {
                        branch = "│";
                    }
                }

                prefix += branch;
                prefix += space;
            }
            TIRO_DEBUG_ASSERT(next_sep == last_sep, "Did not reach the last line.");

            fmt::format_to(buf_, "{}{}\n", prefix, line);
        }
    };

    Printer printer;
    printer.print(tree);
    return fmt::to_string(printer.buf_);
}

} // namespace tiro
