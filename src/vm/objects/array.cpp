#include "vm/objects/array.hpp"

#include "vm/context.ipp"
#include "vm/objects/array_storage_base.ipp"
#include "vm/objects/native_function.hpp"

#include <new>

namespace tiro::vm {

Array Array::make(Context& ctx, size_t initial_capacity) {
    Root<ArrayStorage> storage(ctx);
    if (initial_capacity > 0) {
        storage.set(ArrayStorage::make(ctx, initial_capacity));
    }

    auto type = ctx.types().internal_type<Array>();
    Layout* data = ctx.heap().create<Layout>(type, StaticSlotsInit());
    data->write_static_slot(StorageSlot, storage.get());
    return Array(from_heap(data));
}

Array Array::make(Context& ctx, Span<const Value> initial_content) {
    if (initial_content.empty())
        return make(ctx, 0);

    Root<ArrayStorage> storage(
        ctx, ArrayStorage::make(ctx, initial_content, initial_content.size()));

    auto type = ctx.types().internal_type<Array>();
    Layout* data = ctx.heap().create<Layout>(type, StaticSlotsInit());
    data->write_static_slot(StorageSlot, storage.get());
    return Array(from_heap(data));
}

size_t Array::size() {
    auto storage = get_storage();
    return storage ? storage.size() : 0;
}

size_t Array::capacity() {
    auto storage = get_storage();
    return storage ? storage.capacity() : 0;
}

Value* Array::data() {
    auto storage = get_storage();
    return storage ? storage.data() : nullptr;
}

Value Array::get(size_t index) {
    // TODO Exception
    TIRO_CHECK(index < size(), "Array::get(): index out of bounds.");
    return get_storage().get(index);
}

void Array::set(size_t index, Handle<Value> value) {
    // TODO Exception
    TIRO_CHECK(index < size(), "Array::set(): index out of bounds.");
    get_storage().set(index, value);
}

void Array::append(Context& ctx, Handle<Value> value) {
    // Note: this->get_storage() is rooted because "this" is rooted.

    size_t cap = capacity();
    if (size() >= cap) {
        if (TIRO_UNLIKELY(!checked_add(cap, size_t(1)))) {
            TIRO_ERROR("Array size too large."); // FIXME exception
        }
        cap = next_capacity(cap);

        Root<ArrayStorage> new_storage(ctx);
        if (auto storage = get_storage()) {
            new_storage.set(ArrayStorage::make(ctx, storage.values(), cap));
        } else {
            new_storage.set(ArrayStorage::make(ctx, cap));
        }

        // TODO: This would need a write barrier (assignment to d->storage).
        set_storage(new_storage);
    }

    TIRO_DEBUG_ASSERT(size() < capacity(), "There must be enough free capacity.");
    get_storage().append(value);
}

void Array::remove_last() {
    TIRO_CHECK(size() > 0, "Array::remove_last(): Array is empty.");
    get_storage().remove_last();
}

void Array::clear() {
    get_storage().clear();
}

size_t Array::next_capacity(size_t required) {
    static constexpr size_t max_pow = max_pow2<size_t>();
    if (TIRO_UNLIKELY(required > max_pow)) {
        return std::numeric_limits<size_t>::max();
    }

    if (required == 0)
        return 0;
    if (required <= 8)
        return 8;
    return ceil_pow2(required);
}

static constexpr MethodDesc array_methods[] = {
    {
        "size"sv,
        1,
        [](NativeFunctionFrame& frame) {
            auto array = check_instance<Array>(frame);
            frame.result(frame.ctx().get_integer(static_cast<i64>(array->size())));
        },
    },
    {
        "append"sv,
        2,
        [](NativeFunctionFrame& frame) {
            auto array = check_instance<Array>(frame);
            auto value = frame.arg(1);
            array->append(frame.ctx(), value);
        },
    },
    {
        "clear"sv,
        1,
        [](NativeFunctionFrame& frame) {
            auto array = check_instance<Array>(frame);
            array->clear();
        },
    },
};

constexpr TypeDesc array_type_desc{"Array"sv, array_methods};

} // namespace tiro::vm
