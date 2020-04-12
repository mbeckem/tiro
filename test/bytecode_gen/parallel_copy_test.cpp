#include <catch.hpp>

#include "tiro/bytecode_gen/parallel_copy.hpp"

#include <unordered_map>

using namespace tiro;

namespace {

struct RegisterValue {
    std::string name;
    std::string value;
};

struct ParallelCopy {
    std::string src;
    std::string dest;
};

class Driver {
public:
    void init(std::initializer_list<RegisterValue> initial);
    void parallel_copy(std::initializer_list<ParallelCopy> copies);
    void require(std::initializer_list<RegisterValue> expected);

    u32 spare_used() const { return spare_count_; }
    u32 copies_performed() const { return copies_; }

private:
    std::string alloc_spare();

    std::unordered_map<std::string, std::string> named_values_;
    u32 spare_count_ = 0;
    u32 copies_ = 0;
};

} // namespace

void Driver::init(std::initializer_list<RegisterValue> initial) {
    named_values_.clear();
    spare_count_ = 0;
    copies_ = 0;

    for (const auto& v : initial) {
        CAPTURE(v.name);

        bool inserted = named_values_.emplace(v.name, v.value).second;
        if (!inserted) {
            FAIL("Name is not unique.");
        }
    }
}

void Driver::parallel_copy(std::initializer_list<ParallelCopy> copies) {
    u32 count = 0;
    std::unordered_map<std::string, BytecodeRegister> name_to_reg;
    std::unordered_map<BytecodeRegister, std::string, UseHasher> reg_to_name;

    auto map_register = [&](const std::string& name) {
        const auto reg = BytecodeRegister(count++);
        name_to_reg[name] = reg;
        reg_to_name[reg] = name;
        return reg;
    };

    for (const auto& pair : named_values_) {
        map_register(pair.first);
    }

    std::vector<RegisterCopy> reg_copies;
    for (const auto& copy : copies) {
        CAPTURE(copy.src, copy.dest);
        auto src = name_to_reg.at(copy.src);
        auto dest = name_to_reg.at(copy.dest);
        reg_copies.push_back({src, dest});
    }

    sequentialize_parallel_copies(
        reg_copies, [&]() { return map_register(alloc_spare()); });

    for (const auto& copy : reg_copies) {
        auto src_name = reg_to_name.at(copy.src);
        auto dest_name = reg_to_name.at(copy.dest);
        named_values_[dest_name] = named_values_.at(src_name);
        ++copies_;
    }
}

void Driver::require(std::initializer_list<RegisterValue> expected) {
    for (const auto& ex : expected) {
        CAPTURE(ex.name);

        auto pos = named_values_.find(ex.name);
        REQUIRE(pos != named_values_.end());

        const auto& actual = pos->second;
        REQUIRE(actual == ex.value);
    }
}

std::string Driver::alloc_spare() {
    return fmt::format("##spare_{}", spare_count_++);
}

TEST_CASE(
    "Parallel copy of disjoint assignments should not use spare "
    "registers",
    "[parallel-copy]") {

    Driver d;
    d.init({
        {"A", "1"},
        {"B", "2"},
        {"C", "3"},
        {"D", "4"},
    });
    d.parallel_copy({{"A", "B"}, {"C", "D"}});
    d.require({
        {"A", "1"},
        {"B", "1"},
        {"C", "3"},
        {"D", "3"},
    });
    REQUIRE(d.spare_used() == 0);
}

TEST_CASE(
    "Parallel copy of overwriting assignments without cycle should not use "
    "spare registers",
    "[parallel-copy]") {

    Driver d;
    d.init({
        {"A", "1"},
        {"B", "2"},
        {"C", "3"},
        {"D", "4"},
    });
    d.parallel_copy({
        // A is not a destination
        {"A", "B"},
        {"B", "C"},
        {"C", "D"},
    });
    d.require({
        {"A", "1"},
        {"B", "1"},
        {"C", "2"},
        {"D", "3"},
    });
    REQUIRE(d.spare_used() == 0);
}

TEST_CASE(
    "Parallel copy of a cycle may use a spare register", "[parallel-copy]") {
    Driver d;
    d.init({
        {"A", "1"},
        {"B", "2"},
        {"C", "3"},
        {"D", "4"},
    });
    d.parallel_copy({
        {"A", "B"}, {"B", "C"}, {"C", "A"},
        // D unaffected
    });
    d.require({
        {"A", "3"},
        {"B", "1"},
        {"C", "2"},
        {"D", "4"},
    });
    REQUIRE(d.spare_used() <= 1);
}

TEST_CASE("Parallel copy should handle assignment cycle with inner tree",
    "[parallel-copy]") {
    /*  
        Assignment graph contains a cycle, B is additionally used as a tree root.
            (A, X, B, C, Y, D) = (B, B, C, D, B, A)
    */
    Driver d;
    d.init({
        {"A", "1"},
        {"B", "2"},
        {"C", "3"},
        {"D", "4"},
        {"X", "-1"},
        {"Y", "-2"},
    });
    d.parallel_copy({
        {"B", "A"},
        {"B", "X"},
        {"C", "B"},
        {"D", "C"},
        {"B", "Y"},
        {"A", "D"},
    });
    d.require({
        {"A", "2"},
        {"B", "3"},
        {"C", "4"},
        {"D", "1"},
        {"X", "2"},
        {"Y", "2"},
    });
    REQUIRE(d.spare_used() <= 1);
}

TEST_CASE("Parallel copy should ignore self assignment", "[parallel-copy]") {
    Driver d;
    d.init({
        {"A", "1"},
        {"B", "2"},
    });
    d.parallel_copy({
        {"A", "A"},
        {"A", "B"},
    });
    d.require({
        {"A", "1"},
        {"B", "1"},
    });
    REQUIRE(d.spare_used() == 0);
    REQUIRE(d.copies_performed() == 1);
}

TEST_CASE(
    "Parallel copy should not require multiple spare variables for multiple "
    "cycles",
    "[parallel-copy]") {
    Driver d;
    d.init({
        {"A", "1"},
        {"B", "2"},
        {"C", "3"},
        {"D", "4"},
        {"E", "5"},
        {"F", "6"},
        {"G", "7"},
        {"H", "8"},
    });
    d.parallel_copy({
        // Cycles: c->b->a, d->e
        {"H", "G"},
        {"G", "H"},
        {"G", "F"},

        {"D", "E"},
        {"E", "D"},

        {"C", "B"},
        {"B", "A"},
        {"A", "C"},
    });
    d.require({
        {"H", "7"},
        {"G", "8"},
        {"F", "7"},

        {"E", "4"},
        {"D", "5"},

        {"C", "1"},
        {"B", "3"},
        {"A", "2"},
    });

    REQUIRE(d.spare_used() <= 1);
}
