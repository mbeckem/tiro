#ifndef TIROPP_VM_HPP_INCLUDED
#define TIROPP_VM_HPP_INCLUDED

#include "tiropp/compiler.hpp"
#include "tiropp/def.hpp"
#include "tiropp/detail/resource_holder.hpp"
#include "tiropp/detail/translate.hpp"
#include "tiropp/error.hpp"
#include "tiropp/fwd.hpp"

#include "tiro/vm.h"

#include <any>
#include <optional>

namespace tiro {

/// Settings to control the construction of a virtual machine.
struct vm_settings {
    /// The size (in bytes) of heap pages allocated by the virtual machine for the storage of most objects.
    /// Must be a power of two between 2^16 and 2^24 or zero to use the default value.
    ///
    /// Smaller pages waste less memory if only small workloads are to be expected.
    /// Larger page sizes can be more performant because fewer chunks need to be allocated for the same number of objects.
    ///
    /// Note that objects that do not fit into a single page reasonably well will be
    /// allocated "on the side" using a separate allocation.
    size_t page_size = 0;

    /// The maximum size (in bytes) that can be occupied by the virtual machine's heap.
    /// The virtual machine will throw out of memory errors if this limit is reached.
    ///
    /// The default value (0) will apply a sane default memory limit.
    /// Use `std::numeric_limits<size_t>::max()` for an unconstrained heap size.
    size_t max_heap_size = 0;

    /// Invoked by the vm to print a message to the standard output, e.g. when
    /// `std.print(...)` was called. The vm will print to the process's standard output
    /// when this function is not set.
    std::function<void(std::string_view message)> print_stdout;

    /// Set this to true to enable capturing of the current call stack trace when an exception
    /// is created during a panic.
    /// Capturing stack traces has a significant performance impact because many call frames on the
    /// call stack have to be visited.
    bool enable_panic_stack_traces = false;
};

class vm final {
public:
    /// Constructs a new vm with default settings.
    vm()
        : vm(vm_settings()) {}

    /// Constructs a new vm with the given settings.
    explicit vm(vm_settings settings)
        : settings_(std::move(settings))
        , raw_vm_(construct_vm()) {}

    vm(vm&&) noexcept = delete;
    vm& operator=(vm&&) noexcept = delete;

    /// Returns the vm's page size, in bytes.
    size_t page_size() const { return tiro_vm_page_size(raw_vm_); }

    /// Returns the vm's maximum heap size, in bytes.
    size_t max_heap_size() const { return tiro_vm_max_heap_size(raw_vm_); }

    /// Userdata associated with this virtual machine.
    /// @{
    const std::any& userdata() const { return userdata_; }
    std::any& userdata() { return userdata_; }
    /// @}

    /// Loads the "std" module.
    void load_std() { tiro_vm_load_std(raw_vm_, error_adapter()); }

    /// Loads the given compiled module.
    void load(const compiled_module& mod) {
        tiro_vm_load_bytecode(raw_vm_, mod.raw_module(), error_adapter());
    }

    /// Returns true if the virtual machine has at least one coroutine ready for execution, false otherwise.
    bool has_ready() const { return tiro_vm_has_ready(raw_vm_); }

    /// Runs all ready coroutines. Returns (and does not block) when all coroutines are either waiting or done.
    void run_ready() { tiro_vm_run_ready(raw_vm_, error_adapter()); }

    /// Returns the raw virtual machine instance managed by this object.
    tiro_vm_t raw_vm() const { return raw_vm_; }

    /// Returns a reference to the original tiro::vm instance. The raw_vm MUST have been
    /// created by the tiro::vm constructor.
    static vm& unsafe_from_raw_vm(tiro_vm_t raw_vm) {
        TIRO_ASSERT(raw_vm);

        void* userdata = tiro_vm_userdata(raw_vm);
        TIRO_ASSERT(userdata);
        return *static_cast<tiro::vm*>(userdata);
    }

private:
    tiro_vm_t construct_vm() {
        tiro_vm_settings_t raw_settings;
        tiro_vm_settings_init(&raw_settings);
        raw_settings.page_size = settings_.page_size;
        raw_settings.max_heap_size = settings_.max_heap_size;
        raw_settings.userdata = this;
        raw_settings.enable_panic_stack_trace = settings_.enable_panic_stack_traces;

        if (settings_.print_stdout) {
            raw_settings.print_stdout = [](tiro_string_t message, void* userdata) {
                // FIXME handle exceptions?
                tiro::vm& self = *static_cast<tiro::vm*>(userdata);
                self.settings_.print_stdout(detail::from_raw(message));
            };
        }

        tiro_vm_t raw_vm = tiro_vm_new(&raw_settings, error_adapter());
        TIRO_ASSERT(raw_vm);
        return raw_vm;
    }

private:
    vm_settings settings_;
    detail::resource_holder<tiro_vm_t, tiro_vm_free> raw_vm_;
    std::any userdata_ = nullptr;
};

} // namespace tiro

#endif // TIROPP_VM_HPP_INCLUDED
