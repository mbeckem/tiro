#ifndef HAMMER_VM_TYPES_HPP
#define HAMMER_VM_TYPES_HPP

#include "hammer/core/span.hpp"
#include "hammer/vm/fwd.hpp"
#include "hammer/vm/objects/object.hpp"

namespace hammer::vm {

// TODO classes and stuff
class TypeSystem {
public:
    // Called by context
    void init(Context& ctx);

    template<typename W>
    void walk(W&& w) {
        w(remove_);
    }

    bool invoke_member(Context& ctx, Handle<Value> object, Symbol member,
        /* FIXME rooted */ Span<Value> args, MutableHandle<Value> result);

private:
    Symbol remove_;
};

} // namespace hammer::vm

#endif // HAMMER_VM_TYPES_HPP
