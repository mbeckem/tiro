#ifndef HAMMER_VM_OBJECTS_RAW_ARRAYS_HPP
#define HAMMER_VM_OBJECTS_RAW_ARRAYS_HPP

#include "hammer/vm/context.hpp"

namespace hammer::vm {

/** 
 * Array baseclass for raw data values.
 * DataType MUST NOT contain references to any objects,
 * it must be equivalent to a blob of bytes (i.e. integers, structs, etc.).
 */
template<typename DataType, typename Derived>
class RawArrayBase : public Value {
    static constexpr ValueType concrete_type_id =
        MapTypeToValueType<Derived>::type;

public:
    static Derived make(Context& ctx, size_t size, DataType default_value) {
        size_t allocation_size = variable_allocation<Data, DataType>(size);
        Data* data = ctx.heap().create_varsize<Data>(allocation_size, size);
        std::uninitialized_fill_n(data->values, size, default_value);
        return Derived(Value::from_heap(data));
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
};

class U8Array final : public RawArrayBase<u8, U8Array> {
public:
    using RawArrayBase::RawArrayBase;
};

class U16Array final : public RawArrayBase<u16, U16Array> {
public:
    using RawArrayBase::RawArrayBase;
};

class U32Array final : public RawArrayBase<u32, U32Array> {
public:
    using RawArrayBase::RawArrayBase;
};

class U64Array final : public RawArrayBase<u64, U64Array> {
public:
    using RawArrayBase::RawArrayBase;
};

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_RAW_ARRAYS_HPP
