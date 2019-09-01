#ifndef HAMMER_COMPILER_OUTPUT_HPP
#define HAMMER_COMPILER_OUTPUT_HPP

#include "hammer/compiler/string_table.hpp"
#include "hammer/core/casting.hpp"
#include "hammer/core/defs.hpp"
#include "hammer/core/hash.hpp"

#include <vector>

namespace hammer {

#define HAMMER_COMPILER_OUTPUT_TYPES(X) \
    X(CompiledInteger)                  \
    X(CompiledFloat)                    \
    X(CompiledString)                   \
    X(CompiledSymbol)                   \
    X(CompiledImport)                   \
    X(CompiledFunction)                 \
    X(CompiledModule)

#define HAMMER_DECLARE_TYPE(X) class X;
HAMMER_COMPILER_OUTPUT_TYPES(HAMMER_DECLARE_TYPE)
#undef HAMMER_DECLARE_TYPE

enum class OutputKind : i8 {
#define HAMMER_DECLARE_KIND(Type) Type,
    HAMMER_COMPILER_OUTPUT_TYPES(HAMMER_DECLARE_KIND)
#undef HAMMER_DECLARE_KIND
};

template<typename Type>
struct OutputTypeToKind : undefined_type {};

template<OutputKind kind>
struct OutputKindToType : undefined_type {};

#define HAMMER_DECLARE_MAPPINGS(Type)                        \
    template<>                                               \
    struct OutputTypeToKind<Type> {                          \
        static constexpr OutputKind kind = OutputKind::Type; \
    };                                                       \
                                                             \
    template<>                                               \
    struct OutputKindToType<OutputKind::Type> {              \
        using type = Type;                                   \
    };

HAMMER_COMPILER_OUTPUT_TYPES(HAMMER_DECLARE_MAPPINGS)

#undef HAMMER_DECLARE_MAPPINGS

/*
 * TODO output should not use the string table?
 */

class CompiledOutput {
public:
    virtual ~CompiledOutput();

    OutputKind kind() const noexcept { return kind_; }

    virtual bool equals(const CompiledOutput* o) const noexcept = 0;
    virtual size_t hash() const noexcept = 0;

    CompiledOutput& operator=(const CompiledOutput&) = delete;

protected:
    explicit CompiledOutput(OutputKind kind)
        : kind_(kind) {}

    CompiledOutput(const CompiledOutput& other) = default;
    CompiledOutput(CompiledOutput&& other) = default;

private:
    const OutputKind kind_;
};

template<typename Target>
struct InstanceTestTraits<Target,
    std::enable_if_t<std::is_base_of_v<CompiledOutput, Target>>> {
    static constexpr bool is_instance(const CompiledOutput* out) {
        HAMMER_ASSERT_NOT_NULL(out);
        // Simple case, there is no further inheritance.
        return out->kind() == OutputTypeToKind<Target>::kind;
    }
};

class CompiledInteger : public CompiledOutput {
public:
    CompiledInteger()
        : CompiledOutput(OutputKind::CompiledInteger) {}

    CompiledInteger(i64 v)
        : CompiledInteger() {
        value = v;
    }

    bool equals(const CompiledOutput* o) const noexcept override {
        if (auto that = try_cast<CompiledInteger>(o))
            return value == that->value;
        return false;
    }

    size_t hash() const noexcept override {
        return std::hash<decltype(value)>()(value);
    }

    i64 value = 0;
};

class CompiledFloat : public CompiledOutput {
public:
    CompiledFloat()
        : CompiledOutput(OutputKind::CompiledFloat) {}

    CompiledFloat(double v)
        : CompiledFloat() {
        value = v;
    }

    bool equals(const CompiledOutput* o) const noexcept override {
        if (auto that = try_cast<CompiledFloat>(o))
            return value == that->value;
        return false;
    }

    size_t hash() const noexcept override {
        return std::hash<decltype(value)>()(value);
    }

    double value = 0;
};

class CompiledString : public CompiledOutput {
public:
    CompiledString()
        : CompiledOutput(OutputKind::CompiledString) {}

    CompiledString(InternedString v)
        : CompiledString() {
        value = v;
    }

    bool equals(const CompiledOutput* o) const noexcept override {
        if (auto that = try_cast<CompiledString>(o))
            return value == that->value;
        return false;
    }

    size_t hash() const noexcept override {
        return std::hash<decltype(value)>()(value);
    }

    InternedString value;
};

class CompiledSymbol : public CompiledOutput {
public:
    CompiledSymbol()
        : CompiledOutput(OutputKind::CompiledSymbol) {}

    CompiledSymbol(InternedString v)
        : CompiledSymbol() {
        value = v;
    }

    bool equals(const CompiledOutput* o) const noexcept override {
        if (auto that = try_cast<CompiledSymbol>(o))
            return value == that->value;
        return false;
    }

    size_t hash() const noexcept override {
        return std::hash<decltype(value)>()(value);
    }

    InternedString value;
};

class CompiledImport : public CompiledOutput {
public:
    CompiledImport()
        : CompiledOutput(OutputKind::CompiledImport) {}

    CompiledImport(InternedString v)
        : CompiledImport() {
        value = v;
    }

    bool equals(const CompiledOutput* o) const noexcept override {
        if (auto that = try_cast<CompiledImport>(o))
            return value == that->value;
        return false;
    }

    size_t hash() const noexcept override {
        return std::hash<decltype(value)>()(value);
    }

    InternedString value;
};

class CompiledFunction : public CompiledOutput {
public:
    CompiledFunction()
        : CompiledOutput(OutputKind::CompiledFunction) {}

    bool equals(const CompiledOutput* o) const noexcept override {
        // Reference semantics.
        return this == o;
    }

    size_t hash() const noexcept override {
        return std::hash<const void*>()(this);
    }

    // Can be empty for anonymous functions.
    InternedString name;

    // Number of formal parameters.
    // TODO: Support for varargs, keyword args
    u32 params = 0;

    // Number of local variables required for the function's stack frame.
    u32 locals = 0;

    // Constants required by the function. These can be referenced from the
    // bytecode (via index).
    // TODO: Share this in the module between all functions?
    std::vector<std::unique_ptr<CompiledOutput>> literals;

    // Compiled bytecode.
    std::vector<byte> code;

    // (string, offset) pairs into the code. Offset refers to the byte offset
    // of an instruction.
    std::vector<std::pair<std::string, u32>> labels;
};

class CompiledModule : public CompiledOutput {
public:
    CompiledModule()
        : CompiledOutput(OutputKind::CompiledModule) {}

    bool equals(const CompiledOutput* o) const noexcept override {
        // Reference semantics.
        return this == o;
    }

    size_t hash() const noexcept override {
        return std::hash<const void*>()(this);
    }

    InternedString name;
    std::vector<std::unique_ptr<CompiledOutput>> members;
};

// FIXME Serialization to string
std::string dump(const CompiledFunction& fn, const StringTable& strings);
std::string dump(const CompiledModule& mod, const StringTable& strings);

template<typename Output, typename Visitor>
HAMMER_FORCE_INLINE decltype(auto) visit_output(Output&& out, Visitor&& v) {
    switch (out.kind()) {
#define HAMMER_CASE(Type) \
case OutputKind::Type:    \
    return std::forward<Visitor>(v)(*must_cast<Type>(&out));

        HAMMER_COMPILER_OUTPUT_TYPES(HAMMER_CASE)

#undef HAMMER_CASE
    }
}

} // namespace hammer

#endif // HAMMER_COMPILER_OUTPUT_HPP
