#include <tiropp/api.hpp>

// Forward declarations
static tiro::compiled_module create_example_module();

// This example demonstrates how to embed tiro into a c++ application.
// A simple call from c++ to tiro is done, and the return value is inspected.
int main() {
    // Obtain a compiled module. Compiled modules are independent of any
    // concrete virtual machine instances and may be used multiple times.
    tiro::compiled_module module = create_example_module();

    // Create a virtual machine with default settings and register the module.
    // We also load tiro's standard library, so it could be used from the example module.
    tiro::vm vm;
    vm.load_std();
    vm.load(module);

    // Retrieve the greet() function from the example module.
    tiro::function greet = tiro::get_export(vm, "example", "greet").as<tiro::function>();

    // Invoke the greet function with a single argument. Note that all function calls
    // to tiro are asynchronous by default, the callback will be executed when the function
    // call completes (which is, in our case, almost immediately).
    tiro::tuple arguments = tiro::make_tuple(vm, 1);
    arguments.set(0, tiro::make_string(vm, "World"));
    tiro::run_async(vm, greet, arguments, [](tiro::vm&, const tiro::coroutine& coro) {
        // Functions return a result instance, which might contain an error value if
        // the coroutine panicked.
        tiro::result result = coro.result();
        tiro::string greeting = result.value().as<tiro::string>();
        std::printf("Function call returned: %s\n", greeting.value().c_str());
    });

    // All user code in tiro is executed by a `vm.run_*` method, so the snipped above actually only
    // scheduled execution, it did not begin the actual function call.
    // The following is a primitive example of a program's main loop, which tiro is suited for by design.
    // In a real application, the loop would probably also handle timers, networking or user input.
    while (vm.has_ready()) {
        vm.run_ready();
    }
    return 0;
}

static const char* EXAMPLE_SOURCE = R"(
    export func greet(name) {
        return "Hello ${name}!";
    }
)";

tiro::compiled_module create_example_module() {
    // Create a compiler instance for the new module.
    // For advanced uses, provide custom settings to the compiler to override
    // default error logging (which goes to stdout/stderr by default)
    // or to obtain diagnostic output like the AST, IR or tiro bytecode.
    tiro::compiler compiler;

    // Add files to the compiler.
    // Currently, the filename given here is used for the name of the
    // final compiled module, and the number of files is limited to 1.
    // This is about to change.
    compiler.add_file("example", EXAMPLE_SOURCE);

    // Actually perform the compilation. This will throw on error.
    compiler.run();

    // After successful compilation, take_module() will move the valid
    // module out of the compiler.
    return compiler.take_module();
}
