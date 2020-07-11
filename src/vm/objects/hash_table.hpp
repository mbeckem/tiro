#ifndef TIRO_VM_OBJECTS_HASH_TABLE_HPP
#define TIRO_VM_OBJECTS_HASH_TABLE_HPP

#include "common/math.hpp"
#include "vm/handles/handle.hpp"
#include "vm/handles/scope.hpp"
#include "vm/objects/array_storage_base.hpp"
#include "vm/objects/layout.hpp"
#include "vm/objects/type_desc.hpp"
#include "vm/objects/value.hpp"

#include <optional>

namespace tiro::vm {

/// Represents a hash table's key/value pairs. Hash values are embedded into the struct.
struct HashTableEntry {
private:
    static constexpr size_t deleted_value = size_t(-1);

public:
    // This type prevents misuse of "raw" hashes.
    struct Hash {
        size_t value;
    };

    // Constructs a hash value by discarding reserved bits and bit patterns
    // from the given raw hash. The result is always valid for hash buckets.
    static Hash make_hash(size_t raw_hash);
    static Hash make_hash(Value value);

    // Constructs a deleted hash table entry.
    static constexpr HashTableEntry make_deleted() {
        HashTableEntry entry;
        entry.hash_ = deleted_value;
        entry.key_ = Value::null();
        entry.value_ = Value::null();
        return entry;
    }

    // Constructs a new entry. The entry will not have its deleted flag set.
    HashTableEntry(Hash hash, Value key, Value value)
        : hash_(hash.value)
        , key_(key)
        , value_(value) {
        TIRO_DEBUG_ASSERT(hash_ != deleted_value, "Constructed a deleted entry.");
    }

    bool is_deleted() const noexcept { return hash_ == deleted_value; }

    Hash hash() const {
        TIRO_DEBUG_ASSERT(!is_deleted(), "Cannot retrieve the hash of an deleted entry.");
        return Hash{hash_};
    }

    Value key() const { return key_; }
    Value value() const { return value_; }

    template<typename W>
    void walk(W&& w) {
        w(key_);
        w(value_);
    }

private:
    constexpr HashTableEntry()
        : hash_()
        , key_()
        , value_() {}

private:
    size_t hash_;
    Value key_;
    Value value_;
};

/// The backing storage for the entries of a hash table.
/// The entries are kept in insertion order in a contiugous block of memory.
/// Deleted entries leave holes in the array which are eventually closed
/// by either packing the array or by copying it into a larger array.
///
/// Entries are tuples (key_hash, key, value). Deleted entries are represented
/// using a single bit of the key_hash.
class HashTableStorage final : public ArrayStorageBase<HashTableEntry, HashTableStorage> {
public:
    using ArrayStorageBase::ArrayStorageBase;
};

/// Iterator for hash tables.
///
/// TODO: Modcount support to protect against simultaneous modifications?
class HashTableIterator final : public HeapValue {
private:
    enum Slots {
        TableSlot,
        SlotCount_,
    };

    struct Payload {
        size_t entry_index = 0;
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>, StaticPayloadPiece<Payload>>;

    static HashTableIterator make(Context& ctx, Handle<HashTable> table);

    explicit HashTableIterator(Value v)
        : HeapValue(v, DebugCheck<HashTableIterator>()) {}

    /// Returns the next value, or the stop iteration value from ctx.
    Value next(Context& ctx);

    Layout* layout() const { return access_heap<Layout>(); }
};

/// A general purpose hash table implemented using robin hood hashing.
///
/// TODO: Table never shrinks right now.
/// TODO: Table entries array growth factor?
///
/// See also:
///  - https://www.sebastiansylvan.com/post/robin-hood-hashing-should-be-your-default-hash-table-implementation/
///  - https://gist.github.com/ssylvan/5538011
///  - https://programming.guide/robin-hood-hashing.html
///  - https://github.com/Tessil/robin-map
///
/// For deletion algorithm:
///  - http://codecapsule.com/2013/11/17/robin-hood-hashing-backward-shift-deletion/comment-page-1/
///
/// For the extra indirection employed by indices array:
///  - https://www.youtube.com/watch?v=npw4s1QTmPg
///  - https://mail.python.org/pipermail/python-dev/2012-December/123028.html
///  - https://morepypy.blogspot.com/2015/01/faster-more-memory-efficient-and-more.html
///  - https://github.com/bluss/indexmap
class HashTable final : public HeapValue {
private:
    enum Slots {
        // Raw array buffer storing indices into the entries table.
        // The layout depends on the number of entries (e.g. compact 1 byte indices
        // are used for small hash tables).
        IndexSlot,

        // Stores the entries in insertion order.
        EntriesSlot,
        SlotCount_,
    };

    struct Payload {
        // Number of actual entries in this hash table.
        // There can be holes in the storage if entries have been deleted.
        size_t size = 0;

        // Mask for bucket index modulus computation. Derived from `indicies.size()`.
        size_t mask = 0;
    };

public:
    enum class SizeClass { U8, U16, U32, U64 };

    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>, StaticPayloadPiece<Payload>>;

    static HashTable make(Context& ctx);
    static HashTable make(Context& ctx, size_t initial_capacity);
    // TODO: With initial capacity overload

    explicit HashTable(Value v)
        : HeapValue(v, DebugCheck<HashTable>()) {}

    /// Returns the number of (key, value) pairs in the table.
    size_t size();

    /// Returns the number of entry slots that are occupied by either
    /// live or deleted entries.
    size_t occupied_entries();

    /// Number of occupied entries (live or deleted) that can be supported by the
    /// current table without reallocation.
    size_t entry_capacity();

    /// The current number of buckets in the hash table's index.
    size_t index_capacity();

    /// True iff the hash table is empty.
    bool empty() { return size() == 0; }

    /// Returns true iff key is in the table.
    bool contains(Value key);

    /// Returns the value associated with the given key.
    // TODO key error when key not in map?
    std::optional<Value> get(Value key);

    /// Attempts to find the given key in the map and returns true if it was found.
    /// If the key was found, the existing key and value will be stored in the given handles.
    bool find(Handle<Value> key, OutHandle<Value> existing_key, OutHandle<Value> existing_value);

    /// Associates the given key with the given value.
    /// If there is already an existing entry for the given key,
    /// the old value will be overwritten.
    // TODO maybe return old value?
    void set(Context& ctx, Handle<Value> key, Handle<Value> value);

    /// Removes the given key (and the value associated with it) from the table.
    // TODO old value?
    void remove(Handle<Value> key);

    /// Removes all elements from the hash table.
    void clear();

    /// Returns a new iterator for this table.
    HashTableIterator make_iterator(Context& ctx);

    /// Returns true iff the entries in the table are packed, i.e. if
    /// there are no holes left by deleted entries.
    bool is_packed();

    /// Packs the entries of this table. This closes holes left
    /// behind by previous deletions. Packing is usually done
    /// automatically, it is only exposed for testing.
    void pack();

    /// Invokes the passed function for every key / value pair
    /// in this hash table.
    template<typename Function>
    void for_each(Context& ctx, Function&& fn) {
        (void) ctx;
        Scope sc(ctx);
        Local key = sc.local();
        Local value = sc.local();

        size_t index = 0;
        while (iterator_next(index, key.mut(), value.mut())) {
            fn(static_cast<Handle<Value>>(key), static_cast<Handle<Value>>(value));
        }
    }

    std::string dump();

    Layout* layout() const { return access_heap<Layout>(); }

private:
    using Hash = HashTableEntry::Hash;

private:
    // API used by the iterator class
    friend HashTableIterator;

    bool iterator_next(size_t& entry_index, OutHandle<Value> key, OutHandle<Value> value);

private:
    template<typename ST>
    void set_impl(Layout* data, Value key, Value value);

    template<typename ST>
    void remove_impl(Layout* data, Value key);

    // Called after the successful removal of an entry to close holes
    // in the index array. Bucket content is shifted backwards until
    // we find a hole or an entry at its ideal position.
    template<typename ST>
    void remove_from_index(Layout* data, size_t erased_bucket);

    template<typename ST>
    void clear_impl(Layout* data);

    // Attempts to find the given key. Returns (bucket_index, entry_index)
    // if the key was found.
    template<typename ST>
    std::optional<std::pair<size_t, size_t>> find_impl(Layout* data, Value key);

    // Make sure at least one slot is available for new entries.
    void ensure_free_capacity(Layout* data, Context& ctx);

    // Initialize to non-empty table. This is the first allocation.
    void init_first(Layout* data, Context& ctx);

    // Grows the entries array and the index table.
    // This currently makes rehashing necessary.
    template<typename ST>
    void grow(Layout* data, Context& ctx);

    template<typename ST>
    void grow_to_capacity(Layout* data, Context& ctx, size_t new_entry_cap, size_t new_index_cap);

    // Performs in-place compaction by shifting elements into storage locations
    // that are still occupied by deleted elements.
    template<typename ST>
    void compact(Layout* data);

    // Creates a new index table from an existing entries array.
    // This could be optimized further by using the old index table (?).
    template<typename ST>
    void recreate_index(Layout* data, Context& ctx, size_t capacity);

    // Creates the index from scratch using the existing index array.
    // The index array should have been cleared (if reused) or initialized
    // with empty bucket values (if new).
    // TODO: Take advantage of the old index array and don't do a complete rehash
    // TODO: internal api design is bad.
    template<typename ST>
    void rehash_index(Layout* data);

    // Returns the next bucket index after current_bucket.
    size_t next_bucket(Layout* data, size_t current_bucket);

    // Returns the ideal bucket index for that hash value.
    size_t bucket_for_hash(Layout* data, Hash hash);

    // Returns the distance of current_bucket from hash's ideal bucket.
    size_t distance_from_ideal(Layout* data, Hash hash, size_t current_bucket);

    // Returns the current size class.
    SizeClass index_size_class(Layout* data);

    Nullable<HashTableStorage> get_entries(Layout* data);
    void set_entries(Layout* data, Nullable<HashTableStorage> entries);

    Nullable<Buffer> get_index(Layout* data);
    void set_index(Layout* data, Nullable<Buffer> index);

    // Returns the size class for the given entries capacity.
    static SizeClass index_size_class(size_t entry_count);

    // True if the keys are considered equal. Fast path for keys that are bit-identical.
    static bool key_equal(Value a, Value b) { return a.same(b) || equal(a, b); }
};

extern const TypeDesc hash_table_type_desc;

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_HASH_TABLE_HPP
