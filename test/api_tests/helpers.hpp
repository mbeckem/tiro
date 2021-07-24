#ifndef TIRO_TEST_API_TESTS_HELPERS_HPP_INCLUDED
#define TIRO_TEST_API_TESTS_HELPERS_HPP_INCLUDED

#include "tiro/api.h"
#include "tiropp/api.hpp"

#include "fmt/format.h"

#include <assert.h>
#include <memory>
#include <stdexcept>

/// Catches an error returned by the tiro api and sets the output status code
/// to either the error's error code or TIRO_OK if no error was returned.
class error_observer final {
public:
    explicit error_observer(tiro_errc_t& out)
        : out_(out) {}

    ~error_observer() {
        out_ = error_ ? tiro_error_errc(error_) : TIRO_OK;
        tiro_error_free(error_);
    }

    operator tiro_error_t*() { return &error_; }

private:
    tiro_error_t error_ = nullptr;
    tiro_errc_t& out_;
};

// Helper to load a test module (name: "test") into a fresh vm.
inline void load_test(tiro::vm& vm, const char* source) {
    tiro::compiler compiler;
    compiler.add_file("test", source);
    compiler.run();
    REQUIRE(compiler.has_module());

    tiro::compiled_module module = compiler.take_module();
    vm.load_std();
    vm.load(module);
}

// Helper to run a function. Blocks until the function returns.
inline tiro::result run_sync(tiro::vm& vm, const tiro::function& func, const tiro::handle& args) {
    tiro::coroutine coro = tiro::make_coroutine(vm, func, args);
    coro.start();

    while (vm.has_ready()) {
        vm.run_ready();
    }
    if (!coro.completed())
        throw std::runtime_error("coroutine did not complete");
    return coro.result();
}

#endif // TIRO_TEST_API_TESTS_HELPERS_HPP_INCLUDED
