#include "vm/objects/array.hpp"

#include "vm/objects/array_storage_base.ipp"
#include "vm/objects/factory.hpp"
#include "vm/objects/native.hpp"

namespace tiro::vm {

Array Array::make(Context& ctx, size_t initial_capacity) {
    Scope sc(ctx);

    Local storage = sc.local<Nullable<ArrayStorage>>();
    if (initial_capacity > 0) {
        storage = ArrayStorage::make(ctx, initial_capacity);
    }

    Layout* data = create_object<Array>(ctx, StaticSlotsInit());
    data->write_static_slot(StorageSlot, storage.get());
    return Array(from_heap(data));
}

Array Array::make(Context& ctx, HandleSpan<Value> initial_content) {
    if (initial_content.empty())
        return make(ctx, 0);

    Scope sc(ctx);
    Local storage = sc.local(ArrayStorage::make(ctx, initial_content, initial_content.size()));

    Layout* data = create_object<Array>(ctx, StaticSlotsInit());
    data->write_static_slot(StorageSlot, storage.get());
    return Array(from_heap(data));
}

size_t Array::size() {
    auto storage = get_storage();
    return storage ? storage.value().size() : 0;
}

size_t Array::capacity() {
    auto storage = get_storage();
    return storage ? storage.value().capacity() : 0;
}

Value* Array::data() {
    auto storage = get_storage();
    return storage ? storage.value().data() : nullptr;
}

Value Array::get(size_t index) {
    // TODO Exception
    TIRO_CHECK(index < size(), "Array::get(): index out of bounds.");
    return get_storage().value().get(index);
}

void Array::set(size_t index, Handle<Value> value) {
    // TODO Exception
    TIRO_CHECK(index < size(), "Array::set(): index out of bounds.");
    get_storage().value().set(index, *value);
}

void Array::append(Context& ctx, Handle<Value> value) {
    const size_t current_capacity = capacity();

    // Fast path: enough free capacity to append.
    if (TIRO_LIKELY(size() < current_capacity)) {
        get_storage().value().append(*value);
        return;
    }

    // Slow path: must resize.
    Scope sc(ctx);
    Local storage = sc.local(get_storage());

    size_t new_capacity;
    if (TIRO_UNLIKELY(!checked_add(current_capacity, size_t(1), new_capacity))) {
        TIRO_ERROR("Array size too large."); // FIXME exception
    }
    new_capacity = next_capacity(new_capacity);

    Local new_storage = sc.local(ArrayStorage::make(ctx, new_capacity));
    if (storage->has_value()) {
        new_storage->append_all(storage->value().values());
    }
    new_storage->append(*value);
    set_storage(*new_storage);
}

void Array::remove_last() {
    TIRO_CHECK(size() > 0, "Array::remove_last(): Array is empty.");
    if (auto storage = get_storage()) {
        storage.value().remove_last();
    }
}

void Array::clear() {
    if (auto storage = get_storage()) {
        storage.value().clear();
    }
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
