# Confetti: a configuration language and parser library
# Copyright (c) 2025 Confetti Contributors
#
# This file is part of Confetti, distributed under the MIT License
# For full terms see the included LICENSE file.

from typing import Union, List, Set

def parse(source: Union[str,bytes], c_style_comments: bool = False, expression_arguments: bool = False, punctuator_arguments: Set[str] = set()) -> List[Directive]: ...

class Directive:
    arguments: List[Argument]
    subdirectives: List[Directive]

class Argument:
    value: str
    expression: bool

class IllegalSyntaxError(Exception):
    message: str
    line: int
    column: int
