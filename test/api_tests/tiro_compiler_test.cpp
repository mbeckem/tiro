#include <catch2/catch.hpp>

#include "tiro/api.h"
#include "tiropp/error.hpp"

#include <string>

TEST_CASE("tiro should output concrete syntax tree as json", "[api]") {
    struct Holder {
        char* string = nullptr;

        ~Holder() { std::free(string); }
    } holder;

    tiro_parse_syntax("func foo() {}", &holder.string, tiro::error_adapter());

    printf("%s\n", holder.string);

    REQUIRE(holder.string != nullptr);
    REQUIRE(holder.string[0] == '{');
}
