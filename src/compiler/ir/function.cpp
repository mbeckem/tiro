#include "compiler/ir/function.hpp"

#include "compiler/ir/traversal.hpp"
#include "compiler/utils.hpp"

#include <cmath>
#include <type_traits>

namespace tiro {

template<typename ID, typename Vec>
static bool check_id(const ID& id, const Vec& vec) {
    return id && id.value() < vec.size();
}

std::string_view to_string(FunctionType type) {
    switch (type) {
    case FunctionType::Normal:
        return "Normal";
    case FunctionType::Closure:
        return "Closure";
    }
    TIRO_UNREACHABLE("Invalid function type.");
}

Function::Function(InternedString name, FunctionType type, StringTable& strings)
    : strings_(TIRO_NN(&strings))
    , name_(name)
    , type_(type) {
    entry_ = make(Block(strings.insert("entry")));
    exit_ = make(Block(strings.insert("exit")));

    auto exit = (*this)[exit_];
    exit->terminator(Terminator::Exit{});
}

Function::~Function() {}

BlockId Function::make(Block&& block) {
    return blocks_.push_back(std::move(block));
}

ParamId Function::make(const Param& param) {
    return params_.push_back(param);
}

LocalId Function::make(const Local& local) {
    return locals_.push_back(local);
}

PhiId Function::make(Phi&& phi) {
    return phis_.push_back(std::move(phi));
}

LocalListId Function::make(LocalList&& local_list) {
    return local_lists_.push_back(std::move(local_list));
}

RecordId Function::make(Record&& record) {
    return records_.push_back(std::move(record));
}

BlockId Function::entry() const {
    return entry_;
}

BlockId Function::exit() const {
    return exit_;
}

NotNull<VecPtr<Block>> Function::operator[](BlockId id) {
    TIRO_DEBUG_ASSERT(check_id(id, blocks_), "Invalid block id.");
    return TIRO_NN(blocks_.ptr_to(id));
}

NotNull<VecPtr<Param>> Function::operator[](ParamId id) {
    TIRO_DEBUG_ASSERT(check_id(id, params_), "Invalid param id.");
    return TIRO_NN(params_.ptr_to(id));
}

NotNull<VecPtr<Local>> Function::operator[](LocalId id) {
    TIRO_DEBUG_ASSERT(check_id(id, locals_), "Invalid local id.");
    return TIRO_NN(locals_.ptr_to(id));
}

NotNull<VecPtr<Phi>> Function::operator[](PhiId id) {
    TIRO_DEBUG_ASSERT(check_id(id, phis_), "Invalid phi id.");
    return TIRO_NN(phis_.ptr_to(id));
}

NotNull<VecPtr<LocalList>> Function::operator[](LocalListId id) {
    TIRO_DEBUG_ASSERT(check_id(id, local_lists_), "Invalid local list id.");
    return TIRO_NN(local_lists_.ptr_to(id));
}

NotNull<VecPtr<Record>> Function::operator[](RecordId id) {
    TIRO_DEBUG_ASSERT(check_id(id, local_lists_), "Invalid record id.");
    return TIRO_NN(records_.ptr_to(id));
}

NotNull<VecPtr<const Block>> Function::operator[](BlockId id) const {
    TIRO_DEBUG_ASSERT(check_id(id, blocks_), "Invalid block id.");
    return TIRO_NN(blocks_.ptr_to(id));
}

NotNull<VecPtr<const Param>> Function::operator[](ParamId id) const {
    TIRO_DEBUG_ASSERT(check_id(id, params_), "Invalid param id.");
    return TIRO_NN(params_.ptr_to(id));
}

NotNull<VecPtr<const Local>> Function::operator[](LocalId id) const {
    TIRO_DEBUG_ASSERT(check_id(id, locals_), "Invalid local id.");
    return TIRO_NN(locals_.ptr_to(id));
}

NotNull<VecPtr<const Phi>> Function::operator[](PhiId id) const {
    TIRO_DEBUG_ASSERT(check_id(id, phis_), "Invalid phi id.");
    return TIRO_NN(phis_.ptr_to(id));
}

NotNull<VecPtr<const LocalList>> Function::operator[](LocalListId id) const {
    TIRO_DEBUG_ASSERT(check_id(id, local_lists_), "Invalid local list id.");
    return TIRO_NN(local_lists_.ptr_to(id));
}

NotNull<VecPtr<const Record>> Function::operator[](RecordId id) const {
    TIRO_DEBUG_ASSERT(check_id(id, local_lists_), "Invalid record id.");
    return TIRO_NN(records_.ptr_to(id));
}

void dump_function(const Function& func, FormatStream& stream) {
    using namespace dump_helpers;

    StringTable& strings = func.strings();

    stream.format(
        "Function\n"
        "  Name: {}\n"
        "  Type: {}\n",
        strings.dump(func.name()), func.type());

    // Walk the control flow graph
    stream.format("\n");
    for (auto block_id : ReversePostorderTraversal(func)) {
        if (block_id != func.entry())
            stream.format("\n");

        auto block = func[block_id];

        stream.format("{} (sealed: {}, filled: {})\n", dump(func, block_id), block->sealed(),
            block->filled());

        if (block->predecessor_count() > 0) {
            stream.format("  <- ");
            {
                size_t index = 0;
                for (auto pred : block->predecessors()) {
                    if (index++ != 0)
                        stream.format(", ");
                    stream.format("{}", dump(func, pred));
                }
            }
            stream.format("\n");
        }

        const size_t stmt_count = block->stmt_count();
        const size_t max_index_length = fmt::formatted_size(
            "{}", stmt_count == 0 ? 0 : stmt_count - 1);

        size_t index = 0;
        for (const auto& stmt : block->stmts()) {
            stream.format("  {index:>{width}}: {value}", fmt::arg("index", index),
                fmt::arg("width", max_index_length), fmt::arg("value", dump(func, stmt)));

            stream.format("\n");
            ++index;
        }
        stream.format("  {}\n", dump(func, block->terminator()));
    }
}

Param::Param(InternedString name)
    : name_(name) {
    TIRO_DEBUG_ASSERT(name, "Parameters must have valid names.");
}

InternedString Param::name() const {
    return name_;
}

void Param::format(FormatStream& stream) const {
    stream.format("Param({})", name());
}

/* [[[cog
    from codegen.unions import implement
    from codegen.ir import Terminator
    implement(Terminator.tag)
]]] */
std::string_view to_string(TerminatorType type) {
    switch (type) {
    case TerminatorType::None:
        return "None";
    case TerminatorType::Jump:
        return "Jump";
    case TerminatorType::Branch:
        return "Branch";
    case TerminatorType::Return:
        return "Return";
    case TerminatorType::Exit:
        return "Exit";
    case TerminatorType::AssertFail:
        return "AssertFail";
    case TerminatorType::Never:
        return "Never";
    }
    TIRO_UNREACHABLE("Invalid TerminatorType.");
}
// [[[end]]]

std::string_view to_string(BranchType type) {
    switch (type) {
#define TIRO_CASE(T)    \
    case BranchType::T: \
        return #T;

        TIRO_CASE(IfTrue)
        TIRO_CASE(IfFalse)
        TIRO_CASE(IfNull)
        TIRO_CASE(IfNotNull)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid branch type.");
}

/* [[[cog
    from codegen.unions import implement
    from codegen.ir import Terminator
    implement(Terminator)
]]] */
Terminator Terminator::make_none() {
    return {None{}};
}

Terminator Terminator::make_jump(const BlockId& target) {
    return {Jump{target}};
}

Terminator Terminator::make_branch(const BranchType& type, const LocalId& value,
    const BlockId& target, const BlockId& fallthrough) {
    return {Branch{type, value, target, fallthrough}};
}

Terminator Terminator::make_return(const LocalId& value, const BlockId& target) {
    return {Return{value, target}};
}

Terminator Terminator::make_exit() {
    return {Exit{}};
}

Terminator
Terminator::make_assert_fail(const LocalId& expr, const LocalId& message, const BlockId& target) {
    return {AssertFail{expr, message, target}};
}

Terminator Terminator::make_never(const BlockId& target) {
    return {Never{target}};
}

Terminator::Terminator(None none)
    : type_(TerminatorType::None)
    , none_(std::move(none)) {}

Terminator::Terminator(Jump jump)
    : type_(TerminatorType::Jump)
    , jump_(std::move(jump)) {}

Terminator::Terminator(Branch branch)
    : type_(TerminatorType::Branch)
    , branch_(std::move(branch)) {}

Terminator::Terminator(Return ret)
    : type_(TerminatorType::Return)
    , return_(std::move(ret)) {}

Terminator::Terminator(Exit exit)
    : type_(TerminatorType::Exit)
    , exit_(std::move(exit)) {}

Terminator::Terminator(AssertFail assert_fail)
    : type_(TerminatorType::AssertFail)
    , assert_fail_(std::move(assert_fail)) {}

Terminator::Terminator(Never never)
    : type_(TerminatorType::Never)
    , never_(std::move(never)) {}

const Terminator::None& Terminator::as_none() const {
    TIRO_DEBUG_ASSERT(
        type_ == TerminatorType::None, "Bad member access on Terminator: not a None.");
    return none_;
}

const Terminator::Jump& Terminator::as_jump() const {
    TIRO_DEBUG_ASSERT(
        type_ == TerminatorType::Jump, "Bad member access on Terminator: not a Jump.");
    return jump_;
}

const Terminator::Branch& Terminator::as_branch() const {
    TIRO_DEBUG_ASSERT(
        type_ == TerminatorType::Branch, "Bad member access on Terminator: not a Branch.");
    return branch_;
}

const Terminator::Return& Terminator::as_return() const {
    TIRO_DEBUG_ASSERT(
        type_ == TerminatorType::Return, "Bad member access on Terminator: not a Return.");
    return return_;
}

const Terminator::Exit& Terminator::as_exit() const {
    TIRO_DEBUG_ASSERT(
        type_ == TerminatorType::Exit, "Bad member access on Terminator: not a Exit.");
    return exit_;
}

const Terminator::AssertFail& Terminator::as_assert_fail() const {
    TIRO_DEBUG_ASSERT(
        type_ == TerminatorType::AssertFail, "Bad member access on Terminator: not a AssertFail.");
    return assert_fail_;
}

const Terminator::Never& Terminator::as_never() const {
    TIRO_DEBUG_ASSERT(
        type_ == TerminatorType::Never, "Bad member access on Terminator: not a Never.");
    return never_;
}

void Terminator::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void visit_none([[maybe_unused]] const None& none) { stream.format("None"); }

        void visit_jump([[maybe_unused]] const Jump& jump) {
            stream.format("Jump(target: {})", jump.target);
        }

        void visit_branch([[maybe_unused]] const Branch& branch) {
            stream.format("Branch(type: {}, value: {}, target: {}, fallthrough: {})", branch.type,
                branch.value, branch.target, branch.fallthrough);
        }

        void visit_return([[maybe_unused]] const Return& ret) {
            stream.format("Return(value: {}, target: {})", ret.value, ret.target);
        }

        void visit_exit([[maybe_unused]] const Exit& exit) { stream.format("Exit"); }

        void visit_assert_fail([[maybe_unused]] const AssertFail& assert_fail) {
            stream.format("AssertFail(expr: {}, message: {}, target: {})", assert_fail.expr,
                assert_fail.message, assert_fail.target);
        }

        void visit_never([[maybe_unused]] const Never& never) {
            stream.format("Never(target: {})", never.target);
        }
    };
    visit(FormatVisitor{stream});
}

// [[[end]]]

void visit_targets(const Terminator& terminator, FunctionRef<void(BlockId)> callback) {
    struct Visitor {
        FunctionRef<void(BlockId)>& callback;

        void visit_none([[maybe_unused]] const Terminator::None& none) {}

        void visit_jump(const Terminator::Jump& jump) { callback(jump.target); }

        void visit_branch(const Terminator::Branch& branch) {
            callback(branch.target);
            callback(branch.fallthrough);
        }

        void visit_return(const Terminator::Return& ret) { callback(ret.target); }

        void visit_exit([[maybe_unused]] const Terminator::Exit& ex) {}

        void visit_assert_fail(const Terminator::AssertFail& assert_fail) {
            callback(assert_fail.target);
        }

        void visit_never(const Terminator::Never& never) { callback(never.target); }
    };
    Visitor visitor{callback};

    terminator.visit(visitor);
}

u32 target_count(const Terminator& term) {
    u32 count = 0;
    visit_targets(term, [&](BlockId) { ++count; });
    return count;
}

Block::Block(InternedString label)
    : label_(label) {
    TIRO_DEBUG_ASSERT(label, "Basic blocks must have a valid label.");
}

Block::~Block() {}

BlockId Block::predecessor(size_t index) const {
    TIRO_DEBUG_ASSERT(index < predecessors_.size(), "Index out of bounds.");
    return predecessors_[index];
}

size_t Block::predecessor_count() const {
    return predecessors_.size();
}

void Block::append_predecessor(BlockId predecessor) {
    predecessors_.push_back(predecessor);
}

void Block::replace_predecessor(BlockId old_pred, BlockId new_pred) {
    // TODO: Keep in mind that this will cause problems if the same source block
    // can have multiple edges to the same target. This could happen with
    // more advanced optimizations.
    auto pos = std::find(predecessors_.begin(), predecessors_.end(), old_pred);
    if (pos != predecessors_.end())
        *pos = new_pred;
}

const Stmt& Block::stmt(size_t index) const {
    TIRO_DEBUG_ASSERT(index < stmts_.size(), "Index out of bounds.");
    return stmts_[index];
}

size_t Block::stmt_count() const {
    return stmts_.size();
}

void Block::insert_stmt(size_t index, const Stmt& stmt) {
    TIRO_DEBUG_ASSERT(index <= stmts_.size(), "Index out of bounds.");
    stmts_.insert(stmts_.begin() + index, stmt);
}

void Block::insert_stmts(size_t index, Span<const Stmt> stmts) {
    TIRO_DEBUG_ASSERT(index <= stmts_.size(), "Index out of bounds.");
    stmts_.insert(stmts_.begin() + index, stmts.begin(), stmts.end());
}

void Block::append_stmt(const Stmt& stmt) {
    stmts_.push_back(stmt);
}

size_t Block::phi_count(const Function& parent) const {
    auto non_phi = std::find_if(
        stmts_.begin(), stmts_.end(), [&](const auto& s) { return !is_phi_define(parent, s); });
    return static_cast<size_t>(non_phi - stmts_.begin());
}

void Block::remove_phi(Function& parent, LocalId local_id, const RValue& new_value) {
    TIRO_DEBUG_ASSERT(new_value.type() != RValueType::Phi0 && new_value.type() != RValueType::Phi,
        "New value must not be a phi node.");

    const auto phi_start = stmts_.begin();
    const auto phi_end = stmts_.begin() + phi_count(parent);
    const auto old_pos = std::find_if(phi_start, phi_end, [&](const Stmt& stmt) {
        return stmt.type() == StmtType::Define && stmt.as_define().local == local_id;
    });

    TIRO_DEBUG_ASSERT(old_pos != phi_end, "Failed to find the definition among the phi functions.");

    parent[local_id]->value(new_value);
    std::rotate(old_pos, old_pos + 1, phi_end); // Move after other phis
}

void Block::format(FormatStream& stream) const {
    stream.format("Block(label: {})", label_);
}

/* [[[cog
    from codegen.unions import implement
    from codegen.ir import LValue
    implement(LValue.tag)
]]] */
std::string_view to_string(LValueType type) {
    switch (type) {
    case LValueType::Param:
        return "Param";
    case LValueType::Closure:
        return "Closure";
    case LValueType::Module:
        return "Module";
    case LValueType::Field:
        return "Field";
    case LValueType::TupleField:
        return "TupleField";
    case LValueType::Index:
        return "Index";
    }
    TIRO_UNREACHABLE("Invalid LValueType.");
}
// [[[end]]]

/* [[[cog
    from codegen.unions import implement
    from codegen.ir import LValue
    implement(LValue)
]]] */
LValue LValue::make_param(const ParamId& target) {
    return {Param{target}};
}

LValue LValue::make_closure(const LocalId& env, const u32& levels, const u32& index) {
    return {Closure{env, levels, index}};
}

LValue LValue::make_module(const ModuleMemberId& member) {
    return {Module{member}};
}

LValue LValue::make_field(const LocalId& object, const InternedString& name) {
    return {Field{object, name}};
}

LValue LValue::make_tuple_field(const LocalId& object, const u32& index) {
    return {TupleField{object, index}};
}

LValue LValue::make_index(const LocalId& object, const LocalId& index) {
    return {Index{object, index}};
}

LValue::LValue(Param param)
    : type_(LValueType::Param)
    , param_(std::move(param)) {}

LValue::LValue(Closure closure)
    : type_(LValueType::Closure)
    , closure_(std::move(closure)) {}

LValue::LValue(Module module)
    : type_(LValueType::Module)
    , module_(std::move(module)) {}

LValue::LValue(Field field)
    : type_(LValueType::Field)
    , field_(std::move(field)) {}

LValue::LValue(TupleField tuple_field)
    : type_(LValueType::TupleField)
    , tuple_field_(std::move(tuple_field)) {}

LValue::LValue(Index index)
    : type_(LValueType::Index)
    , index_(std::move(index)) {}

const LValue::Param& LValue::as_param() const {
    TIRO_DEBUG_ASSERT(type_ == LValueType::Param, "Bad member access on LValue: not a Param.");
    return param_;
}

const LValue::Closure& LValue::as_closure() const {
    TIRO_DEBUG_ASSERT(type_ == LValueType::Closure, "Bad member access on LValue: not a Closure.");
    return closure_;
}

const LValue::Module& LValue::as_module() const {
    TIRO_DEBUG_ASSERT(type_ == LValueType::Module, "Bad member access on LValue: not a Module.");
    return module_;
}

const LValue::Field& LValue::as_field() const {
    TIRO_DEBUG_ASSERT(type_ == LValueType::Field, "Bad member access on LValue: not a Field.");
    return field_;
}

const LValue::TupleField& LValue::as_tuple_field() const {
    TIRO_DEBUG_ASSERT(
        type_ == LValueType::TupleField, "Bad member access on LValue: not a TupleField.");
    return tuple_field_;
}

const LValue::Index& LValue::as_index() const {
    TIRO_DEBUG_ASSERT(type_ == LValueType::Index, "Bad member access on LValue: not a Index.");
    return index_;
}

void LValue::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void visit_param([[maybe_unused]] const Param& param) {
            stream.format("Param(target: {})", param.target);
        }

        void visit_closure([[maybe_unused]] const Closure& closure) {
            stream.format("Closure(env: {}, levels: {}, index: {})", closure.env, closure.levels,
                closure.index);
        }

        void visit_module([[maybe_unused]] const Module& module) {
            stream.format("Module(member: {})", module.member);
        }

        void visit_field([[maybe_unused]] const Field& field) {
            stream.format("Field(object: {}, name: {})", field.object, field.name);
        }

        void visit_tuple_field([[maybe_unused]] const TupleField& tuple_field) {
            stream.format(
                "TupleField(object: {}, index: {})", tuple_field.object, tuple_field.index);
        }

        void visit_index([[maybe_unused]] const Index& index) {
            stream.format("Index(object: {}, index: {})", index.object, index.index);
        }
    };
    visit(FormatVisitor{stream});
}

// [[[end]]]

/* [[[cog
    from codegen.unions import implement
    from codegen.ir import Constant
    implement(Constant.tag)
]]] */
std::string_view to_string(ConstantType type) {
    switch (type) {
    case ConstantType::Integer:
        return "Integer";
    case ConstantType::Float:
        return "Float";
    case ConstantType::String:
        return "String";
    case ConstantType::Symbol:
        return "Symbol";
    case ConstantType::Null:
        return "Null";
    case ConstantType::True:
        return "True";
    case ConstantType::False:
        return "False";
    }
    TIRO_UNREACHABLE("Invalid ConstantType.");
}
// [[[end]]]

void FloatConstant::format(FormatStream& stream) const {
    stream.format("Float({})", value);
}

void FloatConstant::hash(Hasher& h) const {
    if (std::isnan(value)) {
        h.append(6.5670192717080285e+99); // Some random value
    } else {
        h.append(value);
    }
}

bool operator==(const FloatConstant& lhs, const FloatConstant& rhs) {
    if (std::isnan(lhs.value) && std::isnan(rhs.value))
        return true;
    return lhs.value == rhs.value;
}

bool operator!=(const FloatConstant& lhs, const FloatConstant& rhs) {
    return !(lhs == rhs);
}

bool operator<(const FloatConstant& lhs, const FloatConstant& rhs) {
    return lhs.value < rhs.value;
}

bool operator>(const FloatConstant& lhs, const FloatConstant& rhs) {
    return rhs < lhs;
}

bool operator<=(const FloatConstant& lhs, const FloatConstant& rhs) {
    return lhs < rhs || lhs == rhs;
}

bool operator>=(const FloatConstant& lhs, const FloatConstant& rhs) {
    return rhs <= lhs;
}

/* [[[cog
    from codegen.unions import implement
    from codegen.ir import Constant
    implement(Constant)
]]] */
Constant Constant::make_integer(const i64& value) {
    return {Integer{value}};
}

Constant Constant::make_float(const Float& f) {
    return f;
}

Constant Constant::make_string(const InternedString& value) {
    return {String{value}};
}

Constant Constant::make_symbol(const InternedString& value) {
    return {Symbol{value}};
}

Constant Constant::make_null() {
    return {Null{}};
}

Constant Constant::make_true() {
    return {True{}};
}

Constant Constant::make_false() {
    return {False{}};
}

Constant::Constant(Integer integer)
    : type_(ConstantType::Integer)
    , integer_(std::move(integer)) {}

Constant::Constant(Float f)
    : type_(ConstantType::Float)
    , float_(std::move(f)) {}

Constant::Constant(String string)
    : type_(ConstantType::String)
    , string_(std::move(string)) {}

Constant::Constant(Symbol symbol)
    : type_(ConstantType::Symbol)
    , symbol_(std::move(symbol)) {}

Constant::Constant(Null null)
    : type_(ConstantType::Null)
    , null_(std::move(null)) {}

Constant::Constant(True t)
    : type_(ConstantType::True)
    , true_(std::move(t)) {}

Constant::Constant(False f)
    : type_(ConstantType::False)
    , false_(std::move(f)) {}

const Constant::Integer& Constant::as_integer() const {
    TIRO_DEBUG_ASSERT(
        type_ == ConstantType::Integer, "Bad member access on Constant: not a Integer.");
    return integer_;
}

const Constant::Float& Constant::as_float() const {
    TIRO_DEBUG_ASSERT(type_ == ConstantType::Float, "Bad member access on Constant: not a Float.");
    return float_;
}

const Constant::String& Constant::as_string() const {
    TIRO_DEBUG_ASSERT(
        type_ == ConstantType::String, "Bad member access on Constant: not a String.");
    return string_;
}

const Constant::Symbol& Constant::as_symbol() const {
    TIRO_DEBUG_ASSERT(
        type_ == ConstantType::Symbol, "Bad member access on Constant: not a Symbol.");
    return symbol_;
}

const Constant::Null& Constant::as_null() const {
    TIRO_DEBUG_ASSERT(type_ == ConstantType::Null, "Bad member access on Constant: not a Null.");
    return null_;
}

const Constant::True& Constant::as_true() const {
    TIRO_DEBUG_ASSERT(type_ == ConstantType::True, "Bad member access on Constant: not a True.");
    return true_;
}

const Constant::False& Constant::as_false() const {
    TIRO_DEBUG_ASSERT(type_ == ConstantType::False, "Bad member access on Constant: not a False.");
    return false_;
}

void Constant::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void visit_integer([[maybe_unused]] const Integer& integer) {
            stream.format("Integer(value: {})", integer.value);
        }

        void visit_float([[maybe_unused]] const Float& f) { stream.format("{}", f); }

        void visit_string([[maybe_unused]] const String& string) {
            stream.format("String(value: {})", string.value);
        }

        void visit_symbol([[maybe_unused]] const Symbol& symbol) {
            stream.format("Symbol(value: {})", symbol.value);
        }

        void visit_null([[maybe_unused]] const Null& null) { stream.format("Null"); }

        void visit_true([[maybe_unused]] const True& t) { stream.format("True"); }

        void visit_false([[maybe_unused]] const False& f) { stream.format("False"); }
    };
    visit(FormatVisitor{stream});
}

void Constant::hash(Hasher& h) const {
    h.append(type());

    struct HashVisitor {
        Hasher& h;

        void visit_integer([[maybe_unused]] const Integer& integer) { h.append(integer.value); }

        void visit_float([[maybe_unused]] const Float& f) { h.append(f); }

        void visit_string([[maybe_unused]] const String& string) { h.append(string.value); }

        void visit_symbol([[maybe_unused]] const Symbol& symbol) { h.append(symbol.value); }

        void visit_null([[maybe_unused]] const Null& null) { return; }

        void visit_true([[maybe_unused]] const True& t) { return; }

        void visit_false([[maybe_unused]] const False& f) { return; }
    };
    return visit(HashVisitor{h});
}

bool operator==(const Constant& lhs, const Constant& rhs) {
    if (lhs.type() != rhs.type())
        return false;

    struct EqualityVisitor {
        const Constant& rhs;

        bool visit_integer([[maybe_unused]] const Constant::Integer& integer) {
            [[maybe_unused]] const auto& other = rhs.as_integer();
            return integer.value == other.value;
        }

        bool visit_float([[maybe_unused]] const Constant::Float& f) {
            [[maybe_unused]] const auto& other = rhs.as_float();
            return f == other;
        }

        bool visit_string([[maybe_unused]] const Constant::String& string) {
            [[maybe_unused]] const auto& other = rhs.as_string();
            return string.value == other.value;
        }

        bool visit_symbol([[maybe_unused]] const Constant::Symbol& symbol) {
            [[maybe_unused]] const auto& other = rhs.as_symbol();
            return symbol.value == other.value;
        }

        bool visit_null([[maybe_unused]] const Constant::Null& null) {
            [[maybe_unused]] const auto& other = rhs.as_null();
            return true;
        }

        bool visit_true([[maybe_unused]] const Constant::True& t) {
            [[maybe_unused]] const auto& other = rhs.as_true();
            return true;
        }

        bool visit_false([[maybe_unused]] const Constant::False& f) {
            [[maybe_unused]] const auto& other = rhs.as_false();
            return true;
        }
    };
    return lhs.visit(EqualityVisitor{rhs});
}

bool operator!=(const Constant& lhs, const Constant& rhs) {
    return !(lhs == rhs);
}
// [[[end]]]

bool is_same(const Constant& lhs, const Constant& rhs) {
    if (lhs.type() == ConstantType::Float && rhs.type() == ConstantType::Float) {

        if (std::isnan(lhs.as_float().value) && std::isnan(lhs.as_float().value))
            return true;
    }

    return lhs == rhs;
}

/* [[[cog
    from codegen.unions import implement
    from codegen.ir import Aggregate
    implement(Aggregate.tag)
]]] */
std::string_view to_string(AggregateType type) {
    switch (type) {
    case AggregateType::Method:
        return "Method";
    case AggregateType::IteratorNext:
        return "IteratorNext";
    }
    TIRO_UNREACHABLE("Invalid AggregateType.");
}
/// [[[end]]]

/* [[[cog
    from codegen.unions import implement
    from codegen.ir import Aggregate
    implement(Aggregate)
]]] */
Aggregate Aggregate::make_method(const LocalId& instance, const InternedString& function) {
    return {Method{instance, function}};
}

Aggregate Aggregate::make_iterator_next(const LocalId& iterator) {
    return {IteratorNext{iterator}};
}

Aggregate::Aggregate(Method method)
    : type_(AggregateType::Method)
    , method_(std::move(method)) {}

Aggregate::Aggregate(IteratorNext iterator_next)
    : type_(AggregateType::IteratorNext)
    , iterator_next_(std::move(iterator_next)) {}

const Aggregate::Method& Aggregate::as_method() const {
    TIRO_DEBUG_ASSERT(
        type_ == AggregateType::Method, "Bad member access on Aggregate: not a Method.");
    return method_;
}

const Aggregate::IteratorNext& Aggregate::as_iterator_next() const {
    TIRO_DEBUG_ASSERT(type_ == AggregateType::IteratorNext,
        "Bad member access on Aggregate: not a IteratorNext.");
    return iterator_next_;
}

void Aggregate::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void visit_method([[maybe_unused]] const Method& method) {
            stream.format("Method(instance: {}, function: {})", method.instance, method.function);
        }

        void visit_iterator_next([[maybe_unused]] const IteratorNext& iterator_next) {
            stream.format("IteratorNext(iterator: {})", iterator_next.iterator);
        }
    };
    visit(FormatVisitor{stream});
}

void Aggregate::hash(Hasher& h) const {
    h.append(type());

    struct HashVisitor {
        Hasher& h;

        void visit_method([[maybe_unused]] const Method& method) {
            h.append(method.instance).append(method.function);
        }

        void visit_iterator_next([[maybe_unused]] const IteratorNext& iterator_next) {
            h.append(iterator_next.iterator);
        }
    };
    return visit(HashVisitor{h});
}

bool operator==(const Aggregate& lhs, const Aggregate& rhs) {
    if (lhs.type() != rhs.type())
        return false;

    struct EqualityVisitor {
        const Aggregate& rhs;

        bool visit_method([[maybe_unused]] const Aggregate::Method& method) {
            [[maybe_unused]] const auto& other = rhs.as_method();
            return method.instance == other.instance && method.function == other.function;
        }

        bool visit_iterator_next([[maybe_unused]] const Aggregate::IteratorNext& iterator_next) {
            [[maybe_unused]] const auto& other = rhs.as_iterator_next();
            return iterator_next.iterator == other.iterator;
        }
    };
    return lhs.visit(EqualityVisitor{rhs});
}

bool operator!=(const Aggregate& lhs, const Aggregate& rhs) {
    return !(lhs == rhs);
}
/// [[[end]]]

AggregateType aggregate_type(AggregateMember member) {
    switch (member) {
    case AggregateMember::MethodInstance:
    case AggregateMember::MethodFunction:
        return AggregateType::Method;
    case AggregateMember::IteratorNextValid:
    case AggregateMember::IteratorNextValue:
        return AggregateType::IteratorNext;
    }

    TIRO_UNREACHABLE("Invalid aggregate member.");
}

std::string_view to_string(AggregateMember member) {
    switch (member) {
#define TIRO_CASE(T)         \
    case AggregateMember::T: \
        return #T;

        TIRO_CASE(MethodInstance)
        TIRO_CASE(MethodFunction)
        TIRO_CASE(IteratorNextValid)
        TIRO_CASE(IteratorNextValue)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid aggregate member.");
}

/* [[[cog
    from codegen.unions import implement
    from codegen.ir import RValue
    implement(RValue.tag)
]]] */
std::string_view to_string(RValueType type) {
    switch (type) {
    case RValueType::UseLValue:
        return "UseLValue";
    case RValueType::UseLocal:
        return "UseLocal";
    case RValueType::Phi:
        return "Phi";
    case RValueType::Phi0:
        return "Phi0";
    case RValueType::Constant:
        return "Constant";
    case RValueType::OuterEnvironment:
        return "OuterEnvironment";
    case RValueType::BinaryOp:
        return "BinaryOp";
    case RValueType::UnaryOp:
        return "UnaryOp";
    case RValueType::Call:
        return "Call";
    case RValueType::Aggregate:
        return "Aggregate";
    case RValueType::GetAggregateMember:
        return "GetAggregateMember";
    case RValueType::MethodCall:
        return "MethodCall";
    case RValueType::MakeEnvironment:
        return "MakeEnvironment";
    case RValueType::MakeClosure:
        return "MakeClosure";
    case RValueType::MakeIterator:
        return "MakeIterator";
    case RValueType::Record:
        return "Record";
    case RValueType::Container:
        return "Container";
    case RValueType::Format:
        return "Format";
    case RValueType::Error:
        return "Error";
    }
    TIRO_UNREACHABLE("Invalid RValueType.");
}
// [[[end]]]

/* [[[cog
    from codegen.unions import implement
    from codegen.ir import RValue
    implement(RValue)
]]] */
RValue RValue::make_use_lvalue(const LValue& target) {
    return {UseLValue{target}};
}

RValue RValue::make_use_local(const LocalId& target) {
    return {UseLocal{target}};
}

RValue RValue::make_phi(const PhiId& value) {
    return {Phi{value}};
}

RValue RValue::make_phi0() {
    return {Phi0{}};
}

RValue RValue::make_constant(const Constant& constant) {
    return constant;
}

RValue RValue::make_outer_environment() {
    return {OuterEnvironment{}};
}

RValue RValue::make_binary_op(const BinaryOpType& op, const LocalId& left, const LocalId& right) {
    return {BinaryOp{op, left, right}};
}

RValue RValue::make_unary_op(const UnaryOpType& op, const LocalId& operand) {
    return {UnaryOp{op, operand}};
}

RValue RValue::make_call(const LocalId& func, const LocalListId& args) {
    return {Call{func, args}};
}

RValue RValue::make_aggregate(const Aggregate& aggregate) {
    return aggregate;
}

RValue RValue::make_get_aggregate_member(const LocalId& aggregate, const AggregateMember& member) {
    return {GetAggregateMember{aggregate, member}};
}

RValue RValue::make_method_call(const LocalId& method, const LocalListId& args) {
    return {MethodCall{method, args}};
}

RValue RValue::make_make_environment(const LocalId& parent, const u32& size) {
    return {MakeEnvironment{parent, size}};
}

RValue RValue::make_make_closure(const LocalId& env, const LocalId& func) {
    return {MakeClosure{env, func}};
}

RValue RValue::make_make_iterator(const LocalId& container) {
    return {MakeIterator{container}};
}

RValue RValue::make_record(const RecordId& value) {
    return {Record{value}};
}

RValue RValue::make_container(const ContainerType& container, const LocalListId& args) {
    return {Container{container, args}};
}

RValue RValue::make_format(const LocalListId& args) {
    return {Format{args}};
}

RValue RValue::make_error() {
    return {Error{}};
}

RValue::RValue(UseLValue use_lvalue)
    : type_(RValueType::UseLValue)
    , use_lvalue_(std::move(use_lvalue)) {}

RValue::RValue(UseLocal use_local)
    : type_(RValueType::UseLocal)
    , use_local_(std::move(use_local)) {}

RValue::RValue(Phi phi)
    : type_(RValueType::Phi)
    , phi_(std::move(phi)) {}

RValue::RValue(Phi0 phi0)
    : type_(RValueType::Phi0)
    , phi0_(std::move(phi0)) {}

RValue::RValue(Constant constant)
    : type_(RValueType::Constant)
    , constant_(std::move(constant)) {}

RValue::RValue(OuterEnvironment outer_environment)
    : type_(RValueType::OuterEnvironment)
    , outer_environment_(std::move(outer_environment)) {}

RValue::RValue(BinaryOp binary_op)
    : type_(RValueType::BinaryOp)
    , binary_op_(std::move(binary_op)) {}

RValue::RValue(UnaryOp unary_op)
    : type_(RValueType::UnaryOp)
    , unary_op_(std::move(unary_op)) {}

RValue::RValue(Call call)
    : type_(RValueType::Call)
    , call_(std::move(call)) {}

RValue::RValue(Aggregate aggregate)
    : type_(RValueType::Aggregate)
    , aggregate_(std::move(aggregate)) {}

RValue::RValue(GetAggregateMember get_aggregate_member)
    : type_(RValueType::GetAggregateMember)
    , get_aggregate_member_(std::move(get_aggregate_member)) {}

RValue::RValue(MethodCall method_call)
    : type_(RValueType::MethodCall)
    , method_call_(std::move(method_call)) {}

RValue::RValue(MakeEnvironment make_environment)
    : type_(RValueType::MakeEnvironment)
    , make_environment_(std::move(make_environment)) {}

RValue::RValue(MakeClosure make_closure)
    : type_(RValueType::MakeClosure)
    , make_closure_(std::move(make_closure)) {}

RValue::RValue(MakeIterator make_iterator)
    : type_(RValueType::MakeIterator)
    , make_iterator_(std::move(make_iterator)) {}

RValue::RValue(Record record)
    : type_(RValueType::Record)
    , record_(std::move(record)) {}

RValue::RValue(Container container)
    : type_(RValueType::Container)
    , container_(std::move(container)) {}

RValue::RValue(Format format)
    : type_(RValueType::Format)
    , format_(std::move(format)) {}

RValue::RValue(Error error)
    : type_(RValueType::Error)
    , error_(std::move(error)) {}

const RValue::UseLValue& RValue::as_use_lvalue() const {
    TIRO_DEBUG_ASSERT(
        type_ == RValueType::UseLValue, "Bad member access on RValue: not a UseLValue.");
    return use_lvalue_;
}

const RValue::UseLocal& RValue::as_use_local() const {
    TIRO_DEBUG_ASSERT(
        type_ == RValueType::UseLocal, "Bad member access on RValue: not a UseLocal.");
    return use_local_;
}

const RValue::Phi& RValue::as_phi() const {
    TIRO_DEBUG_ASSERT(type_ == RValueType::Phi, "Bad member access on RValue: not a Phi.");
    return phi_;
}

const RValue::Phi0& RValue::as_phi0() const {
    TIRO_DEBUG_ASSERT(type_ == RValueType::Phi0, "Bad member access on RValue: not a Phi0.");
    return phi0_;
}

const RValue::Constant& RValue::as_constant() const {
    TIRO_DEBUG_ASSERT(
        type_ == RValueType::Constant, "Bad member access on RValue: not a Constant.");
    return constant_;
}

const RValue::OuterEnvironment& RValue::as_outer_environment() const {
    TIRO_DEBUG_ASSERT(type_ == RValueType::OuterEnvironment,
        "Bad member access on RValue: not a OuterEnvironment.");
    return outer_environment_;
}

const RValue::BinaryOp& RValue::as_binary_op() const {
    TIRO_DEBUG_ASSERT(
        type_ == RValueType::BinaryOp, "Bad member access on RValue: not a BinaryOp.");
    return binary_op_;
}

const RValue::UnaryOp& RValue::as_unary_op() const {
    TIRO_DEBUG_ASSERT(type_ == RValueType::UnaryOp, "Bad member access on RValue: not a UnaryOp.");
    return unary_op_;
}

const RValue::Call& RValue::as_call() const {
    TIRO_DEBUG_ASSERT(type_ == RValueType::Call, "Bad member access on RValue: not a Call.");
    return call_;
}

const RValue::Aggregate& RValue::as_aggregate() const {
    TIRO_DEBUG_ASSERT(
        type_ == RValueType::Aggregate, "Bad member access on RValue: not a Aggregate.");
    return aggregate_;
}

const RValue::GetAggregateMember& RValue::as_get_aggregate_member() const {
    TIRO_DEBUG_ASSERT(type_ == RValueType::GetAggregateMember,
        "Bad member access on RValue: not a GetAggregateMember.");
    return get_aggregate_member_;
}

const RValue::MethodCall& RValue::as_method_call() const {
    TIRO_DEBUG_ASSERT(
        type_ == RValueType::MethodCall, "Bad member access on RValue: not a MethodCall.");
    return method_call_;
}

const RValue::MakeEnvironment& RValue::as_make_environment() const {
    TIRO_DEBUG_ASSERT(type_ == RValueType::MakeEnvironment,
        "Bad member access on RValue: not a MakeEnvironment.");
    return make_environment_;
}

const RValue::MakeClosure& RValue::as_make_closure() const {
    TIRO_DEBUG_ASSERT(
        type_ == RValueType::MakeClosure, "Bad member access on RValue: not a MakeClosure.");
    return make_closure_;
}

const RValue::MakeIterator& RValue::as_make_iterator() const {
    TIRO_DEBUG_ASSERT(
        type_ == RValueType::MakeIterator, "Bad member access on RValue: not a MakeIterator.");
    return make_iterator_;
}

const RValue::Record& RValue::as_record() const {
    TIRO_DEBUG_ASSERT(type_ == RValueType::Record, "Bad member access on RValue: not a Record.");
    return record_;
}

const RValue::Container& RValue::as_container() const {
    TIRO_DEBUG_ASSERT(
        type_ == RValueType::Container, "Bad member access on RValue: not a Container.");
    return container_;
}

const RValue::Format& RValue::as_format() const {
    TIRO_DEBUG_ASSERT(type_ == RValueType::Format, "Bad member access on RValue: not a Format.");
    return format_;
}

const RValue::Error& RValue::as_error() const {
    TIRO_DEBUG_ASSERT(type_ == RValueType::Error, "Bad member access on RValue: not a Error.");
    return error_;
}

void RValue::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void visit_use_lvalue([[maybe_unused]] const UseLValue& use_lvalue) {
            stream.format("UseLValue(target: {})", use_lvalue.target);
        }

        void visit_use_local([[maybe_unused]] const UseLocal& use_local) {
            stream.format("UseLocal(target: {})", use_local.target);
        }

        void visit_phi([[maybe_unused]] const Phi& phi) {
            stream.format("Phi(value: {})", phi.value);
        }

        void visit_phi0([[maybe_unused]] const Phi0& phi0) { stream.format("Phi0"); }

        void visit_constant([[maybe_unused]] const Constant& constant) {
            stream.format("{}", constant);
        }

        void visit_outer_environment([[maybe_unused]] const OuterEnvironment& outer_environment) {
            stream.format("OuterEnvironment");
        }

        void visit_binary_op([[maybe_unused]] const BinaryOp& binary_op) {
            stream.format("BinaryOp(op: {}, left: {}, right: {})", binary_op.op, binary_op.left,
                binary_op.right);
        }

        void visit_unary_op([[maybe_unused]] const UnaryOp& unary_op) {
            stream.format("UnaryOp(op: {}, operand: {})", unary_op.op, unary_op.operand);
        }

        void visit_call([[maybe_unused]] const Call& call) {
            stream.format("Call(func: {}, args: {})", call.func, call.args);
        }

        void visit_aggregate([[maybe_unused]] const Aggregate& aggregate) {
            stream.format("{}", aggregate);
        }

        void visit_get_aggregate_member(
            [[maybe_unused]] const GetAggregateMember& get_aggregate_member) {
            stream.format("GetAggregateMember(aggregate: {}, member: {})",
                get_aggregate_member.aggregate, get_aggregate_member.member);
        }

        void visit_method_call([[maybe_unused]] const MethodCall& method_call) {
            stream.format("MethodCall(method: {}, args: {})", method_call.method, method_call.args);
        }

        void visit_make_environment([[maybe_unused]] const MakeEnvironment& make_environment) {
            stream.format("MakeEnvironment(parent: {}, size: {})", make_environment.parent,
                make_environment.size);
        }

        void visit_make_closure([[maybe_unused]] const MakeClosure& make_closure) {
            stream.format("MakeClosure(env: {}, func: {})", make_closure.env, make_closure.func);
        }

        void visit_make_iterator([[maybe_unused]] const MakeIterator& make_iterator) {
            stream.format("MakeIterator(container: {})", make_iterator.container);
        }

        void visit_record([[maybe_unused]] const Record& record) {
            stream.format("Record(value: {})", record.value);
        }

        void visit_container([[maybe_unused]] const Container& container) {
            stream.format(
                "Container(container: {}, args: {})", container.container, container.args);
        }

        void visit_format([[maybe_unused]] const Format& format) {
            stream.format("Format(args: {})", format.args);
        }

        void visit_error([[maybe_unused]] const Error& error) { stream.format("Error"); }
    };
    visit(FormatVisitor{stream});
}

// [[[end]]]

Local::Local(const RValue& value)
    : value_(value) {}

void Local::format(FormatStream& stream) const {
    stream.format("Local(name: {}, value: {})", name(), value());
}

Phi::Phi() {}

Phi::Phi(std::initializer_list<LocalId> operands)
    : operands_(operands.begin(), operands.end()) {}

Phi::Phi(std::vector<LocalId>&& operands)
    : operands_(std::move(operands)) {}

Phi::~Phi() {}

void Phi::append_operand(LocalId operand) {
    operands_.push_back(operand);
}

LocalId Phi::operand(size_t index) const {
    TIRO_DEBUG_ASSERT(index < operands_.size(), "Operand index out of bounds.");
    return operands_[index];
}

void Phi::operand(size_t index, LocalId local) {
    TIRO_DEBUG_ASSERT(index < operands_.size(), "Operand index out of bounds.");
    operands_[index] = local;
}

void Phi::format(FormatStream& stream) const {
    stream.format("Phi(");

    size_t index = 0;
    for (const auto& op : operands_) {
        if (index++ != 0)
            stream.format(", ");
        stream.format("{}", op);
    }

    stream.format(")");
}

LocalList::LocalList() {}

LocalList::LocalList(std::initializer_list<LocalId> locals)
    : locals_(locals) {}

LocalList::LocalList(std::vector<LocalId>&& locals)
    : locals_(std::move(locals)) {}

LocalList::~LocalList() {}

Record::Record() {}

void Record::set(InternedString name, LocalId value) {
    TIRO_DEBUG_ASSERT(name, "Invalid name.");
    TIRO_DEBUG_ASSERT(value, "Invalid value.");
    props_[name] = value;
}

std::string_view to_string(BinaryOpType type) {
    switch (type) {
#define TIRO_CASE(X, Y)   \
    case BinaryOpType::X: \
        return Y;

        TIRO_CASE(Plus, "+")
        TIRO_CASE(Minus, "-")
        TIRO_CASE(Multiply, "*")
        TIRO_CASE(Divide, "/")
        TIRO_CASE(Modulus, "mod")
        TIRO_CASE(Power, "pow")
        TIRO_CASE(LeftShift, "lsh")
        TIRO_CASE(RightShift, "rsh")
        TIRO_CASE(BitwiseAnd, "band")
        TIRO_CASE(BitwiseOr, "bor")
        TIRO_CASE(BitwiseXor, "bxor")
        TIRO_CASE(Less, "lt")
        TIRO_CASE(LessEquals, "lte")
        TIRO_CASE(Greater, "gt")
        TIRO_CASE(GreaterEquals, "gte")
        TIRO_CASE(Equals, "eq")
        TIRO_CASE(NotEquals, "neq")

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid binary operation type.");
}

std::string_view to_string(UnaryOpType type) {
    switch (type) {
#define TIRO_CASE(X, Y)  \
    case UnaryOpType::X: \
        return Y;

        TIRO_CASE(Plus, "+")
        TIRO_CASE(Minus, "-")
        TIRO_CASE(BitwiseNot, "bnot")
        TIRO_CASE(LogicalNot, "lnot")

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid unary operation type.");
}

std::string_view to_string(ContainerType type) {
    switch (type) {
#define TIRO_CASE(X)       \
    case ContainerType::X: \
        return #X;

        TIRO_CASE(Array)
        TIRO_CASE(Tuple)
        TIRO_CASE(Set)
        TIRO_CASE(Map)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid container type.");
}

/* [[[cog
    from codegen.unions import implement
    from codegen.ir import Stmt
    implement(Stmt.tag)
]]] */
std::string_view to_string(StmtType type) {
    switch (type) {
    case StmtType::Assign:
        return "Assign";
    case StmtType::Define:
        return "Define";
    }
    TIRO_UNREACHABLE("Invalid StmtType.");
}
// [[[end]]]

/* [[[cog
    from codegen.unions import implement
    from codegen.ir import Stmt
    implement(Stmt)
]]] */
Stmt Stmt::make_assign(const LValue& target, const LocalId& value) {
    return {Assign{target, value}};
}

Stmt Stmt::make_define(const LocalId& local) {
    return {Define{local}};
}

Stmt::Stmt(Assign assign)
    : type_(StmtType::Assign)
    , assign_(std::move(assign)) {}

Stmt::Stmt(Define define)
    : type_(StmtType::Define)
    , define_(std::move(define)) {}

const Stmt::Assign& Stmt::as_assign() const {
    TIRO_DEBUG_ASSERT(type_ == StmtType::Assign, "Bad member access on Stmt: not a Assign.");
    return assign_;
}

const Stmt::Define& Stmt::as_define() const {
    TIRO_DEBUG_ASSERT(type_ == StmtType::Define, "Bad member access on Stmt: not a Define.");
    return define_;
}

void Stmt::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void visit_assign([[maybe_unused]] const Assign& assign) {
            stream.format("Assign(target: {}, value: {})", assign.target, assign.value);
        }

        void visit_define([[maybe_unused]] const Define& define) {
            stream.format("Define(local: {})", define.local);
        }
    };
    visit(FormatVisitor{stream});
}

// [[[end]]]

bool is_phi_define(const Function& func, const Stmt& stmt) {
    if (stmt.type() != StmtType::Define)
        return false;

    const auto& def = stmt.as_define();
    if (!def.local)
        return false;

    auto local = func[def.local];
    auto type = local->value().type();
    return type == RValueType::Phi || type == RValueType::Phi0;
}

namespace dump_helpers {

void format(const Dump<BlockId>& d, FormatStream& stream) {
    auto block_id = d.value;
    if (!block_id) {
        stream.format("<INVALID>");
        return;
    }

    auto& func = d.parent;
    auto block = func[block_id];

    if (block->label()) {
        stream.format("${}-{}", func.strings().value(block->label()), block_id.value());
    } else {
        stream.format("${}", block_id.value());
    }
}

void format(const Dump<Terminator>& d, FormatStream& stream) {
    struct Visitor {
        const Function& func;
        FormatStream& stream;

        void visit_none([[maybe_unused]] const Terminator::None& none) { stream.format("-> none"); }

        void visit_jump(const Terminator::Jump& jump) {
            stream.format("-> jump {}", dump(func, jump.target));
        }

        void visit_branch(const Terminator::Branch& branch) {
            stream.format("-> branch {} {} target: {} fallthrough: {}", branch.type,
                dump(func, branch.value), dump(func, branch.target),
                dump(func, branch.fallthrough));
        }

        void visit_return(const Terminator::Return& ret) {
            stream.format("-> return {} target: {}", dump(func, ret.value), dump(func, ret.target));
        }

        void visit_exit([[maybe_unused]] const Terminator::Exit& exit) { stream.format("-> exit"); }

        void visit_assert_fail(const Terminator::AssertFail& fail) {
            stream.format("-> assert fail expr: {} message: {} target: {}", dump(func, fail.expr),
                dump(func, fail.message), dump(func, fail.target));
        }

        void visit_never(const Terminator::Never& never) {
            stream.format("-> never {}", dump(func, never.target));
        }
    };
    Visitor visitor{d.parent, stream};
    d.value.visit(visitor);
}

void format(const Dump<LValue>& d, FormatStream& stream) {
    struct Visitor {
        const Function& func;
        FormatStream& stream;

        void visit_param(const LValue::Param& param) {
            stream.format("<param {}>", param.target.value());
        }

        void visit_closure(const LValue::Closure& closure) {
            stream.format("<closure {} level: {} index: {}>", dump(func, closure.env),
                closure.levels, closure.index);
        }

        void visit_module(const LValue::Module& module) {
            stream.format("<module {}>", module.member.value());
        }

        void visit_field(const LValue::Field& field) {
            stream.format("{}.{}", dump(func, field.object), func.strings().dump(field.name));
        }

        void visit_tuple_field(const LValue::TupleField& field) {
            stream.format("{}.{}", dump(func, field.object), field.index);
        }

        void visit_index(const LValue::Index& index) {
            stream.format("{}[{}]", dump(func, index.object), dump(func, index.index));
        }
    };
    Visitor visitor{d.parent, stream};
    d.value.visit(visitor);
}

void format(const Dump<Constant>& d, FormatStream& stream) {
    struct Visitor {
        const Function& func;
        FormatStream& stream;

        void visit_integer(const Constant::Integer& i) { stream.format("{}", i.value); }

        void visit_float(const Constant::Float& f) { stream.format("{:f}", f.value); }

        void visit_string(const Constant::String& str) {
            if (!str.value) {
                stream.format("\"\"");
                return;
            }

            auto escaped = escape_string(func.strings().value(str.value));
            stream.format("\"{}\"", escaped);
        }

        void visit_symbol(const Constant::Symbol& sym) {
            stream.format("#{}", func.strings().dump(sym.value));
        }

        void visit_null([[maybe_unused]] const Constant::Null& null) { stream.format("null"); }

        void visit_true([[maybe_unused]] const Constant::True& t) { stream.format("true"); }

        void visit_false([[maybe_unused]] const Constant::False& f) { stream.format("false"); }
    };
    Visitor visitor{d.parent, stream};
    d.value.visit(visitor);
}

void format(const Dump<Aggregate>& d, FormatStream& stream) {
    struct Visitor {
        const Function& func;
        FormatStream& stream;

        void visit_method(const Aggregate::Method& m) {
            stream.format(
                "<method {}.{}>", dump(func, m.instance), func.strings().dump(m.function));
        }

        void visit_iterator_next(const Aggregate::IteratorNext& i) {
            stream.format("<iterator-next {}>", dump(func, i.iterator));
        }
    };
    Visitor visitor{d.parent, stream};
    d.value.visit(visitor);
}

void format(const Dump<RValue>& d, FormatStream& stream) {
    struct Visitor {
        const Function& func;
        FormatStream& stream;

        void visit_use_lvalue(const RValue::UseLValue& use) {
            return format(dump(func, use.target), stream);
        }

        void visit_use_local(const RValue::UseLocal& use) {
            return format(dump(func, use.target), stream);
        }

        void visit_phi(const RValue::Phi& phi) { return format(dump(func, phi.value), stream); }

        void visit_phi0([[maybe_unused]] const RValue::Phi0& phi) { stream.format("<phi>"); }

        void visit_constant(const RValue::Constant& constant) {
            return format(dump(func, constant), stream);
        }

        void visit_outer_environment([[maybe_unused]] const RValue::OuterEnvironment& outer) {
            stream.format("<outer-env>");
        }

        void visit_binary_op(const RValue::BinaryOp& binop) {
            stream.format("{} {} {}", dump(func, binop.left), binop.op, dump(func, binop.right));
        }

        void visit_unary_op(const RValue::UnaryOp& unop) {
            stream.format("{} {} ", unop.op, dump(func, unop.operand));
        }

        void visit_call(const RValue::Call& call) {
            stream.format("{}({})", dump(func, call.func), dump(func, call.args));
        }

        void visit_aggregate(const RValue::Aggregate& agg) {
            return format(dump(func, agg), stream);
        }

        void visit_get_aggregate_member(const RValue::GetAggregateMember& get) {
            stream.format("<get-aggregate-member {} {}>", dump(func, get.aggregate), get.member);
        }

        void visit_method_call(const RValue::MethodCall& call) {
            stream.format("{}({})", dump(func, call.method), dump(func, call.args));
        }

        void visit_make_environment(const RValue::MakeEnvironment& env) {
            stream.format("<make-env {} {}>", dump(func, env.parent), env.size);
        }

        void visit_make_closure(const RValue::MakeClosure& closure) {
            stream.format("<make-closure env: {} func: {}>", dump(func, closure.env),
                dump(func, closure.func));
        }

        void visit_make_iterator(const RValue::MakeIterator& iter) {
            stream.format("<make-iterator container: {}>", dump(func, iter.container));
        }

        void visit_record(const RValue::Record& record) {
            return format(dump(func, record.value), stream);
        }

        void visit_container(const RValue::Container& cont) {
            stream.format("{}({})", cont.container, dump(func, cont.args));
        }

        void visit_format(const RValue::Format& format) {
            stream.format("<format {}>", dump(func, format.args));
        }

        void visit_error([[maybe_unused]] const RValue::Error& error) { stream.format("<error>"); }
    };

    Visitor visitor{d.parent, stream};
    d.value.visit(visitor);
}

void format(const Dump<LocalId>& d, FormatStream& stream) {
    auto local_id = d.value;
    if (!local_id) {
        stream.format("<INVALID>");
        return;
    }

    auto& func = d.parent;
    auto& strings = func.strings();
    auto local = func[local_id];
    if (local->name()) {
        stream.format("%{1}_{0}", local_id.value(), strings.value(local->name()));
    } else {
        stream.format("%{}", local_id.value());
    }
}

void format(const Dump<LocalListId>& d, FormatStream& stream) {
    auto list_id = d.value;
    if (!list_id) {
        stream.format("<INVALID>");
        return;
    }

    auto& func = d.parent;
    auto list = func[list_id];

    size_t index = 0;
    for (auto local : *list) {
        if (index++ > 0)
            stream.format(", ");
        format(dump(func, local), stream);
    }
}

void format(const Dump<RecordId>& d, FormatStream& stream) {
    auto record_id = d.value;
    if (!record_id) {
        stream.format("<INVALID>");
        return;
    }

    auto& func = d.parent;
    auto record = func[record_id];

    stream.format("<record");
    size_t index = 0;
    for (const auto [name, value] : *record) {
        if (index++ > 0)
            stream.format(",");
        stream.format(" {}: {}", func.strings().dump(name), dump(func, value));
    }
    stream.format(">");
}

void format(const Dump<PhiId>& d, FormatStream& stream) {
    auto phi_id = d.value;
    if (!phi_id) {
        stream.format("<INVALID>");
        return;
    }

    auto& func = d.parent;
    auto phi = func[phi_id];

    if (phi->operand_count() == 0) {
        stream.format("<phi>");
        return;
    }

    stream.format("<phi");
    for (const auto& op : phi->operands()) {
        stream.format(" {}", dump(func, op));
    }
    stream.format(">");
}

void format(const Dump<Stmt>& d, FormatStream& stream) {
    struct Visitor {
        const Function& func;
        FormatStream& stream;

        void visit_assign(const Stmt::Assign& assign) {
            stream.format("{} = {}", dump(func, assign.target), dump(func, assign.value));
        }

        void visit_define(const Stmt::Define& define) {
            auto local_id = define.local;
            if (!local_id) {
                stream.format("<INVALID>");
                return;
            }

            auto local = func[local_id];
            stream.format("{} = {}", dump(func, local_id), dump(func, local->value()));
        }
    };

    Visitor visitor{d.parent, stream};
    return d.value.visit(visitor);
}

} // namespace dump_helpers

// Check that the most frequently used types are trivial:
/* [[[cog
    import cog
    from codegen.unions import implement
    from codegen.ir import Terminator, LValue, Constant, RValue, Stmt
    types = [Terminator, LValue, Constant, RValue, Stmt]

    def check_trivial(name):
        return (
            f"static_assert(std::is_trivially_copyable_v<{name}>);\n"
            f"static_assert(std::is_trivially_destructible_v<{name}>);"
        )

    for index, type in enumerate(types):
        if index != 0:
            cog.outl()

        for member in type.members:
            cog.outl(check_trivial(f"{type.name}::{member.name}"))
        cog.outl(check_trivial(type.name))

    cog.outl()
    cog.outl(check_trivial("Local"))
    cog.outl(check_trivial("Param"))
]]] */
static_assert(std::is_trivially_copyable_v<Terminator::None>);
static_assert(std::is_trivially_destructible_v<Terminator::None>);
static_assert(std::is_trivially_copyable_v<Terminator::Jump>);
static_assert(std::is_trivially_destructible_v<Terminator::Jump>);
static_assert(std::is_trivially_copyable_v<Terminator::Branch>);
static_assert(std::is_trivially_destructible_v<Terminator::Branch>);
static_assert(std::is_trivially_copyable_v<Terminator::Return>);
static_assert(std::is_trivially_destructible_v<Terminator::Return>);
static_assert(std::is_trivially_copyable_v<Terminator::Exit>);
static_assert(std::is_trivially_destructible_v<Terminator::Exit>);
static_assert(std::is_trivially_copyable_v<Terminator::AssertFail>);
static_assert(std::is_trivially_destructible_v<Terminator::AssertFail>);
static_assert(std::is_trivially_copyable_v<Terminator::Never>);
static_assert(std::is_trivially_destructible_v<Terminator::Never>);
static_assert(std::is_trivially_copyable_v<Terminator>);
static_assert(std::is_trivially_destructible_v<Terminator>);

static_assert(std::is_trivially_copyable_v<LValue::Param>);
static_assert(std::is_trivially_destructible_v<LValue::Param>);
static_assert(std::is_trivially_copyable_v<LValue::Closure>);
static_assert(std::is_trivially_destructible_v<LValue::Closure>);
static_assert(std::is_trivially_copyable_v<LValue::Module>);
static_assert(std::is_trivially_destructible_v<LValue::Module>);
static_assert(std::is_trivially_copyable_v<LValue::Field>);
static_assert(std::is_trivially_destructible_v<LValue::Field>);
static_assert(std::is_trivially_copyable_v<LValue::TupleField>);
static_assert(std::is_trivially_destructible_v<LValue::TupleField>);
static_assert(std::is_trivially_copyable_v<LValue::Index>);
static_assert(std::is_trivially_destructible_v<LValue::Index>);
static_assert(std::is_trivially_copyable_v<LValue>);
static_assert(std::is_trivially_destructible_v<LValue>);

static_assert(std::is_trivially_copyable_v<Constant::Integer>);
static_assert(std::is_trivially_destructible_v<Constant::Integer>);
static_assert(std::is_trivially_copyable_v<Constant::Float>);
static_assert(std::is_trivially_destructible_v<Constant::Float>);
static_assert(std::is_trivially_copyable_v<Constant::String>);
static_assert(std::is_trivially_destructible_v<Constant::String>);
static_assert(std::is_trivially_copyable_v<Constant::Symbol>);
static_assert(std::is_trivially_destructible_v<Constant::Symbol>);
static_assert(std::is_trivially_copyable_v<Constant::Null>);
static_assert(std::is_trivially_destructible_v<Constant::Null>);
static_assert(std::is_trivially_copyable_v<Constant::True>);
static_assert(std::is_trivially_destructible_v<Constant::True>);
static_assert(std::is_trivially_copyable_v<Constant::False>);
static_assert(std::is_trivially_destructible_v<Constant::False>);
static_assert(std::is_trivially_copyable_v<Constant>);
static_assert(std::is_trivially_destructible_v<Constant>);

static_assert(std::is_trivially_copyable_v<RValue::UseLValue>);
static_assert(std::is_trivially_destructible_v<RValue::UseLValue>);
static_assert(std::is_trivially_copyable_v<RValue::UseLocal>);
static_assert(std::is_trivially_destructible_v<RValue::UseLocal>);
static_assert(std::is_trivially_copyable_v<RValue::Phi>);
static_assert(std::is_trivially_destructible_v<RValue::Phi>);
static_assert(std::is_trivially_copyable_v<RValue::Phi0>);
static_assert(std::is_trivially_destructible_v<RValue::Phi0>);
static_assert(std::is_trivially_copyable_v<RValue::Constant>);
static_assert(std::is_trivially_destructible_v<RValue::Constant>);
static_assert(std::is_trivially_copyable_v<RValue::OuterEnvironment>);
static_assert(std::is_trivially_destructible_v<RValue::OuterEnvironment>);
static_assert(std::is_trivially_copyable_v<RValue::BinaryOp>);
static_assert(std::is_trivially_destructible_v<RValue::BinaryOp>);
static_assert(std::is_trivially_copyable_v<RValue::UnaryOp>);
static_assert(std::is_trivially_destructible_v<RValue::UnaryOp>);
static_assert(std::is_trivially_copyable_v<RValue::Call>);
static_assert(std::is_trivially_destructible_v<RValue::Call>);
static_assert(std::is_trivially_copyable_v<RValue::Aggregate>);
static_assert(std::is_trivially_destructible_v<RValue::Aggregate>);
static_assert(std::is_trivially_copyable_v<RValue::GetAggregateMember>);
static_assert(std::is_trivially_destructible_v<RValue::GetAggregateMember>);
static_assert(std::is_trivially_copyable_v<RValue::MethodCall>);
static_assert(std::is_trivially_destructible_v<RValue::MethodCall>);
static_assert(std::is_trivially_copyable_v<RValue::MakeEnvironment>);
static_assert(std::is_trivially_destructible_v<RValue::MakeEnvironment>);
static_assert(std::is_trivially_copyable_v<RValue::MakeClosure>);
static_assert(std::is_trivially_destructible_v<RValue::MakeClosure>);
static_assert(std::is_trivially_copyable_v<RValue::MakeIterator>);
static_assert(std::is_trivially_destructible_v<RValue::MakeIterator>);
static_assert(std::is_trivially_copyable_v<RValue::Record>);
static_assert(std::is_trivially_destructible_v<RValue::Record>);
static_assert(std::is_trivially_copyable_v<RValue::Container>);
static_assert(std::is_trivially_destructible_v<RValue::Container>);
static_assert(std::is_trivially_copyable_v<RValue::Format>);
static_assert(std::is_trivially_destructible_v<RValue::Format>);
static_assert(std::is_trivially_copyable_v<RValue::Error>);
static_assert(std::is_trivially_destructible_v<RValue::Error>);
static_assert(std::is_trivially_copyable_v<RValue>);
static_assert(std::is_trivially_destructible_v<RValue>);

static_assert(std::is_trivially_copyable_v<Stmt::Assign>);
static_assert(std::is_trivially_destructible_v<Stmt::Assign>);
static_assert(std::is_trivially_copyable_v<Stmt::Define>);
static_assert(std::is_trivially_destructible_v<Stmt::Define>);
static_assert(std::is_trivially_copyable_v<Stmt>);
static_assert(std::is_trivially_destructible_v<Stmt>);

static_assert(std::is_trivially_copyable_v<Local>);
static_assert(std::is_trivially_destructible_v<Local>);
static_assert(std::is_trivially_copyable_v<Param>);
static_assert(std::is_trivially_destructible_v<Param>);
// [[[end]]]

} // namespace tiro
