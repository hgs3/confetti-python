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

static int directive_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "cannot instantiate abstract class");
    return -1;
}

static void directive_dealloc(PyObject *self)
{
    PyDirective *dir = (PyDirective * )self;
    Py_XDECREF(dir->arguments);
    Py_XDECREF(dir->subdirectives);
    PyObject_Free(dir);
}

static PyMemberDef directive_members[] = {
    {"arguments", Py_T_OBJECT_EX, offsetof(PyDirective, arguments), 0, "Arguments of this directive"},
    {"subdirectives", Py_T_OBJECT_EX, offsetof(PyDirective, subdirectives), 0, "Subdirectives of this directive"},
    {NULL}
};

PyTypeObject DirectiveType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "cconfetti.Directive",
    .tp_basicsize = sizeof(PyDirective),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "Directive",
    .tp_members = directive_members,
    .tp_init = directive_init,
    .tp_new = PyType_GenericNew,
    .tp_dealloc = directive_dealloc,
};
