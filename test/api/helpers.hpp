#ifndef TIRO_TEST_API_HELPERS_HPP
#define TIRO_TEST_API_HELPERS_HPP

#include "tiro/api.h"

#include <memory>
#include <stdexcept>

namespace tiro::api {

class OwnedVM final {
public:
    OwnedVM() = default;

    explicit OwnedVM(tiro_vm* vm)
        : vm_(vm) {
        if (!vm)
            throw std::invalid_argument("VM is null.");
    }

    tiro_vm* get() { return vm_.get(); }

    operator tiro_vm*() { return get(); }

private:
    struct Deleter {
        void operator()(tiro_vm* vm) noexcept { tiro_vm_free(vm); }
    };

    std::unique_ptr<tiro_vm, Deleter> vm_;
};

class OwnedFrame final {
public:
    OwnedFrame() = default;

    explicit OwnedFrame(tiro_frame* frame)
        : frame_(frame) {
        if (!frame)
            throw std::invalid_argument("Frame is null.");
    }

    tiro_frame* get() { return frame_.get(); }

    operator tiro_frame*() { return get(); }

private:
    struct Deleter {
        void operator()(tiro_frame* frame) noexcept { tiro_frame_free(frame); }
    };

    std::unique_ptr<tiro_frame, Deleter> frame_;
};

} // namespace tiro::api

#endif // TIRO_TEST_API_HELPERS_HPP
