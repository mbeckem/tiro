#include <catch.hpp>

#include "hammer/vm/heap.hpp"

using namespace hammer::vm;

TEST_CASE("object list", "[heap]") {
    Header h1{Header::InvalidTag()}, h2{Header::InvalidTag()}, h3{Header::InvalidTag()};

    ObjectList list;
    list.insert(&h1);
    list.insert(&h2);
    list.insert(&h3);
    REQUIRE_FALSE(list.empty());

    SECTION("simple iteration") {
        ObjectList::Cursor cursor = list.cursor();
        int index = 0;
        while (cursor) {
            CAPTURE(index);

            Header* obj = cursor.get();
            REQUIRE(obj);

            switch (index) {
            case 0:
                REQUIRE(obj == &h3);
                break;
            case 1:
                REQUIRE(obj == &h2);
                break;
            case 2:
                REQUIRE(obj == &h1);
                break;
            }

            cursor.next();
            ++index;
        }

        REQUIRE(index == 3);
    }

    SECTION("remove all") {
        ObjectList::Cursor cursor = list.cursor();
        int index = 0;
        while (cursor) {
            CAPTURE(index);

            Header* obj = cursor.get();
            REQUIRE(obj);

            switch (index) {
            case 0:
                REQUIRE(obj == &h3);
                break;
            case 1:
                REQUIRE(obj == &h2);
                break;
            case 2:
                REQUIRE(obj == &h1);
                break;
            }

            cursor.remove();
            ++index;
        }

        REQUIRE(index == 3);
        REQUIRE(list.empty());
    }
}
