#include <catch.hpp>

#include "hammer/core/ref_counted.hpp"

using namespace hammer;

namespace {

struct Foo : RefCounted {
    Foo(int x) { x_ = x; }

    int x_;
};

struct FooX : Foo {
    FooX()
        : Foo(1) {}
};

struct FooY : Foo {};

} // namespace

TEST_CASE(
    "Weak pointers should not be lockable if the object has been destroyed",
    "[ref-counted]") {

    WeakRef<Foo> outer_weak;
    REQUIRE_FALSE(outer_weak.lock());

    {
        auto foo = make_ref<FooX>();
        REQUIRE(foo->x_ == 1);

        auto weak = WeakRef(foo);

        auto locked = weak.lock();
        REQUIRE(locked);
        REQUIRE(locked.get() == foo.get());

        outer_weak = locked;
        // foo is deleted here
    }

    auto failed_lock = outer_weak.lock();
    REQUIRE_FALSE(failed_lock);
}
