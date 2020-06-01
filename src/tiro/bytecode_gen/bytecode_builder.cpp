#include "tiro/bytecode_gen/bytecode_builder.hpp"

#include "tiro/bytecode/instruction.hpp"
#include "tiro/ir/function.hpp"

#include <type_traits>

namespace tiro {

BytecodeBuilder::BytecodeBuilder(std::vector<byte>& output, size_t total_label_count)
    : wr_(output) {
    labels_.resize(total_label_count);
}

void BytecodeBuilder::emit(const BytecodeInstr& ins) {
    struct Visitor {
        BytecodeBuilder& self;
        BytecodeOp op;

        void visit_load_null(const BytecodeInstr::LoadNull& n) { self.write(op, n.target); }

        void visit_load_false(const BytecodeInstr::LoadFalse& f) { self.write(op, f.target); }

        void visit_load_true(const BytecodeInstr::LoadTrue& t) { self.write(op, t.target); }

        void visit_load_int(const BytecodeInstr::LoadInt& i) {
            self.write(op, i.constant, i.target);
        }

        void visit_load_float(const BytecodeInstr::LoadFloat& f) {
            self.write(op, f.constant, f.target);
        }

        void visit_load_param(const BytecodeInstr::LoadParam& p) {
            self.write(op, p.source, p.target);
        }

        void visit_store_param(const BytecodeInstr::StoreParam& p) {
            self.write(op, p.source, p.target);
        }

        void visit_load_module(const BytecodeInstr::LoadModule& m) {
            self.write(op, m.source, m.target);
        }

        void visit_store_module(const BytecodeInstr::StoreModule& m) {
            self.write(op, m.source, m.target);
        }

        void visit_load_member(const BytecodeInstr::LoadMember& m) {
            self.write(op, m.object, m.name, m.target);
        }

        void visit_store_member(const BytecodeInstr::StoreMember& m) {
            self.write(op, m.source, m.object, m.name);
        }

        void visit_load_tuple_member(const BytecodeInstr::LoadTupleMember& t) {
            self.write(op, t.tuple, t.index, t.target);
        }

        void visit_store_tuple_member(const BytecodeInstr::StoreTupleMember& t) {
            self.write(op, t.source, t.tuple, t.index);
        }

        void visit_load_index(const BytecodeInstr::LoadIndex& i) {
            self.write(op, i.array, i.index, i.target);
        }

        void visit_store_index(const BytecodeInstr::StoreIndex& i) {
            self.write(op, i.source, i.array, i.index);
        }

        void visit_load_closure(const BytecodeInstr::LoadClosure& c) { self.write(op, c.target); }

        void visit_load_env(const BytecodeInstr::LoadEnv& e) {
            self.write(op, e.env, e.level, e.index, e.target);
        }

        void visit_store_env(const BytecodeInstr::StoreEnv& e) {
            self.write(op, e.source, e.env, e.level, e.index);
        }

        void visit_add(const BytecodeInstr::Add& a) { self.write(op, a.lhs, a.rhs, a.target); }

        void visit_sub(const BytecodeInstr::Sub& s) { self.write(op, s.lhs, s.rhs, s.target); }

        void visit_mul(const BytecodeInstr::Mul& m) { self.write(op, m.lhs, m.rhs, m.target); }

        void visit_div(const BytecodeInstr::Div& d) { self.write(op, d.lhs, d.rhs, d.target); }

        void visit_mod(const BytecodeInstr::Mod& m) { self.write(op, m.lhs, m.rhs, m.target); }

        void visit_pow(const BytecodeInstr::Pow& p) { self.write(op, p.lhs, p.rhs, p.target); }

        void visit_uadd(const BytecodeInstr::UAdd& a) { self.write(op, a.value, a.target); }

        void visit_uneg(const BytecodeInstr::UNeg& n) { self.write(op, n.value, n.target); }

        void visit_lsh(const BytecodeInstr::LSh& s) { self.write(op, s.lhs, s.rhs, s.target); }

        void visit_rsh(const BytecodeInstr::RSh& s) { self.write(op, s.lhs, s.rhs, s.target); }

        void visit_band(const BytecodeInstr::BAnd& a) { self.write(op, a.lhs, a.rhs, a.target); }

        void visit_bor(const BytecodeInstr::BOr& o) { self.write(op, o.lhs, o.rhs, o.target); }

        void visit_bxor(const BytecodeInstr::BXor& x) { self.write(op, x.lhs, x.rhs, x.target); }

        void visit_bnot(const BytecodeInstr::BNot& n) { self.write(op, n.value, n.target); }

        void visit_gt(const BytecodeInstr::Gt& g) { self.write(op, g.lhs, g.rhs, g.target); }

        void visit_gte(const BytecodeInstr::Gte& g) { self.write(op, g.lhs, g.rhs, g.target); }

        void visit_lt(const BytecodeInstr::Lt& l) { self.write(op, l.lhs, l.rhs, l.target); }

        void visit_lte(const BytecodeInstr::Lte& l) { self.write(op, l.lhs, l.rhs, l.target); }

        void visit_eq(const BytecodeInstr::Eq& e) { self.write(op, e.lhs, e.rhs, e.target); }

        void visit_neq(const BytecodeInstr::NEq& n) { self.write(op, n.lhs, n.rhs, n.target); }

        void visit_lnot(const BytecodeInstr::LNot& n) { self.write(op, n.value, n.target); }

        void visit_array(const BytecodeInstr::Array& a) { self.write(op, a.count, a.target); }

        void visit_tuple(const BytecodeInstr::Tuple& t) { self.write(op, t.count, t.target); }

        void visit_set(const BytecodeInstr::Set& s) { self.write(op, s.count, s.target); }

        void visit_map(const BytecodeInstr::Map& m) { self.write(op, m.count, m.target); }

        void visit_env(const BytecodeInstr::Env& e) { self.write(op, e.parent, e.size, e.target); }

        void visit_closure(const BytecodeInstr::Closure& c) {
            self.write(op, c.tmpl, c.env, c.target);
        }

        // TODO the formatter stuff should be a runtime function!
        void visit_formatter(const BytecodeInstr::Formatter& f) { self.write(op, f.target); }

        void visit_append_format(const BytecodeInstr::AppendFormat& a) {
            self.write(op, a.value, a.formatter);
        }

        void visit_format_result(const BytecodeInstr::FormatResult& r) {
            self.write(op, r.formatter, r.target);
        }

        void visit_copy(const BytecodeInstr::Copy& c) { self.write(op, c.source, c.target); }

        void visit_swap(const BytecodeInstr::Swap& s) { self.write(op, s.a, s.b); }

        void visit_push(const BytecodeInstr::Push& p) { self.write(op, p.value); }

        void visit_pop(const BytecodeInstr::Pop&) { self.write(op); }

        void visit_pop_to(const BytecodeInstr::PopTo& p) { self.write(op, p.target); }

        void visit_jmp(const BytecodeInstr::Jmp& j) { self.write(op, j.offset); }

        void visit_jmp_true(const BytecodeInstr::JmpTrue& j) {
            self.write(op, j.condition, j.offset);
        }

        void visit_jmp_false(const BytecodeInstr::JmpFalse& j) {
            self.write(op, j.condition, j.offset);
        }

        void visit_call(const BytecodeInstr::Call& c) { self.write(op, c.function, c.count); }

        void visit_load_method(const BytecodeInstr::LoadMethod& m) {
            self.write(op, m.object, m.name, m.thiz, m.method);
        }

        void visit_call_method(const BytecodeInstr::CallMethod& m) {
            self.write(op, m.method, m.count);
        }

        void visit_return(const BytecodeInstr::Return& r) { self.write(op, r.value); }

        void visit_assert_fail(const BytecodeInstr::AssertFail& a) {
            self.write(op, a.expr, a.message);
        }
    };
    ins.visit(Visitor{*this, ins.type()});
}

void BytecodeBuilder::finish() {
    for (const auto& [pos, target] : label_refs_) {
        const auto offset = labels_[target];
        TIRO_CHECK(offset, "Label was never defined.");
        wr_.overwrite_u32(pos, *offset);
    }
}

BytecodeOffset BytecodeBuilder::use_label(BlockId label) {
    static_assert(std::is_same_v<BlockId::UnderlyingType, BytecodeOffset::UnderlyingType>,
        "BlockIds and offset instances can be mapped 1 to 1.");
    TIRO_DEBUG_ASSERT(label, "Invalid target label.");
    return BytecodeOffset(label.value());
}

void BytecodeBuilder::define_label(BlockId label) {
    const auto offset = use_label(label);
    const u32 target_pos = pos();
    TIRO_DEBUG_ASSERT(!labels_[offset], "Label was already defined.");
    labels_[offset] = target_pos;
}

void BytecodeBuilder::write_impl(BytecodeOp op) {
    wr_.emit_u8(static_cast<u8>(op));
}

void BytecodeBuilder::write_impl(BytecodeParam param) {
    wr_.emit_u32(param.value());
}

void BytecodeBuilder::write_impl(BytecodeRegister local) {
    wr_.emit_u32(local.value());
}

void BytecodeBuilder::write_impl(BytecodeOffset offset) {
    TIRO_DEBUG_ASSERT(offset.valid(), "Invalid offset.");
    label_refs_.emplace_back(pos(), offset);
    wr_.emit_u32(BytecodeOffset::invalid_value);
}

void BytecodeBuilder::write_impl(BytecodeMemberId index) {
    TIRO_DEBUG_ASSERT(index.valid(), "Invalid module index.");

    module_refs_.emplace_back(pos(), index);
    wr_.emit_u32(BytecodeMemberId::invalid_value);
}

void BytecodeBuilder::write_impl(u32 value) {
    wr_.emit_u32(value);
}

void BytecodeBuilder::write_impl(i64 value) {
    wr_.emit_i64(value);
}

void BytecodeBuilder::write_impl(f64 value) {
    wr_.emit_f64(value);
}

u32 BytecodeBuilder::pos() const {
    const u32 pos = checked_cast<u32>(wr_.pos());
    return pos;
}

} // namespace tiro
