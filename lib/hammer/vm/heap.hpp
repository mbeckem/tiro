#ifndef HAMMER_VM_HEAP_HPP
#define HAMMER_VM_HEAP_HPP

#include "hammer/core/defs.hpp"
#include "hammer/core/math.hpp"
#include "hammer/core/scope_exit.hpp"
#include "hammer/vm/value.hpp"

#include <new>

namespace hammer::vm {

class Collector;

// Tracks all allocated objects.
// Will be replaced by a parsable, paged heap.
class ObjectList {
public:
    class Cursor {
    public:
        Cursor() = default;

        bool valid() const { return *current_ != end_; }
        explicit operator bool() const { return valid(); }

        Header* get() const {
            HAMMER_ASSERT(valid(), "Invalid cursor.");
            return *current_;
        }
        Header* operator*() const { return get(); }

        // Removes the current element and advances to the next element.
        void remove() {
            HAMMER_ASSERT(valid(), "Invalid cursor.");

            Header* value = *current_;
            HAMMER_ASSERT(value->next, "Header was not linked into the list.");

            *current_ = value->next;
            value->next = nullptr;
        }

        // Advances to the next element.
        void next() {
            HAMMER_ASSERT(valid(), "Invalid cursor.");
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
        HAMMER_ASSERT_NOT_NULL(obj);
        HAMMER_ASSERT(obj->next == nullptr, "Header is already linked.");
        obj->next = head_;
        head_ = obj;
    }

    bool empty() const noexcept {
        HAMMER_ASSERT(head_ != nullptr, "Invalid head pointer.");
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
class Heap {
public:
    Heap() = default;
    ~Heap();

    template<typename T, typename... Args>
    T* create_varsize(size_t total_size, Args&&... args) {
        static_assert(std::is_base_of_v<Header, T>);

        void* storage = allocate(total_size);
        if (!storage) {
            // FIXME launch gc if alloc fails
            HAMMER_ERROR("Out of memory.");
        }
        ScopeExit cleanup = [&] { free(storage); };

        T* result = new (storage) T(std::forward<Args>(args)...);
        Header* header = static_cast<Header*>(result);
        HAMMER_ASSERT((void*) result == (void*) header,
            "Invalid location of header in struct.");
        HAMMER_ASSERT(Value::from_heap(result).object_size() == total_size,
            "Invalid object size.");

        objects_.insert(header);
        cleanup.disable();
        return result;
    }

    template<typename T, typename... Args>
    T* create(Args&&... args) {
        return create_varsize<T>(sizeof(T), std::forward<Args>(args)...);
    }

    void* allocate(size_t size);
    void free(void* ptr);

    Heap(const Heap&) = delete;
    Heap& operator=(const Heap&) = delete;

private:
    friend Collector;

    ObjectList objects_;
};

} // namespace hammer::vm

#endif // HAMMER_VM_HEAP_HPP
