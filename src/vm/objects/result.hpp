#ifndef TIRO_VM_OBJECTS_RESULT_HPP
#define TIRO_VM_OBJECTS_RESULT_HPP

#include "vm/objects/layout.hpp"
#include "vm/objects/type_desc.hpp"
#include "vm/objects/value.hpp"

namespace tiro::vm {

// TODO: Think about (simple) stack traces.
class Result final : public HeapValue {
private:
    enum Slots {
        WhichSlot, // 0 (success) or 1 (error)
        ValueSlot, // value depends on "which" above
        SlotCount_,
    };

public:
    enum Which {
        Success = 0,
        Failure = 1,
    };

    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>>;

    /// Constructs a result that contains a valid value.
    static Result make_success(Context& ctx, Handle<Value> value);

    /// Constructs a result that contains an error.
    static Result make_failure(Context& ctx, Handle<Value> reason);

    explicit Result(Value v)
        : HeapValue(v, DebugCheck<Result>()) {}

    Which which();
    bool is_success();
    bool is_failure();

    Value value();
    Value reason();

    Layout* layout() const { return access_heap<Layout>(); }

private:
    Value get_which();
    Value get_value();
};

extern const TypeDesc result_type_desc;

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_RESULT_HPP
