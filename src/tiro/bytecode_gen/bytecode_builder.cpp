#include "tiro/bytecode_gen/bytecode_builder.hpp"

#include "tiro/bytecode/instruction.hpp"
#include "tiro/ir/types.hpp"

#include <type_traits>

namespace tiro {

BytecodeBuilder::BytecodeBuilder(
    std::vector<byte>& output, size_t total_label_count)
    : wr_(output) {
    labels_.resize(total_label_count);
}

void BytecodeBuilder::emit(const Instruction& ins) {
    struct Visitor {
        BytecodeBuilder& self;
        Opcode op;

        void visit_load_null(const Instruction::LoadNull& n) {
            self.write(op, n.target);
        }

        void visit_load_false(const Instruction::LoadFalse& f) {
            self.write(op, f.target);
        }

        void visit_load_true(const Instruction::LoadTrue& t) {
            self.write(op, t.target);
        }

        void visit_load_int(const Instruction::LoadInt& i) {
            self.write(op, i.value, i.target);
        }

        void visit_load_float(const Instruction::LoadFloat& f) {
            self.write(op, f.value, f.target);
        }

        void visit_load_param(const Instruction::LoadParam& p) {
            self.write(op, p.source, p.target);
        }

        void visit_store_param(const Instruction::StoreParam& p) {
            self.write(op, p.source, p.target);
        }

        void visit_load_module(const Instruction::LoadModule& m) {
            self.write(op, m.source, m.target);
        }

        void visit_store_module(const Instruction::StoreModule& m) {
            self.write(op, m.source, m.target);
        }

        void visit_load_member(const Instruction::LoadMember& m) {
            self.write(op, m.object, m.name, m.target);
        }

        void visit_store_member(const Instruction::StoreMember& m) {
            self.write(op, m.source, m.object, m.name);
        }

        void visit_load_tuple_member(const Instruction::LoadTupleMember& t) {
            self.write(op, t.tuple, t.index, t.target);
        }

        void visit_store_tuple_member(const Instruction::StoreTupleMember& t) {
            self.write(op, t.source, t.tuple, t.index);
        }

        void visit_load_index(const Instruction::LoadIndex& i) {
            self.write(op, i.array, i.index, i.target);
        }

        void visit_store_index(const Instruction::StoreIndex& i) {
            self.write(op, i.source, i.array, i.index);
        }

        void visit_load_closure(const Instruction::LoadClosure& c) {
            self.write(op, c.target);
        }

        void visit_load_env(const Instruction::LoadEnv& e) {
            self.write(op, e.env, e.level, e.index, e.target);
        }

        void visit_store_env(const Instruction::StoreEnv& e) {
            self.write(op, e.source, e.env, e.level, e.index);
        }

        void visit_add(const Instruction::Add& a) {
            self.write(op, a.lhs, a.rhs, a.target);
        }

        void visit_sub(const Instruction::Sub& s) {
            self.write(op, s.lhs, s.rhs, s.target);
        }

        void visit_mul(const Instruction::Mul& m) {
            self.write(op, m.lhs, m.rhs, m.target);
        }

        void visit_div(const Instruction::Div& d) {
            self.write(op, d.lhs, d.rhs, d.target);
        }

        void visit_mod(const Instruction::Mod& m) {
            self.write(op, m.lhs, m.rhs, m.target);
        }

        void visit_pow(const Instruction::Pow& p) {
            self.write(op, p.lhs, p.rhs, p.target);
        }

        void visit_uadd(const Instruction::UAdd& a) {
            self.write(op, a.value, a.target);
        }

        void visit_uneg(const Instruction::UNeg& n) {
            self.write(op, n.value, n.target);
        }

        void visit_lsh(const Instruction::LSh& s) {
            self.write(op, s.lhs, s.rhs, s.target);
        }

        void visit_rsh(const Instruction::RSh& s) {
            self.write(op, s.lhs, s.rhs, s.target);
        }

        void visit_band(const Instruction::BAnd& a) {
            self.write(op, a.lhs, a.rhs, a.target);
        }

        void visit_bor(const Instruction::BOr& o) {
            self.write(op, o.lhs, o.rhs, o.target);
        }

        void visit_bxor(const Instruction::BXor& x) {
            self.write(op, x.lhs, x.rhs, x.target);
        }

        void visit_bnot(const Instruction::BNot& n) {
            self.write(op, n.value, n.target);
        }

        void visit_gt(const Instruction::Gt& g) {
            self.write(op, g.lhs, g.rhs, g.target);
        }

        void visit_gte(const Instruction::Gte& g) {
            self.write(op, g.lhs, g.rhs, g.target);
        }

        void visit_lt(const Instruction::Lt& l) {
            self.write(op, l.lhs, l.rhs, l.target);
        }

        void visit_lte(const Instruction::Lte& l) {
            self.write(op, l.lhs, l.rhs, l.target);
        }

        void visit_eq(const Instruction::Eq& e) {
            self.write(op, e.lhs, e.rhs, e.target);
        }

        void visit_neq(const Instruction::NEq& n) {
            self.write(op, n.lhs, n.rhs, n.target);
        }

        void visit_lnot(const Instruction::LNot& n) {
            self.write(op, n.value, n.target);
        }

        void visit_array(const Instruction::Array& a) {
            self.write(op, a.count, a.target);
        }

        void visit_tuple(const Instruction::Tuple& t) {
            self.write(op, t.count, t.target);
        }

        void visit_set(const Instruction::Set& s) {
            self.write(op, s.count, s.target);
        }

        void visit_map(const Instruction::Map& m) {
            self.write(op, m.count, m.target);
        }

        void visit_env(const Instruction::Env& e) {
            self.write(op, e.parent, e.size, e.target);
        }

        void visit_closure(const Instruction::Closure& c) {
            self.write(op, c.tmpl, c.env, c.target);
        }

        // TODO the formatter stuff should be a runtime function!
        void visit_formatter(const Instruction::Formatter& f) {
            self.write(op, f.target);
        }

        void visit_append_format(const Instruction::AppendFormat& a) {
            self.write(op, a.value, a.formatter);
        }

        void visit_format_result(const Instruction::FormatResult& r) {
            self.write(op, r.formatter, r.target);
        }

        void visit_copy(const Instruction::Copy& c) {
            self.write(op, c.source, c.target);
        }

        void visit_swap(const Instruction::Swap& s) {
            self.write(op, s.a, s.b);
        }

        void visit_push(const Instruction::Push& p) { self.write(op, p.value); }

        void visit_pop(const Instruction::Pop&) { self.write(op); }

        void visit_pop_to(const Instruction::PopTo& p) {
            self.write(op, p.target);
        }

        void visit_jmp(const Instruction::Jmp& j) { self.write(op, j.target); }

        void visit_jmp_true(const Instruction::JmpTrue& j) {
            self.write(op, j.value, j.target);
        }

        void visit_jmp_false(const Instruction::JmpFalse& j) {
            self.write(op, j.value, j.target);
        }

        void visit_call(const Instruction::Call& c) {
            self.write(op, c.function, c.count);
        }

        void visit_load_method(const Instruction::LoadMethod& m) {
            self.write(op, m.object, m.name, m.thiz, m.method);
        }

        void visit_call_method(const Instruction::CallMethod& m) {
            self.write(op, m.method, m.count);
        }

        void visit_return(const Instruction::Return& r) {
            self.write(op, r.value);
        }

        void visit_assert_fail(const Instruction::AssertFail& a) {
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

CompiledOffset BytecodeBuilder::use_label(BlockID label) {
    static_assert(
        std::is_same_v<BlockID::UnderlyingType, CompiledOffset::UnderlyingType>,
        "BlockIDs and offset instances can be mapped 1 to 1.");
    TIRO_ASSERT(label, "Invalid target label.");
    return CompiledOffset(label.value());
}

void BytecodeBuilder::define_label(BlockID label) {
    const auto offset = use_label(label);
    const u32 target_pos = pos();
    TIRO_ASSERT(!labels_[offset], "Label was already defined.");
    labels_[offset] = target_pos;
}

void BytecodeBuilder::write_impl(Opcode op) {
    wr_.emit_u8(static_cast<u8>(op));
}

void BytecodeBuilder::write_impl(CompiledParamID param) {
    wr_.emit_u32(param.value());
}

void BytecodeBuilder::write_impl(CompiledLocalID local) {
    wr_.emit_u32(local.value());
}

void BytecodeBuilder::write_impl(CompiledOffset offset) {
    TIRO_ASSERT(offset.valid(), "Invalid offset.");
    label_refs_.emplace_back(pos(), offset);
    wr_.emit_u32(CompiledOffset::invalid_value);
}

void BytecodeBuilder::write_impl(CompiledModuleMemberID index) {
    TIRO_ASSERT(index.valid(), "Invalid module index.");

    module_refs_.emplace_back(pos(), index);
    wr_.emit_u32(CompiledModuleMemberID::invalid_value);
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
