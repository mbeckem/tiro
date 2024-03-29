#include <catch2/catch.hpp>

#include "common/adt/function_ref.hpp"

namespace tiro::test {

TEST_CASE("function ref should invoke the passed function object", "[function-ref]") {
    int i = 3;
    auto return_int = [&](int j) { return i + j; };

    FunctionRef<int(int)> ref = return_int;
    REQUIRE(ref(4) == 7);
}

TEST_CASE("function ref can wrap function pointers", "[function-ref]") {
    int counter = 7;

    int (*ptr)(int, void*) = [](int i, void* userdata) -> int {
        int& cnt = *(int*) userdata;
        cnt += i;
        return cnt++;
    };

    FunctionRef<int(int)> ref(ptr, &counter);
    int result = ref(9);
    REQUIRE(result == 16);
    REQUIRE(counter == 17);
}

} // namespace tiro::test
