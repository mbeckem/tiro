#include <catch2/catch.hpp>

#include "vm/context.hpp"
#include "vm/objects/native.hpp"

#include <new>

using namespace tiro;
using namespace tiro::vm;

static int type_dummy;
static constexpr void* type_tag = &type_dummy;

TEST_CASE("Native object should support construction and finalization", "[native-object]") {
    int i = 2;

    {
        Context ctx;
        Scope sc(ctx);

        using function_t = std::function<void()>;
        function_t func = [&]() { i -= 1; };

        Local obj = sc.local(NativeObject::make(ctx, sizeof(function_t)));
        REQUIRE(obj->size() == sizeof(function_t));

        // Not constructed yet:
        REQUIRE_THROWS_AS(obj->data(), Error);

        bool construct_called = false;
        obj->construct(
            type_tag,
            // Constructor
            [&](void* data, size_t size) {
                REQUIRE(data != nullptr);
                REQUIRE(size == sizeof(function_t));
                new (data) function_t(std::move(func));
                construct_called = true;
            },
            // Finalizer
            [](void* data, size_t size) {
                REQUIRE(data != nullptr);
                REQUIRE(size == sizeof(function_t));
                function_t* func_ptr = static_cast<function_t*>(data);
                (*func_ptr)();
                static_cast<function_t*>(data)->~function_t();
            });

        REQUIRE(construct_called);
        REQUIRE(obj->data());

        // Invoke manually.
        function_t* func_ptr = static_cast<function_t*>(obj->data());
        (*func_ptr)();
        REQUIRE(i == 1);
    }

    // Finalization was triggered by gc and invoked the function again.
    REQUIRE(i == 0);
}
