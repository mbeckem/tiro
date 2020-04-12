#ifndef TIRO_IRTRAVERSAL_HPP
#define TIRO_IRTRAVERSAL_HPP

#include "tiro/core/not_null.hpp"
#include "tiro/ir/function.hpp"
#include "tiro/ir/fwd.hpp"

#include <vector>

namespace tiro {

/// Preorder traversal visits the cfg depth-first, parents before children.
// TODO: Iterative algorithm to save memory & speed.
class PreorderTraversal final {
public:
    explicit PreorderTraversal(const Function& func);

    auto begin() const { return blocks_.begin(); }
    auto end() const { return blocks_.end(); }

    size_t size() const { return blocks_.size(); }

    const auto& blocks() const { return blocks_; }

private:
    NotNull<const Function*> func_;
    std::vector<BlockID> blocks_;
};

/// Postorder traversal visits the cfg depth-first, children before parents.
// TODO: Create a smarter iterator that performs iterative postorder traversal
// to avoid materializing the entire blocks_ vector.
class PostorderTraversal final {
public:
    explicit PostorderTraversal(const Function& func);

    const Function& func() const { return *func_; }

    auto begin() const { return blocks_.begin(); }
    auto end() const { return blocks_.end(); }

    size_t size() const { return blocks_.size(); }

    const auto& blocks() const { return blocks_; }

private:
    NotNull<const Function*> func_;

    // TODO: It may not be necessary to store all blocks.
    std::vector<BlockID> blocks_;
};

/// Traverse the function's cfg in reverse postorder traversal, i.e. the reverse of
/// PostorderTraversal.
///
/// This kind of traversal is relatively costly because the complete 'order' vector
/// must be materialized in memory. Only use this order if it is actually needed.
class ReversePostorderTraversal final {
public:
    explicit ReversePostorderTraversal(const Function& func);

    const Function& func() const { return postorder_.func(); }

    auto begin() const { return postorder_.blocks().rbegin(); }
    auto end() const { return postorder_.blocks().rend(); }

    size_t size() const { return postorder_.size(); }

private:
    PostorderTraversal postorder_;
};

} // namespace tiro

#endif // TIRO_IRTRAVERSAL_HPP
