#include "tiro/bytecode_gen/parallel_copy.hpp"

namespace tiro {

// Input: Set of parallel copies.
// Output: Serialized copies that implement the parallel input operations.
//
// Implements Algorithm 1 of the following paper:
//
// [BDR+] Benoit Boissinot, Alain Darte, Fabrice Rastello, Benoît Dupont de Dinechin, Christophe Guillon.
//          Revisiting Out-of-SSA Translation for Correctness, Code Quality, and Efficiency.
//          [Research Report] 2008, pp.14. ￿inria-00349925v1
void sequentialize_parallel_copies(std::vector<RegisterCopy>& copies,
    FunctionRef<CompiledLocalID()> alloc_spare) {
    // TODO: Optimize. Containers. Possibly reuse memory.
    // Example: pred is never mutated after init and does not require a map.
    std::vector<CompiledLocalID> ready;
    std::vector<CompiledLocalID> todo;
    std::unordered_map<CompiledLocalID, CompiledLocalID, UseHasher> loc;
    std::unordered_map<CompiledLocalID, CompiledLocalID, UseHasher> pred;
    std::optional<CompiledLocalID> spare;

    // a != b for all copies.
    copies.erase(std::remove_if(copies.begin(), copies.end(),
                     [&](const auto& copy) { return copy.src == copy.dest; }),
        copies.end());
    if (copies.empty())
        return;

    for (const auto& [a, b] : copies) {
        TIRO_DEBUG_ASSERT(a, "Invalid source register in copy.");
        TIRO_DEBUG_ASSERT(b, "Invalid destination register in copy.");

        loc[a] = a;
        pred[b] = a;
        todo.push_back(b);
    }

    for (const auto& [a, b] : copies) {
        if (!loc[b])
            ready.push_back(b);
    }

    copies.clear();
    while (!todo.empty()) {
        while (!ready.empty()) {
            const auto b = ready.back();
            ready.pop_back();

            const auto a = pred[b];
            const auto c = loc[a];
            copies.push_back({c, b});

            loc[a] = b;
            if (a == c && pred[a])
                ready.push_back(a);
        }

        const auto b = todo.back();
        todo.pop_back();

        // I believe the original publication contains an error here.
        // The condition has been inverted.
        if (b != loc[pred[b]]) {
            if (!spare)
                spare = alloc_spare();

            copies.push_back({b, *spare});
            loc[b] = *spare;
            ready.push_back(b);
        }
    }
}

} // namespace tiro
