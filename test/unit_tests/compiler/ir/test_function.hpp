#ifndef TIRO_TEST_IR_CFG_CONTEXT_HPP
#define TIRO_TEST_IR_CFG_CONTEXT_HPP

#include "common/ranges/iter_tools.hpp"
#include "common/text/string_table.hpp"
#include "compiler/ir/function.hpp"

namespace tiro::ir::test {

class TestFunction {
private:
    StringTable strings_;
    ir::Function func_;

public:
    explicit TestFunction(std::string_view function_name = "func")
        : strings_()
        , func_(strings_.insert(function_name), ir::FunctionType::Normal, strings_) {}

    TestFunction(TestFunction&&) = delete;
    TestFunction& operator=(TestFunction&&) = delete;

    StringTable& strings() { return strings_; }
    ir::Function& func() { return func_; }

    std::string_view label(ir::BlockId block) const { return strings_.dump(func_[block]->label()); }

    ir::BlockId entry() const { return func_.entry(); }

    ir::BlockId exit() const { return func_.exit(); }

    ir::BlockId make_block(std::string_view label) {
        return func_.make(ir::Block(strings_.insert(label)));
    }

    void set_jump(ir::BlockId id, ir::BlockId target) {
        func_[id]->terminator(ir::Terminator::make_jump(target));
        func_[target]->append_predecessor(id);
    }

    void set_branch(ir::BlockId id, ir::BlockId target1, ir::BlockId target2) {
        func_[id]->terminator(
            ir::Terminator::make_branch(ir::BranchType::IfTrue, ir::InstId(), target1, target2));
        func_[target1]->append_predecessor(id);
        func_[target2]->append_predecessor(id);
    }

    bool has_predecessor(ir::BlockId id, ir::BlockId pred) const {
        auto block = func_[id];
        return contains(block->predecessors(), pred);
    }
};

} // namespace tiro::ir::test

#endif // TIRO_TEST_IR_CFG_CONTEXT_HPP
