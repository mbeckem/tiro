#ifndef TIRO_CORE_INDEX_MAP_HPP
#define TIRO_CORE_INDEX_MAP_HPP

#include "tiro/core/defs.hpp"
#include "tiro/core/iter_tools.hpp"
#include "tiro/core/vec_ptr.hpp"

#include <limits>
#include <vector>

namespace tiro {

template<typename T>
using IndexMapPtr = VecPtr<T>;

/// An index map consists of an internal vector of elements.
/// Elements are accessed via an abstract key type that is transparently
/// mapped to vector indices and back, allowing for type safe indices.
template<typename Value, typename Mapper>
class IndexMap final {
public:
    using Storage = std::vector<Value>;
    using KeyType = typename Mapper::ValueType;
    using ValueType = Value;
    using Reference = typename Storage::reference;
    using ConstReference = typename Storage::const_reference;

    IndexMap(const Mapper& to_index = Mapper())
        : mapper_(to_index) {}

    auto begin() { return storage_.begin(); }
    auto begin() const { return storage_.begin(); }

    auto end() { return storage_.end(); }
    auto end() const { return storage_.end(); }

    auto keys() const {
        return TransformView(CountingRange<size_t>(0, size()), ToKey{this});
    }

    bool empty() const { return size() == 0; }

    size_t size() const { return storage_.size(); }

    size_t capacity() const { return storage_.capacity(); }

    bool in_bounds(const KeyType& key) const {
        return to_index(key) < storage_.size();
    }

    Reference operator[](const KeyType& key) {
        TIRO_DEBUG_ASSERT(in_bounds(key), "Index out of bounds.");
        return storage_[to_index(key)];
    }

    ConstReference operator[](const KeyType& key) const {
        TIRO_DEBUG_ASSERT(in_bounds(key), "Index out of bounds.");
        return storage_[to_index(key)];
    }

    std::optional<ValueType> try_get(const KeyType& key) const {
        if (!in_bounds(key))
            return {};
        return storage_[to_index(key)];
    }

    /// Returns a pointer to the value associated with key that remains
    /// valid even if the underlying vector resizes (e.g. because of new insertions),
    IndexMapPtr<ValueType> ptr_to(const KeyType& key) {
        TIRO_DEBUG_ASSERT(in_bounds(key), "Index out of bounds.");
        return {storage_, to_index(key)};
    }

    IndexMapPtr<const ValueType> ptr_to(const KeyType& key) const {
        TIRO_DEBUG_ASSERT(in_bounds(key), "Index out of bounds.");
        return {storage_, to_index(key)};
    }

    void clear() { storage_.clear(); }

    void reserve(size_t n) { storage_.reserve(n); }

    void resize(size_t n, const ValueType& filler = ValueType()) {
        storage_.resize(n, filler);
    }

    void reset(size_t n, const ValueType& filler = ValueType()) {
        clear();
        storage_.resize(n, filler);
    }

    /// Grow to ensure that the key is in bounds. Does nothing if the storage
    /// is already large enough.
    void grow(const KeyType& key, const ValueType& filler = ValueType()) {
        const size_t index = to_index(key);
        if (index >= storage_.size())
            resize(index + 1, filler);
    }

    void insert(const KeyType& key, const ValueType& value,
        const ValueType& filler = ValueType()) {
        grow(key, filler);
        (*this)[key] = value;
    }

    KeyType push_back(const ValueType& value) {
        auto key = to_key(storage_.size());
        storage_.push_back(value);
        return key;
    }

    KeyType push_back(ValueType&& value) {
        auto key = to_key(storage_.size());
        storage_.push_back(std::move(value));
        return key;
    }

private:
    using RawIndexType = typename Mapper::IndexType;

    struct ToKey {
        const IndexMap* map;
        KeyType operator()(size_t index) const { return map->to_key(index); };
    };

    static_assert(std::numeric_limits<RawIndexType>::max()
                      <= std::numeric_limits<size_t>::max(),
        "Index type must not allow larger values than size_t.");

    size_t to_index(const KeyType& key) const {
        RawIndexType value = mapper_.to_index(key);
        if constexpr (std::is_signed_v<RawIndexType>) {
            TIRO_DEBUG_ASSERT(value >= 0, "Index value must not be negative.");
        }
        return static_cast<size_t>(value);
    }

    KeyType to_key(size_t index) const {
        TIRO_DEBUG_ASSERT(index < std::numeric_limits<RawIndexType>::max(),
            "Index is out of bounds for target type.");
        return mapper_.to_value(index);
    }

private:
    Mapper mapper_;
    Storage storage_;
};

} // namespace tiro

#endif // TIRO_CORE_INDEX_MAP_HPP
