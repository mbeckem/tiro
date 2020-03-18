#ifndef TIRO_TEST_MIR_CFG_CONTEXT_HPP
#define TIRO_TEST_MIR_CFG_CONTEXT_HPP

#include "tiro/core/iter_tools.hpp"
#include "tiro/core/string_table.hpp"
#include "tiro/mir/types.hpp"

namespace tiro {

class TestFunction {
private:
    StringTable strings_;
    mir::Function func_;

public:
    explicit TestFunction(std::string_view function_name = "func")
        : strings_()
        , func_(strings_.insert(function_name), mir::FunctionType::Normal,
              strings_) {}

    TestFunction(TestFunction&&) = delete;
    TestFunction& operator=(TestFunction&&) = delete;

    StringTable& strings() { return strings_; }
    mir::Function& func() { return func_; }

    std::string_view label(mir::BlockID block) const {
        return strings_.dump(func_[block]->label());
    }

    mir::BlockID entry() const { return func_.entry(); }

    mir::BlockID exit() const { return func_.exit(); }

    mir::BlockID make_block(std::string_view label) {
        return func_.make(mir::Block(strings_.insert(label)));
    }

    void set_jump(mir::BlockID id, mir::BlockID target) {
        func_[id]->terminator(mir::Terminator::make_jump(target));
        func_[target]->append_predecessor(id);
    }

    void
    set_branch(mir::BlockID id, mir::BlockID target1, mir::BlockID target2) {
        func_[id]->terminator(mir::Terminator::make_branch(
            mir::BranchType::IfTrue, mir::LocalID(), target1, target2));
        func_[target1]->append_predecessor(id);
        func_[target2]->append_predecessor(id);
    }

    bool has_predecessor(mir::BlockID id, mir::BlockID pred) const {
        auto block = func_[id];
        return contains(block->predecessors(), pred);
    }
};

} // namespace tiro

#endif // TIRO_TEST_MIR_CFG_CONTEXT_HPP
