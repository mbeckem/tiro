#include "hammer/compiler/output.hpp"

#include "hammer/compiler/opcodes.hpp"
#include "hammer/core/overloaded.hpp"

namespace hammer {

CompiledOutput::~CompiledOutput() {}

template<typename Func>
void for_each_line(std::string_view str, Func&& fn) {
    size_t begin = 0;
    size_t end = str.size();
    while (begin != end) {
        size_t line = str.find("\n", begin);
        if (line == std::string_view::npos) {
            line = end;
        } else {
            line++; // Include newline
        }
        fn(str.substr(begin, line - begin));

        begin = line;
    }
}

// Temporary. TODO: Move disassemble routine here and add indent support for everything.
static std::string add_indent(std::string_view str, size_t n) {
    const std::string indent(n, ' ');

    std::string buf;
    for_each_line(str, [&](std::string_view line) {
        buf += indent;
        buf += line;
        if (buf.back() != '\n')
            buf.push_back('\n');
    });

    return buf;
}

static std::string_view
fmt_str(InternedString str, const StringTable& strings) {
    return str.valid() ? strings.value(str) : "<UNNAMED>";
}

std::string dump(const CompiledFunction& fn, const StringTable& strings) {
    fmt::memory_buffer buf;
    fmt::format_to(buf, "Function {}:\n", fmt_str(fn.name, strings));
    fmt::format_to(buf,
        "  Params: {}\n"
        "  Locals: {}\n",
        fn.params, fn.locals);

    if (fn.literals.size() > 0) {
        fmt::format_to(buf, "  Literals:\n");
        for (size_t i = 0; i < fn.literals.size(); ++i) {
            // FIXME
            // fmt::format_to(buf, "    {}: {}\n", i, dump(fn.constants[i], strings));
        }
    }

    const std::string dis = disassemble_instructions(fn.code);

    fmt::format_to(buf, "  Code:\n{}\n", add_indent(dis, 4));

    // TODO no labels yet
    return to_string(buf);
}

std::string dump(const CompiledModule& mod, const StringTable& strings) {
    fmt::memory_buffer buf;
    fmt::format_to(buf, "Module:\n");
    fmt::format_to(buf, "  Name: {}\n", fmt_str(mod.name, strings));

    if (mod.members.size() > 0) {
        fmt::format_to(buf, "  Members:\n");
        for (auto& member_ptr : mod.members) {
            Overloaded visitor = {[&](const CompiledFunction& fn) {
                                      fmt::format_to(
                                          buf, "{}\n", dump(fn, strings));
                                  },
                [&](const auto&) {
                    HAMMER_ERROR(
                        "Dump not implemented for this type."); // FIXME
                }};

            visit_output(*member_ptr, visitor);
        }
    }

    return to_string(buf);
}

} // namespace hammer
