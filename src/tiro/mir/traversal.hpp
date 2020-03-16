#ifndef TIRO_MIR_TRAVERSAL_HPP
#define TIRO_MIR_TRAVERSAL_HPP

#include "tiro/core/not_null.hpp"
#include "tiro/mir/fwd.hpp"
#include "tiro/mir/types.hpp"

#include <vector>

namespace tiro::compiler::mir {

/// Preorder traversal visits the cfg depth-first, parents before children.
// TODO: Iterative algorithm to save memory & speed.
class PreorderTraversal final {
public:
    explicit PreorderTraversal(const mir::Function& func);

    auto begin() const { return blocks_.begin(); }
    auto end() const { return blocks_.end(); }

    size_t size() const { return blocks_.size(); }

    const auto& blocks() const { return blocks_; }

private:
    NotNull<const mir::Function*> func_;
    std::vector<mir::BlockID> blocks_;
};

/// Postorder traversal visits the cfg depth-first, children before parents.
// TODO: Create a smarter iterator that performs iterative postorder traversal
// to avoid materializing the entire blocks_ vector.
class PostorderTraversal final {
public:
    explicit PostorderTraversal(const mir::Function& func);

    const mir::Function& func() const { return *func_; }

    auto begin() const { return blocks_.begin(); }
    auto end() const { return blocks_.end(); }

    size_t size() const { return blocks_.size(); }

    const auto& blocks() const { return blocks_; }

private:
    NotNull<const mir::Function*> func_;

    // TODO: It may not be necessary to store all blocks.
    std::vector<mir::BlockID> blocks_;
};

/// Traverse the function's cfg in reverse postorder traversal, i.e. the reverse of
/// PostorderTraversal.
///
/// This kind of traversal is relatively costly because the complete 'order' vector
/// must be materialized in memory. Only use this order if it is actually needed.
class ReversePostorderTraversal final {
public:
    explicit ReversePostorderTraversal(const mir::Function& func);

    const mir::Function& func() const { return postorder_.func(); }

    auto begin() const { return postorder_.blocks().rbegin(); }
    auto end() const { return postorder_.blocks().rend(); }

    size_t size() const { return postorder_.size(); }

private:
    PostorderTraversal postorder_;
};

} // namespace tiro::compiler::mir

#endif // TIRO_MIR_TRAVERSAL_HPP
