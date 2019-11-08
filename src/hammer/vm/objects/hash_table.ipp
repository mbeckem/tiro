#ifndef HAMMER_VM_OBJECTS_HASH_TABLE_IPP
#define HAMMER_VM_OBJECTS_HASH_TABLE_IPP

#include "hammer/vm/objects/hash_table.hpp"

#include "hammer/vm/objects/array_storage_base.ipp"

namespace hammer::vm {

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

    // Implements a hash lookup table for the entries in "storage".
    // The indices array only stores indices into the storage array.
    // The type depends on the capacity (one of  U{8/16/32/64}Array).
    //
    // Possible improvement: Just make it 64 bit all the time, but use
    // the unused bits to cache the (shortened) hash of the indexed key.
    Value indices;

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

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_HASH_TABLE_IPP
