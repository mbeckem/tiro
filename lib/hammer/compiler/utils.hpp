#ifndef HAMMER_COMPILER_UTILS_HPP
#define HAMMER_COMPILER_UTILS_HPP

namespace hammer {

// Replaces the value at "storage" with the new value and restores
// it to the old value in the destructor.
template<typename T>
struct [[nodiscard]] ScopedReplace {
    ScopedReplace(T & storage, T new_value)
        : storage_(&storage)
        , old_value_(storage) {
        *storage_ = new_value;
    }

    ~ScopedReplace() { *storage_ = old_value_; }

    ScopedReplace(const ScopedReplace&) = delete;
    ScopedReplace& operator=(const ScopedReplace&) = delete;

private:
    T* storage_;
    T old_value_;
};

} // namespace hammer

#endif // HAMMER_COMPILER_UTILS_HPP
