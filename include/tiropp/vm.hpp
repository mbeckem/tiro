#ifndef TIROPP_VM_HPP_INCLUDED
#define TIROPP_VM_HPP_INCLUDED

#include "tiropp/compiler.hpp"
#include "tiropp/def.hpp"
#include "tiropp/detail/resource_holder.hpp"
#include "tiropp/error.hpp"
#include "tiropp/fwd.hpp"

#include "tiro/vm.h"

#include <any>
#include <optional>

namespace tiro {

/// Settings to control the construction of a virtual machine.
struct vm_settings {
    /// Invoked by the vm to print a message to the standard output, e.g. when
    /// `std.print(...)` was called. The vm will print to the process's standard output
    /// when this function is not set.
    std::function<void(std::string_view message)> print_stdout;
};

class vm final {
public:
    vm()
        : vm(vm_settings()) {}

    explicit vm(vm_settings settings)
        : settings_(std::move(settings))
        , raw_vm_(construct_vm()) {}

    vm(vm&&) noexcept = delete;
    vm& operator=(vm&&) noexcept = delete;

    const std::any& userdata() const { return userdata_; }
    std::any& userdata() { return userdata_; }

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
        raw_settings.userdata = this;

        if (settings_.print_stdout) {
            raw_settings.print_stdout = [](tiro_string_t message, void* userdata) {
                // FIXME handle exceptions?
                tiro::vm& self = *static_cast<tiro::vm*>(userdata);
                self.settings_.print_stdout({message.data, message.length});
            };
        }

        tiro_vm_t raw_vm = tiro_vm_new(&raw_settings, error_adapter());
        TIRO_ASSERT(raw_vm); // Returns error on failure
        return raw_vm;
    }

private:
    vm_settings settings_;
    detail::resource_holder<tiro_vm_t, tiro_vm_free> raw_vm_;
    std::any userdata_ = nullptr;
};

} // namespace tiro

#endif // TIROPP_VM_HPP_INCLUDED
