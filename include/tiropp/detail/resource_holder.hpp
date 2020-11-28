#ifndef TIROPP_DETAIL_RESOURCE_HOLDER_HPP_INCLUDED
#define TIROPP_DETAIL_RESOURCE_HOLDER_HPP_INCLUDED

#include "tiropp/def.hpp"

#include <utility>

namespace tiro::detail {

template<typename Resource, auto Deleter>
class resource_holder {
public:
    resource_holder() = default;

    resource_holder(Resource res)
        : res_(res) {}

    ~resource_holder() { reset(); }

    resource_holder(resource_holder&& other) noexcept
        : res_(std::exchange(other.res_, nullptr)) {}

    resource_holder& operator=(resource_holder&& other) noexcept {
        if (this != &other) {
            reset();
            res_ = std::exchange(other.res_, nullptr);
        }
        return *this;
    }

    Resource get() const { return res_; }

    operator Resource() const { return res_; }

    explicit operator bool() const { return res_ != nullptr; }

    void reset() {
        if (res_) {
            Deleter(res_);
            res_ = nullptr;
        }
    }

    Resource* out() {
        reset();
        return &res_;
    }

private:
    Resource res_ = nullptr;
};

} // namespace tiro::detail

#endif // TIROPP_DETAIL_RESOURCE_HOLDER_HPP_INCLUDED
