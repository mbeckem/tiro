#include <catch.hpp>

#include "hammer/core/casting.hpp"

using namespace hammer;

struct Trivial {};

TEST_CASE("Manual rtti should work for trivial cases", "[rtti]") {
    Trivial obj;
    REQUIRE(isa<Trivial>(&obj));

    const Trivial const_obj;
    REQUIRE(isa<Trivial>(&const_obj));

    Trivial* objptr = must_cast<Trivial>(&obj);
    REQUIRE(objptr == &obj);

    const Trivial* const_objptr = must_cast<Trivial>(&const_obj);
    REQUIRE(const_objptr == &const_obj);
}

struct Base {
    enum Type {
        a,

        b_1,
        b_2,
    };

    Type type;

    explicit Base(Type t)
        : type(t) {}

    static constexpr bool is_instance(const Base*) { return true; }
};

namespace hammer {

template<typename Target>
struct InstanceTestTraits<Target,
    std::enable_if_t<std::is_base_of_v<Base, Target>>> {
    static bool is_instance(const Base* b) { return Target::is_instance(b); }
};

} // namespace hammer

struct A : Base {
    A()
        : Base(Base::a) {}

    static constexpr bool is_instance(const Base* b) {
        return b->type == Base::a;
    }
};

struct B : Base {
    B(Type t)
        : Base(t) {
        HAMMER_ASSERT(is_instance(this), "Invalid type for derived class.");
    }

    static constexpr bool is_instance(const Base* b) {
        return b->type >= b_1 && b->type <= b_2;
    }
};

struct B1 : B {
    B1()
        : B(Base::b_1) {}

    static constexpr bool is_instance(const Base* b) {
        return b->type == Base::b_1;
    }
};

struct B2 : B {
    B2()
        : B(Base::b_2) {}

    static constexpr bool is_instance(const Base* b) {
        return b->type == Base::b_2;
    }
};

TEST_CASE("Manual rtti should work for complex inheritance trees", "[rtti]") {
    struct A a_obj;
    struct B1 b1_obj;
    struct B2 b2_obj;

    const Base* a_base = &a_obj;
    REQUIRE(isa<Base>(a_base));
    REQUIRE(isa<A>(a_base));
    REQUIRE(!isa<B>(a_base));
    REQUIRE(!isa<B1>(a_base));
    REQUIRE(!isa<B2>(a_base));
    REQUIRE(must_cast<A>(a_base) == &a_obj);
    REQUIRE(try_cast<A>(a_base) == &a_obj);
    REQUIRE(try_cast<B>(a_base) == nullptr);

    const Base* b1_base = &b1_obj;
    REQUIRE(isa<Base>(b1_base));
    REQUIRE(isa<B1>(b1_base));
    REQUIRE(isa<B>(b1_base));
    REQUIRE(!isa<B2>(b1_base));
    REQUIRE(!isa<A>(b1_base));
    REQUIRE(must_cast<B1>(b1_base) == &b1_obj);
    REQUIRE(must_cast<B>(b1_base) == &b1_obj);
    REQUIRE(try_cast<B1>(b1_base) == &b1_obj);
    REQUIRE(try_cast<B>(b1_base) == &b1_obj);
    REQUIRE(try_cast<B2>(b1_base) == nullptr);
    REQUIRE(try_cast<A>(b1_base) == nullptr);
}
