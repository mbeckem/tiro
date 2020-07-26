#include <catch2/catch.hpp>

#include "common/span.hpp"
#include "vm/handles/frame.hpp"

#include <vector>

using namespace tiro;
using namespace tiro::vm;

TEST_CASE("The frame collection should create frames", "[frame]") {
    FrameCollection col;

    auto frame = col.create_frame(123);
    REQUIRE(&frame->collection() == &col);
    REQUIRE(frame->size() == 123);
    REQUIRE(frame->raw_slots().size() == 123);
}

TEST_CASE("The frame collection should index active frames", "[frame]") {
    FrameCollection col;

    {
        REQUIRE(col.frame_count() == 0);
        auto frame1 = col.create_frame(2);

        {
            REQUIRE(col.frame_count() == 1);
            auto frame2 = col.create_frame(3);
            REQUIRE(col.frame_count() == 2);
        }

        REQUIRE(col.frame_count() == 1);
    }
    REQUIRE(col.frame_count() == 0);
}

TEST_CASE("The frame collection should trace active frames", "[frames]") {
    FrameCollection col;

    auto frame1 = col.create_frame(123);
    auto frame2 = col.create_frame(5);
    auto frame3 = col.create_frame(66);

    std::map<Value*, size_t> seen;
    col.trace([&](Span<Value> span) {
        if (seen.count(span.data()))
            FAIL("Frame base address visited more than once.");

        seen[span.data()] = span.size();
    });

    auto require_seen = [&](Span<Value> expected) {
        auto pos = seen.find(expected.data());
        if (pos == seen.end())
            FAIL("Failed to find expected span by base address");

        REQUIRE(pos->second == expected.size());
    };

    REQUIRE(seen.size() == 3);
    require_seen(frame1->raw_slots());
    require_seen(frame2->raw_slots());
    require_seen(frame3->raw_slots());
}
