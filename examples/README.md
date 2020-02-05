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

## Server

Runs a very simple tcp server on port 12345. For each incoming connection, the server will read until the first empty line (i.e. `\n` or `\r\n`)
and echo the incoming message as a HTTP/1.0 response.

Start the server:

    $ /workspace/build/bin/tiro_run server/server.tro --invoke main
    Listening on port 12345

Use curl (or point your browser at `http://localhost:12345`):

    $ curl http://localhost:12345/some/invalid/path
    Hello World

    Your address is: 127.0.0.1:45506

    The original request was:

    GET /some/invalid/path HTTP/1.1
    Host: localhost:12345
    User-Agent: curl/7.64.0
    Accept: */*

