# Examples

This folder contains some simple example programs. Examples should be executed using `tiro_run`.
The `tiro_run` command in the command line examples below should point to the binary built by the project (e.g. in the build directory).

You can pass the flags `--dump-ast` or `--disassemble` to `tiro_run` to display the abstract syntax tree or the bytecode instructions.

## Hello

Prints "Hello World".

    $ /workspace/build/bin/tiro_run hello/hello.tro --invoke main
    Hello World!
    Function returned null of type Null.

## Fizzbuzz

Prints the results of the well known [Fizz buzz challenge](https://en.wikipedia.org/wiki/Fizz_buzz).

    $ /workspace/build/bin/tiro_run fizzbuzz/fizzbuzz.tro --invoke fizzbuzz
    1
    2
    Fizz
    4
    Buzz
    ...

## Fibonacci

Computes fibonacci numbers using three different approaches:

    $ /workspace/build/bin/tiro_run fibonacci/fibonacci.tro --invoke start_fibonacci_recursive
    $ /workspace/build/bin/tiro_run fibonacci/fibonacci.tro --invoke start_fibonacci_iterative
    $ /workspace/build/bin/tiro_run fibonacci/fibonacci.tro --invoke start_fibonacci_memo
