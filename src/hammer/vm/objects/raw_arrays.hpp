#ifndef HAMMER_VM_OBJECTS_RAW_ARRAYS_HPP
#define HAMMER_VM_OBJECTS_RAW_ARRAYS_HPP

#include "hammer/vm/context.hpp"

namespace hammer::vm {

/** 
 * Array base class for raw data values.
 * DataType MUST NOT contain references to any objects,
 * it must be equivalent to a blob of bytes (i.e. integers, structs, etc.).
 */
template<typename DataType, typename Derived>
class RawArrayBase : public Value {
    static constexpr ValueType concrete_type_id =
        MapTypeToValueType<Derived>::type;

    static_assert(std::is_trivially_copyable_v<DataType>);
    static_assert(std::is_trivially_destructible_v<DataType>);

public:
    using data_type = DataType;

    static Derived make(Context& ctx, size_t size, DataType default_value) {
        return make_impl(ctx, size, [&](Data* data) {
            std::uninitialized_fill_n(data->values, size, default_value);
        });
    }

    static Derived make(Context& ctx, Span<const DataType> content,
        size_t total_size, DataType default_value) {
        HAMMER_ASSERT(
            total_size >= content.size(), "Invalid size of initial content.");
        return make_impl(ctx, total_size, [&](Data* data) {
            std::memcpy(data->values, content.data(), content.size());
            std::uninitialized_fill_n(data->values + content.size(),
                total_size - content.size(), default_value);
        });
    }

    RawArrayBase() = default;

    explicit RawArrayBase(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<Derived>(), "Value is not an array.");
    }

    size_t size() const { return access_heap()->size; }
    DataType* data() const { return access_heap()->values; }
    Span<DataType> values() const { return {data(), size()}; }

    size_t object_size() const noexcept {
        return sizeof(Data) + size() * sizeof(DataType);
    }

    // does nothing, no references
    template<typename W>
    void walk(W&&) {}

private:
    struct Data : Header {
        Data(size_t size_)
            : Header(concrete_type_id)
            , size(size_) {}

        size_t size;
        DataType values[];
    };

    Data* access_heap() const { return Value::access_heap<Data>(); }

    template<typename Init>
    static Derived make_impl(Context& ctx, size_t total_size, Init&& init) {
        size_t allocation_size = variable_allocation<Data, DataType>(
            total_size);
        Data* data = ctx.heap().create_varsize<Data>(
            allocation_size, total_size);
        init(data);
        return Derived(Value::from_heap(data));
    }
};

#define HAMMER_RAW_ARRAY_TYPE(Name, DataType)                \
    class Name final : public RawArrayBase<DataType, Name> { \
    public:                                                  \
        using RawArrayBase::RawArrayBase;                    \
    };

HAMMER_RAW_ARRAY_TYPE(U8Array, u8)
HAMMER_RAW_ARRAY_TYPE(U16Array, u16)
HAMMER_RAW_ARRAY_TYPE(U32Array, u32)
HAMMER_RAW_ARRAY_TYPE(U64Array, u64)

HAMMER_RAW_ARRAY_TYPE(I8Array, i8)
HAMMER_RAW_ARRAY_TYPE(I16Array, i16)
HAMMER_RAW_ARRAY_TYPE(I32Array, i32)
HAMMER_RAW_ARRAY_TYPE(I64Array, i64)

HAMMER_RAW_ARRAY_TYPE(F32Array, f32)
HAMMER_RAW_ARRAY_TYPE(F64Array, f64)

#undef HAMMER_RAW_ARRAY_TYPE

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_RAW_ARRAYS_HPP
