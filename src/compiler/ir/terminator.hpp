#ifndef TIRO_COMPILER_IR_TERMINATOR_HPP
#define TIRO_COMPILER_IR_TERMINATOR_HPP

#include "common/adt/function_ref.hpp"
#include "common/defs.hpp"
#include "compiler/ir/entities.hpp"
#include "compiler/ir/fwd.hpp"

namespace tiro::ir {

/* [[[cog
    from codegen.unions import define
    from codegen.ir import Terminator
    define(Terminator.tag)
]]] */
enum class TerminatorType : u8 {
    None,
    Entry,
    Exit,
    Jump,
    Branch,
    Return,
    Rethrow,
    AssertFail,
    Never,
};

std::string_view to_string(TerminatorType type);
// [[[end]]]

/// Represents the type of a conditional jump.
enum class BranchType : u8 {
    IfTrue,
    IfFalse,
    IfNull,
    IfNotNull,
};

std::string_view to_string(BranchType type);

/* [[[cog
    from codegen.unions import define
    from codegen.ir import Terminator
    define(Terminator)
]]] */
/// Represents edges connecting different basic blocks.
class Terminator final {
public:
    /// The block has no outgoing edge. This is the initial value after a new block has been created.
    /// must be changed to one of the valid edge types when construction is complete.
    struct None final {};

    /// Contains the actual entry points into the function.
    struct Entry final {
        /// The start of the function under normal control flow.
        BlockId body;

        /// Exception handler blocks.
        std::vector<BlockId> handlers;

        Entry(const BlockId& body_, std::vector<BlockId> handlers_)
            : body(body_)
            , handlers(std::move(handlers_)) {}
    };

    /// Marks the exit block of the function.
    struct Exit final {};

    /// A single successor block, reached through an unconditional jump.
    struct Jump final {
        /// The jump target.
        BlockId target;

        explicit Jump(const BlockId& target_)
            : target(target_) {}
    };

    /// A conditional jump with two successor blocks.
    struct Branch final {
        /// The kind of conditional jump.
        BranchType type;

        /// The value that is being tested.
        InstId value;

        /// The jump target for successful tests.
        BlockId target;

        /// The jump target for failed tests.
        BlockId fallthrough;

        Branch(const BranchType& type_, const InstId& value_, const BlockId& target_,
            const BlockId& fallthrough_)
            : type(type_)
            , value(value_)
            , target(target_)
            , fallthrough(fallthrough_) {}
    };

    /// Returns a value from the function.
    struct Return final {
        /// The value that is being returned.
        InstId value;

        /// The successor block. This must be the exit block.
        /// These edges are needed to make all code paths converge at the exit block.
        BlockId target;

        Return(const InstId& value_, const BlockId& target_)
            : value(value_)
            , target(target_) {}
    };

    /// Throws the currently active exception. Only legal when used in exceptional control flow.
    struct Rethrow final {
        /// The successor block. This must be the exit block.
        /// These edges are needed to make all code paths converge at the exit block.
        BlockId target;

        explicit Rethrow(const BlockId& target_)
            : target(target_) {}
    };

    /// An assertion failure is an unconditional hard exit.
    struct AssertFail final {
        /// The string representation of the failed assertion.
        InstId expr;

        /// The message that will be printed when the assertion fails.
        InstId message;

        /// The successor block. This must be the exit block.
        /// These edges are needed to make all code paths converge at the exit block.
        BlockId target;

        AssertFail(const InstId& expr_, const InstId& message_, const BlockId& target_)
            : expr(expr_)
            , message(message_)
            , target(target_) {}
    };

    /// The block never returns (e.g. contains a statement that never terminates).
    struct Never final {
        /// The successor block. This must be the exit block.
        /// These edges are needed to make all code paths converge at the exit block.
        BlockId target;

        explicit Never(const BlockId& target_)
            : target(target_) {}
    };

    static Terminator make_none();
    static Terminator make_entry(const BlockId& body, std::vector<BlockId> handlers);
    static Terminator make_exit();
    static Terminator make_jump(const BlockId& target);
    static Terminator make_branch(const BranchType& type, const InstId& value,
        const BlockId& target, const BlockId& fallthrough);
    static Terminator make_return(const InstId& value, const BlockId& target);
    static Terminator make_rethrow(const BlockId& target);
    static Terminator
    make_assert_fail(const InstId& expr, const InstId& message, const BlockId& target);
    static Terminator make_never(const BlockId& target);

    Terminator(None none);
    Terminator(Entry entry);
    Terminator(Exit exit);
    Terminator(Jump jump);
    Terminator(Branch branch);
    Terminator(Return ret);
    Terminator(Rethrow rethrow);
    Terminator(AssertFail assert_fail);
    Terminator(Never never);

    ~Terminator();

    Terminator(Terminator&& other) noexcept;
    Terminator& operator=(Terminator&& other) noexcept;

    TerminatorType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    const None& as_none() const;
    None& as_none();

    const Entry& as_entry() const;
    Entry& as_entry();

    const Exit& as_exit() const;
    Exit& as_exit();

    const Jump& as_jump() const;
    Jump& as_jump();

    const Branch& as_branch() const;
    Branch& as_branch();

    const Return& as_return() const;
    Return& as_return();

    const Rethrow& as_rethrow() const;
    Rethrow& as_rethrow();

    const AssertFail& as_assert_fail() const;
    AssertFail& as_assert_fail();

    const Never& as_never() const;
    Never& as_never();

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) {
        return visit_impl(*this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) const {
        return visit_impl(*this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

private:
    void _destroy_value() noexcept;
    void _move_construct_value(Terminator& other) noexcept;
    void _move_assign_value(Terminator& other) noexcept;

    template<typename Self, typename Visitor, typename... Args>
    static TIRO_FORCE_INLINE decltype(auto) visit_impl(Self&& self, Visitor&& vis, Args&&... args);

private:
    TerminatorType type_;
    union {
        None none_;
        Entry entry_;
        Exit exit_;
        Jump jump_;
        Branch branch_;
        Return return_;
        Rethrow rethrow_;
        AssertFail assert_fail_;
        Never never_;
    };
};
// [[[end]]]

/// Invokes the callback for every block reachable via the given terminator.
// TODO This should be an iterator.
void visit_targets(const Terminator& term, FunctionRef<void(BlockId)> callback);

/// Returns the number of targets of the given terminator.
u32 target_count(const Terminator& term);

/* [[[cog
    from cog import outl
    from codegen.unions import implement_inlines
    from codegen.ir import Terminator
    implement_inlines(Terminator)
]]] */
template<typename Self, typename Visitor, typename... Args>
decltype(auto) Terminator::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case TerminatorType::None:
        return vis.visit_none(self.none_, std::forward<Args>(args)...);
    case TerminatorType::Entry:
        return vis.visit_entry(self.entry_, std::forward<Args>(args)...);
    case TerminatorType::Exit:
        return vis.visit_exit(self.exit_, std::forward<Args>(args)...);
    case TerminatorType::Jump:
        return vis.visit_jump(self.jump_, std::forward<Args>(args)...);
    case TerminatorType::Branch:
        return vis.visit_branch(self.branch_, std::forward<Args>(args)...);
    case TerminatorType::Return:
        return vis.visit_return(self.return_, std::forward<Args>(args)...);
    case TerminatorType::Rethrow:
        return vis.visit_rethrow(self.rethrow_, std::forward<Args>(args)...);
    case TerminatorType::AssertFail:
        return vis.visit_assert_fail(self.assert_fail_, std::forward<Args>(args)...);
    case TerminatorType::Never:
        return vis.visit_never(self.never_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid Terminator type.");
}
// [[[end]]]

} // namespace tiro::ir

TIRO_ENABLE_FREE_TO_STRING(tiro::ir::TerminatorType)
TIRO_ENABLE_FREE_TO_STRING(tiro::ir::BranchType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::ir::Terminator)

#endif // TIRO_COMPILER_IR_TERMINATOR_HPP
