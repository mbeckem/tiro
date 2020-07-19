#include "vm/objects/set.hpp"

#include "vm/objects/factory.hpp"

namespace tiro::vm {

Set Set::make(Context& ctx) {
    return make(ctx, 0);
}

Set Set::make(Context& ctx, size_t initial_capacity) {
    Scope sc(ctx);
    Local table = sc.local(
        initial_capacity > 0 ? HashTable::make(ctx, initial_capacity) : HashTable::make(ctx));

    Layout* data = create_object<Set>(ctx, StaticSlotsInit());
    data->write_static_slot(TableSlot, table);
    return Set(from_heap(data));
}

Set Set::make(Context& ctx, HandleSpan<Value> initial_content) {
    Scope sc(ctx);
    Local set = sc.local(Set::make(ctx, initial_content.size()));

    for (auto v : initial_content)
        set->insert(ctx, v);

    return *set;
}

bool Set::contains(Value v) {
    return get_table().contains(v);
}

std::optional<Value> Set::find(Value v) {
    auto found_pair = get_table().find(v);
    if (!found_pair)
        return {};

    return found_pair->first;
}

size_t Set::size() {
    return get_table().size();
}

bool Set::insert(Context& ctx, Handle<Value> v) {
    return get_table().set(ctx, v, null_handle());
}

void Set::remove(Value v) {
    get_table().remove(v);
}

void Set::clear() {
    get_table().clear();
}

HashTable Set::get_table() {
    return layout()->read_static_slot<HashTable>(TableSlot);
}

SetIterator SetIterator::make(Context& ctx, Handle<Set> set) {
    Scope sc(ctx);
    Local table = sc.local(set->get_table());
    Local iter = sc.local(HashTableKeyIterator::make(ctx, table));

    Layout* data = create_object<SetIterator>(ctx, StaticSlotsInit());
    data->write_static_slot(IterSlot, iter);
    return SetIterator(from_heap(data));
}

std::optional<Value> SetIterator::next(Context& ctx) {
    auto iter = layout()->read_static_slot<HashTableKeyIterator>(IterSlot);
    return iter.next(ctx);
}

static constexpr MethodDesc set_methods[] = {
    {
        "size"sv,
        1,
        [](NativeFunctionFrame& frame) {
            auto set = check_instance<Set>(frame);
            i64 size = static_cast<i64>(set->size());
            frame.result(frame.ctx().get_integer(size));
        },
    },
    {
        "contains"sv,
        2,
        [](NativeFunctionFrame& frame) {
            auto set = check_instance<Set>(frame);
            bool result = set->contains(*frame.arg(1));
            frame.result(frame.ctx().get_boolean(result));
        },
    },
    {
        "clear"sv,
        1,
        [](NativeFunctionFrame& frame) {
            auto set = check_instance<Set>(frame);
            set->clear();
        },
    },
    {
        "insert"sv,
        2,
        [](NativeFunctionFrame& frame) {
            auto set = check_instance<Set>(frame);
            bool inserted = set->insert(frame.ctx(), frame.arg(1));
            frame.result(frame.ctx().get_boolean(inserted));
        },
    },
    {
        "remove"sv,
        2,
        [](NativeFunctionFrame& frame) {
            auto set = check_instance<Set>(frame);
            set->remove(*frame.arg(1));
        },
    },
};

constexpr TypeDesc set_type_desc{"Set"sv, set_methods};

} // namespace tiro::vm
