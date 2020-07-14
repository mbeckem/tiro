#ifndef TIRO_VM_HANDLES_SCOPE_HPP
#define TIRO_VM_HANDLES_SCOPE_HPP

#include "common/span.hpp"
#include "common/type_traits.hpp"
#include "vm/handles/fwd.hpp"
#include "vm/handles/handle.hpp"
#include "vm/handles/traits.hpp"
#include "vm/objects/fwd.hpp"

#include <type_traits>

namespace tiro::vm {

namespace detail {

template<typename T>
struct IsLocal : std::false_type {};

template<typename T>
struct IsLocal<Local<T>> : std::true_type {};

template<typename T>
inline constexpr bool is_local = IsLocal<remove_cvref_t<T>>::value;

} // namespace detail

/// Manages a stack of values that can be manipulated through the Scope class.
/// The stack can be traced by the garbage collector: all values stored on the stack
/// are always rooted.
class RootedStack {
private:
    struct Page {
        Page* prev = nullptr;
        Page* next = nullptr;
        size_t used = 0;
        Value slots[];
    };

public:
    static constexpr size_t page_size = 1 << 12;
    static constexpr size_t slots_per_page = (page_size - sizeof(Page)) / sizeof(Value);

    RootedStack();
    ~RootedStack();

    RootedStack(const RootedStack&) = delete;
    RootedStack& operator=(const RootedStack&) = delete;

    // Returns the number of pages that have been allocated by this stack.
    size_t pages() const { return total_pages_; }

    // Returns the number of slots that are currently in use.
    size_t used_slots() const { return used_slots_; }

    // Returns the total number of slots allocated by this stack.
    size_t total_slots() const { return pages() * slots_per_page; }

    // Allocates a new value slot. May lead to an allocation
    // if the current stack space is exhausted. Existing slot pointers
    // will remain valid.
    Value* allocate();

    // Deallocate the last n slots. Called by the scope destructor
    // on scope exit. `slots` must not be out of bounds.
    void deallocate(size_t slots);

    // Trace all allocated slots (for use during garbage collection).
    template<typename Tracer>
    void trace(Tracer&& tracer) {
        for (Page* p = first_; p; p = p->next) {
            tracer(Span(p->slots, p->used));
        }
    }

private:
    Page* new_page();
    Value* allocate_from(Page* page);

private:
    Page* first_ = nullptr;
    Page* current_ = nullptr;
    size_t used_slots_ = 0;
    size_t total_pages_ = 0;
};

struct DeferInit {};

inline constexpr DeferInit defer_init{};

/// Provides storage for rooted local variables.
class [[nodiscard]] Scope final {
public:
    /// Constructs a new scope instance that operates on the given context's managed stack.
    explicit Scope(Context & ctx);

    /// Constructs a new scope instance that operates on the given stack.
    explicit Scope(RootedStack & stack);

    ~Scope();

    /// Returns the stack this scope operates on.
    RootedStack& stack() const { return stack_; }

    /// Creates a new local variable. The variable will remain valid
    /// until the scope is destroyed. The initial value for the new local must be passed
    /// as a parameter.
    ///
    /// Note that every new call to `local()` uses a little bit of stack space
    /// until the scope dies, so you should avoid calling this function in unbounded loops.
    /// Try to reuse local variables instead.
    template<typename T = detail::DeduceValueType, typename U>
    inline Local<detail::DeducedType<T, U>> local(U && initial);

    /// Like the above, but the local does not require an initial value on construction.
    /// It will be initialized with null instead. The local must be initialized before reading
    /// the value from it.
    template<typename T>
    inline Local<T> local(const DeferInit&);

    /// Returns a local variable that is initially default constructed.
    template<typename T = Value>
    inline Local<T> local();

    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

private:
    Value* allocate_slot();

private:
    RootedStack& stack_;
    size_t initial_used_;
    size_t allocated_;
};

/// A typed and rooted local variable. Locals point into the storage
/// managed by their scope and must not outlive it.
/// Use a scope instance to create a local variable.
///
/// Locals are implicitly convertible to immutable handles. Use `mut()`
/// to explicitly convert a local to a mutable handle.
template<typename T>
class Local final : public detail::MutHandleOps<T, Local<T>>,
                    public detail::EnableUpcast<T, Handle, Local<T>> {
public:
    // Use the surrounding `Scope` to create a new local variable.
    Local() = delete;
    Local(const Local&) = delete;

    /// Assigning to a local performs the assignment on the inner value.
    template<typename U, std::enable_if_t<!detail::is_local<U>>* = nullptr>
    Local& operator=(U&& value) {
        this->set(std::forward<U>(value));
        return *this;
    }

    /// Assigning to a local performs the assignment on the inner value (assign-through).
    template<typename U, std::enable_if_t<detail::is_local<U>>* = nullptr>
    Local& operator=(U&& other) {
        this->set(other.get());
        return *this;
    }

    /// Converts this local to a mutable handle that points to the same slot.
    MutHandle<T> mut() { return MutHandle<T>::from_raw_slot(slot_); }

    /// Converts this local to an output handle that points to the same slot.
    OutHandle<T> out() { return OutHandle<T>::from_raw_slot(slot_); }

private:
    friend Scope;

    struct InternalTag {};

    Local(Value* rooted_slot, InternalTag)
        : slot_(rooted_slot) {
        TIRO_DEBUG_ASSERT(slot_, "Invalid slot.");
    }

private:
    friend SlotAccess;

    Value* get_slot() const { return slot_; }

private:
    Value* slot_;
};

template<typename T, typename U>
Local<detail::DeducedType<T, U>> Scope::local(U&& initial) {
    using deduced_t = detail::DeducedType<T, U>;
    Value* slot = allocate_slot();
    *slot = static_cast<deduced_t>(unwrap_value(initial));
    return {slot, typename Local<deduced_t>::InternalTag()};
}

template<typename T>
Local<T> Scope::local(const DeferInit&) {
    Value* slot = allocate_slot();
    *slot = Value::null();
    return {slot, typename Local<T>::InternalTag()};
}

template<typename T>
Local<T> Scope::local() {
    return local<T>(T());
}

} // namespace tiro::vm

#endif // TIRO_VM_HANDLES_SCOPE_HPP