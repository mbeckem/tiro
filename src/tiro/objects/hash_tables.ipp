#ifndef TIRO_OBJECTS_HASH_TABLES_IPP
#define TIRO_OBJECTS_HASH_TABLES_IPP

#include "tiro/objects/buffers.hpp"
#include "tiro/objects/hash_tables.hpp"

#include "tiro/objects/arrays.ipp"

namespace tiro::vm {

struct HashTableIterator::Data : Header {
    explicit Data(HashTable table_)
        : Header(ValueType::HashTableIterator)
        , table(table_) {}

    HashTable table;
    size_t entry_index = 0;
};

size_t HashTableIterator::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
void HashTableIterator::walk(W&& w) {
    Data* d = access_heap();
    w(d->table);
}

HashTableIterator::Data* HashTableIterator::access_heap() const {
    return Value::access_heap<Data>();
}

struct HashTable::Data : Header {
    Data()
        : Header(ValueType::HashTable) {}

    // Number of actual entries in this hash table.
    // There can be holes in the storage if entries have been deleted.
    size_t size = 0;

    // Mask for bucket index modulus computation. Derived from `indicies.size()`.
    size_t mask = 0;

    // Raw array buffer storing indices into the entries table.
    // The layout depends on the number of entries (e.g. compact 1 byte indices
    // are used for small hash tables).
    Buffer indices;

    // Stores the entries in insertion order.
    HashTableStorage entries;
};

size_t HashTable::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
void HashTable::walk(W&& w) {
    Data* d = access_heap();
    w(d->indices);
    w(d->entries);
}

HashTable::Data* HashTable::access_heap() const {
    return Value::access_heap<Data>();
}

} // namespace tiro::vm

#endif // TIRO_OBJECTS_HASH_TABLES_IPP
