#ifndef TIRO_VM_HEAP_HEAP_HPP
#define TIRO_VM_HEAP_HEAP_HPP

#include "common/adt/not_null.hpp"
#include "common/defs.hpp"
#include "common/math.hpp"
#include "common/scope_guards.hpp"
#include "vm/heap/collector.hpp"
#include "vm/heap/header.hpp"
#include "vm/object_support/fwd.hpp"
#include "vm/objects/fwd.hpp"

#include <new>

namespace tiro::vm {

// Tracks all allocated objects.
// Will be replaced by a parsable, paged heap.
class ObjectList final {
public:
    class Cursor {
    public:
        Cursor() = default;

        bool valid() const { return *current_ != end_; }
        explicit operator bool() const { return valid(); }

        Header* get() const {
            TIRO_DEBUG_ASSERT(valid(), "Invalid cursor.");
            return *current_;
        }
        Header* operator*() const { return get(); }

        // Removes the current element and advances to the next element.
        void remove() {
            TIRO_DEBUG_ASSERT(valid(), "Invalid cursor.");

            Header* value = *current_;
            TIRO_DEBUG_ASSERT(value->next, "Header was not linked into the list.");

            *current_ = value->next;
            value->next = nullptr;
        }

        // Advances to the next element.
        void next() {
            TIRO_DEBUG_ASSERT(valid(), "Invalid cursor.");
            current_ = &((*current_)->next);
        }

    private:
        friend ObjectList;

        Cursor(ObjectList* ls)
            : current_(&ls->head_)
            , end_(&ls->dummy_) {}

    private:
        /// Points to the current slot. The current slot, if valid,
        /// points to the current element.
        Header** current_;

        /// Points to the end element (which is invalid).
        Header* end_;
    };

public:
    ObjectList()
        : head_(&dummy_) {}

    Cursor cursor() { return Cursor(this); }

    void insert(NotNull<Header*> obj) noexcept {
        TIRO_DEBUG_ASSERT(obj->next == nullptr, "Header is already linked.");
        obj->next = head_;
        head_ = obj;
    }

    bool empty() const noexcept {
        TIRO_DEBUG_ASSERT(head_ != nullptr, "Invalid head pointer.");
        return head_ == &dummy_;
    }

private:
    // Linked list of all known objects.
    // Terrible and slow, but will be good enough for testing.
    Header* head_;

    // Dummy value for end of list.
    Header dummy_{Header::InvalidTag()};
};

// TODO paged heap
class Heap final {
public:
    Heap(Context* ctx)
        : ctx_(ctx) {}

    ~Heap();

    /// Returns true if the given value is pinned in memory.
    /// This currently always returns true (that will change once
    /// we implement the moving gc).
    bool is_pinned([[maybe_unused]] Value v) const;

    template<typename Layout, typename... Args>
    Layout* create_varsize(size_t total_byte_size, Args&&... args) {
        using Traits = LayoutTraits<Layout>;
        static_assert(
            !Traits::has_static_size, "The layout has static size, use create() instead.");

        Layout* data = create_impl<Layout>(total_byte_size, std::forward<Args>(args)...);
        TIRO_DEBUG_ASSERT(Traits::dynamic_size(data) == total_byte_size,
            "Byte size mismatch between requested and calculated dynamic object size.");
        return data;
    }

    template<typename Layout, typename... Args>
    Layout* create(Args&&... args) {
        using Traits = LayoutTraits<Layout>;
        static_assert(
            Traits::has_static_size, "The layout has dynamic size, use create_varsize instead.");
        return create_impl<Layout>(Traits::static_size, std::forward<Args>(args)...);
    }

    void destroy(Header* hdr);

    size_t allocated_objects() const noexcept { return allocated_objects_; }
    size_t allocated_bytes() const noexcept { return allocated_bytes_; }

    Collector& collector() { return collector_; }

    Heap(const Heap&) = delete;
    Heap& operator=(const Heap&) = delete;

private:
    template<typename T, typename... Args>
    inline T* create_impl(size_t total_size, Args&&... args);

    void* allocate(size_t size);
    void free(void* ptr, size_t size);

private:
    friend Collector;

    Context* ctx_;
    Collector collector_;
    ObjectList objects_;

    size_t allocated_objects_ = 0;
    size_t allocated_bytes_ = 0;
};

template<typename T, typename... Args>
inline T* Heap::create_impl(size_t total_size, Args&&... args) {
    static_assert(std::is_base_of_v<Header, T>);

    TIRO_DEBUG_ASSERT(
        total_size >= sizeof(T), "Allocation size is too small for instances of the given type.");

    void* storage = allocate(total_size);
    ScopeFailure cleanup = [&] { free(storage, total_size); };

    T* result = new (storage) T(std::forward<Args>(args)...);
    NotNull<Header*> header = TIRO_NN(static_cast<Header*>(result));
    TIRO_DEBUG_ASSERT((void*) result == (void*) header, "Invalid location of header in struct.");

    objects_.insert(header);
    allocated_objects_ += 1;
    return result;
}

} // namespace tiro::vm

#endif // TIRO_VM_HEAP_HEAP_HPP
