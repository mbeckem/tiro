#ifndef TIRO_COMPILER_IR_GEN_CONST_EVAL_HPP
#define TIRO_COMPILER_IR_GEN_CONST_EVAL_HPP

#include "common/adt/span.hpp"
#include "common/format.hpp"
#include "compiler/ir/function.hpp"
#include "compiler/ir/fwd.hpp"

namespace tiro::ir {

enum class EvalResultType : u8 {
    Value,
    IntegerOverflow,
    DivideByZero,
    NegativeShift,
    ImaginaryPower,
    TypeError
};

std::string_view to_string(EvalResultType type);

/// Represents the compile time evaluation result for a certain operation.
/// The inner constant value is only available if the evaluation succeeded.
class [[nodiscard]] EvalResult final {
public:
    EvalResult(const Constant& value);

    static EvalResult make_integer_overflow();
    static EvalResult make_divide_by_zero();
    static EvalResult make_negative_shift();
    static EvalResult make_imaginary_power();
    static EvalResult make_type_error();
    static EvalResult make_unsupported();

    static EvalResult make_error(EvalResultType error);

    bool is_value() const noexcept { return type_ == EvalResultType::Value; }
    bool is_error() const noexcept { return !is_value(); }
    EvalResultType type() const noexcept { return type_; }
    explicit operator bool() const noexcept { return is_value(); }

    const Constant& value() const {
        TIRO_DEBUG_ASSERT(is_value(), "ExprResult is not a value.");
        TIRO_DEBUG_ASSERT(value_, "Value must be valid if is_value() is true.");
        return *value_;
    }

    const Constant& operator*() const { return value(); }
    const Constant* operator->() const { return &value(); }

    void format(FormatStream & stream) const;

private:
    explicit EvalResult(EvalResultType type);

private:
    EvalResultType type_;
    std::optional<Constant> value_;
};

/// Evaluates a binary operation whose operands are both constants.
EvalResult eval_binary_operation(BinaryOpType op, const Constant& lhs, const Constant& rhs);

/// Evaluates a unary operation whose operand is a constant.
EvalResult eval_unary_operation(UnaryOpType op, const Constant& value);

/// Evaluates string formatting of constants.
EvalResult eval_format(Span<const Constant> operands, StringTable& strings);

} // namespace tiro::ir

TIRO_ENABLE_FREE_TO_STRING(tiro::ir::EvalResultType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::ir::EvalResult);

#endif // TIRO_COMPILER_IR_GEN_CONST_EVAL_HPP
