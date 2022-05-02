#include <catch2/catch.hpp>

#include "vm/context.hpp"
#include "vm/objects/native.hpp"

#include <new>

namespace tiro::vm::test {

namespace {

using function_t = std::function<void()>;

constexpr tiro_native_type_t native_type = []() {
    std::string_view name = "TestType";

    tiro_native_type_t type{};
    type.name = {name.data(), name.size()};
    type.alignment = alignof(function_t);
    type.finalizer = [](void* data, size_t size) {
        REQUIRE(data != nullptr);
        REQUIRE(size == sizeof(function_t));
        function_t* func_ptr = static_cast<function_t*>(data);
        (*func_ptr)();
        static_cast<function_t*>(data)->~function_t();
    };
    return type;
}();

} // namespace

TEST_CASE("Native object should support construction and finalization", "[native-object]") {
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

    // Finalization was triggered by heap and invoked the function again.
    REQUIRE(i == 0);
}

TEST_CASE("Native object finalizer should be invoked when the object is collected") {
    int i = 1;

    {
        Context ctx;
        {
            function_t func = [&]() { i -= 1; };

            Scope sc(ctx);
            Local obj = sc.local(NativeObject::make(ctx, &native_type, sizeof(function_t)));
            REQUIRE(obj->data() != nullptr);
            REQUIRE(obj->size() == sizeof(function_t));
            new (obj->data()) function_t(std::move(func));

            ctx.heap().collector().collect(GcReason::Forced);
            REQUIRE(i == 1); // Not finalized, still referenced
        }

        ctx.heap().collector().collect(GcReason::Forced);
        REQUIRE(i == 0); // No longer reachable, finalization was triggered

        ctx.heap().collector().collect(GcReason::Forced);
        REQUIRE(i == 0); // But only once
    }
    REQUIRE(i == 0); // And not again from the heap's destructor
}

} // namespace tiro::vm::test
