#include <catch2/catch.hpp>

#include "common/memory/ref_counted.hpp"

namespace tiro::test {

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
    "Weak pointers should not be lockable if the object has been destroyed", "[ref-counted]") {

    WeakRef<Foo> outer_weak;
    REQUIRE_FALSE(outer_weak.lock());

    {
        auto foo = make_ref<FooX>();
        REQUIRE(foo->x_ == 1);

        auto weak = WeakRef<Foo>(foo);

        auto locked = weak.lock();
        REQUIRE(locked);
        REQUIRE(locked.get() == foo.get());

        outer_weak = locked;
        // foo is deleted here
    }

    auto failed_lock = outer_weak.lock();
    REQUIRE_FALSE(failed_lock);
}

TEST_CASE("Ref counted objects should be destroyed if no longer referenced", "[ref-counted]") {
    int objects = 0;

    struct TestClass : public RefCounted {
        TestClass(int* counter)
            : counter_(counter) {
            ++*counter_;
        }

        ~TestClass() { --*counter_; }

        int* counter_;
    };

    SECTION("Ref is destroyed") {
        {
            auto ref = make_ref<TestClass>(&objects);
            REQUIRE(objects == 1);
        }
        REQUIRE(objects == 0);
    }

    SECTION("ref is reset") {
        auto ref = make_ref<TestClass>(&objects);
        REQUIRE(objects == 1);
        ref.reset();
        REQUIRE(objects == 0);
    }

    SECTION("ref is assigned") {
        auto ref = make_ref<TestClass>(&objects);
        REQUIRE(objects == 1);

        auto ref2 = make_ref<TestClass>(&objects);
        REQUIRE(objects == 2);

        ref = ref2;
        REQUIRE(objects == 1);

        ref.reset();
        REQUIRE(objects == 1);

        ref2.reset();
        REQUIRE(objects == 0);
    }

    SECTION("ref is move assigned") {
        auto ref = make_ref<TestClass>(&objects);
        REQUIRE(objects == 1);

        auto ref2 = make_ref<TestClass>(&objects);
        REQUIRE(objects == 2);

        ref = std::move(ref2);
        REQUIRE(objects == 1);
        REQUIRE(!ref2);

        ref.reset();
        REQUIRE(objects == 0);
    }

    SECTION("ref is self assigned") {
        auto ref = make_ref<TestClass>(&objects);
        REQUIRE(objects == 1);

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif
        ref = ref;

#if defined(__clang__)
#    pragma clang diagnostic pop
#endif

        REQUIRE(objects == 1);

        ref.reset();
        REQUIRE(objects == 0);
    }

    SECTION("ref is released") {
        auto ref = make_ref<TestClass>(&objects);
        REQUIRE(objects == 1);

        auto ptr = ref.release();
        REQUIRE(ptr);
        REQUIRE(!ref);
        REQUIRE(objects == 1);

        ref = Ref(ptr, false);
        ref.reset();
        REQUIRE(objects == 0);
    }
}

} // namespace tiro::test
