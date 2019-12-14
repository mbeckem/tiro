#ifndef HAMEMR_VM_OBJECTS_BUFFERS_HPP
#define HAMEMR_VM_OBJECTS_BUFFERS_HPP

#include "hammer/core/span.hpp"
#include "hammer/vm/objects/value.hpp"

namespace hammer::vm {

/** 
 * Array base class for raw data values.
 * DataType MUST NOT contain references to any objects,
 * it must be equivalent to a blob of bytes (i.e. integers, structs, etc.).
 */
template<typename DataType, typename Derived>
class BufferBase : public Value {
    static constexpr ValueType concrete_type_id =
        MapTypeToValueType<Derived>::type;

    static_assert(std::is_trivially_copyable_v<DataType>);
    static_assert(std::is_trivially_destructible_v<DataType>);

protected:
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

public:
    BufferBase() = default;

    explicit BufferBase(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<Derived>(), "Value is not a buffer.");
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
    static Derived make_impl(Context& ctx, size_t total_size, Init&& init);
};

#define HAMMER_BUFFER_TYPES(X) \
    X(U8Buffer, u8)            \
    X(U16Buffer, u16)          \
    X(U32Buffer, u32)          \
    X(U64Buffer, u64)          \
                               \
    X(I8Buffer, i8)            \
    X(I16Buffer, i16)          \
    X(I32Buffer, i32)          \
    X(I64Buffer, i64)          \
                               \
    X(F32Buffer, f32)          \
    X(F64Buffer, f64)

#define HAMMER_DECLARE_BUFFER(Name, DataType)                                \
    class Name final : public BufferBase<DataType, Name> {                   \
    public:                                                                  \
        static Name make(Context& ctx, size_t size, DataType default_value); \
                                                                             \
        static Name make(Context& ctx, Span<const DataType> content,         \
            size_t total_size, DataType default_value);                      \
                                                                             \
        using BufferBase::BufferBase;                                        \
    };

HAMMER_BUFFER_TYPES(HAMMER_DECLARE_BUFFER)
#undef HAMMER_DECLARE_BUFFER

} // namespace hammer::vm

#endif // HAMEMR_VM_OBJECTS_BUFFERS_HPP
