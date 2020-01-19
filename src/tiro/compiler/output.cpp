#include "tiro/compiler/output.hpp"

#include "tiro/compiler/opcodes.hpp"
#include "tiro/core/overloaded.hpp"

#include <new>
#include <type_traits>

namespace tiro::compiler {

ModuleItem ModuleItem::make_integer(i64 value) {
    return Integer(value);
}

ModuleItem ModuleItem::make_float(f64 value) {
    return Float(value);
}

ModuleItem ModuleItem::make_string(InternedString value) {
    return String(value);
}

ModuleItem ModuleItem::make_symbol(u32 string_index) {
    return Symbol(string_index);
}

ModuleItem ModuleItem::make_func(std::unique_ptr<FunctionDescriptor> func) {
    return Function(std::move(func));
}

ModuleItem ModuleItem::make_import(u32 string_index) {
    return Import(string_index);
}

ModuleItem::~ModuleItem() {
    destroy();
}

ModuleItem::ModuleItem(ModuleItem&& other) noexcept {
    move(std::move(other));
}

ModuleItem& ModuleItem::operator=(ModuleItem&& other) noexcept {
    TIRO_CHECK(this != &other, "Self move assignment.");

    destroy();
    move(std::move(other));
    return *this;
}

bool ModuleItem::operator==(const ModuleItem& other) const noexcept {
    if (which_ != other.which_)
        return false;

    switch (which_) {
#define TIRO_COMPARE(Type, var, _a) \
    case Which::Type:               \
        return var == other.var;

        TIRO_MODULE_ITEMS(TIRO_COMPARE)

#undef TIRO_COMPARE
    }

    TIRO_UNREACHABLE("Invalid module item type.");
}

void ModuleItem::build_hash(Hasher& h) const noexcept {
    h.append(static_cast<std::underlying_type_t<Which>>(which_));

    visit(*this, [&](auto&& o) { o.build_hash(h); });
}

void ModuleItem::construct(Integer i) {
    int_ = i;
    which_ = Which::Integer;
}

void ModuleItem::construct(Float f) {
    float_ = f;
    which_ = Which::Float;
}

void ModuleItem::construct(String s) {
    str_ = s;
    which_ = Which::String;
}

void ModuleItem::construct(Symbol s) {
    sym_ = s;
    which_ = Which::Symbol;
}

void ModuleItem::construct(Function f) {
    new (&func_) Function(std::move(f));
    which_ = Which::Function;
}

void ModuleItem::construct(Import i) {
    import_ = i;
    which_ = Which::Import;
}

void ModuleItem::move(ModuleItem&& other) {
    visit(other, [&](auto&& o) { this->construct(std::move(o)); });
}

void ModuleItem::destroy() noexcept {
    switch (which_) {
    case Which::Integer:
    case Which::Float:
    case Which::String:
    case Which::Symbol:
    case Which::Import:
        static_assert(std::is_trivially_destructible_v<Integer>);
        static_assert(std::is_trivially_destructible_v<Float>);
        static_assert(std::is_trivially_destructible_v<String>);
        static_assert(std::is_trivially_destructible_v<Symbol>);
        static_assert(std::is_trivially_destructible_v<Import>);
        break;

    case Which::Function:
        func_.~Function();
        break;
    }
}

std::string_view to_string(ModuleItem::Which which) {
    switch (which) {
#define TIRO_TO_STR(Type, _v, _a) \
    case ModuleItem::Which::Type: \
        return #Type;

        TIRO_MODULE_ITEMS(TIRO_TO_STR)

#undef TIRO_TO_STR
    }

    TIRO_UNREACHABLE("Invalid module item type.");
}

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

std::string_view to_string(FunctionDescriptor::Type type) {
    switch (type) {
    case FunctionDescriptor::FUNCTION:
        return "FUNCTION";
    case FunctionDescriptor::TEMPLATE:
        return "TEMPLATE";
    }
    TIRO_UNREACHABLE("Invalid function type.");
}

static void dump(fmt::memory_buffer& buf,
    [[maybe_unused]] const StringTable& strings, const ModuleItem::Integer& i) {
    fmt::format_to(buf, "Integer ({})", i.value);
}

static void dump(fmt::memory_buffer& buf,
    [[maybe_unused]] const StringTable& strings, const ModuleItem::Float& f) {
    fmt::format_to(buf, "Float ({})", f.value);
}

static void dump(fmt::memory_buffer& buf, const StringTable& strings,
    const ModuleItem::String& s) {
    fmt::format_to(buf, "String ({})", fmt_str(s.value, strings));
}

static void dump(fmt::memory_buffer& buf,
    [[maybe_unused]] const StringTable& strings, const ModuleItem::Symbol& s) {
    fmt::format_to(buf, "Symbol (#{})", s.string_index);
}

static void dump(fmt::memory_buffer& buf, const StringTable& strings,
    const ModuleItem::Function& f) {
    fmt::format_to(buf, "Function (@{})", (void*) f.value.get());

    if (const FunctionDescriptor* func = f.value.get()) {
        fmt::format_to(buf, "\n");
        fmt::format_to(buf, "    type: {}\n", to_string(func->type));
        fmt::format_to(buf, "    name: {}\n", fmt_str(func->name, strings));
        fmt::format_to(buf, "    params: {}\n", func->params);
        fmt::format_to(buf, "    locals: {}\n", func->locals);
        fmt::format_to(buf, "    code:\n");

        std::string dis = disassemble_instructions(func->code);
        dis = add_indent(dis, 6);
        fmt::format_to(buf, "{}", dis);
    }
}

static void dump(fmt::memory_buffer& buf,
    [[maybe_unused]] const StringTable& strings, const ModuleItem::Import& i) {
    fmt::format_to(buf, "Import (#{})", i.string_index);
}

std::string dump(const CompiledModule& module, const StringTable& strings) {
    fmt::memory_buffer buf;

    fmt::format_to(buf, "Module: {}\n", fmt_str(module.name, strings));

    fmt::format_to(buf, "Members:\n");

    size_t index = 0;
    for (const ModuleItem& member : module.members) {
        fmt::format_to(buf, "  {}: ", index++);
        visit(member, [&](auto&& m) { dump(buf, strings, m); });
        fmt::format_to(buf, "\n");
    }

    return to_string(buf);
}

} // namespace tiro::compiler
