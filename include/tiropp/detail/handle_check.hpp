#ifndef TIROPP_DETAIL_HANDLE_CHECK_HPP_INCLUDED
#define TIROPP_DETAIL_HANDLE_CHECK_HPP_INCLUDED

#include "tiropp/def.hpp"
#include "tiropp/error.hpp"

#include "tiro/def.h"

#include <stdexcept>

namespace tiro::detail {

#ifdef TIRO_HANDLE_CHECKS

inline void check_handle_vms(tiro_vm_t expected, const tiro_vm_t* vms, size_t size) {
    if (!expected)
        throw bad_handle_check("Invalid virtual machine");

    for (size_t i = 0; i < size; ++i) {
        tiro_vm_t current = vms[i];
        if (!current)
            throw bad_handle_check("Invalid virtual machine");
        if (current != expected)
            throw bad_handle_check(
                "Handles that belong to different virtual machines must not be mixed");
    }
}

inline void check_handle_values(const tiro_handle_t* handles, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        tiro_handle_t current = handles[i];
        if (!current)
            throw bad_handle_check("Invalid handle");
    }
}

template<typename... Handles>
void check_handles(tiro_vm_t vm, const Handles&... handles) {
    const tiro_vm_t vms[] = {handles.raw_vm()...};
    check_handle_vms(vm, vms, sizeof...(Handles));

    const tiro_handle_t raw_handles[] = {handles.raw_handle()...};
    check_handle_values(raw_handles, sizeof...(Handles));
}

#else

template<typename... Handles>
void check_handles(tiro_vm_t, const Handles&...) {}

#endif

} // namespace tiro::detail

#endif // TIROPP_DETAIL_HANDLE_CHECK_HPP_INCLUDED
