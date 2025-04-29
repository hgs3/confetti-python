/*
 * Python Confetti Bindings
 * Copyright (c) 2025 Henry G. Stratmann III
 * Copyright (c) 2025 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License
 * For full terms see the included LICENSE file.
 */

#include "py_confetti.h"

static PyMethodDef methods[] = {
    {"parse", (PyCFunction)confetti_parse, METH_VARARGS | METH_KEYWORDS, "Parse Confetti source text into a list of directives."},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "cconfetti",
    .m_doc = "Confetti configuration language",
    .m_size = -1,
    .m_methods = methods,
};

PyMODINIT_FUNC PyInit_cconfetti(void)
{
    PyObject *m;
    IllegalSyntaxErrorType.tp_base = (PyTypeObject *)PyExc_Exception;
    
    if ((PyType_Ready(&DirectiveType) < 0) ||
        (PyType_Ready(&IllegalSyntaxErrorType) < 0) ||
        (PyType_Ready(&ArgumentType) < 0))
    {
        return NULL;
    }
    
    m = PyModule_Create(&module);
    if (m == NULL)
    {
        return NULL;
    }

    Py_INCREF(&IllegalSyntaxErrorType);
    Py_INCREF(&DirectiveType);
    Py_INCREF(&ArgumentType);

    if ((PyModule_AddObject(m, "IllegalSyntaxError", (PyObject *)&IllegalSyntaxErrorType) < 0) ||
        (PyModule_AddObject(m, "Directive", (PyObject *)&DirectiveType) < 0) ||
        (PyModule_AddObject(m, "Argument", (PyObject *)&ArgumentType) < 0))
    {
        Py_DECREF(&IllegalSyntaxErrorType);
        Py_DECREF(&DirectiveType);
        Py_DECREF(&ArgumentType);
        Py_DECREF(m);
        return NULL;
    }

    IllegalSyntaxErrorObject = (PyObject *)&IllegalSyntaxErrorType;
    return m;
}
