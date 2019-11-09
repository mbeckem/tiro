#ifndef HAMMER_VM_OBJECTS_OBJECT_HPP
#define HAMMER_VM_OBJECTS_OBJECT_HPP

#include "hammer/core/defs.hpp"
#include "hammer/core/math.hpp"
#include "hammer/core/span.hpp"
#include "hammer/core/type_traits.hpp"
#include "hammer/vm/handles.hpp"
#include "hammer/vm/objects/value.hpp"

#include <new>
#include <string_view>

namespace hammer::vm {

class Context;

/**
 * Represents the null value. All null values have the same representation Value::null().
 */
class Null final : public Value {
public:
    static Null make(Context& ctx);

    Null() = default;

    explicit Null(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is_null(), "Value is not null.");
    }

    size_t object_size() const noexcept { return 0; }

    template<typename W>
    void walk(W&&) {}
};

/**
 * Instances of Undefined are used as a sentinel value for uninitialized values.
 * They are never leaked into user code. Accesses that generate an undefined value
 * produce an error instead.
 * 
 * There is only one instance for each context.
 */
class Undefined final : public Value {
public:
    static Undefined make(Context& ctx);

    Undefined() = default;

    explicit Undefined(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<Undefined>(), "Value is not undefined.");
    }

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&&);

private:
    struct Data;
};

/**
 * Instances represent the boolean "true" or "false.
 * 
 * There is only one instance for each context.
 */
class Boolean final : public Value {
public:
    static Boolean make(Context& ctx, bool value);

    Boolean() = default;

    explicit Boolean(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<Boolean>(), "Value is not a boolean.");
    }

    bool value() const noexcept;
    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&&);

private:
    struct Data;
};

/**
 * Represents a heap-allocated 64-bit integer value.
 * 
 * TODO: Small integer optimization.
 */
class Integer final : public Value {
public:
    static Integer make(Context& ctx, i64 value);

    Integer() = default;

    explicit Integer(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<Integer>(), "Value is not an integer.");
    }

    i64 value() const noexcept;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&&);

private:
    struct Data;
};

/**
 * Represents a heap-allocated 64-bit floating point value.
 */
class Float final : public Value {
public:
    static Float make(Context& ctx, f64 value);

    Float() = default;

    explicit Float(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<Float>(), "Value is not a float.");
    }

    f64 value() const noexcept;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&&);

private:
    struct Data;
};

/**
 * Represents an internal value whose only relevant
 * property is its unique identity.
 * 
 * TODO: Maybe reuse symbols for this once we have them.
 */
class SpecialValue final : public Value {
public:
    static SpecialValue make(Context& ctx, std::string_view name);

    SpecialValue() = default;

    explicit SpecialValue(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<SpecialValue>(), "Value is not a special value.");
    }

    std::string_view name() const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;

    inline Data* access_heap() const;
};

/**
 * Represents a module, which is a collection of exported and private members.
 */
class Module final : public Value {
public:
    static Module
    make(Context& ctx, Handle<String> name, Handle<Tuple> members);

    Module() = default;

    explicit Module(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<Module>(), "Value is not a module.");
    }

    String name() const noexcept;
    Tuple members() const noexcept;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;
};

/**
 * A tuple is a sequence of values allocated in a contiguous block on the heap
 * that does not change its size.
 */
class Tuple final : public Value {
public:
    static Tuple make(Context& ctx, size_t size);

    static Tuple
    make(Context& ctx, /* FIXME must be rooted */ Span<const Value> values);

    // total_size must be greater than values.size()
    static Tuple make(Context& ctx,
        /* FIXME must be rooted */ Span<const Value> values, size_t total_size);

    static Tuple
    make(Context& ctx, std::initializer_list<Handle<Value>> values);

public:
    Tuple() = default;

    explicit Tuple(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<Tuple>(), "Value is not a tuple array.");
    }

    const Value* data() const;
    size_t size() const;
    Span<const Value> values() const { return {data(), size()}; }

    Value get(size_t index) const;
    void set(WriteBarrier, size_t index, Value value) const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;

    template<typename Init>
    static Tuple make_impl(Context& ctx, size_t total_size, Init&& init);

    inline Data* access_heap() const;
};

// Will be used to implement write barriers in the future.
#define HAMMER_WRITE_MEMBER(ctx, obj, member, new_value) \
    (obj).member((ctx).write_barrier(), (new_value))

#define HAMMER_WRITE_INDEX(ctx, obj, index, new_value) \
    (obj).set((ctx).write_barrier(), (index), (new_value))

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_OBJECT_HPP
