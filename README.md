# Python Bindings for Confetti

This repository contains Python bindings for [Confetti](https://github.com/hgs3/confetti).
For a pure Python implementation of Confetti, see [PyConfetti](https://github.com/bobuk/pyconfetti/).

The bindings correspond one-to-one with the Confetti C API whereas the pure Python implementation offers a higher-level interface that deserializes Confetti directly into Python data structures.
The latter is recommended for most use cases.

[![Build Status](https://github.com/hgs3/confetti-python/actions/workflows/build.yml/badge.svg)](https://github.com/hgs3/confetti-python/actions/workflows/build.yml)

## Installation

Install the bindings with

```
$ pip install cconfetti
```

The bindings require you to have a C compiler accessible to Python.

## Complete Example

The following complete Python program reads Confetti source text from standard input and pretty prints it to standard output.

```py
import cconfetti

def print_indent(depth) -> None:
    for i in range(depth):
        print("    ", end="")

def print_directives(directive, depth) -> None:
    print_indent(depth)

    # The directive arguments.
    for index,arg in enumerate(directive.arguments):
        print(arg.value, end=" ")

    # The directive's subdirectives.
    if len(directive.subdirectives) > 0:
        print("{")
        for subdir in directive.subdirectives:
            print_directives(subdir, depth + 1)
        print_indent(depth)
        print("}")
    else:
        print("")

source = input("Enter Confetti: ")
directives = cconfetti.parse(source)
for dir in directives:
    print_directives(dir, 0)
```

## License

MIT License.
