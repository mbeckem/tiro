#ifndef TIRO_COMMON_ENTITIES_ENTITY_STORAGE_VIEW_HPP
#define TIRO_COMMON_ENTITIES_ENTITY_STORAGE_VIEW_HPP

#include "common/defs.hpp"
#include "common/entities/entity_storage.hpp"

#include <memory>
#include <type_traits>

namespace tiro {

/// A class that exposes storage access functions of the underlying entity storage to public consumers.
/// Value access, e.g. retrieving a pointer or reference to an element, is possible.
/// Adding or removing values is not allowed.
///
/// Note: use `const Value` to obtain a view that hands out const pointers/references.
template<typename Value, typename Id>
class EntityStorageView final {
private:
    template<typename IfConst, typename Else>
    using C = std::conditional_t<std::is_const_v<Value>, IfConst, Else>;

    using Raw = EntityStorage<std::remove_const_t<Value>, Id>;

public:
    using Storage = C<const Raw, Raw>;
    using IdType = typename Raw::IdType;
    using ValueType = typename Raw::ValueType;
    using Reference = C<typename Raw::ConstReference, typename Raw::Reference>;
    using Pointer = C<typename Raw::ConstPointer, typename Raw::Pointer>;

    EntityStorageView(Storage& storage)
        : storage_(std::addressof(storage)) {}

    /// \copydoc EntityStorage::operator[](const IdType&)
    Reference operator[](const IdType& id) const { return get()[id]; }

    /// \copydoc EntityStorage::try_get(const IdType&)
    std::optional<ValueType> try_get(const IdType& id) const { return get().try_get(id); }

    /// \copydoc EntityStorage::ptr_to(const IdType&)
    NotNull<Pointer> ptr_to(const IdType& id) const { return get().ptr_to(id); }

private:
    Storage& get() const { return *storage_; }

private:
    Storage* storage_;
};

} // namespace tiro

#endif // TIRO_COMMON_ENTITIES_ENTITY_STORAGE_VIEW_HPP
