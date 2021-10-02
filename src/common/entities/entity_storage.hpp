#ifndef TIRO_COMMON_ENTITIES_ENTITY_STORAGE_HPP
#define TIRO_COMMON_ENTITIES_ENTITY_STORAGE_HPP

#include "common/adt/not_null.hpp"
#include "common/adt/vec_ptr.hpp"
#include "common/assert.hpp"
#include "common/defs.hpp"
#include "common/entities/entity_id.hpp"
#include "common/ranges/iter_tools.hpp"

#include <limits>
#include <optional>
#include <vector>

namespace tiro {

namespace detail {

template<typename Id>
auto entity_id_to_index(const Id& id) {
    TIRO_DEBUG_ASSERT(id, "cannot map an invalid id to an index.");
    return id.value();
}

template<typename Id, typename Index>
auto index_to_entity_id(Index index) {
    TIRO_DEBUG_ASSERT(index != Id::invalid_value, "cannot map an invalid index to an entity id.");
    return Id(index);
}

}; // namespace detail

template<typename T>
using EntityPtr = VecPtr<T, std::vector<std::remove_const_t<T>>>;

/// An index map consists of an internal vector of elements.
/// Elements are accessed via an abstract id type that is transparently
/// mapped to vector indices and back, allowing for type safe indices.
template<typename Value, typename Id>
class EntityStorage final {
public:
    using Storage = std::vector<Value>;
    using IdType = Id;
    using IndexType = typename Id::UnderlyingType;
    using ValueType = Value;
    using Reference = typename Storage::reference;
    using ConstReference = typename Storage::const_reference;
    using Pointer = EntityPtr<Value>;
    using ConstPointer = EntityPtr<const Value>;

    EntityStorage() = default;

    /// Iterate over the entities in this instance.
    auto begin() { return storage_.begin(); }
    auto end() { return storage_.end(); }

    /// Iterate over the entities in this instance (const).
    auto begin() const { return storage_.begin(); }
    auto end() const { return storage_.end(); }

    /// An iterable range over the entity ids in this instance.
    auto keys() const { return TransformView(CountingRange<size_t>(0, size()), IndexToId()); }

    /// Returns true if this instance is empty.
    bool empty() const { return size() == 0; }

    /// Returns the number of values in this instance.
    size_t size() const { return storage_.size(); }

    /// Returns the capacity of this instance's storage (in values).
    size_t capacity() const { return storage_.capacity(); }

    /// Returns true if the id is valid for this instance.
    bool in_bounds(const IdType& id) const { return to_index(id) < storage_.size(); }

    /// Returns a reference to the first value in this map.
    /// \pre `!empty()`.
    ValueType& front() {
        TIRO_DEBUG_ASSERT(!empty(), "The storage must not be empty.");
        return storage_.front();
    }
    const ValueType& front() const {
        TIRO_DEBUG_ASSERT(!empty(), "The storage must not be empty.");
        return storage_.front();
    }

    /// Returns a reference to the last value in this instance.
    /// \pre `!empty()`.
    ValueType& back() {
        TIRO_DEBUG_ASSERT(!empty(), "The storage must not be empty.");
        return storage_.back();
    }
    const ValueType& back() const {
        TIRO_DEBUG_ASSERT(!empty(), "The storage must not be empty.");
        return storage_.back();
    }

    /// Returns the first id in this instance.
    /// \pre `!empty()`.
    IdType front_key() const {
        TIRO_DEBUG_ASSERT(!empty(), "The storage must not be empty.");
        return to_id(0);
    }

    /// Returns the last id in this instance.
    /// \pre `!empty()`.
    IdType back_key() const {
        TIRO_DEBUG_ASSERT(!empty(), "The storage must not be empty.");
        return to_id(size() - 1);
    }

    /// Returns a reference to the value associated with the given id.
    /// \pre `in_bounds(id)`.
    Reference operator[](const IdType& id) {
        TIRO_DEBUG_ASSERT(in_bounds(id), "Index out of bounds.");
        return storage_[to_index(id)];
    }

    /// Returns a reference to the value associated with the given id.
    /// \pre `in_bounds(id)`.
    ConstReference operator[](const IdType& id) const {
        TIRO_DEBUG_ASSERT(in_bounds(id), "Index out of bounds.");
        return storage_[to_index(id)];
    }

    /// Attempts to retrieve the value associated with the given id. Returns an empty optional
    /// if the id is out of bounds.
    std::optional<ValueType> try_get(const IdType& id) const {
        if (!in_bounds(id))
            return {};
        return storage_[to_index(id)];
    }

    /// Returns a pointer to the value associated with id that remains
    /// valid even if the underlying vector resizes (e.g. because of new insertions),
    NotNull<Pointer> ptr_to(const IdType& id) {
        TIRO_DEBUG_ASSERT(in_bounds(id), "Index out of bounds.");
        return TIRO_NN(Pointer(storage_, to_index(id)));
    }

    NotNull<ConstPointer> ptr_to(const IdType& id) const {
        TIRO_DEBUG_ASSERT(in_bounds(id), "Index out of bounds.");
        return TIRO_NN(ConstPointer(storage_, to_index(id)));
    }

    /// Removes all values from this instance.
    void clear() { storage_.clear(); }

    /// Reserves enough space for `n` elements in total.
    void reserve(size_t n) { storage_.reserve(n); }

    /// Resizes to exactly `n` elements. If new elements need to be constructed, the value of `filler`
    /// is used as a template.
    void resize(size_t n, const ValueType& filler = ValueType()) { storage_.resize(n, filler); }

    /// Replaces the contents of this instance with `n` instances of the given `filler` value.
    void reset(size_t n, const ValueType& filler = ValueType()) {
        clear();
        storage_.resize(n, filler);
    }

    /// Grow to ensure that the id is in bounds. Does nothing if the storage
    /// is already large enough.
    void grow(const IdType& id, const ValueType& filler = ValueType()) {
        const size_t index = to_index(id);
        if (index >= storage_.size())
            resize(index + 1, filler);
    }

    /// Inserts the given (id, value) pair. The map is grown if necessary.
    void insert(const IdType& id, const ValueType& value, const ValueType& filler = ValueType()) {
        grow(id, filler);
        (*this)[id] = value;
    }

    /// Appends a value at the end and returns its id.
    IdType push_back(const ValueType& value) {
        auto id = to_id(storage_.size());
        storage_.push_back(value);
        return id;
    }

    /// Appends a value at the end and returns its id.
    IdType push_back(ValueType&& value) {
        auto id = to_id(storage_.size());
        storage_.push_back(std::move(value));
        return id;
    }

    /// Emplaces a value at the end and returns its id.
    template<typename... Args>
    IdType emplace_back(Args&&... args) {
        auto id = to_id(storage_.size());
        storage_.emplace_back(std::forward<Args>(args)...);
        return id;
    }

    /// Removes the last element in this map.
    void pop_back() { storage_.pop_back(); }

private:
    static_assert(std::numeric_limits<IndexType>::max() <= std::numeric_limits<size_t>::max(),
        "Index type must not allow larger values than size_t.");

    struct IndexToId {
        IdType operator()(size_t index) const { return to_id(index); }
    };

    static size_t to_index(const IdType& id) {
        IndexType index = detail::entity_id_to_index(id);
        if constexpr (std::is_signed_v<IndexType>) {
            TIRO_DEBUG_ASSERT(index >= 0, "Index value must not be negative.");
        }
        return static_cast<size_t>(index);
    }

    static IdType to_id(size_t index) {
        TIRO_DEBUG_ASSERT(index < std::numeric_limits<IndexType>::max(),
            "Index is out of bounds for target type.");
        return detail::index_to_entity_id<IdType, IndexType>(index);
    }

private:
    Storage storage_;
};

} // namespace tiro

#endif // TIRO_COMMON_ENTITIES_ENTITY_STORAGE_HPP
