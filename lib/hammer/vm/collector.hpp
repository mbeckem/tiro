#ifndef HAMMER_VM_COLLECTOR_HPP
#define HAMMER_VM_COLLECTOR_HPP

#include "hammer/core/defs.hpp"
#include "hammer/vm/value.hpp"

#include <vector>

namespace hammer::vm {

class Context;

class Collector {
public:
    Collector();

    Collector(const Collector&) = delete;
    Collector& operator=(const Collector&) = delete;

    void collect(Context& ctx);

private:
    struct Walker;

private:
    void run_mark();
    void mark(Value v);
    void trace(Walker& w, Value v);

private:
    // For marking. Should be replaced by some preallocated memory in the future.
    std::vector<Value> stack_;
};

} // namespace hammer::vm

#endif // HAMMER_VM_COLLECTOR_HPP
