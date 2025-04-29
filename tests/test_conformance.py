#!/usr/bin/env python3

# Confetti: a configuration language and parser library
# Copyright (c) 2025 Henry G. Stratmann III
# Copyright (c) 2025 Confetti Contributors
#
# This file is part of Confetti, distributed under the MIT License.
# For full terms see the included LICENSE file.

import pytest
import cconfetti
import os
from typing import List
from pathlib import Path

# Collect all conformance test files.
DATA_DIR = Path(__file__).parent.parent / "confetti" / "tests" / "conformance"
EXTENSION = ".conf"
FILES = list(DATA_DIR.glob(f"*{EXTENSION}"))

def print_indent(output: List[str], depth: int) -> None:
    for i in range(depth):
        output.append("    ")

def print_dir(output: List[str], dir: cconfetti.Directive, depth: int) -> None:
    print_indent(output, depth)
    # The directive's arguments.
    for index,arg in enumerate(dir.arguments):
        output.append("<" + arg.value + ">")
        if index < len(dir.arguments) -1 :
            output.append(" ")
    # The directives subdirectives.
    if len(dir.subdirectives) > 0:
        output.append(" [\n")
        for subdir in dir.subdirectives:
            print_dir(output, subdir, depth + 1)
        print_indent(output, depth)
        output.append("]\n")
    else:
        output.append("\n")

# Verify the Python implementation against the official conformance test suite.
@pytest.mark.parametrize("file", FILES)
def test_conformance(file: Path) -> None:
    dir_name = file.parent
    raw_name = file.name.split('.')[0]
    pass_file = os.path.join(dir_name, raw_name + ".pass")
    fail_file = os.path.join(dir_name, raw_name + ".fail")

    c_style_comments = os.path.exists(os.path.join(dir_name, raw_name + ".ext_c_style_comments"))
    expression_arguments = os.path.exists(os.path.join(dir_name, raw_name + ".ext_expression_arguments"))

    punctuator_arguments = set()
    expr_args = os.path.join(dir_name, raw_name + ".ext_punctuator_arguments")
    if os.path.exists(expr_args):
        for entry in open(expr_args, "r", encoding="utf-8").readlines():
            punctuator_arguments.add(entry.strip())

    with open(file, "rb") as f:
        input = f.read()

        if os.path.exists(pass_file):
            expected_output = open(pass_file, "rb").read()
        else:
            expected_output = open(fail_file, "rb").read()

        try:
            directives = cconfetti.parse(input, c_style_comments=c_style_comments, expression_arguments=expression_arguments, punctuator_arguments=punctuator_arguments)

            output: List[str] = []
            for dir in directives:
                print_dir(output, dir, 0)
            actual_output = "".join(output)
            assert actual_output == expected_output.decode("utf-8")

        except cconfetti.IllegalSyntaxError as e:
            message = "error: " + e.message + "\n"
            assert message.encode("utf-8") == expected_output
        except UnicodeDecodeError as e:
            message = "error: " + e.reason + "\n"
            assert message.encode("utf-8") == expected_output

# Verify the Confetti parse interface accepts strings, not just bytes.
def test_parsing_a_string() -> None:
    dirs = cconfetti.parse("foo bar")
    assert len(dirs) == 1
    assert len(dirs[0].arguments) == 2
    assert dirs[0].arguments[0].value == "foo"
    assert dirs[0].arguments[1].value == "bar"

# Verify the Confetti parse interface accepts bytes, not just strings.
def test_parsing_bytes() -> None:
    dirs = cconfetti.parse(b"foo bar")
    assert len(dirs) == 1
    assert len(dirs[0].arguments) == 2
    assert dirs[0].arguments[0].value == "foo"
    assert dirs[0].arguments[1].value == "bar"

# Verify the C-style comment extension can be enabled, independent of any other extension.
def test_c_style_comment_extension() -> None:
    dirs = cconfetti.parse("foo // bar", c_style_comments=True)
    assert len(dirs) == 1
    assert len(dirs[0].arguments) == 1
    assert dirs[0].arguments[0].value == "foo"

# Verify the expression arguments extension can be enabled, independent of any other extension.
def test_expession_arguments_extension() -> None:
    dirs = cconfetti.parse("if (x > 1)", expression_arguments=True)
    assert len(dirs) == 1
    assert len(dirs[0].arguments) == 2
    assert dirs[0].arguments[0].value == "if"
    assert dirs[0].arguments[1].value == "x > 1"

# Verify the punctuator argument extension can be enabled, independent of any other extension.
def test_punctuator_arguments_extension() -> None:
    dirs = cconfetti.parse("x:=y", punctuator_arguments=set([":="]))
    assert len(dirs) == 1
    assert len(dirs[0].arguments) == 3
    assert dirs[0].arguments[0].value == "x"
    assert dirs[0].arguments[1].value == ":="
    assert dirs[0].arguments[2].value == "y"

# Verify an empty punctuator argument set doesn't cause havoc.
def test_empty_punctuator_arguments() -> None:
    dirs = cconfetti.parse("a b c", punctuator_arguments=set())
    assert len(dirs) == 1
    assert len(dirs[0].arguments) == 3
    assert dirs[0].arguments[0].value == "a"
    assert dirs[0].arguments[1].value == "b"
    assert dirs[0].arguments[2].value == "c"

# Verify directives cannot be directly instantiated.
def test_directive_instantiation() -> None:
    with pytest.raises(NotImplementedError) as excinfo:
        cconfetti.Directive()
    assert str(excinfo.value) == "cannot instantiate abstract class"

# Verify arguments cannot be directly instantiated.
def test_argument_instantiation() -> None:
    with pytest.raises(NotImplementedError) as excinfo:
        cconfetti.Argument()
    assert str(excinfo.value) == "cannot instantiate abstract class"

# Verify arguments cannot be directly instantiated.
def test_max_nesting_depth() -> None:
    # Construct a deeply tested Confetti structure.
    source = ""
    for i in range(500):
        source += f"{i} {{ "
    # Try parsing it and verify the max depth is caught.
    with pytest.raises(OverflowError) as excinfo:
        cconfetti.parse(source)
    assert str(excinfo.value) == "maximum nesting depth exceeded"
