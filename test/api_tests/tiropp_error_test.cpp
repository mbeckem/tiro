#include <catch2/catch.hpp>

#include "tiropp/error.hpp"
#include "tiropp/objects.hpp"

#include <cstring>

TEST_CASE("tiro::api_errc should wrap tiro_errc_t", "[api]") {
    REQUIRE(static_cast<tiro_errc_t>(tiro::api_errc::bad_arg) == TIRO_ERROR_BAD_ARG);
    REQUIRE(tiro::name(tiro::api_errc::bad_arg) == tiro_errc_name(TIRO_ERROR_BAD_ARG));
    REQUIRE(tiro::message(tiro::api_errc::bad_arg) == tiro_errc_message(TIRO_ERROR_BAD_ARG));
}

TEST_CASE("tiro::error_adapter should rethrow errors", "[api]") {
    tiro::vm vm;
    tiro::handle null = tiro::make_null(vm);
    tiro::handle result = tiro::make_null(vm);

    try {
        tiro_tuple_get(
            vm.raw_vm(), null.raw_handle(), 0, result.raw_handle(), tiro::error_adapter());
        FAIL("Should have thrown");
    } catch (const tiro::api_error& err) {
        REQUIRE(err.code() == tiro::api_errc::bad_type);
        REQUIRE(std::strcmp(err.message(), tiro_errc_message(TIRO_ERROR_BAD_TYPE)) == 0);
    } catch (...) {
        FAIL("Unexpected exception type");
    }
}
