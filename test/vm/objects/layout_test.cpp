#include <catch.hpp>

#include "vm/objects/layout.hpp"
#include "vm/objects/primitives.hpp"

#include <memory>
#include <new>

using namespace tiro;
using namespace tiro::vm;

namespace {

struct NativePayload {
    int foo = 1234;
};

struct GlobalDelete {
    void operator()(char* buffer) const { ::operator delete(buffer); }
};

struct ManualDelete {
    template<typename T>
    void operator()(T* data) const {
        data->~T();
        ::operator delete(data);
    }
};

struct CountingTracer {
    size_t values = 0;

    void operator()(Value&) { ++values; }

    void operator()(Span<Value> slots) { values += slots.size(); }
};

} // namespace

static Header* invalid_type = nullptr;

template<typename Layout, typename... Args>
auto make_dynamic(size_t capacity, Args&&... args) {
    using Traits = LayoutTraits<Layout>;

    size_t alloc_size = Traits::dynamic_size(capacity);
    std::unique_ptr<char, GlobalDelete> buffer(static_cast<char*>(::operator new(alloc_size)));

    Layout* layout = new (buffer.get()) Layout(std::forward<Args>(args)...);
    buffer.release();
    return std::unique_ptr<Layout, ManualDelete>(layout);
}

template<typename Layout>
size_t trace_count(Layout* instance) {
    using Traits = LayoutTraits<Layout>;

    CountingTracer tracer;
    Traits::trace(instance, tracer);
    return tracer.values;
}

static Value make_int(int small) {
    return SmallInteger::make(small);
}

TEST_CASE("Static layout should be traceable", "[layout]") {
    using ObjectLayout = StaticLayout<StaticSlotsPiece<3>>;
    using Traits = LayoutTraits<ObjectLayout>;

    STATIC_REQUIRE(Traits::has_static_size);
    STATIC_REQUIRE(Traits::may_contain_references);
    STATIC_REQUIRE(Traits::static_size >= sizeof(Value) * 3);

    ObjectLayout layout(invalid_type, StaticSlotsInit());
    REQUIRE(layout.static_slot_count() == 3);
    REQUIRE(trace_count(&layout) == 3);

    REQUIRE(layout.static_slot(0)->is_null());
    REQUIRE(layout.static_slot(1)->is_null());
    REQUIRE(layout.static_slot(2)->is_null());
}

TEST_CASE("Static layout without slots should have no references", "[layout]") {
    using ObjectLayout = StaticLayout<StaticPayloadPiece<NativePayload>>;
    using Traits = LayoutTraits<ObjectLayout>;

    STATIC_REQUIRE(Traits::has_static_size);
    STATIC_REQUIRE_FALSE(Traits::may_contain_references);
    STATIC_REQUIRE(Traits::static_size >= sizeof(NativePayload));

    ObjectLayout layout(invalid_type, StaticPayloadInit());
    REQUIRE(layout.static_payload()->foo == 1234);
    REQUIRE(trace_count(&layout) == 0);
}

TEST_CASE("Static layout should support combination of pieces", "[layout]") {
    using ObjectLayout = StaticLayout<StaticSlotsPiece<3>, StaticPayloadPiece<NativePayload>>;
    using Traits = LayoutTraits<ObjectLayout>;

    STATIC_REQUIRE(Traits::has_static_size);
    STATIC_REQUIRE(Traits::may_contain_references);
    STATIC_REQUIRE(Traits::static_size >= sizeof(Value) * 3 + sizeof(NativePayload));

    ObjectLayout layout(invalid_type, StaticSlotsInit(), StaticPayloadInit());
    REQUIRE(layout.static_slot_count() == 3);
    REQUIRE(layout.static_payload()->foo == 1234);
    REQUIRE(trace_count(&layout) == 3);
}

TEST_CASE("Fixed slots layout should support tracing", "[layout]") {
    using ObjectLayout = FixedSlotsLayout<Value, StaticSlotsPiece<2>>;
    using Traits = LayoutTraits<ObjectLayout>;

    STATIC_REQUIRE(Traits::may_contain_references);
    STATIC_REQUIRE_FALSE(Traits::has_static_size);

    const size_t dynamic_elements = 7;
    auto init_slots = [&](auto raw_span) {
        REQUIRE(raw_span.size() == dynamic_elements);
        std::uninitialized_fill(raw_span.begin(), raw_span.end(), make_int(1234));
    };

    auto object = make_dynamic<ObjectLayout>(dynamic_elements, invalid_type,
        FixedSlotsInit(dynamic_elements, init_slots), StaticSlotsInit());

    REQUIRE(object->static_slot_count() == 2);
    REQUIRE(object->fixed_slot_capacity() == 7);
    REQUIRE(Traits::dynamic_size(object.get()) >= sizeof(Value) * 9);
    REQUIRE(Traits::dynamic_size(object.get()) == Traits::dynamic_size(dynamic_elements));
    REQUIRE(trace_count(object.get()) == 9);

    REQUIRE(object->static_slot(0)->is_null());
    REQUIRE(object->static_slot(1)->is_null());
    for (size_t i = 0; i < dynamic_elements; ++i) {
        CAPTURE(i);

        auto value = *object->fixed_slot(i);
        REQUIRE(value.is<SmallInteger>());
        REQUIRE(value.as<SmallInteger>().value() == 1234);
    }
}

TEST_CASE("Dynamic slots layout should support tracing", "[layout]") {
    using ObjectLayout = DynamicSlotsLayout<Value, StaticSlotsPiece<2>>;
    using Traits = LayoutTraits<ObjectLayout>;

    STATIC_REQUIRE(Traits::may_contain_references);
    STATIC_REQUIRE_FALSE(Traits::has_static_size);

    const size_t dynamic_capacity = 3;
    auto object = make_dynamic<ObjectLayout>(
        dynamic_capacity, invalid_type, DynamicSlotsInit(dynamic_capacity), StaticSlotsInit());

    REQUIRE(object->static_slot_count() == 2);
    REQUIRE(object->dynamic_slot_capacity() == 3);
    REQUIRE(Traits::dynamic_size(object.get()) >= sizeof(Value) * 5);
    REQUIRE(Traits::dynamic_size(object.get()) == Traits::dynamic_size(dynamic_capacity));
    REQUIRE(trace_count(object.get()) == 2);

    REQUIRE(object->static_slot(0)->is_null());
    REQUIRE(object->static_slot(1)->is_null());
    REQUIRE(object->dynamic_slot_count() == 0);
}

TEST_CASE("Dynamic slots layout should support adding and removing elements", "[layout]") {
    using ObjectLayout = DynamicSlotsLayout<Value, StaticSlotsPiece<2>>;

    const size_t dynamic_capacity = 3;
    auto object = make_dynamic<ObjectLayout>(
        dynamic_capacity, invalid_type, DynamicSlotsInit(dynamic_capacity), StaticSlotsInit());

    auto require_slots = [&](std::vector<int> expected) {
        REQUIRE(object->dynamic_slot_count() == expected.size());
        for (size_t i = 0; i < expected.size(); ++i) {
            CAPTURE(i, expected[i]);

            auto value = *object->dynamic_slot(i);
            REQUIRE(value.is<SmallInteger>());
            REQUIRE(value.as<SmallInteger>().value() == expected[i]);
        }
    };

    object->add_dynamic_slot(make_int(1));
    require_slots({1});

    object->add_dynamic_slot(make_int(2));
    require_slots({1, 2});

    object->add_dynamic_slot(make_int(3));
    require_slots({1, 2, 3});

    object->remove_dynamic_slot();
    require_slots({1, 2});

    object->remove_dynamic_slot();
    require_slots({1});

    object->remove_dynamic_slot();
    require_slots({});
}

TEST_CASE("Dynamic slots layout should support tracing with dynamic elements", "[layout]") {
    using ObjectLayout = DynamicSlotsLayout<Value, StaticSlotsPiece<2>>;
    using Traits = LayoutTraits<ObjectLayout>;

    STATIC_REQUIRE(Traits::may_contain_references);
    STATIC_REQUIRE_FALSE(Traits::has_static_size);

    const size_t dynamic_capacity = 3;
    auto object = make_dynamic<ObjectLayout>(
        dynamic_capacity, invalid_type, DynamicSlotsInit(dynamic_capacity), StaticSlotsInit());

    object->add_dynamic_slot(make_int(1));
    object->add_dynamic_slot(make_int(2));
    object->add_dynamic_slot(make_int(3));
    REQUIRE(object->dynamic_slot_count() == 3);
    REQUIRE(trace_count(object.get()) == 5);
}

TEST_CASE("Buffer layout should construct a valid buffer", "[layout]") {
    using ObjectLayout = BufferLayout<u32, alignof(u32)>;
    using Traits = LayoutTraits<ObjectLayout>;

    const size_t buffer_capacity = 123;
    auto init_buffer = [&](Span<u32> buffer_span) {
        std::uninitialized_fill(buffer_span.begin(), buffer_span.end(), 12345);
    };

    auto object = make_dynamic<ObjectLayout>(
        buffer_capacity, invalid_type, BufferInit(buffer_capacity, init_buffer));
    REQUIRE(object->buffer_capacity() == 123);
    REQUIRE(Traits::dynamic_size(object.get()) >= 4 * buffer_capacity);
    REQUIRE(Traits::dynamic_size(object.get()) == Traits::dynamic_size(buffer_capacity));

    auto buffer = object->buffer();
    REQUIRE(buffer.size() == buffer_capacity);
    REQUIRE(std::all_of(buffer.begin(), buffer.end(), [&](auto item) { return item == 12345; }));
}

TEST_CASE("Buffer layout without slots should have no references", "[layout]") {
    using ObjectLayout = BufferLayout<u32, alignof(u32), StaticPayloadPiece<NativePayload>>;
    using Traits = LayoutTraits<ObjectLayout>;

    STATIC_REQUIRE_FALSE(Traits::has_static_size);
    STATIC_REQUIRE_FALSE(Traits::may_contain_references);

    const size_t buffer_capacity = 123;
    auto init_buffer = [&](Span<u32> buffer_span) {
        std::uninitialized_fill(buffer_span.begin(), buffer_span.end(), 12345);
    };

    auto object = make_dynamic<ObjectLayout>(buffer_capacity, invalid_type,
        BufferInit(buffer_capacity, init_buffer), StaticPayloadInit());
    REQUIRE(object->static_payload()->foo == 1234);
    REQUIRE(object->buffer_capacity() == 123);
}

TEST_CASE("Buffer layout with slots should have references", "[layout]") {
    using ObjectLayout = BufferLayout<u32, alignof(u32), StaticSlotsPiece<3>>;
    using Traits = LayoutTraits<ObjectLayout>;

    STATIC_REQUIRE_FALSE(Traits::has_static_size);
    STATIC_REQUIRE(Traits::may_contain_references);

    const size_t buffer_capacity = 123;
    auto init_buffer = [&](Span<u32> buffer_span) {
        std::uninitialized_fill(buffer_span.begin(), buffer_span.end(), 12345);
    };

    auto object = make_dynamic<ObjectLayout>(
        buffer_capacity, invalid_type, BufferInit(buffer_capacity, init_buffer), StaticSlotsInit());
    REQUIRE(object->static_slot_count() == 3);
    REQUIRE(trace_count(object.get()) == 3);
}
