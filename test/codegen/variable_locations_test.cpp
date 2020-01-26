#include <catch.hpp>

#include "tiro/codegen/variable_locations.hpp"
#include "tiro/semantics/analyzer.hpp"
#include "tiro/syntax/ast.hpp"
#include "tiro/syntax/parser.hpp"

#include <iostream>
#include <unordered_set>

using namespace tiro;
using namespace tiro::compiler;

struct FunctionResult {
    std::unique_ptr<SymbolTable> symbols_;
    std::unique_ptr<StringTable> strings_;
    NodePtr<Root> root_;
    NodePtr<FuncDecl> func_;

    SymbolTable& symbols() { return *symbols_; }
    StringTable& strings() { return *strings_; }
    NodePtr<FuncDecl> get() { return func_; }
};

static FunctionResult parse_function(std::string_view source) {
    FunctionResult result;
    result.strings_ = std::make_unique<StringTable>();
    result.symbols_ = std::make_unique<SymbolTable>();

    Diagnostics diag;

    auto check_diag = [&]() {
        if (diag.message_count() > 0) {
            for (const auto& msg : diag.messages()) {
                UNSCOPED_INFO(msg.text);
            }
            FAIL();
        }
    };

    // Parse
    {
        Parser parser("test", source, result.strings(), diag);
        check_diag();

        auto node_result = parser.parse_toplevel_item({});
        REQUIRE(node_result.has_node());

        auto node = node_result.take_node();
        REQUIRE(isa<FuncDecl>(node));

        auto func = must_cast<FuncDecl>(node);

        NodePtr<Root> root = make_ref<Root>();
        NodePtr<File> file = make_ref<File>();
        NodePtr<NodeList> items = make_ref<NodeList>();

        root->file(file);
        file->items(items);
        items->append(func);

        result.root_ = root;
        result.func_ = func;
    }

    // Analyze
    {
        Analyzer ana(result.symbols(), result.strings(), diag);
        result.root_ = ana.analyze(result.root_);
        check_diag();
    }

    return result;
}

FunctionLocations compute_locations(FunctionResult& result) {
    return FunctionLocations::compute(
        result.get(), nullptr, result.symbols(), result.strings());
}

template<typename Predicate>
static NodePtr<> find_node_impl(Node* node, Predicate&& pred) {
    if (pred(node))
        return ref(node);

    NodePtr<> result = nullptr;
    traverse_children(node, [&](Node* child) {
        if (result || !child)
            return;

        result = find_node_impl(child, pred);
    });
    return result;
}

static NodePtr<Decl> find_decl(FunctionResult& func, std::string_view name) {
    auto interned = func.strings().find(name);
    REQUIRE(interned); // name does not exist as a string

    auto decl = find_node_impl(func.get(), [&](Node* node) {
        if (auto d = try_cast<Decl>(node)) {
            if (d->name() == *interned)
                return true;
        }
        return false;
    });
    REQUIRE(decl); // decl not found
    return must_cast<Decl>(decl);
}

static NodePtr<WhileStmt> find_while_loop(FunctionResult& func) {
    auto loop = find_node_impl(
        func.get(), [&](Node* node) { return isa<WhileStmt>(node); });

    REQUIRE(loop);
    return must_cast<WhileStmt>(loop);
}

static auto require_loc(FunctionLocations& locations, const NodePtr<Decl>& decl,
    VarLocationType expected_type) {
    auto loc = locations.get_location(decl->declared_symbol());
    REQUIRE(loc);
    REQUIRE(loc->type == expected_type);
    return *loc;
};

static auto
require_param(FunctionLocations& locations, const NodePtr<Decl>& decl) {
    return require_loc(locations, decl, VarLocationType::Param).param;
};

static auto
require_local(FunctionLocations& locations, const NodePtr<Decl>& decl) {
    return require_loc(locations, decl, VarLocationType::Local).local;
};

static auto
require_context(FunctionLocations& locations, const NodePtr<Decl>& decl) {
    return require_loc(locations, decl, VarLocationType::Context).context;
};

TEST_CASE("Normal variable locations should be computed correctly",
    "[variable-locations]") {
    static constexpr std::string_view source =
        "func test(a, b) {\n"
        "  var i = 0;\n"
        "  var j = 1;\n"
        "  if (a) {\n"
        "    var k = 2;\n"
        "  } else {\n"
        "    var l = 3;\n"
        "  }\n"
        "}";

    auto func = parse_function(source);
    auto locations = compute_locations(func);

    REQUIRE(locations.params() == 2);
    REQUIRE(locations.locals() == 3); // k and l share slot

    {
        auto param_a = find_decl(func, "a");
        auto param_b = find_decl(func, "b");

        auto check_param = [&](const NodePtr<Decl>& decl,
                               u32 expected_param_index) {
            u32 param_index = require_param(locations, decl).index;
            REQUIRE(param_index == expected_param_index);
        };

        check_param(param_a, 0);
        check_param(param_b, 1);
    }

    {
        auto local_i = find_decl(func, "i");
        auto local_j = find_decl(func, "j");
        auto local_k = find_decl(func, "k");
        auto local_l = find_decl(func, "l");

        std::unordered_set<u32> used_locals{};
        const std::unordered_set<u32> expected_locals{0, 1, 2};

        auto check_local = [&](const NodePtr<Decl>& decl) {
            u32 local_index = require_local(locations, decl).index;

            // local index must not be in used
            REQUIRE(used_locals.count(local_index) == 0);

            used_locals.insert(local_index);
            return local_index;
        };

        check_local(local_i);
        check_local(local_j);

        const u32 k_index = check_local(local_k);
        used_locals.erase(k_index); // can reuse
        check_local(local_l);

        REQUIRE(used_locals == expected_locals);
    }
}

TEST_CASE(
    "Closure variables should be computed correctly", "[variable-locations]") {
    static constexpr std::string_view source =
        "func test(a, b) {\n"
        "  var i = 0;\n"
        "  var j = 1;\n"
        "  func() {\n"
        "    return b + j;\n"
        "  }();\n"
        "}";

    auto func = parse_function(source);
    auto locations = compute_locations(func);

    {
        auto param_a = find_decl(func, "a");
        auto param_b = find_decl(func, "b");

        u32 index_a = require_param(locations, param_a).index;
        REQUIRE(index_a == 0);

        auto context_b = require_context(locations, param_b);
        REQUIRE(context_b.ctx);
        REQUIRE(context_b.index == 0);
        REQUIRE(locations.get_closure_context(func.get()->param_scope())
                == context_b.ctx);
        REQUIRE(context_b.ctx->local_index == 0);
    }

    {
        auto local_i = find_decl(func, "i");
        auto local_j = find_decl(func, "j");

        u32 index_i = require_local(locations, local_i).index;
        REQUIRE(index_i == 1); // Context is local 0

        auto context_j = require_context(locations, local_j);
        REQUIRE(context_j.ctx);
        REQUIRE(context_j.index == 1);
        REQUIRE(locations.get_closure_context(func.get()->param_scope())
                == context_j.ctx);
    }
}

TEST_CASE(
    "Captured variables in loops "
    "should get a new closure context",
    "[variable-locations]") {

    static constexpr std::string_view source =
        "func test() {\n"
        "  var i = 0;\n"
        "  while (1) {\n"
        "    var j = 1;\n"
        "    func() {\n"
        "       return i + j;\n"
        "    }();\n"
        "  }\n"
        "}";

    auto func = parse_function(source);
    auto locations = compute_locations(func);

    auto local_i = find_decl(func, "i");
    auto local_j = find_decl(func, "j");
    auto loop = find_while_loop(func);

    auto context_loc_i = require_context(locations, local_i);
    REQUIRE(context_loc_i.ctx);
    REQUIRE(context_loc_i.ctx
            == locations.get_closure_context(func.get()->param_scope()));
    REQUIRE(context_loc_i.index == 0);

    auto context_loc_j = require_context(locations, local_j);
    REQUIRE(context_loc_j.ctx);
    REQUIRE(
        context_loc_j.ctx == locations.get_closure_context(loop->body_scope()));
    REQUIRE(context_loc_j.index == 0);

    REQUIRE(locations.params() == 0);
    REQUIRE(locations.locals() == 2); // 2 contexts
}
