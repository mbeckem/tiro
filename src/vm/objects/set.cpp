#include "vm/objects/set.hpp"

#include "vm/error_utils.hpp"
#include "vm/object_support/factory.hpp"
#include "vm/object_support/type_desc.hpp"

namespace tiro::vm {

Set Set::make(Context& ctx) {
    return make(ctx, 0).must("failed to allocate empty set");
}

Fallible<Set> Set::make(Context& ctx, size_t initial_capacity) {
    Scope sc(ctx);
    TIRO_TRY_LOCAL(sc, table,
        (initial_capacity == 0) ? Fallible(HashTable::make(ctx, 0))
                                : HashTable::make(ctx, initial_capacity));

    Layout* data = create_object<Set>(ctx, StaticSlotsInit());
    data->write_static_slot(TableSlot, table);
    return Set(from_heap(data));
}

Fallible<Set> Set::make(Context& ctx, HandleSpan<Value> initial_content) {
    Scope sc(ctx);
    TIRO_TRY_LOCAL(sc, set, Set::make(ctx, initial_content.size()));
    for (auto v : initial_content)
        set->insert(ctx, v).must("failed to insert set item");

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

Fallible<bool> Set::insert(Context& ctx, Handle<Value> v) {
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

static void set_size_impl(SyncFrameContext& frame) {
    auto set = check_instance<Set>(frame);
    i64 size = static_cast<i64>(set->size());
    frame.return_value(frame.ctx().get_integer(size));
}

static void set_contains_impl(SyncFrameContext& frame) {
    auto set = check_instance<Set>(frame);
    bool result = set->contains(*frame.arg(1));
    frame.return_value(frame.ctx().get_boolean(result));
}

static void set_clear_impl(SyncFrameContext& frame) {
    auto set = check_instance<Set>(frame);
    set->clear();
}

static void set_insert_impl(SyncFrameContext& frame) {
    auto set = check_instance<Set>(frame);
    TIRO_FRAME_TRY(inserted, set->insert(frame.ctx(), frame.arg(1)));
    frame.return_value(frame.ctx().get_boolean(inserted));
}

static void set_remove_impl(SyncFrameContext& frame) {
    auto set = check_instance<Set>(frame);
    set->remove(*frame.arg(1));
}

static constexpr FunctionDesc set_methods[] = {
    FunctionDesc::method("size"sv, 1, NativeFunctionStorage::static_sync<set_size_impl>()),
    FunctionDesc::method("contains"sv, 2, NativeFunctionStorage::static_sync<set_contains_impl>()),
    FunctionDesc::method("clear"sv, 1, NativeFunctionStorage::static_sync<set_clear_impl>()),
    FunctionDesc::method("insert"sv, 2, NativeFunctionStorage::static_sync<set_insert_impl>()),
    FunctionDesc::method("remove"sv, 2, NativeFunctionStorage::static_sync<set_remove_impl>()),
};

constexpr TypeDesc set_type_desc{"Set"sv, set_methods};

} // namespace tiro::vm
