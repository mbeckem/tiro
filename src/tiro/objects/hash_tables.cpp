#include "tiro/objects/hash_tables.hpp"

#include "tiro/objects/arrays.hpp"
#include "tiro/objects/buffers.hpp"
#include "tiro/vm/context.hpp"

#include "tiro/objects/hash_tables.ipp"
#include "tiro/vm/context.ipp"

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
        TIRO_ASSERT(is_aligned(reinterpret_cast<uintptr_t>(buffer.data()),
                        static_cast<uintptr_t>(alignof(IndexType))),
            "Buffer must be aligned correctly.");
        std::uninitialized_fill_n(data(buffer), size, initial);
        return buffer;
    }

    static Span<IndexType> values(Buffer buffer) {
        return {data(buffer), size(buffer)};
    }

    static IndexType* data(Buffer buffer) {
        return reinterpret_cast<IndexType*>(buffer.data());
    }

    static size_t size(Buffer buffer) {
        size_t size_in_bytes = buffer.size();
        TIRO_ASSERT(size_in_bytes % sizeof(IndexType) == 0,
            "Byte size must always be a multiple of the data type size.");
        return size_in_bytes / sizeof(IndexType);
    }
};

template<>
struct SizeClassTraits<HashTable::SizeClass::U8> {
    using BufferAccess = BufferView<u8>;
    using IndexType = u8;
    static constexpr IndexType empty_value =
        std::numeric_limits<IndexType>::max();
};

template<>
struct SizeClassTraits<HashTable::SizeClass::U16> {
    using BufferAccess = BufferView<u16>;
    using IndexType = u16;
    static constexpr IndexType empty_value =
        std::numeric_limits<IndexType>::max();
};

template<>
struct SizeClassTraits<HashTable::SizeClass::U32> {
    using BufferAccess = BufferView<u32>;
    using IndexType = u32;
    static constexpr IndexType empty_value =
        std::numeric_limits<IndexType>::max();
};

template<>
struct SizeClassTraits<HashTable::SizeClass::U64> {
    using BufferAccess = BufferView<u64>;
    using IndexType = u64;
    static constexpr IndexType empty_value =
        std::numeric_limits<IndexType>::max();
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
    TIRO_ASSERT(
        is_pow2(index_size), "Index size must always be a power of two.");
    TIRO_ASSERT(index_size >= initial_index_capacity, "Index size too small.");
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
static decltype(auto)
dispatch_size_class(HashTable::SizeClass size_class, Func&& fn) {
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
    TIRO_ASSERT(index < Traits::empty_value,
        "Index must fit into the target index type.");
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

HashTableIterator
HashTableIterator::make(Context& ctx, Handle<HashTable> table) {
    TIRO_ASSERT(table.get(), "Invalid table reference.");

    Data* d = ctx.heap().create<Data>(table);
    return HashTableIterator(from_heap(d));
}

Value HashTableIterator::next(Context& ctx) const {
    Data* d = access_heap();

    // TODO performance, reuse the same tuple every time?
    Root<Value> key(ctx);
    Root<Value> value(ctx);
    bool more = d->table.iterator_next(
        d->entry_index, key.mut_handle(), value.mut_handle());
    if (!more) {
        return ctx.get_stop_iteration();
    }

    return Tuple::make(ctx, {key.handle(), value.handle()});
}

HashTable HashTable::make(Context& ctx) {
    Data* d = ctx.heap().create<Data>();
    return HashTable(from_heap(d));
}

HashTable HashTable::make(Context& ctx, size_t initial_capacity) {
    Root<HashTable> table(ctx, HashTable::make(ctx));
    if (initial_capacity == 0)
        return table.get();

    size_t index_cap = index_capacity_for_entries_capacity(initial_capacity);
    size_t entries_cap = table_capacity_for_index_capacity(index_cap);
    TIRO_ASSERT(entries_cap >= initial_capacity,
        "Capacity calculation wrong: not enough space.");

    table->grow_to_capacity<SizeClassTraits<SizeClass::U8>>(
        table->access_heap(), ctx, entries_cap, index_cap);
    return table.get();
}

size_t HashTable::size() const {
    return access_heap()->size;
}

size_t HashTable::occupied_entries() const {
    Data* d = access_heap();
    if (!d->entries)
        return 0;
    return d->entries.size();
}

size_t HashTable::entry_capacity() const {
    Data* d = access_heap();
    if (!d->entries)
        return 0;
    return d->entries.capacity();
}

size_t HashTable::index_capacity() const {
    Data* d = access_heap();
    if (!d->indices)
        return 0;

    return dispatch_size_class(index_size_class(d), [&](auto traits) -> size_t {
        using ST = decltype(traits);
        return ST::BufferAccess::size(d->indices);
    });
}

bool HashTable::contains(Value key) const {
    Data* const d = access_heap();
    if (d->size == 0) {
        return false;
    }

    return dispatch_size_class(index_size_class(d), [&](auto traits) {
        auto pos = this->template find_impl<decltype(traits)>(d, key);
        return pos.has_value();
    });
}

std::optional<Value> HashTable::get(Value key) const {
    Data* const d = access_heap();
    if (d->size == 0) {
        return {};
    }

    const auto pos = dispatch_size_class(index_size_class(d), [&](auto traits) {
        return this->template find_impl<decltype(traits)>(d, key);
    });

    if (!pos) {
        return {};
    }

    const size_t entry_index = pos->second;
    TIRO_ASSERT(entry_index < d->entries.size(), "Invalid entry index.");

    const HashTableEntry& entry = d->entries.get(entry_index);
    TIRO_ASSERT(!entry.is_deleted(), "Found entry must not be deleted.");
    return entry.value();
}

bool HashTable::find(Handle<Value> key, MutableHandle<Value> existing_key,
    MutableHandle<Value> existing_value) {
    Data* const d = access_heap();
    if (d->size == 0) {
        return false;
    }

    const auto pos = dispatch_size_class(index_size_class(d), [&](auto traits) {
        return this->template find_impl<decltype(traits)>(d, key);
    });

    if (!pos) {
        return false;
    }

    const size_t entry_index = pos->second;
    TIRO_ASSERT(entry_index < d->entries.size(), "Invalid entry index.");

    const HashTableEntry& entry = d->entries.get(entry_index);
    TIRO_ASSERT(!entry.is_deleted(), "Found entry must not be deleted.");
    existing_key.set(entry.key());
    existing_value.set(entry.value());
    return true;
}

void HashTable::set(
    Context& ctx, Handle<Value> key, Handle<Value> value) const {
    TIRO_TABLE_TRACE(
        "Insert {} -> {}", to_string(key.get()), to_string(value.get()));

    Data* const d = access_heap();
    ensure_free_capacity(d, ctx);
    dispatch_size_class(index_size_class(d), [&](auto traits) {
        return this->template set_impl<decltype(traits)>(d, key, value);
    });
}

void HashTable::remove(Handle<Value> key) const {
    TIRO_TABLE_TRACE("Remove {}", to_string(key.get()));

    Data* const d = access_heap();
    if (d->size == 0) {
        return;
    }

    dispatch_size_class(index_size_class(d), [&](auto traits) {
        this->template remove_impl<decltype(traits)>(d, key);
    });
}

HashTableIterator HashTable::make_iterator(Context& ctx) {
    return HashTableIterator::make(ctx, Handle<HashTable>::from_slot(this));
}

bool HashTable::is_packed() const {
    Data* const data = access_heap();
    if (data->size == 0) {
        return true;
    }

    return data->size == data->entries.size();
}

bool HashTable::iterator_next(size_t& entry_index, MutableHandle<Value> key,
    MutableHandle<Value> value) const {
    HashTableStorage entries_storage = access_heap()->entries;

    // TODO modcount
    Span<const HashTableEntry> entries = entries_storage.values();
    TIRO_CHECK(entry_index <= entries.size(),
        "Invalid iterator position, was the table modified during iteration?");

    while (entry_index < entries.size()) {
        const HashTableEntry& entry = entries[entry_index++];
        if (!entry.is_deleted()) {
            key.set(entry.key());
            value.set(entry.value());
            return true;
        }
    }
    return false;
}

template<typename ST>
void HashTable::set_impl(Data* d, Value key, Value value) const {
    const auto indices = index_values<ST>(d->indices);
    const Hash key_hash = HashTableEntry::make_hash(key);

    TIRO_ASSERT(d->size < indices.size(),
        "There must be at least one free slot in the index table.");
    TIRO_ASSERT(d->entries && !d->entries.full(),
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
    auto index_to_insert = cast_index<ST>(d->entries.size());
    size_t bucket_index = bucket_for_hash(d, key_hash);
    size_t distance = 0;

    TIRO_TABLE_TRACE("Inserting index {}, ideal bucket is {}", index_to_insert,
        bucket_index);

    while (1) {
        auto& index = indices[bucket_index];

        if (index == ST::empty_value) {
            index = index_to_insert;
            TIRO_TABLE_TRACE("Inserted into free bucket {}", bucket_index);
            break; // Case 2.
        }

        const auto& entry = d->entries.get(static_cast<size_t>(index));
        const Hash entry_hash = entry.hash();
        size_t entry_distance = distance_from_ideal(
            d, entry_hash, bucket_index);

        if (entry_distance < distance) {
            slot_stolen = true;
            std::swap(index_to_insert, index);
            std::swap(distance, entry_distance);
            TIRO_TABLE_TRACE(
                "Robin hood swap with index {}, new distance is {}",
                index_to_insert, distance);
            break; // Case 3.
        }

        if (entry_hash.value == key_hash.value && key_equal(entry.key(), key)) {
            d->entries.set(index, HashTableEntry(key_hash, entry.key(), value));
            TIRO_TABLE_TRACE("Existing key was overwritten.");
            return; // Case 1.
        }

        bucket_index = next_bucket(d, bucket_index);
        distance += 1;
        TIRO_TABLE_TRACE("Continuing with bucket {} and distance {}",
            bucket_index, distance);
    }

    d->entries.append(HashTableEntry(key_hash, key, value));
    d->size += 1;

    if (slot_stolen) {
        // Continuation from case 3.
        while (1) {
            bucket_index = next_bucket(d, bucket_index);
            distance += 1;

            auto& index = indices[bucket_index];
            if (index == ST::empty_value) {
                index = index_to_insert;
                TIRO_TABLE_TRACE(
                    "Inserted stolen index into free bucket {}", bucket_index);
                break;
            }

            const auto& entry = d->entries.get(static_cast<size_t>(index));
            size_t entry_distance = distance_from_ideal(
                d, entry.hash(), bucket_index);
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
}

template<typename ST>
void HashTable::remove_impl(Data* d, Value key) const {
    static constexpr HashTableEntry sentinel = HashTableEntry::make_deleted();

    const auto found = find_impl<ST>(d, key);
    if (!found) {
        return;
    }

    TIRO_ASSERT(d->size > 0, "Cannot be empty if a value has been found.");
    const auto [removed_bucket, removed_entry] = *found;

    // Mark the entry as deleted. We can just pop if this was the last element,
    // otherwise we have to leave a hole.
    if (removed_entry == d->entries.size() - 1) {
        d->entries.remove_last();
    } else {
        d->entries.set(removed_entry, sentinel);
    }

    d->size -= 1;
    if (d->size == 0) {
        // We know that we can start from the beginning since we're empty.
        d->entries.clear();
    }

    // Erase the reference in the index array.
    remove_from_index<ST>(d, removed_bucket);

    // Close holes if 50% or more of the entries in the table have been deleted.
    if (d->size <= d->entries.size() / 2) {
        compact<ST>(d);
    }
}

template<typename ST>
void HashTable::remove_from_index(Data* d, size_t erased_bucket) const {
    const auto indices = index_values<ST>(d->indices);
    indices[erased_bucket] = ST::empty_value;

    size_t current_bucket = next_bucket(d, erased_bucket);
    while (1) {
        auto& index = indices[current_bucket];
        if (index == ST::empty_value) {
            break;
        }

        const auto& entry = d->entries.get(index);
        const size_t entry_distance = distance_from_ideal(
            d, entry.hash(), current_bucket);
        if (entry_distance > 0) {
            TIRO_ASSERT(distance_from_ideal(d, entry.hash(), erased_bucket)
                            <= entry_distance,
                "Backshift invariant: distance does not get worse.");
            indices[erased_bucket] = index;
            indices[current_bucket] = ST::empty_value;
        } else {
            break;
        }
    }
}

template<typename ST>
std::optional<std::pair<size_t, size_t>>
HashTable::find_impl(Data* d, Value key) const {
    const auto indices = index_values<ST>(d->indices);
    const Hash key_hash = HashTableEntry::make_hash(key);

    size_t bucket_index = bucket_for_hash(d, key_hash);
    size_t distance = 0;
    while (1) {
        const auto& index = indices[bucket_index];
        if (index == ST::empty_value) {
            return {};
        }

        // Improvement: storing some bits of the hash together with the
        // index would reduce the number of random-access-like dereferences
        // into the entries array.
        const auto& entry = d->entries.get(index);
        const Hash entry_hash = entry.hash();
        if (distance > distance_from_ideal(d, entry_hash, bucket_index)) {
            // If we were in the hash table, we would have enounctered ourselves
            // already: we would have swapped us into this bucket!
            // This is the invariant established by robin hood insertion.
            return {};
        }

        if (entry_hash.value == key_hash.value && key_equal(entry.key(), key)) {
            return std::pair(bucket_index, index);
        }

        bucket_index = next_bucket(d, bucket_index);
        distance += 1;
    }
}

// Makes sure that at least one slot is available at the end of the entries array.
// Also makes sure that at least one slot is available in the index table.
// Note: index and entries arrays currently grow together (with the index array
// having a higher number of slots). This could change in the future to improve performance.
void HashTable::ensure_free_capacity(Data* d, Context& ctx) const {
    // Invariant: d->entries.capacity() <= d->indices.size(), i.e.
    // the index table is always at least as large as the entries array.

    if (!d->entries) {
        init_first(d, ctx);
        return;
    }

    TIRO_ASSERT(
        d->entries.capacity() > 0, "Entries array must not have 0 capacity.");
    if (d->entries.full()) {
        const bool should_grow = (d->size / 3) >= (d->entries.capacity() / 4);

        dispatch_size_class(index_size_class(d), [&](auto traits) {
            if (should_grow) {
                this->template grow<decltype(traits)>(d, ctx);
            } else {
                this->template compact<decltype(traits)>(d);
            }
        });
    }

    TIRO_ASSERT(!d->entries.full(), "Must have made room for a new element.");
}

void HashTable::init_first(Data* d, Context& ctx) const {
    using InitialSizeClass = SizeClassTraits<SizeClass::U8>;

    TIRO_TABLE_TRACE("Initializing hash table to initial capacity");
    d->entries = HashTableStorage::make(ctx, initial_table_capacity);
    d->indices = InitialSizeClass::BufferAccess::make(
        ctx, initial_index_capacity, InitialSizeClass::empty_value);
    d->size = 0;
    d->mask = initial_index_capacity - 1;
}

template<typename ST>
void HashTable::grow(Data* d, Context& ctx) const {
    TIRO_ASSERT(d->entries, "Entries array must not be null.");
    TIRO_ASSERT(d->indices, "Indices table must not be null.");

    TIRO_ASSERT(this->index_capacity() >= initial_index_capacity,
        "Invalid index size (too small).");

    size_t new_index_cap = grow_index_capacity(this->index_capacity());
    size_t new_entry_cap = table_capacity_for_index_capacity(new_index_cap);
    grow_to_capacity<ST>(d, ctx, new_entry_cap, new_index_cap);
}

template<typename ST>
void HashTable::grow_to_capacity(Data* d, Context& ctx,
    size_t new_entry_capacity, size_t new_index_capacity) const {
    TIRO_ASSERT(new_entry_capacity > this->entry_capacity(),
        "Must grow to a larger entry capacity.");
    TIRO_ASSERT(new_index_capacity > this->index_capacity(),
        "Must grow to a larger index capacity.");
    TIRO_ASSERT(d->size == 0 || !d->entries.is_null(),
        "Either empty or non-null entries array.");

    TIRO_TABLE_TRACE(
        "Growing table from {} entries to {} entries ({} index slots)",
        entry_capacity(), new_entry_capacity, new_index_capacity);

    Root<HashTableStorage> new_entries(ctx);
    if (d->size == 0) {
        new_entries.set(HashTableStorage::make(ctx, new_entry_capacity));
    } else if (d->size == d->entries.size()) {
        new_entries.set(HashTableStorage::make(
            ctx, d->entries.values(), new_entry_capacity));
    } else {
        new_entries.set(HashTableStorage::make(ctx, new_entry_capacity));
        for (const HashTableEntry& entry : d->entries.values()) {
            if (!entry.is_deleted())
                new_entries->append(entry);
        }
    }
    d->entries = new_entries;

    // TODO: make rehashing cheaper by reusing the old index table...
    const SizeClass next_size_class = index_size_class(new_entry_capacity);
    dispatch_size_class(next_size_class, [&](auto traits) {
        this->template recreate_index<decltype(traits)>(
            d, ctx, new_index_capacity);
    });
}

template<typename ST>
void HashTable::compact(Data* d) const {
    TIRO_ASSERT(d->entries, "Entries array must not be null.");

    if (d->entries.size() == d->size) {
        return; // No holes.
    }

    const size_t size = d->entries.size();
    TIRO_TABLE_TRACE(
        "Compacting table from size {} to {}.", d->entries.size(), d->size);

    auto find_deleted_entry = [&] {
        for (size_t i = 0; i < size; ++i) {
            const auto& entry = d->entries.get(i);
            if (entry.is_deleted())
                return i;
        }
        TIRO_UNREACHABLE("There must be a deleted entry.");
    };

    size_t write_pos = find_deleted_entry();
    for (size_t read_pos = write_pos + 1; read_pos < size; ++read_pos) {
        const auto& entry = d->entries.get(read_pos);
        if (!entry.is_deleted()) {
            d->entries.set(write_pos, entry);
            ++write_pos;
        }
    }

    d->entries.remove_last(size - write_pos);
    TIRO_ASSERT(d->entries.size() == d->size, "Must have packed all entries.");

    // TODO inefficient
    auto indices = index_values<ST>(d->indices);
    std::fill(indices.begin(), indices.end(), ST::empty_value);
    rehash_index<ST>(d);
}

template<typename ST>
void HashTable::recreate_index(Data* d, Context& ctx, size_t capacity) const {
    TIRO_ASSERT(d->size == d->entries.size(),
        "Entries array must not have any deleted elements.");
    TIRO_ASSERT(
        is_pow2(capacity), "New index capacity must be a power of two.");

    // TODO rehashing can be made faster, see rust index map at https://github.com/bluss/indexmap
    d->indices = ST::BufferAccess::make(ctx, capacity, ST::empty_value);
    d->mask = capacity - 1;
    rehash_index<ST>(d);
}

template<typename ST>
void HashTable::rehash_index(Data* d) const {
    TIRO_ASSERT(d->entries, "Entries array must not be null.");
    TIRO_ASSERT(d->indices, "Indices table must not be null.");

    TIRO_TABLE_TRACE("Rehashing table index");

    // TODO deduplicate code with insert
    const auto entries = d->entries.values();
    const auto indices = index_values<ST>(d->indices);
    for (size_t entry_index = 0; entry_index < entries.size(); ++entry_index) {
        const HashTableEntry& entry = entries[entry_index];

        auto index_to_insert = cast_index<ST>(entry_index);
        size_t bucket_index = bucket_for_hash(d, entry.hash());
        size_t distance = 0;
        while (1) {
            auto& index = indices[bucket_index];
            if (index == ST::empty_value) {
                index = index_to_insert;
                break;
            }

            const auto& other_entry = d->entries.get(
                static_cast<size_t>(index));
            size_t other_entry_distance = distance_from_ideal(
                d, other_entry.hash(), bucket_index);
            if (other_entry_distance < distance) {
                std::swap(index_to_insert, index);
                std::swap(distance, other_entry_distance);
            }

            bucket_index = next_bucket(d, bucket_index);
            distance += 1;
        }
    }
}

size_t HashTable::next_bucket(Data* d, size_t current_bucket) const {
    TIRO_ASSERT(!d->indices.is_null(), "Must have an index table.");
    return (current_bucket + 1) & d->mask;
}

size_t HashTable::bucket_for_hash(Data* d, Hash hash) const {
    TIRO_ASSERT(!d->indices.is_null(), "Must have an index table.");
    return hash.value & d->mask;
}

size_t HashTable::distance_from_ideal(
    Data* d, Hash hash, size_t current_bucket) const {
    size_t desired_bucket = bucket_for_hash(d, hash);
    return (current_bucket - desired_bucket) & d->mask;
}

HashTable::SizeClass HashTable::index_size_class(Data* d) const {
    TIRO_ASSERT(d->entries,
        "Must have a valid entries table in order to have an index.");
    size_t capacity = d->entries.capacity();
    return index_size_class(capacity);
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

std::string HashTable::dump() const {
    Data* d = access_heap();

    fmt::memory_buffer buf;
    fmt::format_to(buf, "Hash table @{}\n", (void*) access_heap());
    fmt::format_to(buf,
        "  Size: {}\n"
        "  Capacity: {}\n"
        "  Mask: {}\n",
        d->size, d->entries.capacity(), d->mask);

    fmt::format_to(buf, "  Entries:\n");
    if (!d->entries) {
        fmt::format_to(buf, "    NULL\n");
    } else {
        const size_t count = d->entries.size();
        for (size_t i = 0; i < count; ++i) {
            const HashTableEntry& entry = d->entries.get(i);
            fmt::format_to(buf, "    {}: ", i);
            if (entry.is_deleted()) {
                fmt::format_to(buf, "<DELETED>\n");
            } else {
                fmt::format_to(buf, "{} -> {} (Hash {})\n",
                    to_string(entry.key()), to_string(entry.value()),
                    entry.hash().value);
            }
        }
    }

    fmt::format_to(buf, "  Indices:\n");
    if (!d->indices) {
        fmt::format_to(buf, "    NULL\n");
    } else {
        fmt::format_to(buf, "    Type: {}\n", to_string(d->indices.type()));
        dispatch_size_class(index_size_class(d), [&](auto traits) {
            using Traits = decltype(traits);

            auto indices = Traits::BufferAccess::values(d->indices);
            for (size_t current_bucket = 0; current_bucket < indices.size();
                 ++current_bucket) {
                auto index = indices[current_bucket];
                fmt::format_to(buf, "    {}: ", current_bucket);
                if (index == Traits::empty_value) {
                    fmt::format_to(buf, "EMPTY");
                } else {
                    const HashTableEntry& entry = d->entries.get(index);
                    size_t distance = this->distance_from_ideal(
                        d, entry.hash(), current_bucket);

                    fmt::format_to(buf, "{} (distance {})", index, distance);
                }
                fmt::format_to(buf, "\n");
            }
        });
    }

    return to_string(buf);
}

} // namespace tiro::vm
