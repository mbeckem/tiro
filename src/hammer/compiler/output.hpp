#ifndef HAMMER_COMPILER_OUTPUT_HPP
#define HAMMER_COMPILER_OUTPUT_HPP

#include "hammer/compiler/string_table.hpp"
#include "hammer/core/casting.hpp"
#include "hammer/core/defs.hpp"
#include "hammer/core/hash.hpp"

#include <vector>

namespace hammer {

class FunctionDescriptor;

class ModuleItem final {
public:
// Format: Enum name (same as type name), variable name, accessor name.
#define HAMMER_MODULE_ITEMS(X)       \
    X(Integer, int_, get_integer)    \
    X(Float, float_, get_float)      \
    X(String, str_, get_string)      \
    X(Symbol, sym_, get_symbol)      \
    X(Function, func_, get_function) \
    X(Import, import_, get_import)

    enum class Which : u8 {
#define HAMMER_MODULE_ENUM(Name, _v, _a) Name,
        HAMMER_MODULE_ITEMS(HAMMER_MODULE_ENUM)
#undef HAMMER_MODULE_ENUM
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
    static ModuleItem make_func(std::unique_ptr<FunctionDescriptor> func);
    static ModuleItem make_import(u32 string_index);

// Defines an implicit conversion constructor for every module item type above.
#define HAMMER_MODULE_CONVERSION(Type, _v, _a) \
    /* implicit */ ModuleItem(Type v) { construct(std::move(v)); }

    HAMMER_MODULE_ITEMS(HAMMER_MODULE_CONVERSION)

#undef HAMMER_MODULE_CONVERSION

    ~ModuleItem();

    ModuleItem(ModuleItem&& other) noexcept;
    ModuleItem& operator=(ModuleItem&& other) noexcept;

    Which which() const noexcept { return which_; }

#define HAMMER_ACCESSOR(Type, var, accessor)                          \
    const Type& accessor() const {                                    \
        HAMMER_ASSERT(which_ == Which::Type, "Invalid type access."); \
        return var;                                                   \
    }                                                                 \
                                                                      \
    Type& accessor() { return const_cast<Type&>(const_ptr(this)->accessor()); }

    HAMMER_MODULE_ITEMS(HAMMER_ACCESSOR)

#undef HAMMER_ACCESSOR

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
    void construct(Function f);
    void construct(Import i);

    // pre: no active object
    void move(ModuleItem&& other);

    // pre: valid active object
    void destroy() noexcept;

    template<typename Item, typename Visitor>
    friend decltype(auto) visit_impl(Item&& item, Visitor&& visitor) {
        switch (item.which()) {
#define HAMMER_VISIT(type, var, accessor) \
case Which::type:                         \
    return std::forward<Visitor>(visitor)(item.accessor());

            HAMMER_MODULE_ITEMS(HAMMER_VISIT)

#undef HAMMER_VISIT
        }

        HAMMER_UNREACHABLE("Invalid module item type.");
    }

private:
    Which which_;
    union {
#define HAMMER_DEFINE_MEMBER(Type, var, _a) Type var;
        HAMMER_MODULE_ITEMS(HAMMER_DEFINE_MEMBER)
#undef HAMMER_DEFINE_MEMBER
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
};

// Serialization to string
std::string dump(const CompiledModule& mod, const StringTable& strings);

} // namespace hammer

#endif // HAMMER_COMPILER_OUTPUT_HPP
