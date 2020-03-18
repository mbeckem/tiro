#ifndef TIRO_COMPILER_OUTPUT_HPP
#define TIRO_COMPILER_OUTPUT_HPP

#include "tiro/core/defs.hpp"
#include "tiro/core/hash.hpp"
#include "tiro/core/string_table.hpp"

#include <vector>

namespace tiro {

class FunctionDescriptor;

class ModuleItem final {
public:
// Format: Enum name (same as type name), variable name, accessor name.
#define TIRO_MODULE_ITEMS(X)         \
    X(Integer, int_, get_integer)    \
    X(Float, float_, get_float)      \
    X(String, str_, get_string)      \
    X(Symbol, sym_, get_symbol)      \
    X(Variable, var_, get_variable)  \
    X(Function, func_, get_function) \
    X(Import, import_, get_import)

    enum class Which : u8 {
#define TIRO_MODULE_ENUM(Name, _v, _a) Name,
        TIRO_MODULE_ITEMS(TIRO_MODULE_ENUM)
#undef TIRO_MODULE_ENUM
    };

    struct Integer {
        i64 value = 0;

        Integer() = default;
        Integer(i64 v)
            : value(v) {}

        bool operator==(const Integer& other) const noexcept {
            return value == other.value;
        }

        bool operator!=(const Integer& other) const noexcept {
            return !(*this == other);
        }

        void build_hash(Hasher& h) const noexcept { h.append(value); }
    };

    struct Float {
        f64 value = 0;

        Float() = default;
        Float(f64 v)
            : value(v) {}

        bool operator==(const Float& other) const noexcept {
            return value == other.value;
        }

        bool operator!=(const Float& other) const noexcept {
            return !(*this == other);
        }

        void build_hash(Hasher& h) const noexcept { h.append(value); }
    };

    struct String {
        InternedString value;

        String() = default;
        String(InternedString s)
            : value(s) {}

        bool operator==(const String& other) const noexcept {
            return value == other.value;
        }

        bool operator!=(const String& other) const noexcept {
            return !(*this == other);
        }

        void build_hash(Hasher& h) const noexcept { value.build_hash(h); }
    };

    struct Symbol {
        // Refers to a string previously added to the set of items
        u32 string_index = 0;

        Symbol() = default;
        Symbol(u32 i)
            : string_index(i) {}

        bool operator==(const Symbol& other) const noexcept {
            return string_index == other.string_index;
        }

        bool operator!=(const Symbol& other) const noexcept {
            return !(*this == other);
        }

        void build_hash(Hasher& h) const noexcept { h.append(string_index); }
    };

    struct Function {
        std::unique_ptr<FunctionDescriptor> value;

        Function() = default;
        Function(std::unique_ptr<FunctionDescriptor> v)
            : value(std::move(v)) {}

        bool operator==(const Function& other) const noexcept {
            return value.get() == other.value.get();
        }

        bool operator!=(const Function& other) const noexcept {
            return !(*this == other);
        }

        void build_hash(Hasher& h) const noexcept {
            h.append((void*) value.get());
        }
    };

    struct Variable {
        // TODO Debug info? Just make this a counter?

        bool operator==([[maybe_unused]] const Variable& other) const noexcept {
            return false;
        }

        bool operator!=([[maybe_unused]] const Variable& other) const noexcept {
            return true;
        }

        void build_hash(Hasher& h) const noexcept {}
    };

    struct Import {
        // Refers to a string previously added to the set of items.
        // TODO import members of other modules.
        u32 string_index = 0;

        Import() = default;
        Import(u32 i)
            : string_index(i) {}

        bool operator==(const Import& other) const noexcept {
            return string_index == other.string_index;
        }

        bool operator!=(const Import& other) const noexcept {
            return !(*this == other);
        }

        void build_hash(Hasher& h) const noexcept { h.append(string_index); }
    };

public:
    static ModuleItem make_integer(i64 value);
    static ModuleItem make_float(f64 value);
    static ModuleItem make_string(InternedString value);
    static ModuleItem make_symbol(u32 string_index);
    static ModuleItem make_variable(); // TODO: Constant values for direct init
    static ModuleItem make_func(std::unique_ptr<FunctionDescriptor> func);
    static ModuleItem make_import(u32 string_index);

// Defines an implicit conversion constructor for every module item type above.
#define TIRO_MODULE_CONVERSION(Type, _v, _a) \
    /* implicit */ ModuleItem(Type v) { construct(std::move(v)); }

    TIRO_MODULE_ITEMS(TIRO_MODULE_CONVERSION)

#undef TIRO_MODULE_CONVERSION

    ~ModuleItem();

    ModuleItem(ModuleItem&& other) noexcept;
    ModuleItem& operator=(ModuleItem&& other) noexcept;

    Which which() const noexcept { return which_; }

#define TIRO_ACCESSOR(Type, var, accessor)                          \
    const Type& accessor() const {                                  \
        TIRO_ASSERT(which_ == Which::Type, "Invalid type access."); \
        return var;                                                 \
    }                                                               \
                                                                    \
    Type& accessor() { return const_cast<Type&>(const_ptr(this)->accessor()); }

    TIRO_MODULE_ITEMS(TIRO_ACCESSOR)

#undef TIRO_ACCESSOR

    bool operator==(const ModuleItem& other) const noexcept;
    bool operator!=(const ModuleItem& other) const noexcept {
        return !(*this == other);
    }

    void build_hash(Hasher& h) const noexcept;

private:
    ModuleItem() {}

    // pre: no active object
    void construct(Integer i);
    void construct(Float f);
    void construct(String s);
    void construct(Symbol s);
    void construct(Variable v);
    void construct(Function f);
    void construct(Import i);

    // pre: no active object
    void move(ModuleItem&& other);

    // pre: valid active object
    void destroy() noexcept;

    template<typename Item, typename Visitor>
    friend decltype(auto) visit_impl(Item&& item, Visitor&& visitor) {
        switch (item.which()) {
#define TIRO_VISIT(type, var, accessor) \
case Which::type:                       \
    return std::forward<Visitor>(visitor)(item.accessor());

            TIRO_MODULE_ITEMS(TIRO_VISIT)

#undef TIRO_VISIT
        }

        TIRO_UNREACHABLE("Invalid module item type.");
    }

private:
    Which which_;
    union {
#define TIRO_DEFINE_MEMBER(Type, var, _a) Type var;
        TIRO_MODULE_ITEMS(TIRO_DEFINE_MEMBER)
#undef TIRO_DEFINE_MEMBER
    };
};

std::string_view to_string(ModuleItem::Which which);

template<typename Visitor>
decltype(auto) visit(const ModuleItem& item, Visitor&& visitor) {
    return visit_impl(item, std::forward<Visitor>(visitor));
}

template<typename Visitor>
decltype(auto) visit(ModuleItem& item, Visitor&& visitor) {
    return visit_impl(item, std::forward<Visitor>(visitor));
}

class FunctionDescriptor final {
public:
    enum Type {
        FUNCTION,
        TEMPLATE,
    };

    FunctionDescriptor(Type type_)
        : type(type_) {}

    // The type of this function
    Type type;

    // Can be empty for anonymous functions.
    InternedString name;

    // Number of formal parameters.
    // TODO: Support for varargs, keyword args
    u32 params = 0;

    // Number of local variables required for the function's stack frame.
    u32 locals = 0;

    // Compiled bytecode.
    std::vector<byte> code;

    // (string, offset) pairs into the code. Offset refers to the byte offset
    // of an instruction.
    // TODO: Not generated yet.
    std::vector<std::pair<std::string, u32>> labels;
};

std::string_view to_string(FunctionDescriptor::Type type);

class CompiledModule final {
public:
    CompiledModule() = default;

    InternedString name;
    std::vector<ModuleItem> members;
    std::optional<u32> init; // Index of init function, if present.
};

// Serialization to string
std::string
disassemble_module(const CompiledModule& mod, const StringTable& strings);

} // namespace tiro

TIRO_ENABLE_BUILD_HASH(::tiro::ModuleItem)
TIRO_ENABLE_BUILD_HASH(::tiro::ModuleItem::Integer)
TIRO_ENABLE_BUILD_HASH(::tiro::ModuleItem::Float)
TIRO_ENABLE_BUILD_HASH(::tiro::ModuleItem::String)
TIRO_ENABLE_BUILD_HASH(::tiro::ModuleItem::Symbol)
TIRO_ENABLE_BUILD_HASH(::tiro::ModuleItem::Variable)
TIRO_ENABLE_BUILD_HASH(::tiro::ModuleItem::Function)
TIRO_ENABLE_BUILD_HASH(::tiro::ModuleItem::Import)

#endif // TIRO_COMPILER_OUTPUT_HPP
