#include "vm/objects/hash_table.hpp"

#include "vm/context.hpp"
#include "vm/handles/scope.hpp"
#include "vm/objects/buffer.hpp"
#include "vm/objects/factory.hpp"
#include "vm/objects/native.hpp"

#include <ostream>

// #define TIRO_TABLE_TRACE_ENABLED

#ifdef TIRO_TABLE_TRACE_ENABLED
#    include <iostream>
#    define TIRO_TABLE_TRACE(...) \
        (std::cout << "HashTable: " << fmt::format(__VA_ARGS__) << std::endl)
#else
#    define TIRO_TABLE_TRACE(...)
#endif

namespace tiro::vm {

namespace {

template<HashTable::SizeClass>
struct SizeClassTraits;

template<typename IndexType>
struct BufferView {
    static Buffer make(Context& ctx, size_t size, IndexType initial) {
        // TODO Overflow
        const size_t size_in_bytes = size * sizeof(IndexType);

        auto buffer = Buffer::make(ctx, size_in_bytes, Buffer::uninitialized);
        TIRO_DEBUG_ASSERT(is_aligned(reinterpret_cast<uintptr_t>(buffer.data()),
                              static_cast<uintptr_t>(alignof(IndexType))),
            "Buffer must be aligned correctly.");
        std::uninitialized_fill_n(data(buffer), size, initial);
        return buffer;
    }

    static Span<IndexType> values(Buffer buffer) { return {data(buffer), size(buffer)}; }

    static IndexType* data(Buffer buffer) { return reinterpret_cast<IndexType*>(buffer.data()); }

    static size_t size(Buffer buffer) {
        size_t size_in_bytes = buffer.size();
        TIRO_DEBUG_ASSERT(size_in_bytes % sizeof(IndexType) == 0,
            "Byte size must always be a multiple of the data type size.");
        return size_in_bytes / sizeof(IndexType);
    }
};

template<>
struct SizeClassTraits<HashTable::SizeClass::U8> {
    using BufferAccess = BufferView<u8>;
    using IndexType = u8;
    static constexpr IndexType empty_value = std::numeric_limits<IndexType>::max();
};

template<>
struct SizeClassTraits<HashTable::SizeClass::U16> {
    using BufferAccess = BufferView<u16>;
    using IndexType = u16;
    static constexpr IndexType empty_value = std::numeric_limits<IndexType>::max();
};

template<>
struct SizeClassTraits<HashTable::SizeClass::U32> {
    using BufferAccess = BufferView<u32>;
    using IndexType = u32;
    static constexpr IndexType empty_value = std::numeric_limits<IndexType>::max();
};

template<>
struct SizeClassTraits<HashTable::SizeClass::U64> {
    using BufferAccess = BufferView<u64>;
    using IndexType = u64;
    static constexpr IndexType empty_value = std::numeric_limits<IndexType>::max();
};

} // namespace

// The hash table maintains a load factor of at most 75%.
// The index size doubles with every growth operation. The table
// size is adjusted down to 3/4 of the index size.
static constexpr size_t initial_table_capacity = 6;
static constexpr size_t initial_index_capacity = 8;

static size_t grow_index_capacity(size_t old_index_size) {
    if (TIRO_UNLIKELY(old_index_size >= max_pow2<size_t>())) {
        // TODO Exception
        TIRO_ERROR("Hash table is too large.");
    }

    return old_index_size << 1;
}

static size_t table_capacity_for_index_capacity(size_t index_size) {
    TIRO_DEBUG_ASSERT(is_pow2(index_size), "Index size must always be a power of two.");
    TIRO_DEBUG_ASSERT(index_size >= initial_index_capacity, "Index size too small.");
    return index_size - index_size / 4;
}

static size_t index_capacity_for_entries_capacity(size_t table_size) {
    // size_t index_size = ceil_pow2(table_size + (table_size + 2) / 3);
    size_t index_size = table_size;
    if (TIRO_UNLIKELY(!checked_add(index_size, size_t(2))))
        goto overflow;

    index_size /= 3;
    if (TIRO_UNLIKELY(!checked_add(index_size, table_size)))
        goto overflow;

    if (TIRO_UNLIKELY(index_size > max_pow2<size_t>()))
        goto overflow;

    return std::max(initial_index_capacity, ceil_pow2(index_size));

overflow:
    // TODO Exception
    TIRO_ERROR("Requested hash table size is too large.");
}

template<typename Func>
static decltype(auto) dispatch_size_class(HashTable::SizeClass size_class, Func&& fn) {
    switch (size_class) {
    case HashTable::SizeClass::U8:
        return fn(SizeClassTraits<HashTable::SizeClass::U8>());
    case HashTable::SizeClass::U16:
        return fn(SizeClassTraits<HashTable::SizeClass::U16>());
    case HashTable::SizeClass::U32:
        return fn(SizeClassTraits<HashTable::SizeClass::U32>());
    case HashTable::SizeClass::U64:
        return fn(SizeClassTraits<HashTable::SizeClass::U64>());
    }
    TIRO_UNREACHABLE("Invalid size class.");
}

template<typename Traits>
static auto cast_index(size_t index) {
    TIRO_DEBUG_ASSERT(index < Traits::empty_value, "Index must fit into the target index type.");
    return static_cast<typename Traits::IndexType>(index);
}

template<typename Traits>
static auto index_values(Buffer indices) {
    return Traits::BufferAccess::values(indices);
}

HashTableEntry::Hash HashTableEntry::make_hash(size_t raw_hash) {
    // Truncate the arbitrary hash value to the valid range (some bits
    // and values are reserved).
    size_t hash = raw_hash == deleted_value ? 0 : raw_hash;
    static_assert(deleted_value != 0);
    return Hash{hash};
}

HashTableEntry::Hash HashTableEntry::make_hash(Value value) {
    return make_hash(tiro::vm::hash(value));
}

HashTable HashTable::make(Context& ctx) {
    Layout* data = create_object<HashTable>(ctx, StaticSlotsInit(), StaticPayloadInit());
    return HashTable(from_heap(data));
}

HashTable HashTable::make(Context& ctx, size_t initial_capacity) {
    Scope sc(ctx);
    Local table = sc.local(HashTable::make(ctx));

    if (initial_capacity == 0)
        return table.get();

    size_t index_cap = index_capacity_for_entries_capacity(initial_capacity);
    size_t entries_cap = table_capacity_for_index_capacity(index_cap);
    TIRO_DEBUG_ASSERT(
        entries_cap >= initial_capacity, "Capacity calculation wrong: not enough space.");

    table->grow_to_capacity<SizeClassTraits<SizeClass::U8>>(
        table->layout(), ctx, entries_cap, index_cap);
    return *table;
}

size_t HashTable::size() {
    return layout()->static_payload()->size;
}

size_t HashTable::occupied_entries() {
    auto entries = get_entries(layout());
    if (!entries)
        return 0;
    return entries.value().size();
}

size_t HashTable::entry_capacity() {
    auto entries = get_entries(layout());
    if (!entries)
        return 0;
    return entries.value().capacity();
}

size_t HashTable::index_capacity() {
    Layout* data = layout();

    auto index = get_index(data);
    if (!index)
        return 0;

    return dispatch_size_class(index_size_class(data), [&](auto traits) -> size_t {
        using ST = decltype(traits);
        return ST::BufferAccess::size(index.value());
    });
}

bool HashTable::contains(Value key) {
    if (empty())
        return false;

    Layout* data = layout();
    return dispatch_size_class(index_size_class(data), [&](auto traits) {
        auto pos = this->template find_impl<decltype(traits)>(data, key);
        return pos.has_value();
    });
}

std::optional<Value> HashTable::get(Value key) {
    if (empty())
        return {};

    const auto pos = dispatch_size_class(index_size_class(layout()),
        [&](auto traits) { return this->template find_impl<decltype(traits)>(layout(), key); });
    if (!pos)
        return {};

    const size_t entry_index = pos->second;
    TIRO_DEBUG_ASSERT(entry_index < entry_capacity(), "Invalid entry index.");

    const HashTableEntry& entry = get_entries(layout()).value().get(entry_index);
    TIRO_DEBUG_ASSERT(!entry.is_deleted(), "Found entry must not be deleted.");
    return entry.value();
}

std::optional<std::pair<Value, Value>> HashTable::find(Value key) {
    if (empty())
        return {};

    const auto pos = dispatch_size_class(index_size_class(layout()),
        [&](auto traits) { return this->template find_impl<decltype(traits)>(layout(), key); });
    if (!pos)
        return {};

    const size_t entry_index = pos->second;
    TIRO_DEBUG_ASSERT(entry_index < entry_capacity(), "Invalid entry index.");

    const HashTableEntry& entry = get_entries(layout()).value().get(entry_index);
    TIRO_DEBUG_ASSERT(!entry.is_deleted(), "Found entry must not be deleted.");
    return std::make_pair(entry.key(), entry.value());
}

bool HashTable::set(Context& ctx, Handle<Value> key, Handle<Value> value) {
    TIRO_TABLE_TRACE("Insert {} -> {}", to_string(key.get()), to_string(value.get()));

    Layout* data = layout();
    ensure_free_capacity(data, ctx);
    return dispatch_size_class(index_size_class(data), [&](auto traits) {
        return this->template set_impl<decltype(traits)>(data, key.get(), value.get());
    });
}

void HashTable::remove(Value key) {
    TIRO_TABLE_TRACE("Remove {}", to_string(key));

    if (empty())
        return;

    Layout* data = layout();
    dispatch_size_class(index_size_class(data),
        [&](auto traits) { this->template remove_impl<decltype(traits)>(data, key); });
}

void HashTable::clear() {
    TIRO_TABLE_TRACE("Clear");

    if (empty())
        return;

    Layout* data = layout();
    dispatch_size_class(index_size_class(data),
        [&](auto traits) { this->template clear_impl<decltype(traits)>(data); });
}

HashTableIterator HashTable::make_iterator(Context& ctx) {
    return HashTableIterator::make(ctx, Handle<HashTable>(this));
}

bool HashTable::is_packed() {
    if (empty())
        return true;
    return size() == occupied_entries();
}

std::optional<std::pair<Value, Value>> HashTable::iterator_next(size_t& entry_index) {
    auto entries_storage = get_entries(layout());
    if (!entries_storage)
        return {};

    // TODO modcount
    Span<const HashTableEntry> entries = entries_storage.value().values();
    TIRO_CHECK(entry_index <= entries.size(),
        "Invalid iterator position, was the table modified during iteration?");

    while (entry_index < entries.size()) {
        const HashTableEntry& entry = entries[entry_index++];
        if (!entry.is_deleted())
            return std::make_pair(entry.key(), entry.value());
    }
    return {};
}

template<typename ST>
bool HashTable::set_impl(Layout* data, Value key, Value value) {
    auto indices = index_values<ST>(get_index(data).value());
    auto entries = get_entries(data).value();
    Hash key_hash = HashTableEntry::make_hash(key);

    TIRO_DEBUG_ASSERT(
        size() < indices.size(), "There must be at least one free slot in the index table.");
    TIRO_DEBUG_ASSERT(get_entries(data).has_value() && !get_entries(data).value().full(),
        "There must be at least one free slot in the entries array.");

    // The code below does one of three things:
    //  1. Its finds the key in the map, in which case it overwrites the value and returns.
    //  2. It finds an empty bucket, in which case it can simply insert the new index.
    //  3. It steals an existing bucket (robin hood hashing).
    //
    // After case 2 and 3 we can insert the new key-value pair into the entries array.
    // After case 3, we must additionally continue inserting into the table to re-register
    // the stolen bucket's content. All loops in this function terminate because there is
    // at least one free bucket available at all times.

    bool slot_stolen = false; // True: continue with stolen data.
    auto index_to_insert = cast_index<ST>(entries.size());
    size_t bucket_index = bucket_for_hash(data, key_hash);
    size_t distance = 0;

    TIRO_TABLE_TRACE("Inserting index {}, ideal bucket is {}", index_to_insert, bucket_index);

    while (1) {
        auto& index = indices[bucket_index];

        if (index == ST::empty_value) {
            index = index_to_insert;
            TIRO_TABLE_TRACE("Inserted into free bucket {}", bucket_index);
            break; // Case 2.
        }

        const auto& entry = entries.get(static_cast<size_t>(index));
        const Hash entry_hash = entry.hash();
        size_t entry_distance = distance_from_ideal(data, entry_hash, bucket_index);

        if (entry_distance < distance) {
            slot_stolen = true;
            std::swap(index_to_insert, index);
            std::swap(distance, entry_distance);
            TIRO_TABLE_TRACE(
                "Robin hood swap with index {}, new distance is {}", index_to_insert, distance);
            break; // Case 3.
        }

        if (entry_hash.value == key_hash.value && key_equal(entry.key(), key)) {
            entries.set(index, HashTableEntry(key_hash, entry.key(), value));
            TIRO_TABLE_TRACE("Existing value was overwritten.");
            return false; // Case 1.
        }

        bucket_index = next_bucket(data, bucket_index);
        distance += 1;
        TIRO_TABLE_TRACE("Continuing with bucket {} and distance {}", bucket_index, distance);
    }

    entries.append(HashTableEntry(key_hash, key, value));
    data->static_payload()->size += 1;

    if (slot_stolen) {
        // Continuation from case 3.
        while (1) {
            bucket_index = next_bucket(data, bucket_index);
            distance += 1;

            auto& index = indices[bucket_index];
            if (index == ST::empty_value) {
                index = index_to_insert;
                TIRO_TABLE_TRACE("Inserted stolen index into free bucket {}", bucket_index);
                break;
            }

            const auto& entry = entries.get(static_cast<size_t>(index));
            size_t entry_distance = distance_from_ideal(data, entry.hash(), bucket_index);
            if (entry_distance < distance) {
                std::swap(index_to_insert, index);
                std::swap(distance, entry_distance);
                TIRO_TABLE_TRACE(
                    "Robin hood of index, swap with index {}, new "
                    "distance is {}",
                    index_to_insert, distance);
            }
            // Same key impossible because we're only considering entries
            // already in the map.
        }
    }

    return true;
}

template<typename ST>
void HashTable::remove_impl(Layout* data, Value key) {
    static constexpr HashTableEntry sentinel = HashTableEntry::make_deleted();

    const auto found = find_impl<ST>(data, key);
    if (!found)
        return;

    TIRO_DEBUG_ASSERT(size(), "Cannot be empty if a value has been found.");
    const auto [removed_bucket, removed_entry] = *found;

    // Mark the entry as deleted. We can just pop if this was the last element,
    // otherwise we have to leave a hole.
    auto entries = get_entries(data).value();
    if (removed_entry == entries.size() - 1) {
        entries.remove_last();
    } else {
        entries.set(removed_entry, sentinel);
    }

    data->static_payload()->size -= 1;
    if (data->static_payload()->size == 0) {
        // We know that we can start from the beginning since we're empty.
        entries.clear();
    }

    // Erase the reference in the index array.
    remove_from_index<ST>(data, removed_bucket);

    // Close holes if 50% or more of the entries in the table have been deleted.
    if (data->static_payload()->size <= entries.size() / 2)
        compact<ST>(data);
}

template<typename ST>
void HashTable::remove_from_index(Layout* data, size_t erased_bucket) {
    auto indices = index_values<ST>(get_index(data).value());
    auto entries = get_entries(data).value();
    indices[erased_bucket] = ST::empty_value;

    size_t current_bucket = next_bucket(data, erased_bucket);
    while (1) {
        auto& index = indices[current_bucket];
        if (index == ST::empty_value) {
            break;
        }

        const auto& entry = entries.get(index);
        const size_t entry_distance = distance_from_ideal(data, entry.hash(), current_bucket);
        if (entry_distance > 0) {
            TIRO_DEBUG_ASSERT(
                distance_from_ideal(data, entry.hash(), erased_bucket) <= entry_distance,
                "Backshift invariant: distance does not get worse.");
            indices[erased_bucket] = index;
            indices[current_bucket] = ST::empty_value;
        } else {
            break;
        }
    }
}

template<typename ST>
void HashTable::clear_impl(Layout* data) {
    auto indices = index_values<ST>(get_index(data).value());
    auto entries = get_entries(data).value();

    entries.clear();
    std::fill(indices.begin(), indices.end(), ST::empty_value);
    data->static_payload()->size = 0;
}

template<typename ST>
std::optional<std::pair<size_t, size_t>> HashTable::find_impl(Layout* data, Value key) {
    auto indices = index_values<ST>(get_index(data).value());
    auto entries = get_entries(data).value();
    Hash key_hash = HashTableEntry::make_hash(key);

    size_t bucket_index = bucket_for_hash(data, key_hash);
    size_t distance = 0;
    while (1) {
        const auto& index = indices[bucket_index];
        if (index == ST::empty_value) {
            return {};
        }

        // Improvement: storing some bits of the hash together with the
        // index would reduce the number of random-access-like dereferences
        // into the entries array.
        const auto& entry = entries.get(index);
        const Hash entry_hash = entry.hash();
        if (distance > distance_from_ideal(data, entry_hash, bucket_index)) {
            // If we were in the hash table, we would have enounctered ourselves
            // already: we would have swapped us into this bucket!
            // This is the invariant established by robin hood insertion.
            return {};
        }

        if (entry_hash.value == key_hash.value && key_equal(entry.key(), key)) {
            return std::pair(bucket_index, index);
        }

        bucket_index = next_bucket(data, bucket_index);
        distance += 1;
    }
}

// Makes sure that at least one slot is available at the end of the entries array.
// Also makes sure that at least one slot is available in the index table.
// Note: index and entries arrays currently grow together (with the index array
// having a higher number of slots). This could change in the future to improve performance.
void HashTable::ensure_free_capacity(Layout* data, Context& ctx) {
    // Invariant: data->entries.capacity() <= data->indices.size(), i.e.
    // the index table is always at least as large as the entries array.

    if (!get_entries(data).has_value()) {
        init_first(data, ctx);
        return;
    }

    TIRO_DEBUG_ASSERT(entry_capacity() > 0, "Entries array must not have 0 capacity.");
    if (get_entries(data).value().full()) {
        const bool should_grow = (size() / 3) >= (entry_capacity() / 4);

        dispatch_size_class(index_size_class(data), [&](auto traits) {
            if (should_grow) {
                this->template grow<decltype(traits)>(data, ctx);
            } else {
                this->template compact<decltype(traits)>(data);
            }
        });
    }

    TIRO_DEBUG_ASSERT(!get_entries(data).value().full(), "Must have made room for a new element.");
}

void HashTable::init_first(Layout* data, Context& ctx) {
    using InitialSizeClass = SizeClassTraits<SizeClass::U8>;

    TIRO_TABLE_TRACE("Initializing hash table to initial capacity");
    set_entries(data, HashTableStorage::make(ctx, initial_table_capacity));
    set_index(data, InitialSizeClass::BufferAccess::make(
                        ctx, initial_index_capacity, InitialSizeClass::empty_value));
    data->static_payload()->size = 0;
    data->static_payload()->mask = initial_index_capacity - 1;
}

template<typename ST>
void HashTable::grow(Layout* data, Context& ctx) {
    TIRO_DEBUG_ASSERT(get_entries(data).has_value(), "Entries array must not be null.");
    TIRO_DEBUG_ASSERT(get_index(data).has_value(), "Indices table must not be null.");
    TIRO_DEBUG_ASSERT(
        index_capacity() >= initial_index_capacity, "Invalid index size (too small).");

    size_t new_index_cap = grow_index_capacity(index_capacity());
    size_t new_entry_cap = table_capacity_for_index_capacity(new_index_cap);
    grow_to_capacity<ST>(data, ctx, new_entry_cap, new_index_cap);
}

template<typename ST>
void HashTable::grow_to_capacity(
    Layout* data, Context& ctx, size_t new_entry_capacity, size_t new_index_capacity) {
    TIRO_DEBUG_ASSERT(
        new_entry_capacity > entry_capacity(), "Must grow to a larger entry capacity.");
    TIRO_DEBUG_ASSERT(
        new_index_capacity > index_capacity(), "Must grow to a larger index capacity.");
    TIRO_DEBUG_ASSERT(
        size() == 0 || get_entries(data).has_value(), "Either empty or non-null entries array.");

    TIRO_TABLE_TRACE("Growing table from {} entries to {} entries ({} index slots)",
        entry_capacity(), new_entry_capacity, new_index_capacity);

    Scope sc(ctx);
    Local new_entries = sc.local(HashTableStorage::make(ctx, new_entry_capacity));
    if (const auto sz = size(); sz == occupied_entries()) {
        if (sz > 0) {
            new_entries->append_all(get_entries(data).value().values());
        }
    } else {
        for (const HashTableEntry& entry : get_entries(data).value().values()) {
            if (!entry.is_deleted())
                new_entries->append(entry);
        }
    }
    set_entries(data, *new_entries);

    // TODO: make rehashing cheaper by reusing the old index table...
    const SizeClass next_size_class = index_size_class(new_entry_capacity);
    dispatch_size_class(next_size_class, [&](auto traits) {
        this->template recreate_index<decltype(traits)>(data, ctx, new_index_capacity);
    });
}

template<typename ST>
void HashTable::compact(Layout* data) {
    TIRO_DEBUG_ASSERT(get_entries(data).has_value(), "Entries array must not be null.");

    auto entries = get_entries(data).value();
    size_t entries_size = entries.size();
    if (entries_size == size())
        return; // No holes.

    TIRO_TABLE_TRACE("Compacting table from size {} to {}.", entries_size, size());

    auto find_deleted_entry = [&] {
        for (size_t i = 0; i < entries_size; ++i) {
            const auto& entry = entries.get(i);
            if (entry.is_deleted())
                return i;
        }
        TIRO_UNREACHABLE("There must be a deleted entry.");
    };

    size_t write_pos = find_deleted_entry();
    for (size_t read_pos = write_pos + 1; read_pos < entries_size; ++read_pos) {
        const auto& entry = entries.get(read_pos);
        if (!entry.is_deleted()) {
            entries.set(write_pos, entry);
            ++write_pos;
        }
    }

    entries.remove_last(entries_size - write_pos);
    TIRO_DEBUG_ASSERT(entries.size() == size(), "Must have packed all entries.");

    // TODO inefficient
    auto indices = index_values<ST>(get_index(data).value());
    std::fill(indices.begin(), indices.end(), ST::empty_value);
    rehash_index<ST>(data);
}

template<typename ST>
void HashTable::recreate_index(Layout* data, Context& ctx, size_t capacity) {
    TIRO_DEBUG_ASSERT(
        size() == occupied_entries(), "Entries array must not have any deleted elements.");
    TIRO_DEBUG_ASSERT(is_pow2(capacity), "New index capacity must be a power of two.");

    // TODO rehashing can be made faster, see rust index map at https://github.com/bluss/indexmap
    set_index(data, ST::BufferAccess::make(ctx, capacity, ST::empty_value));
    data->static_payload()->mask = capacity - 1;
    rehash_index<ST>(data);
}

template<typename ST>
void HashTable::rehash_index(Layout* data) {
    TIRO_DEBUG_ASSERT(get_entries(data).has_value(), "Entries array must not be null.");
    TIRO_DEBUG_ASSERT(get_index(data).has_value(), "Indices table must not be null.");

    TIRO_TABLE_TRACE("Rehashing table index");

    // TODO deduplicate code with insert
    const auto entries = get_entries(data).value().values();
    const auto indices = index_values<ST>(get_index(data).value());
    for (size_t entry_index = 0; entry_index < entries.size(); ++entry_index) {
        const HashTableEntry& entry = entries[entry_index];

        auto index_to_insert = cast_index<ST>(entry_index);
        size_t bucket_index = bucket_for_hash(data, entry.hash());
        size_t distance = 0;
        while (1) {
            auto& index = indices[bucket_index];
            if (index == ST::empty_value) {
                index = index_to_insert;
                break;
            }

            const auto& other_entry = entries[static_cast<size_t>(index)];
            size_t other_entry_distance = distance_from_ideal(
                data, other_entry.hash(), bucket_index);
            if (other_entry_distance < distance) {
                std::swap(index_to_insert, index);
                std::swap(distance, other_entry_distance);
            }

            bucket_index = next_bucket(data, bucket_index);
            distance += 1;
        }
    }
}

size_t HashTable::next_bucket(Layout* data, size_t current_bucket) {
    TIRO_DEBUG_ASSERT(get_index(data).has_value(), "Must have an index table.");
    return (current_bucket + 1) & data->static_payload()->mask;
}

size_t HashTable::bucket_for_hash(Layout* data, Hash hash) {
    TIRO_DEBUG_ASSERT(get_index(data).has_value(), "Must have an index table.");
    return hash.value & data->static_payload()->mask;
}

size_t HashTable::distance_from_ideal(Layout* data, Hash hash, size_t current_bucket) {
    size_t desired_bucket = bucket_for_hash(data, hash);
    return (current_bucket - desired_bucket) & data->static_payload()->mask;
}

HashTable::SizeClass HashTable::index_size_class([[maybe_unused]] Layout* data) {
    TIRO_DEBUG_ASSERT(get_entries(data).has_value(),
        "Must have a valid entries table in order to have an index.");
    return index_size_class(entry_capacity());
}

Nullable<HashTableStorage> HashTable::get_entries(Layout* data) {
    return data->read_static_slot<Nullable<HashTableStorage>>(EntriesSlot);
}

void HashTable::set_entries(Layout* data, Nullable<HashTableStorage> entries) {
    data->write_static_slot(EntriesSlot, entries);
}

Nullable<Buffer> HashTable::get_index(Layout* data) {
    return data->read_static_slot<Nullable<Buffer>>(IndexSlot);
}

void HashTable::set_index(Layout* data, Nullable<Buffer> index) {
    data->write_static_slot(IndexSlot, index);
}

HashTable::SizeClass HashTable::index_size_class(size_t entry_count) {
    // max is always reserved as the sentinel value to signal an empty bucket.
    if (entry_count <= SizeClassTraits<SizeClass::U8>::empty_value) {
        return SizeClass::U8;
    } else if (entry_count <= SizeClassTraits<SizeClass::U16>::empty_value) {
        return SizeClass::U16;
    } else if (entry_count <= SizeClassTraits<SizeClass::U32>::empty_value) {
        return SizeClass::U32;
    } else if (entry_count <= SizeClassTraits<SizeClass::U64>::empty_value) {
        return SizeClass::U64;
    }
    TIRO_ERROR("Too many values: {}", entry_count);
}

std::string HashTable::dump() {
    Layout* data = layout();

    auto entries = get_entries(data);
    auto index = get_index(data);

    fmt::memory_buffer buf;
    fmt::format_to(buf, "Hash table @{}\n", (void*) data);
    fmt::format_to(buf,
        "  Size: {}\n"
        "  Capacity: {}\n"
        "  Mask: {}\n",
        size(), entry_capacity(), data->static_payload()->mask);

    fmt::format_to(buf, "  Entries:\n");
    if (!entries.has_value()) {
        fmt::format_to(buf, "    NULL\n");
    } else {
        const size_t count = entries.value().size();
        for (size_t i = 0; i < count; ++i) {
            const HashTableEntry& entry = entries.value().get(i);
            fmt::format_to(buf, "    {}: ", i);
            if (entry.is_deleted()) {
                fmt::format_to(buf, "<DELETED>\n");
            } else {
                fmt::format_to(buf, "{} -> {} (Hash {})\n", to_string(entry.key()),
                    to_string(entry.value()), entry.hash().value);
            }
        }
    }

    fmt::format_to(buf, "  Indices:\n");
    if (!index.has_value()) {
        fmt::format_to(buf, "    NULL\n");
    } else {
        fmt::format_to(buf, "    Type: {}\n", to_string(index.value().type()));
        dispatch_size_class(index_size_class(data), [&](auto traits) {
            using Traits = decltype(traits);

            auto indices = Traits::BufferAccess::values(index.value());
            for (size_t current_bucket = 0; current_bucket < indices.size(); ++current_bucket) {
                auto i = indices[current_bucket];
                fmt::format_to(buf, "    {}: ", current_bucket);
                if (i == Traits::empty_value) {
                    fmt::format_to(buf, "EMPTY");
                } else {
                    const HashTableEntry& entry = entries.value().get(i);
                    size_t distance = this->distance_from_ideal(data, entry.hash(), current_bucket);

                    fmt::format_to(buf, "{} (distance {})", i, distance);
                }
                fmt::format_to(buf, "\n");
            }
        });
    }

    return to_string(buf);
}

template<typename Derived>
Derived HashTableViewBase<Derived>::make(Context& ctx, Handle<HashTable> table) {
    Layout* data = create_object<Derived>(ctx, StaticSlotsInit());
    data->write_static_slot(TableSlot, table);
    return Derived(from_heap(data));
}

template<typename Derived>
HashTable HashTableViewBase<Derived>::table() {
    return layout()->template read_static_slot<HashTable>(TableSlot);
}

template class HashTableViewBase<HashTableKeyView>;
template class HashTableViewBase<HashTableValueView>;

template<typename Derived>
Derived HashTableIteratorBase<Derived>::make(Context& ctx, Handle<HashTable> table) {
    Layout* data = create_object<Derived>(ctx, StaticSlotsInit(), StaticPayloadInit());
    data->write_static_slot(TableSlot, table);
    return Derived(from_heap(data));
}

template<typename Derived>
std::optional<Value> HashTableIteratorBase<Derived>::next(Context& ctx) {
    Layout* data = layout();
    HashTable table = data->template read_static_slot<HashTable>(TableSlot);
    auto next = table.iterator_next(data->static_payload()->entry_index);
    if (!next)
        return {};

    return derived().return_value(ctx, next->first, next->second);
}

template class HashTableIteratorBase<HashTableIterator>;
template class HashTableIteratorBase<HashTableKeyIterator>;
template class HashTableIteratorBase<HashTableValueIterator>;

Value HashTableIterator::return_value(Context& ctx, Value key, Value value) {
    // TODO performance, reuse the same tuple every time?
    // XXX: key/value must be rooted before performing any allocations.
    Scope sc(ctx);
    Local rooted_key = sc.local(key);
    Local rooted_value = sc.local(value);
    return Tuple::make(ctx, {rooted_key, rooted_value});
}

Value HashTableKeyIterator::return_value(
    [[maybe_unused]] Context& ctx, Value key, [[maybe_unused]] Value value) {
    return key;
}

Value HashTableValueIterator::return_value(
    [[maybe_unused]] Context& ctx, [[maybe_unused]] Value key, Value value) {
    return value;
}

static constexpr MethodDesc hash_table_methods[] = {
    {
        "size"sv,
        1,
        [](NativeFunctionFrame& frame) {
            auto table = check_instance<HashTable>(frame);
            i64 size = static_cast<i64>(table->size());
            frame.result(frame.ctx().get_integer(size));
        },
    },
    {
        "contains"sv,
        2,
        [](NativeFunctionFrame& frame) {
            auto table = check_instance<HashTable>(frame);
            bool result = table->contains(*frame.arg(1));
            frame.result(frame.ctx().get_boolean(result));
        },
    },
    {
        "keys"sv,
        1,
        [](NativeFunctionFrame& frame) {
            auto table = check_instance<HashTable>(frame);
            frame.result(HashTableKeyView::make(frame.ctx(), table));
        },
    },
    {
        "values"sv,
        1,
        [](NativeFunctionFrame& frame) {
            auto table = check_instance<HashTable>(frame);
            frame.result(HashTableValueView::make(frame.ctx(), table));
        },
    },
    {
        "clear"sv,
        1,
        [](NativeFunctionFrame& frame) {
            auto table = check_instance<HashTable>(frame);
            table->clear();
        },
    },
    {
        "remove"sv,
        2,
        [](NativeFunctionFrame& frame) {
            auto table = check_instance<HashTable>(frame);
            table->remove(*frame.arg(1));
        },
    },
};

constexpr TypeDesc hash_table_type_desc{"Map"sv, hash_table_methods};

} // namespace tiro::vm
