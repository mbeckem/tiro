#ifndef HAMMER_VM_OBJECTS_HASH_TABLES_HPP
#define HAMMER_VM_OBJECTS_HASH_TABLES_HPP

#include "hammer/core/math.hpp"
#include "hammer/vm/objects/arrays.hpp"
#include "hammer/vm/objects/object.hpp"
#include "hammer/vm/objects/value.hpp"

#include <optional>

namespace hammer::vm {

/**
 * Represents a hash table's key/value pairs. Hash values are embedded into the struct.
 */
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
        HAMMER_ASSERT(hash_ != deleted_value, "Constructed a deleted entry.");
    }

    bool is_deleted() const noexcept { return hash_ == deleted_value; }

    Hash hash() const {
        HAMMER_ASSERT(
            !is_deleted(), "Cannot retrieve the hash of an deleted entry.");
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

/**
 * The backing storage for the entries of a hash table.
 * The entries are kept in insertion order in a contiugous block of memory.
 * Deleted entries leave holes in the array which are eventually closed
 * by either packing the array or by copying it into a larger array.
 *
 * Entries are tuples (key_hash, key, value). Deleted entries are represented
 * using a single bit of the key_hash.
 */
class HashTableStorage final
    : public ArrayStorageBase<HashTableEntry, HashTableStorage> {
public:
    using ArrayStorageBase::ArrayStorageBase;
};

/**
 * Iterator for hash tables.
 * 
 * TODO: Modcount support to protect against simultaneous modifications?
 */
class HashTableIterator final : public Value {
public:
    static HashTableIterator make(Context& ctx, Handle<HashTable> table);

    HashTableIterator() = default;

    explicit HashTableIterator(Value v)
        : Value(v) {
        HAMMER_ASSERT(
            v.is<HashTableIterator>(), "Value is not a hash table iterator.");
    }

    /// Returns the next value, or the stop iteration value from ctx.
    Value next(Context& ctx) const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;

    inline Data* access_heap() const;
};

/**
 * A general purpose hash table implemented using robin hood hashing.
 * 
 * TODO: Table never shrinks right now.
 * TODO: Table entries array growth factor?
 * 
 * See also:
 *  - https://www.sebastiansylvan.com/post/robin-hood-hashing-should-be-your-default-hash-table-implementation/
 *  - https://gist.github.com/ssylvan/5538011
 *  - https://programming.guide/robin-hood-hashing.html
 *  - https://github.com/Tessil/robin-map
 * 
 * For deletion algorithm:
 *  - http://codecapsule.com/2013/11/17/robin-hood-hashing-backward-shift-deletion/comment-page-1/
 * 
 * For the extra indirection employed by indices array:
 *  - https://www.youtube.com/watch?v=npw4s1QTmPg
 *  - https://mail.python.org/pipermail/python-dev/2012-December/123028.html
 *  - https://morepypy.blogspot.com/2015/01/faster-more-memory-efficient-and-more.html
 *  - https://github.com/bluss/indexmap
 */
class HashTable final : public Value {
public:
    enum class SizeClass { U8, U16, U32, U64 };

public:
    static HashTable make(Context& ctx);
    static HashTable make(Context& ctx, size_t initial_capacity);
    // TODO: With initial capacity overload

    HashTable() = default;

    explicit HashTable(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<HashTable>(), "Value is not a hash table.");
    }

    /// Returns the number of (key, value) pairs in the table.
    size_t size() const;

    /// Returns the number of entry slots that are occupied by either
    /// live or deleted entries.
    size_t occupied_entries() const;

    /// Number of occupied entries (live or deleted) that can be supported by the
    /// current table without reallocation.
    size_t entry_capacity() const;

    /// The current number of buckets in the hash table's index.
    size_t index_capacity() const;

    /// True iff the hash table is empty.
    bool empty() const { return size() == 0; }

    /// Returns true iff key is in the table.
    bool contains(Value key) const;

    /// Returns the value associated with the given key.
    // TODO key error when key not in map?
    std::optional<Value> get(Value key) const;

    /// Attempts to find the given key in the map and returns true if it was found.
    /// If the key was found, the existing key and value will be stored in the given handles.
    bool find(Handle<Value> key, MutableHandle<Value> existing_key,
        MutableHandle<Value> existing_value);

    /// Associates the given key with the given value.
    /// If there is already an existing entry for the given key,
    /// the old value will be overwritten.
    // TODO maybe return old value?
    void set(Context& ctx, Handle<Value> key, Handle<Value> value) const;

    /// Removes the given key (and the value associated with it) from the table.
    // TODO old value?
    void remove(Handle<Value> key) const;

    /// Returns a new iterator for this table.
    HashTableIterator make_iterator(Context& ctx);

    /// Returns true iff the entries in the table are packed, i.e. if
    /// there are no holes left by deleted entries.
    bool is_packed() const;

    /// Packs the entries of this table. This closes holes left
    /// behind by previous deletions. Packing is usually done
    /// automatically, it is only exposed for testing.
    void pack();

    /// Invokes the passed function for every key / value pair
    /// in this hash table.
    template<typename Function>
    void for_each(Context& ctx, Function&& fn) {
        Root<Value> key(ctx);
        Root<Value> value(ctx);

        size_t index = 0;
        while (iterator_next(index, key.mut_handle(), value.mut_handle())) {
            fn(key.handle(), value.handle());
        }
    }

    void dump(std::ostream& os) const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    using Hash = HashTableEntry::Hash;

    struct Data;

private:
    // API used by the iterator class
    friend HashTableIterator;

    bool iterator_next(size_t& entry_index, MutableHandle<Value> key,
        MutableHandle<Value> value) const;

private:
    template<typename ST>
    void set_impl(Data* d, Value key, Value value) const;

    template<typename ST>
    void remove_impl(Data* d, Value key) const;

    // Called after the successful removal of an entry to close holes
    // in the index array. Bucket content is shifted backwards until
    // we find a hole or an entry at its ideal position.
    template<typename ST>
    void remove_from_index(Data* d, size_t erased_bucket) const;

    // Attempts to find the given key. Returns (bucket_index, entry_index)
    // if the key was found.
    template<typename ST>
    std::optional<std::pair<size_t, size_t>>
    find_impl(Data* d, Value key) const;

    // Make sure at least one slot is available for new entries.
    void ensure_free_capacity(Data* d, Context& ctx) const;

    // Initialize to non-empty table. This is the first allocation.
    void init_first(Data* d, Context& ctx) const;

    // Grows the entries array and the index table.
    // This currently makes rehashing necessary.
    template<typename ST>
    void grow(Data* d, Context& ctx) const;

    template<typename ST>
    void grow_to_capacity(Data* d, Context& ctx, size_t new_entry_cap,
        size_t new_index_cap) const;

    // Performs in-place compaction by shifting elements into storage locations
    // that are still occupied by deleted elements.
    template<typename ST>
    void compact(Data* d) const;

    // Creates a new index table from an existing entries array.
    // This could be optimized further by using the old index table (?).
    template<typename ST>
    void recreate_index(Data* d, Context& ctx, size_t capacity) const;

    // Creates the index from scratch using the existing index array.
    // The index array should have been cleared (if reused) or initialized
    // with empty bucket values (if new).
    // TODO: Take advantage of the old index array and don't do a complete rehash
    // TODO: internal api design is bad.
    template<typename ST>
    void rehash_index(Data* d) const;

    // Returns the next bucket index after current_bucket.
    size_t next_bucket(Data* d, size_t current_bucket) const;

    // Returns the ideal bucket index for that hash value.
    size_t bucket_for_hash(Data* d, Hash hash) const;

    // Returns the distance of current_bucket from hash's ideal bucket.
    size_t distance_from_ideal(Data* d, Hash hash, size_t current_bucket) const;

    // Returns the current size class.
    SizeClass index_size_class(Data* d) const;

    // Returns the size class for the given entries capacity.
    static SizeClass index_size_class(size_t entry_count);

    // True if the keys are considered equal. Fast path for keys that are bit-identical.
    static bool key_equal(Value a, Value b) { return a.same(b) || equal(a, b); }

    inline Data* access_heap() const;
};

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_HASH_TABLES_HPP
