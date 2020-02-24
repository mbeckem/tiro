#include "tiro/mir/types.hpp"

#include "tiro/compiler/utils.hpp"

#include <queue>
#include <type_traits>
#include <unordered_set>

namespace tiro::compiler::mir {

static u32 check_size(size_t vector_index) {
    return checked_cast<u32>(vector_index);
}

template<typename ID, typename Vec>
bool check_id(const ID& id, const Vec& vec) {
    return id && id.value() < vec.size();
}

template<typename ID, typename Vec, typename T>
static ID add_impl(Vec& vec, T&& value) {
    const u32 id = check_size(vec.size());
    vec.push_back(std::forward<T>(value));
    return ID(id);
}

Module::Module(InternedString name, StringTable& strings)
    : strings_(TIRO_NN(&strings))
    , name_(name) {}

Module::~Module() {}

ModuleMemberID Module::make(const ModuleMember& member) {
    return add_impl<ModuleMemberID>(members_, member);
}

FunctionID Module::make(Function&& function) {
    return add_impl<FunctionID>(functions_, std::move(function));
}

NotNull<VecPtr<ModuleMember>> Module::operator[](ModuleMemberID id) {
    TIRO_ASSERT(check_id(id, members_), "Invalid member id.");
    return TIRO_NN(VecPtr(members_, id.value()));
}

NotNull<VecPtr<Function>> Module::operator[](FunctionID id) {
    TIRO_ASSERT(check_id(id, functions_), "Invalid function id.");
    return TIRO_NN(VecPtr(functions_, id.value()));
}

NotNull<VecPtr<const ModuleMember>> Module::
operator[](ModuleMemberID id) const {
    TIRO_ASSERT(check_id(id, members_), "Invalid member id.");
    return TIRO_NN(VecPtr(members_, id.value()));
}

NotNull<VecPtr<const Function>> Module::operator[](FunctionID id) const {
    TIRO_ASSERT(check_id(id, functions_), "Invalid function id.");
    return TIRO_NN(VecPtr(functions_, id.value()));
}

void dump_module(const Module& module, FormatStream& stream) {
    stream.format(
        "Module\n"
        "  Name: {}\n"
        "  Members: {}\n"
        "  Functions: {}\n",
        module.strings().dump(module.name()), module.member_count(),
        module.function_count());

    // Dump all members
    {
        stream.format("\nMembers:\n");

        const size_t member_count = module.member_count();
        const size_t max_index_length = fmt::formatted_size(
            "{}", member_count == 0 ? 0 : member_count - 1);

        size_t index = 0;
        for (const auto& member : module.members()) {
            stream.format("  {index:>{width}}: {value}\n",
                fmt::arg("index", index), fmt::arg("width", max_index_length),
                fmt::arg("value", member));

            if (member.type() == ModuleMemberType::Function) {
                auto function_id = member.as_function().id;
                if (function_id) {
                    const auto& function = module[function_id];
                    IndentStream indent(stream, 4);
                    dump_function(*function, indent);
                }
            }

            stream.format("\n");
            ++index;
        }
    }
}

/* [[[cog
    import mir
    codegen.implement_type(mir.ModuleMemberType)
]]] */
std::string_view to_string(ModuleMemberType type) {
    switch (type) {
    case ModuleMemberType::Import:
        return "Import";
    case ModuleMemberType::Variable:
        return "Variable";
    case ModuleMemberType::Function:
        return "Function";
    }
    TIRO_UNREACHABLE("Invalid ModuleMemberType.");
}
// [[[end]]]

/* [[[cog
    import mir
    codegen.implement_type(mir.ModuleMember)
]]] */
ModuleMember ModuleMember::make_import(const InternedString& name) {
    return Import{name};
}

ModuleMember ModuleMember::make_variable(const InternedString& name) {
    return Variable{name};
}

ModuleMember ModuleMember::make_function(const FunctionID& id) {
    return Function{id};
}

ModuleMember::ModuleMember(const Import& import)
    : type_(ModuleMemberType::Import)
    , import_(import) {}

ModuleMember::ModuleMember(const Variable& variable)
    : type_(ModuleMemberType::Variable)
    , variable_(variable) {}

ModuleMember::ModuleMember(const Function& function)
    : type_(ModuleMemberType::Function)
    , function_(function) {}

const ModuleMember::Import& ModuleMember::as_import() const {
    TIRO_ASSERT(type_ == ModuleMemberType::Import,
        "Bad member access on ModuleMember: not a Import.");
    return import_;
}

const ModuleMember::Variable& ModuleMember::as_variable() const {
    TIRO_ASSERT(type_ == ModuleMemberType::Variable,
        "Bad member access on ModuleMember: not a Variable.");
    return variable_;
}

const ModuleMember::Function& ModuleMember::as_function() const {
    TIRO_ASSERT(type_ == ModuleMemberType::Function,
        "Bad member access on ModuleMember: not a Function.");
    return function_;
}

void ModuleMember::format(FormatStream& stream) const {
    struct Formatter {
        FormatStream& stream;

        void visit_import([[maybe_unused]] const Import& import) {
            stream.format("Import(name: {})", import.name);
        }

        void visit_variable([[maybe_unused]] const Variable& variable) {
            stream.format("Variable(name: {})", variable.name);
        }

        void visit_function([[maybe_unused]] const Function& function) {
            stream.format("Function(id: {})", function.id);
        }
    };

    Formatter formatter{stream};
    visit(formatter);
}
// [[[end]]]

std::string_view to_string(FunctionType type) {
    switch (type) {
    case FunctionType::Plain:
        return "Plain";
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
    exit->edge(Edge::Exit{});
}

Function::~Function() {}

BlockID Function::make(Block&& block) {
    return add_impl<BlockID>(blocks_, std::move(block));
}

ParamID Function::make(const Param& param) {
    return add_impl<ParamID>(params_, param);
}

LocalID Function::make(const Local& local) {
    return add_impl<LocalID>(locals_, local);
}

PhiID Function::make(Phi&& phi) {
    return add_impl<PhiID>(phis_, std::move(phi));
}

LocalListID Function::make(LocalList&& local_list) {
    return add_impl<LocalListID>(local_lists_, std::move(local_list));
}

BlockID Function::entry() const {
    return entry_;
}

BlockID Function::exit() const {
    return exit_;
}

NotNull<VecPtr<Block>> Function::operator[](BlockID id) {
    TIRO_ASSERT(check_id(id, blocks_), "Invalid block id.");
    return TIRO_NN(VecPtr(blocks_, id.value()));
}

NotNull<VecPtr<Param>> Function::operator[](ParamID id) {
    TIRO_ASSERT(check_id(id, params_), "Invalid param id.");
    return TIRO_NN(VecPtr(params_, id.value()));
}

NotNull<VecPtr<Local>> Function::operator[](LocalID id) {
    TIRO_ASSERT(check_id(id, locals_), "Invalid local id.");
    return TIRO_NN(VecPtr(locals_, id.value()));
}

NotNull<VecPtr<Phi>> Function::operator[](PhiID id) {
    TIRO_ASSERT(check_id(id, phis_), "Invalid phi id.");
    return TIRO_NN(VecPtr(phis_, id.value()));
}

NotNull<VecPtr<LocalList>> Function::operator[](LocalListID id) {
    TIRO_ASSERT(check_id(id, local_lists_), "Invalid local list id.");
    return TIRO_NN(VecPtr(local_lists_, id.value()));
}

NotNull<VecPtr<const Block>> Function::operator[](BlockID id) const {
    TIRO_ASSERT(check_id(id, blocks_), "Invalid block id.");
    return TIRO_NN(VecPtr(blocks_, id.value()));
}

NotNull<VecPtr<const Param>> Function::operator[](ParamID id) const {
    TIRO_ASSERT(check_id(id, params_), "Invalid param id.");
    return TIRO_NN(VecPtr(params_, id.value()));
}

NotNull<VecPtr<const Local>> Function::operator[](LocalID id) const {
    TIRO_ASSERT(check_id(id, locals_), "Invalid local id.");
    return TIRO_NN(VecPtr(locals_, id.value()));
}

NotNull<VecPtr<const Phi>> Function::operator[](PhiID id) const {
    TIRO_ASSERT(check_id(id, phis_), "Invalid phi id.");
    return TIRO_NN(VecPtr(phis_, id.value()));
}

NotNull<VecPtr<const LocalList>> Function::operator[](LocalListID id) const {
    TIRO_ASSERT(check_id(id, local_lists_), "Invalid local list id.");
    return TIRO_NN(VecPtr(local_lists_, id.value()));
}

// TODO: Implement a mor readable format for the cfg.
void dump_function(const Function& func, FormatStream& stream) {
    using namespace dump_helpers;

    StringTable& strings = func.strings();

    stream.format(
        "Function\n"
        "  Name: {}\n"
        "  Type: {}\n"
        "  Blocks: {}\n"
        "  Locals: {}\n"
        "  Phi Nodes: {}\n"
        "  Local Lists: {}\n"
        "  Entry Block: {}\n"
        "  Exit Block: {}\n",
        strings.dump(func.name()), func.type(), func.block_count(),
        func.local_count(), func.phi_count(), func.local_list_count(),
        func.entry(), func.exit());

    // Walk the control flow graph
    stream.format("\n");
    {
        std::vector<BlockID> stack;
        std::unordered_set<BlockID, UseHasher> seen;
        const auto visit = [&](BlockID block) {
            bool inserted = seen.emplace(block).second;
            if (inserted) {
                stack.push_back(block);
            }
        };

        visit(func.entry());
        while (!stack.empty()) {
            auto block_id = stack.back();
            auto block = func[block_id];
            stack.pop_back();

            stream.format("{} (sealed: {}, filled: {})\n",
                DumpBlock{func, block_id}, block->sealed(), block->filled());

            if (block->predecessor_count() > 0) {
                stream.format("  <- ");
                {
                    size_t index = 0;
                    for (auto pred : block->predecessors()) {
                        if (index++ != 0)
                            stream.format(", ");
                        stream.format("{}", DumpBlock{func, pred});
                    }
                }
                stream.format("\n");
            }

            const size_t stmt_count = block->stmt_count();
            const size_t max_index_length = fmt::formatted_size(
                "{}", stmt_count == 0 ? 0 : stmt_count - 1);

            size_t index = 0;
            for (const auto& stmt : block->stmts()) {
                stream.format("  {index:>{width}}: {value}",
                    fmt::arg("index", index),
                    fmt::arg("width", max_index_length),
                    fmt::arg("value", DumpStmt{func, stmt}));

                stream.format("\n");
                ++index;
            }
            stream.format("  {}\n", DumpEdge{func, block->edge()});

            // Depth first search through the cfg.
            visit_targets(
                block->edge(), [&](BlockID target) { visit(target); });

            if (!stack.empty())
                stream.format("\n");
        }
    }
}

Param::Param(InternedString name)
    : name_(name) {
    TIRO_ASSERT(name, "Parameters must have valid names.");
}

InternedString Param::name() const {
    return name_;
}

void Param::format(FormatStream& stream) const {
    stream.format("Param({})", name());
}

/* [[[cog
    import mir
    codegen.implement_type(mir.EdgeType)
]]] */
std::string_view to_string(EdgeType type) {
    switch (type) {
    case EdgeType::None:
        return "None";
    case EdgeType::Jump:
        return "Jump";
    case EdgeType::Branch:
        return "Branch";
    case EdgeType::Return:
        return "Return";
    case EdgeType::Exit:
        return "Exit";
    case EdgeType::AssertFail:
        return "AssertFail";
    case EdgeType::Never:
        return "Never";
    }
    TIRO_UNREACHABLE("Invalid EdgeType.");
}
// [[[end]]]

std::string_view to_string(BranchType type) {
    switch (type) {
#define TIRO_CASE(T)    \
    case BranchType::T: \
        return #T;

        TIRO_CASE(IfTrue)
        TIRO_CASE(IfFalse)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid branch type.");
}

/* [[[cog
    import mir
    codegen.implement_type(mir.Edge)
]]] */
Edge Edge::make_none() {
    return None{};
}

Edge Edge::make_jump(const BlockID& target) {
    return Jump{target};
}

Edge Edge::make_branch(const BranchType& type, const LocalID& value,
    const BlockID& target, const BlockID& fallthrough) {
    return Branch{type, value, target, fallthrough};
}

Edge Edge::make_return(const LocalID& value, const BlockID& target) {
    return Return{value, target};
}

Edge Edge::make_exit() {
    return Exit{};
}

Edge Edge::make_assert_fail(
    const LocalID& expr, const LocalID& message, const BlockID& target) {
    return AssertFail{expr, message, target};
}

Edge Edge::make_never(const BlockID& target) {
    return Never{target};
}

Edge::Edge(const None& none)
    : type_(EdgeType::None)
    , none_(none) {}

Edge::Edge(const Jump& jump)
    : type_(EdgeType::Jump)
    , jump_(jump) {}

Edge::Edge(const Branch& branch)
    : type_(EdgeType::Branch)
    , branch_(branch) {}

Edge::Edge(const Return& ret)
    : type_(EdgeType::Return)
    , return_(ret) {}

Edge::Edge(const Exit& exit)
    : type_(EdgeType::Exit)
    , exit_(exit) {}

Edge::Edge(const AssertFail& assert_fail)
    : type_(EdgeType::AssertFail)
    , assert_fail_(assert_fail) {}

Edge::Edge(const Never& never)
    : type_(EdgeType::Never)
    , never_(never) {}

const Edge::None& Edge::as_none() const {
    TIRO_ASSERT(
        type_ == EdgeType::None, "Bad member access on Edge: not a None.");
    return none_;
}

const Edge::Jump& Edge::as_jump() const {
    TIRO_ASSERT(
        type_ == EdgeType::Jump, "Bad member access on Edge: not a Jump.");
    return jump_;
}

const Edge::Branch& Edge::as_branch() const {
    TIRO_ASSERT(
        type_ == EdgeType::Branch, "Bad member access on Edge: not a Branch.");
    return branch_;
}

const Edge::Return& Edge::as_return() const {
    TIRO_ASSERT(
        type_ == EdgeType::Return, "Bad member access on Edge: not a Return.");
    return return_;
}

const Edge::Exit& Edge::as_exit() const {
    TIRO_ASSERT(
        type_ == EdgeType::Exit, "Bad member access on Edge: not a Exit.");
    return exit_;
}

const Edge::AssertFail& Edge::as_assert_fail() const {
    TIRO_ASSERT(type_ == EdgeType::AssertFail,
        "Bad member access on Edge: not a AssertFail.");
    return assert_fail_;
}

const Edge::Never& Edge::as_never() const {
    TIRO_ASSERT(
        type_ == EdgeType::Never, "Bad member access on Edge: not a Never.");
    return never_;
}

void Edge::format(FormatStream& stream) const {
    struct Formatter {
        FormatStream& stream;

        void visit_none([[maybe_unused]] const None& none) {
            stream.format("None");
        }

        void visit_jump([[maybe_unused]] const Jump& jump) {
            stream.format("Jump(target: {})", jump.target);
        }

        void visit_branch([[maybe_unused]] const Branch& branch) {
            stream.format(
                "Branch(type: {}, value: {}, target: {}, fallthrough: {})",
                branch.type, branch.value, branch.target, branch.fallthrough);
        }

        void visit_return([[maybe_unused]] const Return& ret) {
            stream.format(
                "Return(value: {}, target: {})", ret.value, ret.target);
        }

        void visit_exit([[maybe_unused]] const Exit& exit) {
            stream.format("Exit");
        }

        void visit_assert_fail([[maybe_unused]] const AssertFail& assert_fail) {
            stream.format("AssertFail(expr: {}, message: {}, target: {})",
                assert_fail.expr, assert_fail.message, assert_fail.target);
        }

        void visit_never([[maybe_unused]] const Never& never) {
            stream.format("Never(target: {})", never.target);
        }
    };

    Formatter formatter{stream};
    visit(formatter);
}
// [[[end]]]

void visit_targets(const Edge& edge, FunctionRef<void(BlockID)> callback) {
    struct Visitor {
        FunctionRef<void(BlockID)>& callback;

        void visit_none([[maybe_unused]] const Edge::None& none) {}

        void visit_jump(const Edge::Jump& jump) { callback(jump.target); }

        void visit_branch(const Edge::Branch& branch) {
            callback(branch.target);
            callback(branch.fallthrough);
        }

        void visit_return(const Edge::Return& ret) { callback(ret.target); }

        void visit_exit([[maybe_unused]] const Edge::Exit& ex) {}

        void visit_assert_fail(const Edge::AssertFail& assert_fail) {
            callback(assert_fail.target);
        }

        void visit_never(const Edge::Never& never) { callback(never.target); }
    };
    Visitor visitor{callback};

    edge.visit(visitor);
}

Block::Block(InternedString label)
    : label_(label) {
    TIRO_ASSERT(label, "Basic blocks must have a valid label.");
}

Block::~Block() {}

BlockID Block::predecessor(size_t index) const {
    TIRO_ASSERT(index < predecessors_.size(), "Index out of bounds.");
    return predecessors_[index];
}

size_t Block::predecessor_count() const {
    return predecessors_.size();
}

void Block::append_predecessor(BlockID predecessor) {
    predecessors_.push_back(predecessor);
}

size_t Block::stmt_count() const {
    return stmts_.size();
}

void Block::append_stmt(const Stmt& stmt) {
    stmts_.push_back(stmt);
}

void Block::format(FormatStream& stream) const {
    stream.format("Block(label: {})", label_);
}

/* [[[cog
    import mir
    codegen.implement_type(mir.LValueType)
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
    import mir
    codegen.implement_type(mir.LValue)
]]] */
LValue LValue::make_param(const ParamID& target) {
    return Param{target};
}

LValue
LValue::make_closure(const LocalID& env, const u32& levels, const u32& index) {
    return Closure{env, levels, index};
}

LValue LValue::make_module(const ModuleMemberID& member) {
    return Module{member};
}

LValue LValue::make_field(const LocalID& object, const InternedString& name) {
    return Field{object, name};
}

LValue LValue::make_tuple_field(const LocalID& object, const u32& index) {
    return TupleField{object, index};
}

LValue LValue::make_index(const LocalID& object, const LocalID& index) {
    return Index{object, index};
}

LValue::LValue(const Param& param)
    : type_(LValueType::Param)
    , param_(param) {}

LValue::LValue(const Closure& closure)
    : type_(LValueType::Closure)
    , closure_(closure) {}

LValue::LValue(const Module& module)
    : type_(LValueType::Module)
    , module_(module) {}

LValue::LValue(const Field& field)
    : type_(LValueType::Field)
    , field_(field) {}

LValue::LValue(const TupleField& tuple_field)
    : type_(LValueType::TupleField)
    , tuple_field_(tuple_field) {}

LValue::LValue(const Index& index)
    : type_(LValueType::Index)
    , index_(index) {}

const LValue::Param& LValue::as_param() const {
    TIRO_ASSERT(type_ == LValueType::Param,
        "Bad member access on LValue: not a Param.");
    return param_;
}

const LValue::Closure& LValue::as_closure() const {
    TIRO_ASSERT(type_ == LValueType::Closure,
        "Bad member access on LValue: not a Closure.");
    return closure_;
}

const LValue::Module& LValue::as_module() const {
    TIRO_ASSERT(type_ == LValueType::Module,
        "Bad member access on LValue: not a Module.");
    return module_;
}

const LValue::Field& LValue::as_field() const {
    TIRO_ASSERT(type_ == LValueType::Field,
        "Bad member access on LValue: not a Field.");
    return field_;
}

const LValue::TupleField& LValue::as_tuple_field() const {
    TIRO_ASSERT(type_ == LValueType::TupleField,
        "Bad member access on LValue: not a TupleField.");
    return tuple_field_;
}

const LValue::Index& LValue::as_index() const {
    TIRO_ASSERT(type_ == LValueType::Index,
        "Bad member access on LValue: not a Index.");
    return index_;
}

void LValue::format(FormatStream& stream) const {
    struct Formatter {
        FormatStream& stream;

        void visit_param([[maybe_unused]] const Param& param) {
            stream.format("Param(target: {})", param.target);
        }

        void visit_closure([[maybe_unused]] const Closure& closure) {
            stream.format("Closure(env: {}, levels: {}, index: {})",
                closure.env, closure.levels, closure.index);
        }

        void visit_module([[maybe_unused]] const Module& module) {
            stream.format("Module(member: {})", module.member);
        }

        void visit_field([[maybe_unused]] const Field& field) {
            stream.format(
                "Field(object: {}, name: {})", field.object, field.name);
        }

        void visit_tuple_field([[maybe_unused]] const TupleField& tuple_field) {
            stream.format("TupleField(object: {}, index: {})",
                tuple_field.object, tuple_field.index);
        }

        void visit_index([[maybe_unused]] const Index& index) {
            stream.format(
                "Index(object: {}, index: {})", index.object, index.index);
        }
    };

    Formatter formatter{stream};
    visit(formatter);
}
// [[[end]]]

/* [[[cog
    import mir
    codegen.implement_type(mir.ConstantType)
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

/* [[[cog
    import mir
    codegen.implement_type(mir.Constant)
]]] */
Constant Constant::make_integer(const i64& value) {
    return Integer{value};
}

Constant Constant::make_float(const f64& value) {
    return Float{value};
}

Constant Constant::make_string(const InternedString& value) {
    return String{value};
}

Constant Constant::make_symbol(const InternedString& value) {
    return Symbol{value};
}

Constant Constant::make_null() {
    return Null{};
}

Constant Constant::make_true() {
    return True{};
}

Constant Constant::make_false() {
    return False{};
}

Constant::Constant(const Integer& integer)
    : type_(ConstantType::Integer)
    , integer_(integer) {}

Constant::Constant(const Float& f)
    : type_(ConstantType::Float)
    , float_(f) {}

Constant::Constant(const String& string)
    : type_(ConstantType::String)
    , string_(string) {}

Constant::Constant(const Symbol& symbol)
    : type_(ConstantType::Symbol)
    , symbol_(symbol) {}

Constant::Constant(const Null& null)
    : type_(ConstantType::Null)
    , null_(null) {}

Constant::Constant(const True& t)
    : type_(ConstantType::True)
    , true_(t) {}

Constant::Constant(const False& f)
    : type_(ConstantType::False)
    , false_(f) {}

const Constant::Integer& Constant::as_integer() const {
    TIRO_ASSERT(type_ == ConstantType::Integer,
        "Bad member access on Constant: not a Integer.");
    return integer_;
}

const Constant::Float& Constant::as_float() const {
    TIRO_ASSERT(type_ == ConstantType::Float,
        "Bad member access on Constant: not a Float.");
    return float_;
}

const Constant::String& Constant::as_string() const {
    TIRO_ASSERT(type_ == ConstantType::String,
        "Bad member access on Constant: not a String.");
    return string_;
}

const Constant::Symbol& Constant::as_symbol() const {
    TIRO_ASSERT(type_ == ConstantType::Symbol,
        "Bad member access on Constant: not a Symbol.");
    return symbol_;
}

const Constant::Null& Constant::as_null() const {
    TIRO_ASSERT(type_ == ConstantType::Null,
        "Bad member access on Constant: not a Null.");
    return null_;
}

const Constant::True& Constant::as_true() const {
    TIRO_ASSERT(type_ == ConstantType::True,
        "Bad member access on Constant: not a True.");
    return true_;
}

const Constant::False& Constant::as_false() const {
    TIRO_ASSERT(type_ == ConstantType::False,
        "Bad member access on Constant: not a False.");
    return false_;
}

void Constant::format(FormatStream& stream) const {
    struct Formatter {
        FormatStream& stream;

        void visit_integer([[maybe_unused]] const Integer& integer) {
            stream.format("Integer(value: {})", integer.value);
        }

        void visit_float([[maybe_unused]] const Float& f) {
            stream.format("Float(value: {})", f.value);
        }

        void visit_string([[maybe_unused]] const String& string) {
            stream.format("String(value: {})", string.value);
        }

        void visit_symbol([[maybe_unused]] const Symbol& symbol) {
            stream.format("Symbol(value: {})", symbol.value);
        }

        void visit_null([[maybe_unused]] const Null& null) {
            stream.format("Null");
        }

        void visit_true([[maybe_unused]] const True& t) {
            stream.format("True");
        }

        void visit_false([[maybe_unused]] const False& f) {
            stream.format("False");
        }
    };

    Formatter formatter{stream};
    visit(formatter);
}
// [[[end]]]

/* [[[cog
    import mir
    codegen.implement_type(mir.RValueType)
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
    case RValueType::MethodCall:
        return "MethodCall";
    case RValueType::MakeEnvironment:
        return "MakeEnvironment";
    case RValueType::MakeClosure:
        return "MakeClosure";
    case RValueType::Container:
        return "Container";
    case RValueType::Format:
        return "Format";
    }
    TIRO_UNREACHABLE("Invalid RValueType.");
}
// [[[end]]]

/* [[[cog
    import mir
    codegen.implement_type(mir.RValue)
]]] */
RValue RValue::make_use_lvalue(const LValue& target) {
    return UseLValue{target};
}

RValue RValue::make_use_local(const LocalID& target) {
    return UseLocal{target};
}

RValue RValue::make_phi(const PhiID& value) {
    return Phi{value};
}

RValue RValue::make_phi0() {
    return Phi0{};
}

RValue RValue::make_constant(const Constant& constant) {
    return constant;
}

RValue RValue::make_outer_environment() {
    return OuterEnvironment{};
}

RValue RValue::make_binary_op(
    const BinaryOpType& op, const LocalID& left, const LocalID& right) {
    return BinaryOp{op, left, right};
}

RValue RValue::make_unary_op(const UnaryOpType& op, const LocalID& operand) {
    return UnaryOp{op, operand};
}

RValue RValue::make_call(const LocalID& func, const LocalListID& args) {
    return Call{func, args};
}

RValue RValue::make_method_call(const LocalID& object,
    const InternedString& method, const LocalListID& args) {
    return MethodCall{object, method, args};
}

RValue RValue::make_make_environment(const LocalID& parent, const u32& size) {
    return MakeEnvironment{parent, size};
}

RValue RValue::make_make_closure(const LocalID& env, const LocalID& func) {
    return MakeClosure{env, func};
}

RValue RValue::make_container(
    const ContainerType& container, const LocalListID& args) {
    return Container{container, args};
}

RValue RValue::make_format(const LocalListID& args) {
    return Format{args};
}

RValue::RValue(const UseLValue& use_lvalue)
    : type_(RValueType::UseLValue)
    , use_lvalue_(use_lvalue) {}

RValue::RValue(const UseLocal& use_local)
    : type_(RValueType::UseLocal)
    , use_local_(use_local) {}

RValue::RValue(const Phi& phi)
    : type_(RValueType::Phi)
    , phi_(phi) {}

RValue::RValue(const Phi0& phi0)
    : type_(RValueType::Phi0)
    , phi0_(phi0) {}

RValue::RValue(const Constant& constant)
    : type_(RValueType::Constant)
    , constant_(constant) {}

RValue::RValue(const OuterEnvironment& outer_environment)
    : type_(RValueType::OuterEnvironment)
    , outer_environment_(outer_environment) {}

RValue::RValue(const BinaryOp& binary_op)
    : type_(RValueType::BinaryOp)
    , binary_op_(binary_op) {}

RValue::RValue(const UnaryOp& unary_op)
    : type_(RValueType::UnaryOp)
    , unary_op_(unary_op) {}

RValue::RValue(const Call& call)
    : type_(RValueType::Call)
    , call_(call) {}

RValue::RValue(const MethodCall& method_call)
    : type_(RValueType::MethodCall)
    , method_call_(method_call) {}

RValue::RValue(const MakeEnvironment& make_environment)
    : type_(RValueType::MakeEnvironment)
    , make_environment_(make_environment) {}

RValue::RValue(const MakeClosure& make_closure)
    : type_(RValueType::MakeClosure)
    , make_closure_(make_closure) {}

RValue::RValue(const Container& container)
    : type_(RValueType::Container)
    , container_(container) {}

RValue::RValue(const Format& format)
    : type_(RValueType::Format)
    , format_(format) {}

const RValue::UseLValue& RValue::as_use_lvalue() const {
    TIRO_ASSERT(type_ == RValueType::UseLValue,
        "Bad member access on RValue: not a UseLValue.");
    return use_lvalue_;
}

const RValue::UseLocal& RValue::as_use_local() const {
    TIRO_ASSERT(type_ == RValueType::UseLocal,
        "Bad member access on RValue: not a UseLocal.");
    return use_local_;
}

const RValue::Phi& RValue::as_phi() const {
    TIRO_ASSERT(
        type_ == RValueType::Phi, "Bad member access on RValue: not a Phi.");
    return phi_;
}

const RValue::Phi0& RValue::as_phi0() const {
    TIRO_ASSERT(
        type_ == RValueType::Phi0, "Bad member access on RValue: not a Phi0.");
    return phi0_;
}

const RValue::Constant& RValue::as_constant() const {
    TIRO_ASSERT(type_ == RValueType::Constant,
        "Bad member access on RValue: not a Constant.");
    return constant_;
}

const RValue::OuterEnvironment& RValue::as_outer_environment() const {
    TIRO_ASSERT(type_ == RValueType::OuterEnvironment,
        "Bad member access on RValue: not a OuterEnvironment.");
    return outer_environment_;
}

const RValue::BinaryOp& RValue::as_binary_op() const {
    TIRO_ASSERT(type_ == RValueType::BinaryOp,
        "Bad member access on RValue: not a BinaryOp.");
    return binary_op_;
}

const RValue::UnaryOp& RValue::as_unary_op() const {
    TIRO_ASSERT(type_ == RValueType::UnaryOp,
        "Bad member access on RValue: not a UnaryOp.");
    return unary_op_;
}

const RValue::Call& RValue::as_call() const {
    TIRO_ASSERT(
        type_ == RValueType::Call, "Bad member access on RValue: not a Call.");
    return call_;
}

const RValue::MethodCall& RValue::as_method_call() const {
    TIRO_ASSERT(type_ == RValueType::MethodCall,
        "Bad member access on RValue: not a MethodCall.");
    return method_call_;
}

const RValue::MakeEnvironment& RValue::as_make_environment() const {
    TIRO_ASSERT(type_ == RValueType::MakeEnvironment,
        "Bad member access on RValue: not a MakeEnvironment.");
    return make_environment_;
}

const RValue::MakeClosure& RValue::as_make_closure() const {
    TIRO_ASSERT(type_ == RValueType::MakeClosure,
        "Bad member access on RValue: not a MakeClosure.");
    return make_closure_;
}

const RValue::Container& RValue::as_container() const {
    TIRO_ASSERT(type_ == RValueType::Container,
        "Bad member access on RValue: not a Container.");
    return container_;
}

const RValue::Format& RValue::as_format() const {
    TIRO_ASSERT(type_ == RValueType::Format,
        "Bad member access on RValue: not a Format.");
    return format_;
}

void RValue::format(FormatStream& stream) const {
    struct Formatter {
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

        void visit_phi0([[maybe_unused]] const Phi0& phi0) {
            stream.format("Phi0");
        }

        void visit_constant([[maybe_unused]] const Constant& constant) {
            stream.format("{}", constant);
        }

        void visit_outer_environment(
            [[maybe_unused]] const OuterEnvironment& outer_environment) {
            stream.format("OuterEnvironment");
        }

        void visit_binary_op([[maybe_unused]] const BinaryOp& binary_op) {
            stream.format("BinaryOp(op: {}, left: {}, right: {})", binary_op.op,
                binary_op.left, binary_op.right);
        }

        void visit_unary_op([[maybe_unused]] const UnaryOp& unary_op) {
            stream.format(
                "UnaryOp(op: {}, operand: {})", unary_op.op, unary_op.operand);
        }

        void visit_call([[maybe_unused]] const Call& call) {
            stream.format("Call(func: {}, args: {})", call.func, call.args);
        }

        void visit_method_call([[maybe_unused]] const MethodCall& method_call) {
            stream.format("MethodCall(object: {}, method: {}, args: {})",
                method_call.object, method_call.method, method_call.args);
        }

        void visit_make_environment(
            [[maybe_unused]] const MakeEnvironment& make_environment) {
            stream.format("MakeEnvironment(parent: {}, size: {})",
                make_environment.parent, make_environment.size);
        }

        void
        visit_make_closure([[maybe_unused]] const MakeClosure& make_closure) {
            stream.format("MakeClosure(env: {}, func: {})", make_closure.env,
                make_closure.func);
        }

        void visit_container([[maybe_unused]] const Container& container) {
            stream.format("Container(container: {}, args: {})",
                container.container, container.args);
        }

        void visit_format([[maybe_unused]] const Format& format) {
            stream.format("Format(args: {})", format.args);
        }
    };

    Formatter formatter{stream};
    visit(formatter);
}
// [[[end]]]

Local::Local(const RValue& value)
    : value_(value) {}

void Local::format(FormatStream& stream) const {
    stream.format("Local(name: {}, value: {})", name(), value());
}

Phi::Phi() {}

Phi::Phi(std::initializer_list<LocalID> operands)
    : operands_(operands.begin(), operands.end()) {}

Phi::Phi(std::vector<LocalID>&& operands)
    : operands_(std::move(operands)) {}

Phi::~Phi() {}

void Phi::append_operand(LocalID operand) {
    // TODO set behaviour?
    operands_.push_back(operand);
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

LocalList::LocalList(std::initializer_list<LocalID> locals)
    : locals_(locals) {}

LocalList::~LocalList() {}

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
    import mir
    codegen.implement_type(mir.StmtType)
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
    import mir
    codegen.implement_type(mir.Stmt)
]]] */
Stmt Stmt::make_assign(const LValue& target, const LocalID& value) {
    return Assign{target, value};
}

Stmt Stmt::make_define(const LocalID& local) {
    return Define{local};
}

Stmt::Stmt(const Assign& assign)
    : type_(StmtType::Assign)
    , assign_(assign) {}

Stmt::Stmt(const Define& define)
    : type_(StmtType::Define)
    , define_(define) {}

const Stmt::Assign& Stmt::as_assign() const {
    TIRO_ASSERT(
        type_ == StmtType::Assign, "Bad member access on Stmt: not a Assign.");
    return assign_;
}

const Stmt::Define& Stmt::as_define() const {
    TIRO_ASSERT(
        type_ == StmtType::Define, "Bad member access on Stmt: not a Define.");
    return define_;
}

void Stmt::format(FormatStream& stream) const {
    struct Formatter {
        FormatStream& stream;

        void visit_assign([[maybe_unused]] const Assign& assign) {
            stream.format(
                "Assign(target: {}, value: {})", assign.target, assign.value);
        }

        void visit_define([[maybe_unused]] const Define& define) {
            stream.format("Define(local: {})", define.local);
        }
    };

    Formatter formatter{stream};
    visit(formatter);
}
// [[[end]]]

namespace dump_helpers {

void format(const DumpBlock& d, FormatStream& stream) {
    if (!d.block) {
        stream.format("<INVALID>");
        return;
    }

    auto& func = d.parent;
    auto block = func[d.block];

    stream.format("${}", d.block.value());
    if (block->label()) {
        stream.format("-{}", func.strings().value(block->label()));
    }
}

void format(const DumpEdge& d, FormatStream& stream) {
    struct Visitor {
        const Function& func;
        FormatStream& stream;

        void visit_none([[maybe_unused]] const Edge::None& none) {
            stream.format("-> none");
        }

        void visit_jump(const Edge::Jump& jump) {
            stream.format("-> jump {}", DumpBlock{func, jump.target});
        }

        void visit_branch(const Edge::Branch& branch) {
            stream.format("-> branch {} {} target: {} fallthrough: {}",
                branch.type, DumpLocal{func, branch.value},
                DumpBlock{func, branch.target},
                DumpBlock{func, branch.fallthrough});
        }

        void visit_return(const Edge::Return& ret) {
            stream.format("-> return {} target: {}", DumpLocal{func, ret.value},
                DumpBlock{func, ret.target});
        }

        void visit_exit([[maybe_unused]] const Edge::Exit& exit) {
            stream.format("-> exit");
        }

        void visit_assert_fail(const Edge::AssertFail& fail) {
            stream.format("-> assert fail expr: {} message: {} target: {}",
                DumpLocal{func, fail.expr}, DumpLocal{func, fail.message},
                DumpBlock{func, fail.target});
        }

        void visit_never(const Edge::Never& never) {
            stream.format("-> never {}", DumpBlock{func, never.target});
        }
    };
    Visitor visitor{d.parent, stream};
    d.value.visit(visitor);
}

void format(const DumpLValue& d, FormatStream& stream) {
    struct Visitor {
        const Function& func;
        FormatStream& stream;

        void visit_param(const LValue::Param& param) {
            stream.format("<param {}>", param.target.value());
        }

        void visit_closure(const LValue::Closure& closure) {
            stream.format("<closure {} level: {} index: {}>",
                DumpLocal{func, closure.env}, closure.levels, closure.index);
        }

        void visit_module(const LValue::Module& module) {
            stream.format("<module {}>", module.member.value());
        }

        void visit_field(const LValue::Field& field) {
            stream.format("{}.{}", DumpLocal{func, field.object},
                func.strings().dump(field.name));
        }

        void visit_tuple_field(const LValue::TupleField& field) {
            stream.format("{}.{}", DumpLocal{func, field.object}, field.index);
        }

        void visit_index(const LValue::Index& index) {
            stream.format("{}[{}]", DumpLocal{func, index.object},
                DumpLocal{func, index.index});
        }
    };
    Visitor visitor{d.parent, stream};
    d.value.visit(visitor);
}

void format(const DumpConstant& d, FormatStream& stream) {
    struct Visitor {
        const Function& func;
        FormatStream& stream;

        void visit_integer(const Constant::Integer& i) {
            stream.format("{}", i.value);
        }

        void visit_float(const Constant::Float& f) {
            stream.format("{:f}", f.value);
        }

        void visit_string(const Constant::String& str) {
            if (!str.value) {
                stream.format("\"\"");
                return;
            }

            auto escaped = escape_string(func.strings().value(str.value));
            stream.format("{}", escaped);
        }

        void visit_symbol(const Constant::Symbol& sym) {
            stream.format("#{}", func.strings().dump(sym.value));
        }

        void visit_null([[maybe_unused]] const Constant::Null& null) {
            stream.format("null");
        }

        void visit_true([[maybe_unused]] const Constant::True& t) {
            stream.format("true");
        }

        void visit_false([[maybe_unused]] const Constant::False& f) {
            stream.format("false");
        }
    };
    Visitor visitor{d.parent, stream};
    d.value.visit(visitor);
}

void format(const DumpRValue& d, FormatStream& stream) {
    struct Visitor {
        const Function& func;
        FormatStream& stream;

        void visit_use_lvalue(const RValue::UseLValue& use) {
            return format(DumpLValue{func, use.target}, stream);
        }

        void visit_use_local(const RValue::UseLocal& use) {
            return format(DumpLocal{func, use.target}, stream);
        }

        void visit_phi(const RValue::Phi& phi) {
            return format(DumpPhi{func, phi.value}, stream);
        }

        void visit_phi0([[maybe_unused]] const RValue::Phi0& phi) {
            stream.format("<phi>");
        }

        void visit_constant(const RValue::Constant& constant) {
            return format(DumpConstant{func, constant}, stream);
        }

        void visit_outer_environment(
            [[maybe_unused]] const RValue::OuterEnvironment& outer) {
            stream.format("<outer-env>");
        }

        void visit_binary_op(const RValue::BinaryOp& binop) {
            stream.format("{} {} {}", DumpLocal{func, binop.left}, binop.op,
                DumpLocal{func, binop.right});
        }

        void visit_unary_op(const RValue::UnaryOp& unop) {
            stream.format("{} {} ", unop.op, DumpLocal{func, unop.operand});
        }

        void visit_call(const RValue::Call& call) {
            stream.format("{}({})", DumpLocal{func, call.func},
                DumpLocalList{func, call.args});
        }

        void visit_method_call(const RValue::MethodCall& call) {
            stream.format("{}.{}({})", DumpLocal{func, call.object},
                func.strings().dump(call.method),
                DumpLocalList{func, call.args});
        }

        void visit_make_environment(const RValue::MakeEnvironment& env) {
            stream.format(
                "<make-env {} {}>", DumpLocal{func, env.parent}, env.size);
        }

        void visit_make_closure(const RValue::MakeClosure& closure) {
            stream.format("<make-closure env: {} func: {}>",
                DumpLocal{func, closure.env}, DumpLocal{func, closure.func});
        }

        void visit_container(const RValue::Container& cont) {
            stream.format(
                "{}({})", cont.container, DumpLocalList{func, cont.args});
        }

        void visit_format(const RValue::Format& format) {
            stream.format("<format {}>", DumpLocalList{func, format.args});
        }
    };

    Visitor visitor{d.parent, stream};
    d.value.visit(visitor);
}

void format(const DumpLocal& d, FormatStream& stream) {
    if (!d.local) {
        stream.format("<INVALID>");
        return;
    }

    auto& func = d.parent;
    auto& strings = func.strings();
    auto local = func[d.local];
    if (local->name()) {
        stream.format(
            "%{1}_{0}", d.local.value(), strings.value(local->name()));
    } else {
        stream.format("%{}", d.local.value());
    }
}

void format(const DumpDefine& d, FormatStream& stream) {
    if (!d.local) {
        stream.format("<INVALID>");
        return;
    }

    auto& func = d.parent;
    auto local = func[d.local];
    stream.format(
        "{} = {}", DumpLocal{func, d.local}, DumpRValue{func, local->value()});
}

void format(const DumpLocalList& d, FormatStream& stream) {
    if (!d.list) {
        stream.format("<INVALID>");
        return;
    }

    auto& func = d.parent;
    auto list = func[d.list];

    size_t index = 0;
    for (auto local : *list) {
        if (index++ > 0)
            stream.format(", ");
        format(DumpLocal{func, local}, stream);
    }
}

void format(const DumpPhi& d, FormatStream& stream) {
    if (!d.phi) {
        stream.format("<INVALID>");
        return;
    }

    auto& func = d.parent;
    auto phi = func[d.phi];

    if (phi->operand_count() == 0) {
        stream.format("<phi>");
        return;
    }

    stream.format("<phi");
    for (const auto& op : phi->operands()) {
        stream.format(" {}", DumpLocal{func, op});
    }
    stream.format(">");
}

void format(const DumpStmt& d, FormatStream& stream) {
    struct Visitor {
        const Function& func;
        FormatStream& stream;

        void visit_assign(const Stmt::Assign& assign) {
            stream.format("{} = {}", DumpLValue{func, assign.target},
                DumpLocal{func, assign.value});
        }

        void visit_define(const Stmt::Define& define) {
            stream.format("{}", DumpDefine{func, define.local});
        }
    };

    Visitor visitor{d.parent, stream};
    return d.stmt.visit(visitor);
}

} // namespace dump_helpers

// Check that the most frequently used types are trivial:
/* [[[cog
    import cog
    import mir
    types = [mir.ModuleMember, mir.Edge, mir.LValue, mir.Constant, mir.RValue, mir.Stmt]

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
static_assert(std::is_trivially_copyable_v<ModuleMember::Import>);
static_assert(std::is_trivially_destructible_v<ModuleMember::Import>);
static_assert(std::is_trivially_copyable_v<ModuleMember::Variable>);
static_assert(std::is_trivially_destructible_v<ModuleMember::Variable>);
static_assert(std::is_trivially_copyable_v<ModuleMember::Function>);
static_assert(std::is_trivially_destructible_v<ModuleMember::Function>);
static_assert(std::is_trivially_copyable_v<ModuleMember>);
static_assert(std::is_trivially_destructible_v<ModuleMember>);

static_assert(std::is_trivially_copyable_v<Edge::None>);
static_assert(std::is_trivially_destructible_v<Edge::None>);
static_assert(std::is_trivially_copyable_v<Edge::Jump>);
static_assert(std::is_trivially_destructible_v<Edge::Jump>);
static_assert(std::is_trivially_copyable_v<Edge::Branch>);
static_assert(std::is_trivially_destructible_v<Edge::Branch>);
static_assert(std::is_trivially_copyable_v<Edge::Return>);
static_assert(std::is_trivially_destructible_v<Edge::Return>);
static_assert(std::is_trivially_copyable_v<Edge::Exit>);
static_assert(std::is_trivially_destructible_v<Edge::Exit>);
static_assert(std::is_trivially_copyable_v<Edge::AssertFail>);
static_assert(std::is_trivially_destructible_v<Edge::AssertFail>);
static_assert(std::is_trivially_copyable_v<Edge::Never>);
static_assert(std::is_trivially_destructible_v<Edge::Never>);
static_assert(std::is_trivially_copyable_v<Edge>);
static_assert(std::is_trivially_destructible_v<Edge>);

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
static_assert(std::is_trivially_copyable_v<RValue::MethodCall>);
static_assert(std::is_trivially_destructible_v<RValue::MethodCall>);
static_assert(std::is_trivially_copyable_v<RValue::MakeEnvironment>);
static_assert(std::is_trivially_destructible_v<RValue::MakeEnvironment>);
static_assert(std::is_trivially_copyable_v<RValue::MakeClosure>);
static_assert(std::is_trivially_destructible_v<RValue::MakeClosure>);
static_assert(std::is_trivially_copyable_v<RValue::Container>);
static_assert(std::is_trivially_destructible_v<RValue::Container>);
static_assert(std::is_trivially_copyable_v<RValue::Format>);
static_assert(std::is_trivially_destructible_v<RValue::Format>);
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

} // namespace tiro::compiler::mir
