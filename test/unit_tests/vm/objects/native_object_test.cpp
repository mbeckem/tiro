#include <catch2/catch.hpp>

#include "vm/context.hpp"
#include "vm/objects/native.hpp"

#include <new>

using namespace tiro;
using namespace tiro::vm;

TEST_CASE("Native object should support construction and finalization", "[native-object]") {
    using function_t = std::function<void()>;

    constexpr tiro_native_type_t native_type = []() {
        tiro_native_type_t type{};
        type.name = "TestType";
        type.finalizer = [](void* data, size_t size) {
            REQUIRE(data != nullptr);
            REQUIRE(size == sizeof(function_t));
            function_t* func_ptr = static_cast<function_t*>(data);
            (*func_ptr)();
            static_cast<function_t*>(data)->~function_t();
        };
        return type;
    }();

    int i = 2;
    {
        Context ctx;
        Scope sc(ctx);

        function_t func = [&]() { i -= 1; };

        Local obj = sc.local(NativeObject::make(ctx, &native_type, sizeof(function_t)));
        REQUIRE(obj->data() != nullptr);
        REQUIRE(obj->size() == sizeof(function_t));

        void* data = obj->data();
        new (data) function_t(std::move(func));

        // Invoke manually.
        function_t* func_ptr = static_cast<function_t*>(obj->data());
        (*func_ptr)();
        REQUIRE(i == 1);
    }

    // Finalization was triggered by gc and invoked the function again.
    REQUIRE(i == 0);
}
