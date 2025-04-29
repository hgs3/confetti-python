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

PyObject *IllegalSyntaxErrorObject = NULL;

static int IllegalSyntaxError_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"message", "line", "column", NULL};
    PyObject *message = NULL;
    int line = 0;
    int column = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|Uii", kwlist, &message, &line, &column))
    {
        return -1;
    }

    // Call base exception init.
    PyTypeObject *base = (PyTypeObject *)PyExc_Exception;
    if (base->tp_init(self, args, kwds) < 0)
    {
        return -1;
    }

    IllegalSyntaxError *error = (IllegalSyntaxError *)self;
    Py_XINCREF(message);
    error->message = message;
    error->line = line;
    error->column = column;
    return 0;
}
static void IllegalSyntaxError_dealloc(PyObject *self)
{
    IllegalSyntaxError *e = (IllegalSyntaxError *)self;
    Py_XDECREF(e->message);
    Py_TYPE(self)->tp_free(self);
}

static PyMemberDef IllegalSyntaxError_members[] = {
    {"message", Py_T_OBJECT_EX, offsetof(IllegalSyntaxError, message), 0, "Detailed message"},
    {"line", Py_T_INT, offsetof(IllegalSyntaxError, line), 0, "Line"},
    {"column", Py_T_INT, offsetof(IllegalSyntaxError, column), 0, "Column"},
    {NULL}
};

PyTypeObject IllegalSyntaxErrorType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "cconfetti.IllegalSyntaxError",
    .tp_basicsize = sizeof(IllegalSyntaxError),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc)IllegalSyntaxError_init,
    .tp_dealloc = (destructor)IllegalSyntaxError_dealloc,
    .tp_members = IllegalSyntaxError_members,
    .tp_doc = "Custom exception with detail and code",
};

