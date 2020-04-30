#include "tiro/ast/ast.hpp"

#include <new>

namespace tiro {

/* [[[cog
    from codegen.unions import implement_type
    from codegen.ast import Expr
    implement_type(Expr)
]]] */
ASTExprData ASTExprData::make_test(const int& todo) {
    return Test{todo};
}

ASTExprData::ASTExprData(const Test& test)
    : type_(ASTExprType::Test)
    , test_(test) {}

ASTExprData::~ASTExprData() {
    _destroy_value();
}

static_assert(
    std::is_nothrow_move_constructible_v<ASTExprData::
            Test> && std::is_nothrow_move_assignable_v<ASTExprData::Test>,
    "Only nothrow movable types are supported in generated unions.");

ASTExprData::ASTExprData(ASTExprData&& other) noexcept
    : type_(other.type()) {
    _move_construct_value(other);
}

ASTExprData& ASTExprData::operator=(ASTExprData&& other) noexcept {
    TIRO_DEBUG_ASSERT(this != &other, "Self move assignement is invalid.");
    if (type() == other.type()) {
        _move_assign_value(other);
    } else {
        _destroy_value();
        _move_construct_value(other);
        type_ = other.type();
    }
    return *this;
}

const ASTExprData::Test& ASTExprData::as_test() const {
    TIRO_DEBUG_ASSERT(type_ == ASTExprType::Test,
        "Bad member access on ASTExprData: not a Test.");
    return test_;
}

void ASTExprData::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void visit_test([[maybe_unused]] const Test& test) {
            stream.format("Test(todo: {})", test.todo);
        }
    };
    visit(FormatVisitor{stream});
}

void ASTExprData::_destroy_value() noexcept {
    struct DestroyVisitor {
        void visit_test(Test& test) { test.~Test(); }
    };
    visit(DestroyVisitor{});
}

void ASTExprData::_move_construct_value(ASTExprData& other) noexcept {
    struct ConstructVisitor {
        ASTExprData* self;

        void visit_test(Test& test) {
            new (&self->test_) Test(std::move(test));
        }
    };
    other.visit(ConstructVisitor{this});
}

void ASTExprData::_move_assign_value(ASTExprData& other) noexcept {
    struct AssignVisitor {
        ASTExprData* self;

        void visit_test(Test& test) { self->test_ = std::move(test); }
    };
    other.visit(AssignVisitor{this});
}

// [[[end]]]

} // namespace tiro
