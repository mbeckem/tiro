#include <tiropp/api.hpp>

#include <iostream>

namespace {

extern const char* EXAMPLE_SOURCE;

} // namespace

tiro::compiled_module compile(std::string_view source) {
    tiro::compiler_settings settings;
    settings.message_callback = [&](auto& message) {
        std::cout << message.line << ":" << message.column << ": " << message.text << "\n";
    };

    tiro::compiler compiler("perf", settings);
    compiler.add_file("main", EXAMPLE_SOURCE);
    compiler.run();

    return compiler.take_module();
}

int __attribute__((optimize("O0"))) main() {
    auto mod = compile(EXAMPLE_SOURCE);
    printf("%p\n", mod.raw_module());
    return 0;
}

namespace {

const char* EXAMPLE_SOURCE = R"(
    export func greet(name) {
        return "Hello ${name}!";
    }

    export func with_deep_expressions(a, b, c) {
        if (a) {
            return {
                {
                    {
                        {
                            {
                                {
                                    {
                                        b + {
                                            1 * c() ** 3;
                                        };
                                    };
                                };
                            };
                        };
                    };
                };
            };
        }

        if (b) {
            var x = 3;
            while (c) {
                defer a();

                if (b(x)) {
                    break;
                }
            }

            return a(1) + a(2) + (a(3) + a(x + a(x)));
        }

        return a(b(c)) / b(c(a));
    }

    export func with_deep_expressions2(a, b, c) {
        if (a) {
            return {
                {
                    {
                        {
                            {
                                {
                                    {
                                        b + {
                                            1 * c() ** 3;
                                        };
                                    };
                                };
                            };
                        };
                    };
                };
            };
        }

        if (b) {
            var x = 3;
            while (c) {
                defer a();

                if (b(x)) {
                    break;
                }
            }

            return a(1) + a(2) + (a(3) + a(x + a(x)));
        }

        return a(b(c)) / b(c(a));
    }

    export func with_deep_expressions3(a, b, c) {
        if (a) {
            return {
                {
                    {
                        {
                            {
                                {
                                    {
                                        b + {
                                            1 * c() ** 3;
                                        };
                                    };
                                };
                            };
                        };
                    };
                };
            };
        }

        if (b) {
            var x = 3;
            while (c) {
                defer a();

                if (b(x)) {
                    break;
                }
            }

            return a(1) + a(2) + (a(3) + a(x + a(x)));
        }

        return a(b(c)) / b(c(a));
    }

    export func with_deep_expressions4(a, b, c) {
        if (a) {
            return {
                {
                    {
                        {
                            {
                                {
                                    {
                                        b + {
                                            1 * c() ** 3;
                                        };
                                    };
                                };
                            };
                        };
                    };
                };
            };
        }

        if (b) {
            var x = 3;
            while (c) {
                defer a();

                if (b(x)) {
                    break;
                }
            }

            return a(1) + a(2) + (a(3) + a(x + a(x)));
        }

        return a(b(c)) / b(c(a));
    }

    export func with_deep_expressions5(a, b, c) {
        if (a) {
            return {
                {
                    {
                        {
                            {
                                {
                                    {
                                        b + {
                                            1 * c() ** 3;
                                        };
                                    };
                                };
                            };
                        };
                    };
                };
            };
        }

        if (b) {
            var x = 3;
            while (c) {
                defer a();

                if (b(x)) {
                    break;
                }
            }

            return a(1) + a(2) + (a(3) + a(x + a(x)));
        }

        return a(b(c)) / b(c(a));
    }

    export func with_deep_expressions6(a, b, c) {
        if (a) {
            return {
                {
                    {
                        {
                            {
                                {
                                    {
                                        b + {
                                            1 * c() ** 3;
                                        };
                                    };
                                };
                            };
                        };
                    };
                };
            };
        }

        if (b) {
            var x = 3;
            while (c) {
                defer a();

                if (b(x)) {
                    break;
                }
            }

            return a(1) + a(2) + (a(3) + a(x + a(x)));
        }

        return a(b(c)) / b(c(a));
    }

    export func with_deep_expressions7(a, b, c) {
        if (a) {
            return {
                {
                    {
                        {
                            {
                                {
                                    {
                                        b + {
                                            1 * c() ** 3;
                                        };
                                    };
                                };
                            };
                        };
                    };
                };
            };
        }

        if (b) {
            var x = 3;
            while (c) {
                defer a();

                if (b(x)) {
                    break;
                }
            }

            return a(1) + a(2) + (a(3) + a(x + a(x)));
        }

        return a(b(c)) / b(c(a));
    }

    export func with_deep_expressions8(a, b, c) {
        if (a) {
            return {
                {
                    {
                        {
                            {
                                {
                                    {
                                        b + {
                                            1 * c() ** 3;
                                        };
                                    };
                                };
                            };
                        };
                    };
                };
            };
        }

        if (b) {
            var x = 3;
            while (c) {
                defer a();

                if (b(x)) {
                    break;
                }
            }

            return a(1) + a(2) + (a(3) + a(x + a(x)));
        }

        return a(b(c)) / b(c(a));
    }

    export func with_deep_expressions9(a, b, c) {
        if (a) {
            return {
                {
                    {
                        {
                            {
                                {
                                    {
                                        b + {
                                            1 * c() ** 3;
                                        };
                                    };
                                };
                            };
                        };
                    };
                };
            };
        }

        if (b) {
            var x = 3;
            while (c) {
                defer a();

                if (b(x)) {
                    break;
                }
            }

            return a(1) + a(2) + (a(3) + a(x + a(x)));
        }

        return a(b(c)) / b(c(a));
    }

    export func with_deep_expressions10(a, b, c) {
        if (a) {
            return {
                {
                    {
                        {
                            {
                                {
                                    {
                                        b + {
                                            1 * c() ** 3;
                                        };
                                    };
                                };
                            };
                        };
                    };
                };
            };
        }

        if (b) {
            var x = 3;
            while (c) {
                defer a();

                if (b(x)) {
                    break;
                }
            }

            return a(1) + a(2) + (a(3) + a(x + a(x)));
        }

        return a(b(c)) / b(c(a));
    }

    export func with_deep_expressions11(a, b, c) {
        if (a) {
            return {
                {
                    {
                        {
                            {
                                {
                                    {
                                        b + {
                                            1 * c() ** 3;
                                        };
                                    };
                                };
                            };
                        };
                    };
                };
            };
        }

        if (b) {
            var x = 3;
            while (c) {
                defer a();

                if (b(x)) {
                    break;
                }
            }

            return a(1) + a(2) + (a(3) + a(x + a(x)));
        }

        return a(b(c)) / b(c(a));
    }

    export func with_deep_expressions12(a, b, c) {
        if (a) {
            return {
                {
                    {
                        {
                            {
                                {
                                    {
                                        b + {
                                            1 * c() ** 3;
                                        };
                                    };
                                };
                            };
                        };
                    };
                };
            };
        }

        if (b) {
            var x = 3;
            while (c) {
                defer a();

                if (b(x)) {
                    break;
                }
            }

            return a(1) + a(2) + (a(3) + a(x + a(x)));
        }

        return a(b(c)) / b(c(a));
    }

    export func with_deep_expressions13(a, b, c) {
        if (a) {
            return {
                {
                    {
                        {
                            {
                                {
                                    {
                                        b + {
                                            1 * c() ** 3;
                                        };
                                    };
                                };
                            };
                        };
                    };
                };
            };
        }

        if (b) {
            var x = 3;
            while (c) {
                defer a();

                if (b(x)) {
                    break;
                }
            }

            return a(1) + a(2) + (a(3) + a(x + a(x)));
        }

        return a(b(c)) / b(c(a));
    }

    export func with_deep_expressions14(a, b, c) {
        if (a) {
            return {
                {
                    {
                        {
                            {
                                {
                                    {
                                        b + {
                                            1 * c() ** 3;
                                        };
                                    };
                                };
                            };
                        };
                    };
                };
            };
        }

        if (b) {
            var x = 3;
            while (c) {
                defer a();

                if (b(x)) {
                    break;
                }
            }

            return a(1) + a(2) + (a(3) + a(x + a(x)));
        }

        return a(b(c)) / b(c(a));
    }

    export func with_deep_expressions15(a, b, c) {
        if (a) {
            return {
                {
                    {
                        {
                            {
                                {
                                    {
                                        b + {
                                            1 * c() ** 3;
                                        };
                                    };
                                };
                            };
                        };
                    };
                };
            };
        }

        if (b) {
            var x = 3;
            while (c) {
                defer a();

                if (b(x)) {
                    break;
                }
            }

            return a(1) + a(2) + (a(3) + a(x + a(x)));
        }

        return a(b(c)) / b(c(a));
    }

    export func with_deep_expressions16(a, b, c) {
        if (a) {
            return {
                {
                    {
                        {
                            {
                                {
                                    {
                                        b + {
                                            1 * c() ** 3;
                                        };
                                    };
                                };
                            };
                        };
                    };
                };
            };
        }

        if (b) {
            var x = 3;
            while (c) {
                defer a();

                if (b(x)) {
                    break;
                }
            }

            return a(1) + a(2) + (a(3) + a(x + a(x)));
        }

        return a(b(c)) / b(c(a));
    }
)";

} // namespace
