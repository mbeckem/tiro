#ifndef TIRO_AST_AST_HPP
#define TIRO_AST_AST_HPP

#include "tiro/ast/fwd.hpp"
#include "tiro/core/format.hpp"

#include <string_view>

namespace tiro {

/* [[[cog
    from codegen.unions import define_type
    from codegen.ast import ExprType
    define_type(ExprType)
]]] */
enum class ASTExprType : u8 {
    Test,
};

std::string_view to_string(ASTExprType type);
// [[[end]]]

/* [[[cog
    from codegen.unions import define_type
    from codegen.ast import Expr
    define_type(Expr)
]]] */
/// Represents the contents of an expression in the abstract syntax tree.
class ASTExprData final {
public:
    /// TODO.
    struct Test final {
        /// TODO.
        int todo;

        explicit Test(const int& todo_)
            : todo(todo_) {}
    };

    static ASTExprData make_test(const int& todo);

    ASTExprData(const Test& test);

    ~ASTExprData();

    ASTExprData(ASTExprData&& other) noexcept;
    ASTExprData& operator=(ASTExprData&& other) noexcept;

    ASTExprType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    const Test& as_test() const;

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) {
        return visit_impl(
            *this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto)
    visit(Visitor&& vis, Args&&... args) const {
        return visit_impl(
            *this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

private:
    void _destroy_value() noexcept;
    void _move_construct_value(ASTExprData& other) noexcept;
    void _move_assign_value(ASTExprData& other) noexcept;

    template<typename Self, typename Visitor, typename... Args>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis, Args&&... args);

private:
    ASTExprType type_;
    union {
        Test test_;
    };
};
// [[[end]]]

/* [[[cog
    from codegen.unions import define_inlines
    from codegen.ast import Expr
    define_inlines(Expr)
]]] */
template<typename Self, typename Visitor, typename... Args>
decltype(auto)
ASTExprData::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case ASTExprType::Test:
        return vis.visit_test(self.test_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid ASTExprData type.");
}
// [[[end]]]

} // namespace tiro

#endif // TIRO_AST_AST_HPP
