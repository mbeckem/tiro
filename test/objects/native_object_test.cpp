#include <catch.hpp>

#include "tiro/objects/native_objects.hpp"
#include "tiro/vm/context.hpp"

#include <new>

using namespace tiro;
using namespace tiro::vm;

TEST_CASE("Native object should support construction and finalization", "[native-object]") {
    int i = 2;

    {
        Context ctx;

        using function_t = std::function<void()>;
        function_t func = [&]() { i -= 1; };

        Root obj(ctx, NativeObject::make(ctx, sizeof(function_t)));
        REQUIRE(obj->data());
        REQUIRE(obj->size() == sizeof(function_t));

        new (obj->data()) function_t(std::move(func));
        obj->set_finalizer([](void* data, size_t size) {
            REQUIRE(size == sizeof(function_t));
            function_t* func_ptr = static_cast<function_t*>(data);
            (*func_ptr)();
            static_cast<function_t*>(data)->~function_t();
        });

        function_t* func_ptr = static_cast<function_t*>(obj->data());
        (*func_ptr)();
        REQUIRE(i == 1);
    }

    REQUIRE(i == 0);
}
