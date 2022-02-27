#include <catch2/catch.hpp>

#include "bytecode/function.hpp"
#include "bytecode/module.hpp"
#include "bytecode/writer.hpp"
#include "common/adt/function_ref.hpp"
#include "vm/modules/verify.hpp"

#include "support/matchers.hpp"

namespace tiro::vm::test {

using test_support::exception_contains_string;

static BytecodeModule empty_module();
static BytecodeFunction empty_function();
static BytecodeFunction
simple_function(u32 params, u32 locals, FunctionRef<void(BytecodeWriter& writer)> write_insts);
static BytecodeMemberId add_simple_function(BytecodeModule& mod, u32 params, u32 locals,
    FunctionRef<void(BytecodeWriter& writer)> write_insts);

// NOTE: non-error cases are not tested here, they are tested implicitly by many other tests
// running their code through the verifier when compiling and loading test code.

TEST_CASE("verifier rejects modules without a name", "[module-verify]") {
    BytecodeModule mod;
    REQUIRE_THROWS_MATCHES(verify_module(mod), Error, exception_contains_string("valid name"));
}

TEST_CASE("verifier rejects invalid order of members", "[module-verify]") {
    BytecodeModule mod = empty_module();
    const auto sym_id = mod.members().push_back(BytecodeMember::make_symbol(BytecodeMemberId(1)));
    const auto str_id = mod.members().push_back(
        BytecodeMember::make_string(mod.strings().insert("foo")));

    REQUIRE(sym_id.value() == 0);
    REQUIRE(str_id.value() == 1);
    REQUIRE_THROWS_MATCHES(
        verify_module(mod), Error, exception_contains_string("has not been visited yet"));
}

TEST_CASE("verifier rejects invalid member reference", "[module-verify]") {
    BytecodeModule mod = empty_module();
    mod.members().push_back(BytecodeMember::make_symbol({}));
    REQUIRE_THROWS_MATCHES(
        verify_module(mod), Error, exception_contains_string("invalid module member"));
}

TEST_CASE("verifier rejects out of bounds member reference", "[module-verify]") {
    BytecodeModule mod = empty_module();
    mod.members().push_back(BytecodeMember::make_symbol(BytecodeMemberId(12345)));
    REQUIRE_THROWS_MATCHES(verify_module(mod), Error, exception_contains_string("out of bounds"));
}

TEST_CASE("verifier rejects symbols that do not reference a string", "[module-verify]") {
    BytecodeModule mod = empty_module();
    const auto int_id = mod.members().push_back(BytecodeMember::make_integer(123));
    mod.members().push_back(BytecodeMember::make_symbol(int_id));
    REQUIRE_THROWS_MATCHES(verify_module(mod), Error, exception_contains_string("is not a string"));
}

TEST_CASE("verifier rejects imports that do not reference a string", "[module-verify]") {
    BytecodeModule mod = empty_module();
    const auto int_id = mod.members().push_back(BytecodeMember::make_integer(123));
    mod.members().push_back(BytecodeMember::make_import(int_id));
    REQUIRE_THROWS_MATCHES(verify_module(mod), Error, exception_contains_string("is not a string"));
}

TEST_CASE("verifier rejects invalid function references", "[module-verify]") {
    BytecodeModule mod = empty_module();
    mod.members().push_back(BytecodeMember::make_function({}));
    REQUIRE_THROWS_MATCHES(
        verify_module(mod), Error, exception_contains_string("invalid function reference"));
}

TEST_CASE(
    "verifier rejects named function whose name does not reference a string", "[module-verify]") {
    BytecodeModule mod = empty_module();
    const auto int_id = mod.members().push_back(BytecodeMember::make_integer(123));
    const auto fn_id = mod.functions().push_back(BytecodeFunction());
    mod[fn_id].name(int_id);
    mod.members().push_back(BytecodeMember::make_function(fn_id));
    REQUIRE_THROWS_MATCHES(verify_module(mod), Error, exception_contains_string("is not a string"));
}

TEST_CASE("verifier rejects invalid record schema references", "[module-verify]") {
    BytecodeModule mod = empty_module();
    mod.members().push_back(BytecodeMember::make_record_schema({}));
    REQUIRE_THROWS_MATCHES(
        verify_module(mod), Error, exception_contains_string("invalid record schema reference"));
}

TEST_CASE("verifier rejects record schemas whose keys are not symbols", "[module-verify]") {
    BytecodeModule mod = empty_module();
    const auto int_id = mod.members().push_back(BytecodeMember::make_integer(123));
    const auto tmpl_id = mod.records().emplace_back();
    mod[tmpl_id].keys().push_back(int_id);
    mod.members().push_back(BytecodeMember::make_record_schema(tmpl_id));
    REQUIRE_THROWS_MATCHES(verify_module(mod), Error, exception_contains_string("is not a symbol"));
}

TEST_CASE(
    "verifier rejects modules whose init field does not reference a function", "[module-verify]") {
    BytecodeModule mod = empty_module();
    const auto int_id = mod.members().push_back(BytecodeMember::make_integer(123));
    mod.init(int_id);
    REQUIRE_THROWS_MATCHES(
        verify_module(mod), Error, exception_contains_string("is not a function"));
}

TEST_CASE(
    "verifier rejects modules whose init function is not a normal function", "[module-verify]") {
    BytecodeModule mod = empty_module();
    BytecodeFunction fn = empty_function();
    fn.type(BytecodeFunctionType::Closure);
    auto fn_id = mod.functions().push_back(std::move(fn));
    auto fn_member_id = mod.members().push_back(BytecodeMember::make_function(fn_id));
    mod.init(fn_member_id);
    REQUIRE_THROWS_MATCHES(
        verify_module(mod), Error, exception_contains_string("is not a normal function"));
}

TEST_CASE("verifier rejects exports where the name is not a symbol", "[module-verify]") {
    BytecodeModule mod = empty_module();
    auto int_id = mod.members().push_back(BytecodeMember::make_integer(123));
    mod.add_export(int_id, int_id);
    REQUIRE_THROWS_MATCHES(verify_module(mod), Error, exception_contains_string("is not a symbol"));
}

TEST_CASE("verifier rejects forbidden export values", "[module-verify]") {
    BytecodeModule mod = empty_module();
    auto str_id = mod.members().push_back(
        BytecodeMember::make_string(mod.strings().insert("my_export")));
    auto sym_id = mod.members().push_back(BytecodeMember::make_symbol(str_id));

    SECTION("exported import") {
        auto imp_id = mod.members().push_back(BytecodeMember::make_import(str_id));
        mod.add_export(sym_id, imp_id);
        REQUIRE_THROWS_MATCHES(
            verify_module(mod), Error, exception_contains_string("forbidden export"));
    }

    SECTION("exported record schema") {
        auto rec_id = mod.records().emplace_back();
        auto rec_member_id = mod.members().push_back(BytecodeMember::make_record_schema(rec_id));
        mod.add_export(sym_id, rec_member_id);
        REQUIRE_THROWS_MATCHES(
            verify_module(mod), Error, exception_contains_string("forbidden export"));
    }

    SECTION("exported closure function") {
        BytecodeFunction fn = empty_function();
        fn.type(BytecodeFunctionType::Closure);
        auto fn_id = mod.functions().push_back(std::move(fn));
        auto fn_member_id = mod.members().push_back(BytecodeMember::make_function(fn_id));
        mod.add_export(sym_id, fn_member_id);
        REQUIRE_THROWS_MATCHES(
            verify_module(mod), Error, exception_contains_string("is not a normal function"));
    }
}

TEST_CASE("verifier rejects functions without any code", "[module-verify]") {
    BytecodeModule mod = empty_module();
    BytecodeFunction fn;
    auto fn_id = mod.functions().push_back(std::move(fn));
    mod.members().push_back(BytecodeMember::make_function(fn_id));
    REQUIRE_THROWS_MATCHES(
        verify_module(mod), Error, exception_contains_string("function body must not be empty"));
}

TEST_CASE(
    "verifier rejects functions that do not end with a halting instruction", "[module-verify]") {
    BytecodeModule mod = empty_module();
    add_simple_function(mod, 0, 0, [&](BytecodeWriter& writer) { writer.pop(); });
    REQUIRE_THROWS_MATCHES(
        verify_module(mod), Error, exception_contains_string("halting instruction"));
}

TEST_CASE("verifier rejects exception handlers", "[module-verify]") {
    BytecodeModule mod = empty_module();

    BytecodeOffset i1_pos;
    BytecodeOffset i2_pos;
    BytecodeOffset i3_pos;
    BytecodeFunctionId func_id = mod.functions().push_back(
        simple_function(0, 1, [&](BytecodeWriter& writer) {
            writer.load_null(BytecodeRegister(0));

            i1_pos = BytecodeOffset(writer.pos());
            writer.load_null(BytecodeRegister(0));
            i2_pos = BytecodeOffset(writer.pos());
            writer.load_null(BytecodeRegister(0));
            i3_pos = BytecodeOffset(writer.pos());
            writer.load_null(BytecodeRegister(0));
            writer.ret(BytecodeRegister(0));
        }));
    mod.members().push_back(BytecodeMember::make_function(func_id));

    auto& handlers = mod[func_id].handlers();

    SECTION("invalid from") {
        handlers.emplace_back(BytecodeOffset(), i1_pos, i2_pos);
        REQUIRE_THROWS_MATCHES(verify_module(mod), Error,
            exception_contains_string("invalid exception handler start"));
    }

    SECTION("invalid from: not an instruction start") {
        handlers.emplace_back(BytecodeOffset(i1_pos.value() + 1), i2_pos, i3_pos);
        REQUIRE_THROWS_MATCHES(verify_module(mod), Error,
            exception_contains_string("invalid exception handler start"));
    }

    SECTION("invalid from: out of bounds") {
        handlers.emplace_back(BytecodeOffset(12345), BytecodeOffset(12346), i3_pos);
        REQUIRE_THROWS_MATCHES(verify_module(mod), Error,
            exception_contains_string("invalid exception handler start"));
    }

    SECTION("invalid to") {
        handlers.emplace_back(i1_pos, BytecodeOffset(), i2_pos);
        REQUIRE_THROWS_MATCHES(
            verify_module(mod), Error, exception_contains_string("invalid exception handler end"));
    }

    SECTION("invalid to: neither an instruction start nor the end of function") {
        handlers.emplace_back(i1_pos, BytecodeOffset(i2_pos.value() + 1), i3_pos);
        REQUIRE_THROWS_MATCHES(
            verify_module(mod), Error, exception_contains_string("invalid exception handler end"));
    }

    SECTION("invalid to: not greater than the start") {
        handlers.emplace_back(i1_pos, i1_pos, i3_pos);
        REQUIRE_THROWS_MATCHES(verify_module(mod), Error,
            exception_contains_string("invalid exception handler interval"));
    }

    SECTION("invalid target") {
        handlers.emplace_back(i1_pos, i2_pos, BytecodeOffset());
        REQUIRE_THROWS_MATCHES(verify_module(mod), Error,
            exception_contains_string("invalid exception handler target"));
    }

    SECTION("invalid target: not an instruction start") {
        handlers.emplace_back(i1_pos, i2_pos, BytecodeOffset(i3_pos.value() + 1));
        REQUIRE_THROWS_MATCHES(verify_module(mod), Error,
            exception_contains_string("invalid exception handler target"));
    }

    SECTION("invalid target: out of bounds") {
        handlers.emplace_back(i1_pos, i2_pos, BytecodeOffset(12345));
        REQUIRE_THROWS_MATCHES(verify_module(mod), Error,
            exception_contains_string("invalid exception handler target"));
    }

    SECTION("intervals overlap") {
        handlers.emplace_back(i1_pos, i3_pos, i1_pos);
        handlers.emplace_back(i2_pos, i3_pos, i1_pos);
        REQUIRE_THROWS_MATCHES(
            verify_module(mod), Error, exception_contains_string("entries must be ordered"));
    }

    SECTION("intervals reversed") {
        handlers.emplace_back(i2_pos, i3_pos, i1_pos);
        handlers.emplace_back(i1_pos, i3_pos, i1_pos);
        REQUIRE_THROWS_MATCHES(
            verify_module(mod), Error, exception_contains_string("entries must be ordered"));
    }
}

TEST_CASE("verifier rejects functions that reference undeclared locals", "[module-verify]") {
    BytecodeModule mod = empty_module();
    add_simple_function(mod, 0, 0, [&](BytecodeWriter& writer) {
        writer.load_null(BytecodeRegister(0));
        writer.ret(BytecodeRegister(0));
    });
    REQUIRE_THROWS_MATCHES(
        verify_module(mod), Error, exception_contains_string("local index out of bounds"));
}

TEST_CASE("verifier rejects functions that reference undeclared parameters", "[module-verify]") {
    BytecodeModule mod = empty_module();

    SECTION("load param") {
        add_simple_function(mod, 1, 1, [&](BytecodeWriter& writer) {
            writer.load_param(BytecodeParam(1), BytecodeRegister(0));
            writer.ret(BytecodeRegister(0));
        });
        REQUIRE_THROWS_MATCHES(
            verify_module(mod), Error, exception_contains_string("parameter index out of bounds"));
    }

    SECTION("store param") {
        add_simple_function(mod, 1, 1, [&](BytecodeWriter& writer) {
            writer.store_param(BytecodeRegister(0), BytecodeParam(1));
            writer.ret(BytecodeRegister(0));
        });
        REQUIRE_THROWS_MATCHES(
            verify_module(mod), Error, exception_contains_string("parameter index out of bounds"));
    }
}

TEST_CASE(
    "verifier rejects functions that reference undeclared module members", "[module-verify]") {
    BytecodeModule mod = empty_module();
    add_simple_function(mod, 0, 1, [&](BytecodeWriter& writer) {
        writer.load_module(BytecodeMemberId(12345), BytecodeRegister(0));
        writer.ret(BytecodeRegister(0));
    });
    REQUIRE_THROWS_MATCHES(verify_module(mod), Error, exception_contains_string("out of bounds"));
}

TEST_CASE("verifier rejects member references that do not use symbols", "[module-verify]") {
    BytecodeModule mod = empty_module();
    auto str_id = mod.members().push_back(BytecodeMember::make_string(mod.strings().insert("foo")));

    SECTION("load member") {
        add_simple_function(mod, 0, 2, [&](BytecodeWriter& writer) {
            writer.load_member(BytecodeRegister(0), str_id, BytecodeRegister(1));
            writer.ret(BytecodeRegister(0));
        });
        REQUIRE_THROWS_MATCHES(
            verify_module(mod), Error, exception_contains_string("must reference a symbol"));
    }

    SECTION("store member") {
        add_simple_function(mod, 0, 2, [&](BytecodeWriter& writer) {
            writer.store_member(BytecodeRegister(0), BytecodeRegister(1), str_id);
            writer.ret(BytecodeRegister(0));
        });
        REQUIRE_THROWS_MATCHES(
            verify_module(mod), Error, exception_contains_string("must reference a symbol"));
    }

    SECTION("load method") {
        add_simple_function(mod, 0, 3, [&](BytecodeWriter& writer) {
            writer.load_method(
                BytecodeRegister(0), str_id, BytecodeRegister(1), BytecodeRegister(2));
            writer.ret(BytecodeRegister(0));
        });
        REQUIRE_THROWS_MATCHES(
            verify_module(mod), Error, exception_contains_string("must reference a symbol"));
    }
}

TEST_CASE("verifier rejects non-closure functions that use the LoadClosure instruction",
    "[module-verify]") {
    BytecodeModule mod = empty_module();
    add_simple_function(mod, 0, 1, [&](BytecodeWriter& writer) {
        writer.load_closure(BytecodeRegister(0));
        writer.ret(BytecodeRegister(0));
    });
    REQUIRE_THROWS_MATCHES(
        verify_module(mod), Error, exception_contains_string("only closure functions"));
}

TEST_CASE("verifier rejects array instructions with too many arguments", "[module-verify]") {
    BytecodeModule mod = empty_module();
    add_simple_function(mod, 0, 1, [&](BytecodeWriter& writer) {
        writer.array(9999999, BytecodeRegister(0));
        writer.ret(BytecodeRegister(0));
    });
    REQUIRE_THROWS_MATCHES(verify_module(mod), Error,
        exception_contains_string("Too many arguments in array construction"));
}

TEST_CASE("verifier rejects tuple instructions with too many arguments", "[module-verify]") {
    BytecodeModule mod = empty_module();
    add_simple_function(mod, 0, 1, [&](BytecodeWriter& writer) {
        writer.tuple(9999999, BytecodeRegister(0));
        writer.ret(BytecodeRegister(0));
    });
    REQUIRE_THROWS_MATCHES(verify_module(mod), Error,
        exception_contains_string("Too many arguments in tuple construction"));
}

TEST_CASE("verifier rejects set instructions with too many arguments", "[module-verify]") {
    BytecodeModule mod = empty_module();
    add_simple_function(mod, 0, 1, [&](BytecodeWriter& writer) {
        writer.set(9999999, BytecodeRegister(0));
        writer.ret(BytecodeRegister(0));
    });
    REQUIRE_THROWS_MATCHES(verify_module(mod), Error,
        exception_contains_string("Too many arguments in set construction"));
}

TEST_CASE("verifier rejects map instructions with too many arguments", "[module-verify]") {
    BytecodeModule mod = empty_module();
    add_simple_function(mod, 0, 1, [&](BytecodeWriter& writer) {
        writer.map(9999998, BytecodeRegister(0));
        writer.ret(BytecodeRegister(0));
    });
    REQUIRE_THROWS_MATCHES(verify_module(mod), Error,
        exception_contains_string("Too many arguments in map construction"));
}

TEST_CASE("verifier rejects map instructions with odd number of params", "[module-verify]") {
    BytecodeModule mod = empty_module();
    add_simple_function(mod, 0, 1, [&](BytecodeWriter& writer) {
        writer.map(123, BytecodeRegister(0));
        writer.ret(BytecodeRegister(0));
    });
    REQUIRE_THROWS_MATCHES(
        verify_module(mod), Error, exception_contains_string("even number of keys and values"));
}

TEST_CASE(
    "verifier rejects closure instructions that do not reference a function", "[module-verify]") {
    BytecodeModule mod = empty_module();
    auto int_id = mod.members().push_back(BytecodeMember::make_integer(123));
    add_simple_function(mod, 0, 2, [&](BytecodeWriter& writer) {
        writer.closure(int_id, BytecodeRegister(0), BytecodeRegister(1));
        writer.ret(BytecodeRegister(1));
    });
    REQUIRE_THROWS_MATCHES(
        verify_module(mod), Error, exception_contains_string("must reference a closure function"));
}

TEST_CASE("verifier rejects closure instructions that do not reference a closure function",
    "[module-verify]") {
    BytecodeModule mod = empty_module();
    BytecodeFunction fn = empty_function();
    auto fn_id = mod.functions().push_back(std::move(fn));
    auto fn_member_id = mod.members().push_back(BytecodeMember::make_function(fn_id));

    add_simple_function(mod, 0, 2, [&](BytecodeWriter& writer) {
        writer.closure(fn_member_id, BytecodeRegister(0), BytecodeRegister(1));
        writer.ret(BytecodeRegister(1));
    });
    REQUIRE_THROWS_MATCHES(
        verify_module(mod), Error, exception_contains_string("must reference a closure function"));
}

TEST_CASE(
    "verifier rejects jumps that do not point to the start of an instruction", "[module-verify]") {
    BytecodeModule mod = empty_module();
    add_simple_function(mod, 0, 1, [&](BytecodeWriter& writer) {
        writer.load_null(BytecodeRegister(0));
        u32 pos = writer.pos();
        writer.jmp(BytecodeOffset(pos + 1));
        writer.ret(BytecodeRegister(0));
    });
    REQUIRE_THROWS_MATCHES(verify_module(mod), Error,
        exception_contains_string("destination does not point to the start of an instruction"));
}

static BytecodeModule empty_module() {
    BytecodeModule mod;
    mod.name(mod.strings().insert("test"));
    return mod;
}

static BytecodeFunction empty_function() {
    BytecodeFunction func;
    func.locals(1);

    BytecodeWriter writer(func);
    writer.load_null(BytecodeRegister(0));
    writer.ret(BytecodeRegister(0));
    writer.finish();
    return func;
}

static BytecodeFunction
simple_function(u32 params, u32 locals, FunctionRef<void(BytecodeWriter& writer)> write_insts) {
    BytecodeFunction func;
    func.params(params);
    func.locals(locals);

    BytecodeWriter writer(func);
    write_insts(writer);
    writer.finish();
    return func;
}

static BytecodeMemberId add_simple_function(BytecodeModule& mod, u32 params, u32 locals,
    FunctionRef<void(BytecodeWriter& writer)> write_insts) {
    BytecodeFunction func = simple_function(params, locals, write_insts);
    auto fn_id = mod.functions().push_back(std::move(func));
    auto member_id = mod.members().push_back(BytecodeMember::make_function(fn_id));
    return member_id;
}

} // namespace tiro::vm::test
