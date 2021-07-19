#include "bytecode/writer.hpp"

#include "bytecode/instruction.hpp"
#include "common/math.hpp"

#include <type_traits>

namespace tiro {

BytecodeWriter::BytecodeWriter(BytecodeFunction& output)
    : output_(output)
    , wr_(output.code()) {}

void BytecodeWriter::define_label(BytecodeLabel label) {
    TIRO_DEBUG_ASSERT(label, "invalid label");

    TIRO_DEBUG_ASSERT(label_defs_.count(label) == 0, "label was already defined");
    label_defs_[label] = pos();
}

void BytecodeWriter::start_handler(BytecodeLabel handler) {
    if (handler == handler_)
        return;

    finish_handler();
    handler_ = handler;
    handler_start_ = pos();
}

void BytecodeWriter::finish() {
    // Close current handler entry, if any.
    finish_handler();

    for (const auto& [pos, target] : label_refs_) {
        const auto def = label_defs_.find(target);
        TIRO_CHECK(def != label_defs_.end(), "label was never defined.");
        wr_.overwrite_u32(pos, def->second);
    }

    auto& complete_handlers = output_.handlers();
    complete_handlers.reserve(handlers_.size());
    for (auto& entry : handlers_) {
        const auto def = label_defs_.find(entry.target);
        TIRO_CHECK(def != label_defs_.end(), "label was never defined.");
        complete_handlers.push_back(
            ExceptionHandler(entry.from, entry.to, BytecodeOffset(def->second)));
    }

    simplify_handlers(complete_handlers);
}

void BytecodeWriter::finish_handler() {
    const auto current_pos = pos();
    if (handler_ && handler_start_ != current_pos) {
        // Note: raw value used for target until finish() patches it.
        handlers_.push_back(
            {BytecodeOffset(handler_start_), BytecodeOffset(current_pos), handler_});
    }

    handler_ = {};
    handler_start_ = 0;
}

// Merge adjacent handler entries that  have the same destination offset.
// This can happen when when some labels are empty.
void BytecodeWriter::simplify_handlers(std::vector<ExceptionHandler>& handlers) {
    auto out = handlers.begin();
    auto end = handlers.end();
    if (out == end)
        return;

    auto in = out;
    ++in;
    for (; in != end; ++in) {
        if (in->from == out->to && in->target == out->target) {
            out->to = in->to;
        } else {
            ++out;
            *out = *in;
        }
    }
    ++out;
    handlers.erase(out, end);
}

void BytecodeWriter::write_impl(BytecodeOp op) {
    wr_.emit_u8(static_cast<u8>(op));
}

void BytecodeWriter::write_impl(BytecodeParam param) {
    wr_.emit_u32(param.value());
}

void BytecodeWriter::write_impl(BytecodeRegister local) {
    wr_.emit_u32(local.value());
}

void BytecodeWriter::write_impl(BytecodeOffset offset) {
    TIRO_DEBUG_ASSERT(offset.valid(), "invalid offset");
    wr_.emit_u32(offset.value());
}

void BytecodeWriter::write_impl(BytecodeLabel label) {
    TIRO_DEBUG_ASSERT(label.valid(), "invalid label");
    label_refs_.emplace_back(pos(), label);
    wr_.emit_u32(BytecodeOffset::invalid_value);
}

void BytecodeWriter::write_impl(BytecodeMemberId index) {
    TIRO_DEBUG_ASSERT(index.valid(), "invalid module index");

    module_refs_.emplace_back(pos(), index);
    wr_.emit_u32(BytecodeMemberId::invalid_value);
}

void BytecodeWriter::write_impl(u32 value) {
    wr_.emit_u32(value);
}

void BytecodeWriter::write_impl(i64 value) {
    wr_.emit_i64(value);
}

void BytecodeWriter::write_impl(f64 value) {
    wr_.emit_f64(value);
}

u32 BytecodeWriter::pos() const {
    const u32 pos = checked_cast<u32>(wr_.pos());
    return pos;
}

} // namespace tiro
