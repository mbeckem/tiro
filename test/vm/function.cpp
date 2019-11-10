#include <catch.hpp>

#include "hammer/vm/context.hpp"
#include "hammer/vm/objects/function.hpp"
#include "hammer/vm/objects/string.hpp"

using namespace hammer;
using namespace hammer::vm;

TEST_CASE("Native function execution", "[function]") {
    Context ctx;

    int i = 0;

    Root<NativeFunction> func(ctx);
    {
        Root<String> name(ctx, String::make(ctx, "test"));
        auto callable = [&](NativeFunction::Frame& frame) {
            i = 12345;
            frame.result(Integer::make(frame.ctx(), 123));
        };
        func.set(NativeFunction::make(ctx, name, 3, callable));
    }

    REQUIRE(func->name().view() == "test");
    REQUIRE(func->min_params() == 3);

    auto& native = func->function();
    Root<Value> result(ctx, Value::null());
    NativeFunction::Frame frame(ctx, {}, result.mut_handle());
    native(frame);

    REQUIRE(result->as<Integer>().value() == 123);
    REQUIRE(i == 12345);
}
