#include <catch2/catch.hpp>

#include "tiro/api.h"

#include "./helpers.hpp"

using namespace tiro::api;

TEST_CASE("Frames should be constructible", "[api]") {
    OwnedVM vm(tiro_vm_new(nullptr));

    OwnedFrame frame(tiro_frame_new(vm, 123));
    REQUIRE(frame.get() != nullptr);
    REQUIRE(tiro_frame_size(frame) == 123);

    tiro_handle a = tiro_frame_slot(frame, 0);
    REQUIRE(a != nullptr);

    tiro_handle b = tiro_frame_slot(frame, 1);
    REQUIRE(b != nullptr);

    REQUIRE(a != b);
}

TEST_CASE("Accessing an invalid slot returns null", "[api]") {
    OwnedVM vm(tiro_vm_new(nullptr));
    OwnedFrame frame(tiro_frame_new(vm, 1));
    tiro_handle handle = tiro_frame_slot(frame, 1);
    REQUIRE(handle == nullptr);
}

TEST_CASE("Frame slots should be null by default", "[api]") {
    OwnedVM vm(tiro_vm_new(nullptr));
    OwnedFrame frame(tiro_frame_new(vm, 1));
    tiro_handle handle = tiro_frame_slot(frame, 0);
    REQUIRE(tiro_value_kind(handle) == TIRO_KIND_NULL);
}
