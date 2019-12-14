#include <catch.hpp>

#include "hammer/vm/context.hpp"
#include "hammer/vm/objects/native_objects.hpp"

#include <new>

using namespace hammer;
using namespace hammer::vm;

TEST_CASE("Native object finalization", "[native-object]") {
    Context ctx;

    int i = 0;

    using function_t = std::function<void()>;

    std::vector<int> dummy;
    dummy.resize(1234);

    function_t func = [&, dummy = std::move(dummy)]() { i = 1; };

    Root obj(ctx, NativeObject::make(ctx, sizeof(function_t)));
    REQUIRE(obj->data());
    REQUIRE(obj->size() == sizeof(function_t));

    new (obj->data()) function_t(std::move(func));
    obj->set_finalizer([](void* data, size_t size) {
        REQUIRE(size == sizeof(function_t));
        static_cast<function_t*>(data)->~function_t();
    });

    function_t* func_ptr = static_cast<function_t*>(obj->data());
    (*func_ptr)();
    REQUIRE(i == 1);
}
