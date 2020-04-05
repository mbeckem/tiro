#ifndef TIRO_HEAP_HEAP_HPP
#define TIRO_HEAP_HEAP_HPP

#include "tiro/core/defs.hpp"
#include "tiro/core/math.hpp"
#include "tiro/core/scope.hpp"
#include "tiro/heap/collector.hpp"
#include "tiro/objects/value.hpp"

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
            TIRO_DEBUG_ASSERT(
                value->next, "Header was not linked into the list.");

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

    void insert(Header* obj) noexcept {
        TIRO_DEBUG_NOT_NULL(obj);
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
    bool is_pinned([[maybe_unused]] Value v) const { return true; }

    template<typename T, typename... Args>
    T* create_varsize(size_t total_size, Args&&... args) {
        return create_impl<T>(total_size, std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    T* create(Args&&... args) {
        return create_impl<T>(sizeof(T), std::forward<Args>(args)...);
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

    TIRO_DEBUG_ASSERT(total_size >= sizeof(T),
        "Allocation size is too small for instances of the given type.");

    void* storage = allocate(total_size);
    ScopeExit cleanup = [&] { free(storage, total_size); };

    T* result = new (storage) T(std::forward<Args>(args)...);
    Header* header = static_cast<Header*>(result);
    TIRO_DEBUG_ASSERT((void*) result == (void*) header,
        "Invalid location of header in struct.");
    TIRO_DEBUG_ASSERT(object_size(Value::from_heap(result)) == total_size,
        "Invalid object size.");

    objects_.insert(header);
    allocated_objects_ += 1;

    cleanup.disable();
    return result;
}

} // namespace tiro::vm

#endif // TIRO_HEAP_HEAP_HPP
