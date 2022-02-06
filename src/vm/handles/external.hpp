#ifndef TIRO_VM_HANDLES_EXTERNAL_HPP
#define TIRO_VM_HANDLES_EXTERNAL_HPP

#include "common/adt/not_null.hpp"
#include "common/defs.hpp"
#include "vm/handles/fwd.hpp"
#include "vm/handles/handle.hpp"
#include "vm/handles/traits.hpp"
#include "vm/heap/memory.hpp"
#include "vm/objects/fwd.hpp"
#include "vm/objects/value.hpp"

#include <absl/container/flat_hash_set.h>

#include <bitset>
#include <climits>
#include <utility>

namespace tiro::vm {

namespace detail {

constexpr size_t external_slots_per_page(size_t page_bytes) {
    size_t H = sizeof(void*); // Fixed header size
    size_t B = page_bytes;    // Block size
    size_t P = sizeof(void*); // Pointer size
    size_t C = 64;            // Bitset representation in bits (pessimistic)
    size_t M = CHAR_BIT;
    return ((M * (B - H)) - C + 1) / (1 + M * P);
};

} // namespace detail

/// Implements a set of handles suitable for use in external code.
/// Handles can be allocated and deallocated manually.
class ExternalStorage final {
public:
    /// Returns a pointer to the external storage instance that created the given `external` handle.
    /// \pre `external` must be a valid handle
    /// \pre the storage that created the handle must still be valid
    template<typename T>
    inline static NotNull<ExternalStorage*> from_external(const External<T>& external);

    ExternalStorage();
    ~ExternalStorage();

    ExternalStorage(const ExternalStorage&) = delete;
    ExternalStorage& operator=(const ExternalStorage&) = delete;

    /// Allocates a new handle constructed from the given initial value.
    /// The returned handle must be freed.
    template<typename T = detail::DeduceValueType, typename U>
    inline External<detail::DeducedType<T, U>> allocate(U&& initial);

    /// Allocates a new handle initialized with a default constructed `T`.
    /// The returned handle must be freed.
    template<typename T = Value>
    inline External<T> allocate();

    /// Frees a handle previously allocated through one of the `allocate` functions.
    template<typename T>
    inline void free(External<T> handle);

    /// Returns the number of handles that are currently in use.
    size_t used_slots() const { return total_slots() - free_slots(); }

    /// Returns the number of handles that are in the free list.
    size_t free_slots() const { return free_slots_; }

    /// Returns the total number of slots (free and in use).
    size_t total_slots() const { return total_slots_; }

    /// Returns the back pointer to the owning context.
    /// Not initialized in tests, always valid otherwise.
    Context* ctx() const { return ctx_; }

    NotNull<Context*> must_ctx() const { return TIRO_NN(ctx()); }

    void set_ctx(Context& ctx) { ctx_ = &ctx; }

    template<typename Tracer>
    inline void trace(Tracer&& tracer);

private:
    static constexpr size_t page_size = 1 << 12;
    static constexpr auto page_mask = aligned_container_mask(page_size);
    static constexpr size_t page_slots = detail::external_slots_per_page(page_size);

    union Slot {
        Value value;
        Slot* next_free;
    };

    // TODO: Bitset algorithms, required elsewhere as well. Make 'allocated' a raw array
    // with guaranteed size then.
    struct Page {
        Page(ExternalStorage* parent_)
            : parent(parent_) {}

        ExternalStorage* parent;
        std::bitset<page_slots> allocated;
        Slot slots[];
    };

    Value* allocate_slot();
    void free_slot(Value* handle);

    void link_free_slot(Slot* slot);
    Slot* pop_free_slot();

    static Page* page_from_slot(Slot* slot);
    static bool slot_allocated(Page* page, Slot* slot);
    static size_t slot_index(Page* page, Slot* slot);

private:
    Context* ctx_ = nullptr;

    // Contains all allocated pages.
    absl::flat_hash_set<Page*> pages_;

    // Linked list of free slots. Free slots have the "allocated" bit set to free in their page.
    Slot* first_free_ = nullptr;

    size_t free_slots_ = 0;
    size_t total_slots_ = 0;
};

/// A typed and rooted variable with dynamic lifetime, suitable for use in external code.
/// Externals must be freed manully, their destructor will not release them.
///
/// Externals are implicitly convertible to immutable handles. Use `mut()` to
/// explicitly convert an external to a mutable handle.
template<typename T>
class External final : public detail::MutHandleOps<T, External<T>>,
                       public detail::EnableUpcast<T, Handle, External<T>> {
public:
    static External from_raw_slot(Value* slot) { return External(slot, InternalTag()); }

    External() = delete;

    External(const External&) = default;
    External& operator=(const External&) = default;

    /// Converts this external to a mutable handle that points to the same slot.
    MutHandle<T> mut() { return MutHandle<T>::from_raw_slot(slot_); }

    /// Converts this external to an output handle that points to the same slot.
    OutHandle<T> out() { return OutHandle<T>::from_raw_slot(slot_); }

private:
    friend ExternalStorage;

    struct InternalTag {};

    External(Value* rooted_slot, InternalTag)
        : slot_(rooted_slot) {
        TIRO_DEBUG_ASSERT(slot_, "Invalid slot.");
    }

private:
    friend SlotAccess;

    Value* get_slot() const { return slot_; }

private:
    Value* slot_;
};

template<typename T>
class UniqueExternal : public detail::MutHandleOps<T, UniqueExternal<T>>,
                       public detail::EnableUpcast<T, Handle, UniqueExternal<T>> {
public:
    /// Creates an invalid instance.
    explicit UniqueExternal()
        : slot_(nullptr) {}

    /// Takes ownership of the external, which must be valid.
    template<typename U, std::enable_if_t<std::is_convertible_v<U, T>>* = nullptr>
    explicit UniqueExternal(External<U> external)
        : slot_(get_valid_slot(external)) {}

    ~UniqueExternal() { reset(); }

    UniqueExternal(UniqueExternal&& other) noexcept
        : slot_(std::exchange(other.slot_, nullptr)) {}

    UniqueExternal& operator=(UniqueExternal&& other) noexcept {
        if (this != &other) {
            reset();
            slot_ = std::exchange(other.slot_, nullptr);
        }
        return *this;
    }

    UniqueExternal(const UniqueExternal&) = delete;
    UniqueExternal& operator=(const UniqueExternal&) = delete;

    /// \pre `valid()`
    ExternalStorage& storage() const {
        TIRO_DEBUG_ASSERT(slot_, "UniqueExternal::storage(): Invalid instance.");
        return *ExternalStorage::from_external(get());
    }

    bool valid() const { return slot_; }
    explicit operator bool() const { return slot_; }

    /// Returns a mutable handle that points to the same slot. The instance must be in a valid state.
    MutHandle<T> mut() {
        TIRO_DEBUG_ASSERT(slot_, "UniqueExternal::mut(): Invalid instance.");
        return MutHandle<T>::from_raw_slot(slot_);
    }

    /// Returns an output handle that points to the same slot. The instance must be in a valid state.
    OutHandle<T> out() {
        TIRO_DEBUG_ASSERT(slot_, "UniqueExternal::out(): Invalid instance.");
        return OutHandle<T>::from_raw_slot(slot_);
    }

    /// Returns an external instance that refers to the same slot.
    /// The instance remains valid for as long as the unique external stays valid.
    External<T> get() const {
        TIRO_DEBUG_ASSERT(slot_, "UniqueExternal::get(): Invalid instance.");
        return External<T>::from_raw_slot(slot_);
    }

    External<T> release() {
        TIRO_DEBUG_ASSERT(slot_, "UniqueExternal::release(): Invalid instance.");
        return External<T>::from_raw_slot(std::exchange(slot_, nullptr));
    }

    void reset() {
        if (slot_) {
            storage().free(get());
            slot_ = nullptr;
        }
    }

private:
    friend SlotAccess;

    Value* get_slot() const { return slot_; }

private:
    Value* slot_;
};

template<typename T>
UniqueExternal(External<T> ext) -> UniqueExternal<T>;

template<typename T>
NotNull<ExternalStorage*> ExternalStorage::from_external(const External<T>& external) {
    auto slot = get_valid_slot(external);
    TIRO_DEBUG_ASSERT(slot, "invalid slot");
    auto page = page_from_slot(reinterpret_cast<Slot*>(slot));
    TIRO_DEBUG_ASSERT(page, "invalid page");
    return TIRO_NN(page->parent);
}

template<typename T, typename U>
External<detail::DeducedType<T, U>> ExternalStorage::allocate(U&& initial) {
    using deduced_t = detail::DeducedType<T, U>;
    Value* slot = allocate_slot();
    *slot = static_cast<deduced_t>(unwrap_value(initial));
    return {slot, typename External<deduced_t>::InternalTag()};
}

template<typename T>
External<T> ExternalStorage::allocate() {
    return allocate<T>(T());
}

template<typename T>
void ExternalStorage::free(External<T> handle) {
    free_slot(get_valid_slot(handle));
}

template<typename Tracer>
void ExternalStorage::trace(Tracer&& tracer) {
    for (Page* page : pages_) {
        // TODO: std::bitset does not have efficient iteration
        for (size_t i = 0, n = page->allocated.size(); i != n; ++i) {
            if (page->allocated.test(i)) {
                tracer(page->slots[i].value);
            }
        }
    }
}

} // namespace tiro::vm

#endif // TIRO_VM_HANDLES_EXTERNAL_HPP
