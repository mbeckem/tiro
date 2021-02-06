#include "compiler/ir/terminator.hpp"

namespace tiro::ir {

/* [[[cog
    from codegen.unions import implement
    from codegen.ir import Terminator
    implement(Terminator.tag)
]]] */
std::string_view to_string(TerminatorType type) {
    switch (type) {
    case TerminatorType::None:
        return "None";
    case TerminatorType::Entry:
        return "Entry";
    case TerminatorType::Exit:
        return "Exit";
    case TerminatorType::Jump:
        return "Jump";
    case TerminatorType::Branch:
        return "Branch";
    case TerminatorType::Return:
        return "Return";
    case TerminatorType::Rethrow:
        return "Rethrow";
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

Terminator Terminator::make_entry(const BlockId& body, std::vector<BlockId> handlers) {
    return {Entry{body, std::move(handlers)}};
}

Terminator Terminator::make_exit() {
    return {Exit{}};
}

Terminator Terminator::make_jump(const BlockId& target) {
    return {Jump{target}};
}

Terminator Terminator::make_branch(const BranchType& type, const InstId& value,
    const BlockId& target, const BlockId& fallthrough) {
    return {Branch{type, value, target, fallthrough}};
}

Terminator Terminator::make_return(const InstId& value, const BlockId& target) {
    return {Return{value, target}};
}

Terminator Terminator::make_rethrow(const BlockId& target) {
    return {Rethrow{target}};
}

Terminator
Terminator::make_assert_fail(const InstId& expr, const InstId& message, const BlockId& target) {
    return {AssertFail{expr, message, target}};
}

Terminator Terminator::make_never(const BlockId& target) {
    return {Never{target}};
}

Terminator::Terminator(None none)
    : type_(TerminatorType::None)
    , none_(std::move(none)) {}

Terminator::Terminator(Entry entry)
    : type_(TerminatorType::Entry)
    , entry_(std::move(entry)) {}

Terminator::Terminator(Exit exit)
    : type_(TerminatorType::Exit)
    , exit_(std::move(exit)) {}

Terminator::Terminator(Jump jump)
    : type_(TerminatorType::Jump)
    , jump_(std::move(jump)) {}

Terminator::Terminator(Branch branch)
    : type_(TerminatorType::Branch)
    , branch_(std::move(branch)) {}

Terminator::Terminator(Return ret)
    : type_(TerminatorType::Return)
    , return_(std::move(ret)) {}

Terminator::Terminator(Rethrow rethrow)
    : type_(TerminatorType::Rethrow)
    , rethrow_(std::move(rethrow)) {}

Terminator::Terminator(AssertFail assert_fail)
    : type_(TerminatorType::AssertFail)
    , assert_fail_(std::move(assert_fail)) {}

Terminator::Terminator(Never never)
    : type_(TerminatorType::Never)
    , never_(std::move(never)) {}

Terminator::~Terminator() {
    _destroy_value();
}

static_assert(std::is_nothrow_move_constructible_v<
                  Terminator::None> && std::is_nothrow_move_assignable_v<Terminator::None>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  Terminator::Entry> && std::is_nothrow_move_assignable_v<Terminator::Entry>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  Terminator::Exit> && std::is_nothrow_move_assignable_v<Terminator::Exit>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  Terminator::Jump> && std::is_nothrow_move_assignable_v<Terminator::Jump>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  Terminator::Branch> && std::is_nothrow_move_assignable_v<Terminator::Branch>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  Terminator::Return> && std::is_nothrow_move_assignable_v<Terminator::Return>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  Terminator::Rethrow> && std::is_nothrow_move_assignable_v<Terminator::Rethrow>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(
    std::is_nothrow_move_constructible_v<
        Terminator::AssertFail> && std::is_nothrow_move_assignable_v<Terminator::AssertFail>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  Terminator::Never> && std::is_nothrow_move_assignable_v<Terminator::Never>,
    "Only nothrow movable types are supported in generated unions.");

Terminator::Terminator(Terminator&& other) noexcept
    : type_(other.type()) {
    _move_construct_value(other);
}

Terminator& Terminator::operator=(Terminator&& other) noexcept {
    TIRO_DEBUG_ASSERT(this != &other, "Self move assignment is invalid.");
    if (type() == other.type()) {
        _move_assign_value(other);
    } else {
        _destroy_value();
        _move_construct_value(other);
        type_ = other.type();
    }
    return *this;
}

const Terminator::None& Terminator::as_none() const {
    TIRO_DEBUG_ASSERT(
        type_ == TerminatorType::None, "Bad member access on Terminator: not a None.");
    return none_;
}

Terminator::None& Terminator::as_none() {
    return const_cast<None&>(const_cast<const Terminator*>(this)->as_none());
}

const Terminator::Entry& Terminator::as_entry() const {
    TIRO_DEBUG_ASSERT(
        type_ == TerminatorType::Entry, "Bad member access on Terminator: not a Entry.");
    return entry_;
}

Terminator::Entry& Terminator::as_entry() {
    return const_cast<Entry&>(const_cast<const Terminator*>(this)->as_entry());
}

const Terminator::Exit& Terminator::as_exit() const {
    TIRO_DEBUG_ASSERT(
        type_ == TerminatorType::Exit, "Bad member access on Terminator: not a Exit.");
    return exit_;
}

Terminator::Exit& Terminator::as_exit() {
    return const_cast<Exit&>(const_cast<const Terminator*>(this)->as_exit());
}

const Terminator::Jump& Terminator::as_jump() const {
    TIRO_DEBUG_ASSERT(
        type_ == TerminatorType::Jump, "Bad member access on Terminator: not a Jump.");
    return jump_;
}

Terminator::Jump& Terminator::as_jump() {
    return const_cast<Jump&>(const_cast<const Terminator*>(this)->as_jump());
}

const Terminator::Branch& Terminator::as_branch() const {
    TIRO_DEBUG_ASSERT(
        type_ == TerminatorType::Branch, "Bad member access on Terminator: not a Branch.");
    return branch_;
}

Terminator::Branch& Terminator::as_branch() {
    return const_cast<Branch&>(const_cast<const Terminator*>(this)->as_branch());
}

const Terminator::Return& Terminator::as_return() const {
    TIRO_DEBUG_ASSERT(
        type_ == TerminatorType::Return, "Bad member access on Terminator: not a Return.");
    return return_;
}

Terminator::Return& Terminator::as_return() {
    return const_cast<Return&>(const_cast<const Terminator*>(this)->as_return());
}

const Terminator::Rethrow& Terminator::as_rethrow() const {
    TIRO_DEBUG_ASSERT(
        type_ == TerminatorType::Rethrow, "Bad member access on Terminator: not a Rethrow.");
    return rethrow_;
}

Terminator::Rethrow& Terminator::as_rethrow() {
    return const_cast<Rethrow&>(const_cast<const Terminator*>(this)->as_rethrow());
}

const Terminator::AssertFail& Terminator::as_assert_fail() const {
    TIRO_DEBUG_ASSERT(
        type_ == TerminatorType::AssertFail, "Bad member access on Terminator: not a AssertFail.");
    return assert_fail_;
}

Terminator::AssertFail& Terminator::as_assert_fail() {
    return const_cast<AssertFail&>(const_cast<const Terminator*>(this)->as_assert_fail());
}

const Terminator::Never& Terminator::as_never() const {
    TIRO_DEBUG_ASSERT(
        type_ == TerminatorType::Never, "Bad member access on Terminator: not a Never.");
    return never_;
}

Terminator::Never& Terminator::as_never() {
    return const_cast<Never&>(const_cast<const Terminator*>(this)->as_never());
}

void Terminator::_destroy_value() noexcept {
    struct DestroyVisitor {
        void visit_none(None& none) { none.~None(); }

        void visit_entry(Entry& entry) { entry.~Entry(); }

        void visit_exit(Exit& exit) { exit.~Exit(); }

        void visit_jump(Jump& jump) { jump.~Jump(); }

        void visit_branch(Branch& branch) { branch.~Branch(); }

        void visit_return(Return& ret) { ret.~Return(); }

        void visit_rethrow(Rethrow& rethrow) { rethrow.~Rethrow(); }

        void visit_assert_fail(AssertFail& assert_fail) { assert_fail.~AssertFail(); }

        void visit_never(Never& never) { never.~Never(); }
    };
    visit(DestroyVisitor{});
}

void Terminator::_move_construct_value(Terminator& other) noexcept {
    struct ConstructVisitor {
        Terminator* self;

        void visit_none(None& none) { new (&self->none_) None(std::move(none)); }

        void visit_entry(Entry& entry) { new (&self->entry_) Entry(std::move(entry)); }

        void visit_exit(Exit& exit) { new (&self->exit_) Exit(std::move(exit)); }

        void visit_jump(Jump& jump) { new (&self->jump_) Jump(std::move(jump)); }

        void visit_branch(Branch& branch) { new (&self->branch_) Branch(std::move(branch)); }

        void visit_return(Return& ret) { new (&self->return_) Return(std::move(ret)); }

        void visit_rethrow(Rethrow& rethrow) { new (&self->rethrow_) Rethrow(std::move(rethrow)); }

        void visit_assert_fail(AssertFail& assert_fail) {
            new (&self->assert_fail_) AssertFail(std::move(assert_fail));
        }

        void visit_never(Never& never) { new (&self->never_) Never(std::move(never)); }
    };
    other.visit(ConstructVisitor{this});
}

void Terminator::_move_assign_value(Terminator& other) noexcept {
    struct AssignVisitor {
        Terminator* self;

        void visit_none(None& none) { self->none_ = std::move(none); }

        void visit_entry(Entry& entry) { self->entry_ = std::move(entry); }

        void visit_exit(Exit& exit) { self->exit_ = std::move(exit); }

        void visit_jump(Jump& jump) { self->jump_ = std::move(jump); }

        void visit_branch(Branch& branch) { self->branch_ = std::move(branch); }

        void visit_return(Return& ret) { self->return_ = std::move(ret); }

        void visit_rethrow(Rethrow& rethrow) { self->rethrow_ = std::move(rethrow); }

        void visit_assert_fail(AssertFail& assert_fail) {
            self->assert_fail_ = std::move(assert_fail);
        }

        void visit_never(Never& never) { self->never_ = std::move(never); }
    };
    other.visit(AssignVisitor{this});
}

// [[[end]]]

void Terminator::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void visit_none(const None&) { stream.format("None"); }

        void visit_never(const Never& never) { stream.format("Never(target: {})", never.target); }

        void visit_entry(const Entry& entry) {
            stream.format(
                "Entry(body: {}, handlers: {})", entry.body, fmt::join(entry.handlers, ","));
        }

        void visit_exit(const Exit&) { stream.format("Exit"); }

        void visit_jump(const Jump& jump) { stream.format("Jump(target: {})", jump.target); }

        void visit_branch(const Branch& branch) {
            stream.format("Branch(type: {}, value: {}, target: {}, fallthrough: {})", branch.type,
                branch.value, branch.target, branch.fallthrough);
        }

        void visit_return(const Return& ret) {
            stream.format("Return(value: {}, target: {})", ret.value, ret.target);
        }

        void visit_rethrow(const Rethrow& rethrow) {
            stream.format("Rethrow(target: {})", rethrow.target);
        }

        void visit_assert_fail(const AssertFail& assert_fail) {
            stream.format("AssertFail(expr: {}, message: {}, target: {})", assert_fail.expr,
                assert_fail.message, assert_fail.target);
        }
    };
    visit(FormatVisitor{stream});
}

void visit_targets(const Terminator& terminator, FunctionRef<void(BlockId)> callback) {
    struct Visitor {
        FunctionRef<void(BlockId)>& callback;

        void visit_none([[maybe_unused]] const Terminator::None& none) {}

        void visit_never(const Terminator::Never& never) { callback(never.target); }

        void visit_entry(const Terminator::Entry& e) {
            callback(e.body);
            for (const auto& handler : e.handlers) {
                callback(handler);
            }
        }

        void visit_exit([[maybe_unused]] const Terminator::Exit& ex) {}

        void visit_jump(const Terminator::Jump& jump) { callback(jump.target); }

        void visit_branch(const Terminator::Branch& branch) {
            callback(branch.target);
            callback(branch.fallthrough);
        }

        void visit_return(const Terminator::Return& ret) { callback(ret.target); }

        void visit_rethrow(const Terminator::Rethrow& rethrow) { callback(rethrow.target); }

        void visit_assert_fail(const Terminator::AssertFail& assert_fail) {
            callback(assert_fail.target);
        }
    };
    Visitor visitor{callback};

    terminator.visit(visitor);
}

u32 target_count(const Terminator& term) {
    u32 count = 0;
    visit_targets(term, [&](BlockId) { ++count; });
    return count;
}

} // namespace tiro::ir
