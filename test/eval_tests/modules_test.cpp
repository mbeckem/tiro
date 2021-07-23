#include <catch2/catch.hpp>

#include "eval_test.hpp"

namespace tiro::eval_tests {

TEST_CASE("Constants at module scope should be supported", "[modules]") {
    std::string_view source = R"(
        const x = 3;
        const y = "world";
        const z = "Hello $y!";

        export func get_x() { return x; }    
        export func get_y() { return y; }
        export func get_z() { return z; }
    )";

    eval_test test(source);
    test.call("get_x").returns_int(3);
    test.call("get_y").returns_string("world");
    test.call("get_z").returns_string("Hello world!");
}

TEST_CASE("Variables on module scope should be supported", "[modules]") {
    std::string_view source = R"(
        var foo = 1;

        export func test() {
            return foo += 1;
        }
    )";

    eval_test test(source);
    test.call("test").returns_int(2);
    test.call("test").returns_int(3);
    test.call("test").returns_int(4);
}

TEST_CASE("Complex init logic at module scope should be possible", "[modules]") {
    std::string_view source = R"(
        const data = [1, 2, 3, "end"];
        
        export const next = {
            var index = 0;
            
            func next() {
                var result = data[index];
                index += 1;
                return result;
            };
        };
    )";

    eval_test test(source);
    test.call("next").returns_int(1);
    test.call("next").returns_int(2);
    test.call("next").returns_int(3);
    test.call("next").returns_string("end");
}

} // namespace tiro::eval_tests