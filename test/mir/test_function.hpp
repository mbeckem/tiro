#ifndef TIRO_TEST_MIR_CFG_CONTEXT_HPP
#define TIRO_TEST_MIR_CFG_CONTEXT_HPP

#include "tiro/core/iter_tools.hpp"
#include "tiro/core/string_table.hpp"
#include "tiro/mir/types.hpp"

namespace tiro {

class TestFunction {
private:
    StringTable strings_;
    Function func_;

public:
    explicit TestFunction(std::string_view function_name = "func")
        : strings_()
        , func_(
              strings_.insert(function_name), FunctionType::Normal, strings_) {}

    TestFunction(TestFunction&&) = delete;
    TestFunction& operator=(TestFunction&&) = delete;

    StringTable& strings() { return strings_; }
    Function& func() { return func_; }

    std::string_view label(BlockID block) const {
        return strings_.dump(func_[block]->label());
    }

    BlockID entry() const { return func_.entry(); }

    BlockID exit() const { return func_.exit(); }

    BlockID make_block(std::string_view label) {
        return func_.make(Block(strings_.insert(label)));
    }

    void set_jump(BlockID id, BlockID target) {
        func_[id]->terminator(Terminator::make_jump(target));
        func_[target]->append_predecessor(id);
    }

    void set_branch(BlockID id, BlockID target1, BlockID target2) {
        func_[id]->terminator(Terminator::make_branch(
            BranchType::IfTrue, LocalID(), target1, target2));
        func_[target1]->append_predecessor(id);
        func_[target2]->append_predecessor(id);
    }

    bool has_predecessor(BlockID id, BlockID pred) const {
        auto block = func_[id];
        return contains(block->predecessors(), pred);
    }
};

} // namespace tiro

#endif // TIRO_TEST_MIR_CFG_CONTEXT_HPP
