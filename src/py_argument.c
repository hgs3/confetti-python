/*
 * Python Confetti Bindings
 * Copyright (c) 2025 Henry G. Stratmann III
 * Copyright (c) 2025 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License
 * For full terms see the included LICENSE file.
 */

#include "py_confetti.h"
#include <stddef.h>

static int Arguments_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "cannot instantiate abstract class");
    return -1;
}

static void Argument_dealloc(PyObject *self)
{
    PyArgument *arg = (PyArgument * )self;
    Py_XDECREF(arg->value);
    Py_XDECREF(arg->is_expression);
    PyObject_Free(arg);
}

static PyMemberDef Argument_members[] = {
    {"value", Py_T_OBJECT_EX, offsetof(PyArgument, value), 0, "Value of an argument"},
    {"is_expression", Py_T_OBJECT_EX, offsetof(PyArgument, is_expression), 0, "Checks if this argument is an expression argument (requires the expressiona argument extension)"},
    {NULL}
};

// Define the type (class) itself
PyTypeObject ArgumentType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "cconfetti.Argument",
    .tp_basicsize = sizeof(PyArgument),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "Argument",
    .tp_members = Argument_members,
    .tp_init = Arguments_init,
    .tp_new = PyType_GenericNew,
    .tp_dealloc = Argument_dealloc,
};
