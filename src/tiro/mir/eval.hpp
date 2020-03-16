#ifndef TIRO_MIR_EVAL_HPP
#define TIRO_MIR_EVAL_HPP

#include "tiro/core/format.hpp"
#include "tiro/core/span.hpp"
#include "tiro/mir/fwd.hpp"
#include "tiro/mir/types.hpp"

namespace tiro::compiler::mir_transform {

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
    EvalResult(const mir::Constant& value);

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

    const mir::Constant& value() const {
        TIRO_ASSERT(is_value(), "ExprResult is not a value.");
        TIRO_ASSERT(value_, "Value must be valid if is_value() is true.");
        return *value_;
    }

    const mir::Constant& operator*() const { return value(); }
    const mir::Constant* operator->() const { return &value(); }

    void format(FormatStream & stream) const;

private:
    explicit EvalResult(EvalResultType type);

private:
    EvalResultType type_;
    std::optional<mir::Constant> value_;
};

/// Evaluates a binary operation whose operands are both constants.
EvalResult eval_binary_operation(
    mir::BinaryOpType op, const mir::Constant& lhs, const mir::Constant& rhs);

/// Evaluates a unary operation whose operand is a constant.
EvalResult
eval_unary_operation(mir::UnaryOpType op, const mir::Constant& value);

/// Evaluates string formatting of constants.
EvalResult
eval_format(Span<const mir::Constant> operands, StringTable& strings);

} // namespace tiro::compiler::mir_transform

TIRO_ENABLE_FREE_TO_STRING(tiro::compiler::mir_transform::EvalResultType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::compiler::mir_transform::EvalResult);

#endif // TIRO_MIR_EVAL_HPP
