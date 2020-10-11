#ifndef TIRO_COMMON_ADT_INDEX_MAP_HPP
#define TIRO_COMMON_ADT_INDEX_MAP_HPP

#include "common/adt/vec_ptr.hpp"
#include "common/defs.hpp"
#include "common/iter_tools.hpp"

#include <limits>
#include <optional>
#include <vector>

namespace tiro {

template<typename T>
using IndexMapPtr = VecPtr<T, std::vector<std::remove_const_t<T>>>;

template<typename T>
struct IdentityMapper {
    using ValueType = T;
    using IndexType = T;

    T to_index(const T& value) const { return value; }
    T to_value(const T& index) const { return index; }
};

/// An index map consists of an internal vector of elements.
/// Elements are accessed via an abstract key type that is transparently
/// mapped to vector indices and back, allowing for type safe indices.
template<typename Value, typename Mapper = IdentityMapper<size_t>>
class IndexMap final {
public:
    using Storage = std::vector<Value>;
    using KeyType = typename Mapper::ValueType;
    using ValueType = Value;
    using Reference = typename Storage::reference;
    using ConstReference = typename Storage::const_reference;

    IndexMap(const Mapper& to_index = Mapper())
        : mapper_(to_index) {}

    /// Iterate over the values in this map.
    auto begin() { return storage_.begin(); }
    auto end() { return storage_.end(); }

    /// Iterate over the values in this map (const).
    auto begin() const { return storage_.begin(); }
    auto end() const { return storage_.end(); }

    /// An iterable range over the keys in this map.
    auto keys() const { return TransformView(CountingRange<size_t>(0, size()), ToKey{this}); }

    /// Returns true if this map is empty.
    bool empty() const { return size() == 0; }

    /// Returns the number of values in this map.
    size_t size() const { return storage_.size(); }

    /// Returns the capacity of this map's storage (in values).
    size_t capacity() const { return storage_.capacity(); }

    /// Returns true if the key is valid for this map's storage.
    bool in_bounds(const KeyType& key) const { return to_index(key) < storage_.size(); }

    /// Returns a reference to the first value in this map.
    /// \pre `!empty()`.
    ValueType& front() {
        TIRO_DEBUG_ASSERT(!empty(), "The map must not be empty.");
        return storage_.front();
    }
    const ValueType& front() const {
        TIRO_DEBUG_ASSERT(!empty(), "The map must not be empty.");
        return storage_.front();
    }

    /// Returns a reference to the last value in this map.
    /// \pre `!empty()`.
    ValueType& back() {
        TIRO_DEBUG_ASSERT(!empty(), "The map must not be empty.");
        return storage_.back();
    }
    const ValueType& back() const {
        TIRO_DEBUG_ASSERT(!empty(), "The map must not be empty.");
        return storage_.back();
    }

    /// Returns the first key in this map.
    /// \pre `!empty()`.
    KeyType front_key() const {
        TIRO_DEBUG_ASSERT(!empty(), "The map must not be empty.");
        return to_key(0);
    }

    /// Returns the last key in this map.
    /// \pre `!empty()`.
    KeyType back_key() const {
        TIRO_DEBUG_ASSERT(!empty(), "The map must not be empty.");
        return to_key(size() - 1);
    }

    /// Returns a reference to the value associated with the given key.
    /// \pre `in_bounds(key)`.
    Reference operator[](const KeyType& key) {
        TIRO_DEBUG_ASSERT(in_bounds(key), "Index out of bounds.");
        return storage_[to_index(key)];
    }

    /// Returns a reference to the value associated with the given key.
    /// \pre `in_bounds(key)`.
    ConstReference operator[](const KeyType& key) const {
        TIRO_DEBUG_ASSERT(in_bounds(key), "Index out of bounds.");
        return storage_[to_index(key)];
    }

    /// Attempts to retrieve the value associated with the given key. Returns an empty optional
    /// if the key is out of bounds.
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

    /// Removes all elements.
    void clear() { storage_.clear(); }

    /// Reserves enough space for `n` elements.
    void reserve(size_t n) { storage_.reserve(n); }

    /// Resizes to exactly `n` elements. If new elements need to be constructed, the value of `filler`
    /// is used as a template.
    void resize(size_t n, const ValueType& filler = ValueType()) { storage_.resize(n, filler); }

    /// Replaces the contents of this map with `n` instances of the given `filler` value.
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

    /// Inserts the given key, value pair. The map is grown if necessary.
    void insert(const KeyType& key, const ValueType& value, const ValueType& filler = ValueType()) {
        grow(key, filler);
        (*this)[key] = value;
    }

    /// Appends a value at the end and returns its key.
    KeyType push_back(const ValueType& value) {
        auto key = to_key(storage_.size());
        storage_.push_back(value);
        return key;
    }

    /// Appends a value at the end and returns its key.
    KeyType push_back(ValueType&& value) {
        auto key = to_key(storage_.size());
        storage_.push_back(std::move(value));
        return key;
    }

    /// Removes the last element in this map.
    void pop_back() { storage_.pop_back(); }

private:
    using RawIndexType = typename Mapper::IndexType;

    struct ToKey {
        const IndexMap* map;
        KeyType operator()(size_t index) const { return map->to_key(index); };
    };

    static_assert(std::numeric_limits<RawIndexType>::max() <= std::numeric_limits<size_t>::max(),
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

#endif // TIRO_COMMON_ADT_INDEX_MAP_HPP
