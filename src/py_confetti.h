/*
 * Python Confetti Bindings
 * Copyright (c) 2025 Henry G. Stratmann III
 * Copyright (c) 2025 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License
 * For full terms see the included LICENSE file.
 */

#pragma once

// The Python header uses a #pramga directive on Windows to automatically
// link with the debug version of Python. The problem is the standard Python
// distribution only includes release libraries so, as a workaround, we'll
// trick Python into thinking we're running in release even if we're building
// in debug mode.
#if defined(WIN32) && defined(_DEBUG)
  #undef _DEBUG
  #include <python.h>
  #define _DEBUG
#else
  #include <Python.h>
#endif

extern PyTypeObject DirectiveType;
extern PyTypeObject ArgumentType;
extern PyTypeObject IllegalSyntaxErrorType;

extern PyObject *IllegalSyntaxErrorObject;

typedef struct
{
    PyObject_HEAD
    PyObject *arguments;
    PyObject *subdirectives;
} PyDirective;

typedef struct
{
    PyObject_HEAD
    PyObject *value;
    PyObject *is_expression;
} PyArgument;

typedef struct
{
    PyBaseExceptionObject base;
    PyObject *message;
    int line;
    int column;
} IllegalSyntaxError;

PyObject *confetti_parse(PyObject *self, PyObject *args, PyObject *kwargs);

PyMODINIT_FUNC PyInit_cconfetti(void);
