/*
 * Python Confetti Bindings
 * Copyright (c) 2025 Henry G. Stratmann III
 * Copyright (c) 2025 Confetti Contributors
 *
 * This file is part of Confetti, distributed under the MIT License
 * For full terms see the included LICENSE file.
 */

#include "py_confetti.h"
#include "confetti.h"

struct PunctuatorArguments
{
    char **argv;
    int argc;
    bool ok;
};

struct Location
{
    Py_ssize_t line;
    Py_ssize_t column;
    bool ok;
};

static struct Location utf8_to_ucs4(const char *source, Py_ssize_t utf8_index)
{
    struct Location location = {
        .line = 1,
        .column = 1,
    };

    PyObject *ucs4_string = PyUnicode_FromString(source);
    if (ucs4_string == NULL)
    {
        PyErr_SetString(PyExc_UnicodeDecodeError, "failed to create a Python string from UTF-8 source code");
        return location;
    }

    // Get the length of the Unicode string (in code points).
    Py_ssize_t utf8_offset = 0;
    const Py_ssize_t ucs4_string_length = PyUnicode_GET_LENGTH(ucs4_string);
    for (Py_ssize_t ucs4_index = 0; ucs4_index < ucs4_string_length; ucs4_index++)
    {
        // Calculate the number of UTF-8 bytes used by the current code point.
        const Py_UCS4 codepoint = PyUnicode_READ_CHAR(ucs4_string, ucs4_index);
        Py_ssize_t utf8_bytes;
        if (codepoint <= 0x7F)
        {
            utf8_bytes = 1;
        }
        else if (codepoint <= 0x7FF)
        {
            utf8_bytes = 2;
        }
        else if (codepoint <= 0xFFFF)
        {
            utf8_bytes = 3;
        }
        else
        {
            utf8_bytes = 4;
        }

        // Check if we've reached or went pass the desired UTF-8 index.
        if (utf8_offset >= utf8_index)
        {
            break;
        }
        utf8_offset += utf8_bytes;

        // Update line/column numbers.
        if (codepoint == '\r')
        {
            const Py_UCS4 next_codepoint = PyUnicode_READ_CHAR(ucs4_string, ucs4_index + 1);
            if (next_codepoint == '\n')
            {
                ucs4_index += 1; // Consume the '\r' here, the for-loop will consume the '\n'.
            }
            location.line += 1;
            location.column = 1;
        }
        else
        {
            switch (codepoint)
            {
            case 0x000A: // Line feed
            case 0x000B: // Vertical tab
            case 0x000C: // Form feed
            case 0x0085: // Next line
            case 0x2028: // Line separator
            case 0x2029: // Paragraph separator
                location.line += 1;
                location.column = 1;
                break;
            default:
                location.column += 1;
                break;
            }
        }
    }

    Py_DECREF(ucs4_string);
    location.ok = true;
    return location;
}

static void *allocator(void *user_data, void *ptr, size_t size)
{
    if (ptr == NULL)
    {
        return PyMem_Malloc(size);
    }
    else
    {
        PyMem_Free(ptr);
        return NULL;
    }
}

static PyObject *confetti_add_directive(const conf_directive *dir)
{
    PyDirective *py_dir = PyObject_New(PyDirective, &DirectiveType);
    if (py_dir == NULL)
    {
        return NULL;
    }
    py_dir->arguments = NULL;
    py_dir->subdirectives = NULL;

    const long arg_count = conf_get_argument_count(dir);
    py_dir->arguments = PyList_New(arg_count);
    if (py_dir->arguments == NULL)
    {
        goto failure;
    }

    const long subdir_count = conf_get_directive_count(dir);
    py_dir->subdirectives = PyList_New(subdir_count);
    if (py_dir->subdirectives == NULL)
    {
        goto failure;
    }

    for (long i = 0; i < arg_count; i++)
    {
        PyArgument *py_arg = PyObject_New(PyArgument, &ArgumentType);
        if (py_arg == NULL)
        {
            goto failure;
        }

        const conf_argument *arg = conf_get_argument(dir, i);
        py_arg->is_expression = arg->is_expression ? Py_True : Py_False;

        py_arg->value = PyUnicode_FromString(arg->value);
        if (py_arg->value == NULL)
        {
            Py_DECREF(py_arg);
            goto failure;
        }
        PyList_SetItem(py_dir->arguments, i, (PyObject *)py_arg);
    }

    for (long i = 0; i < subdir_count; i++)
    {
        const conf_directive *subdir = conf_get_directive(dir, i);
        PyObject *py_subdir = confetti_add_directive(subdir);
        if (py_subdir == NULL)
        {
            goto failure;
        }
        PyList_SetItem(py_dir->subdirectives, i, py_subdir);
    }

    return (PyObject *)py_dir;

failure:
    if (py_dir != NULL)
    {
        Py_XDECREF(py_dir->arguments);
        Py_XDECREF(py_dir->subdirectives);
        Py_DECREF(py_dir);
    }
    return NULL;
}

static char *check_source_argument(PyObject *py_source)
{
    char *source = NULL;
    if (PyUnicode_Check(py_source))
    {
        Py_ssize_t length = 0;
        const char *string = PyUnicode_AsUTF8AndSize(py_source, &length);
        if (string == NULL)
        {
            return NULL;
        }

        source = PyMem_Calloc(length + 1, sizeof(source[0]));
        if (source == NULL)
        {
            PyErr_SetString(PyExc_MemoryError, "cannot allocate buffer for source argument");
            return NULL;
        }
        memcpy(source, string, length);
    }
    else if (PyBytes_Check(py_source))
    {
        const Py_ssize_t length = PyBytes_GET_SIZE(py_source);
        source = PyMem_Calloc(length + 1, sizeof(source[0]));
        if (source == NULL)
        {
            PyErr_SetString(PyExc_MemoryError, "cannot allocate buffer for source argument");
            return NULL;
        }

        char *bytes = PyBytes_AS_STRING(py_source);
        memcpy(source, bytes, length);
    }
    else
    {
        PyErr_SetString(PyExc_TypeError, "Argument must be a string or bytes object");
        return NULL;
    }
    return source;
}

static void free_punctuator_arguments(struct PunctuatorArguments *punct)
{
    if (punct->argv != NULL)
    {
        for (int i = 0; i < punct->argc; i++)
        {
            PyMem_Free(punct->argv[i]);
            punct->argv[i] = NULL;
        }
        PyMem_Free(punct->argv);
        punct->argv = NULL;
        punct->argc = 0;
    }
}

static struct PunctuatorArguments check_punctuator_arguments(PyObject *punctuator_arguments)
{
    struct PunctuatorArguments punct = {0};

    if (punctuator_arguments != NULL && !Py_IsNone(punctuator_arguments))
    {
        if (!PySet_Check(punctuator_arguments))
        {
            PyErr_SetString(PyExc_TypeError, "Expected a set of strings");
            goto error;
        }

        const Py_ssize_t custom_length = PySet_Size(punctuator_arguments);
        punct.argv = PyMem_Calloc(custom_length + 1, sizeof(char **));
        if (punct.argv == NULL)
        {
            PyErr_SetString(PyExc_MemoryError, "failed to allocate a list for punctuator arguments");
            goto error;
        }

        // Create an iterator for the set
        PyObject *iterator = PyObject_GetIter(punctuator_arguments);
        if (iterator == NULL)
        {
            PyErr_SetString(PyExc_MemoryError, "failed to create a set iterator");
            goto error;
        }

        for (;;)
        {
            PyObject *item = PyIter_Next(iterator);
            if (item == NULL)
            {
                break;
            }

            if (!PyUnicode_Check(item))
            {
                PyErr_SetString(PyExc_TypeError, "Expected a set of strings");
                Py_DECREF(item);  // Don't forget to DECREF to avoid memory leaks
                Py_DECREF(iterator);
                goto error;
            }

            // Convert the item to a C string (using PyUnicode_AsUTF8)
            Py_ssize_t length = 0;
            const char *str = PyUnicode_AsUTF8AndSize(item, &length);
            if (str == NULL)
            {
                Py_DECREF(item);
                Py_DECREF(iterator);
                goto error;
            }

            // Copy the string to a null-termintaed buffer.
            char *copy = PyMem_Malloc(length + 1);
            if (copy == NULL)
            {
                Py_DECREF(item);
                Py_DECREF(iterator);
                goto error;
            }
            strcpy(copy, str);

            // Do something with the string (printing here for demonstration)
            punct.argv[punct.argc] = copy;
            punct.argc += 1;
            Py_DECREF(item);
        }
        punct.argv[punct.argc] = NULL;
        Py_DECREF(iterator);
    }

    punct.ok = true;
    return punct;

error:
    free_punctuator_arguments(&punct);
    punct.ok = false;
    return punct;
}

static void set_error(const conf_error *error, const char *source)
{
    if (error->code == CONF_BAD_SYNTAX)
    {
        const struct Location loc = utf8_to_ucs4(source, error->where);
        if (loc.ok)
        {
            PyObject *message = NULL;
            PyObject *line = NULL;
            PyObject *column = NULL;
            PyObject *arguments = NULL;

            message = PyUnicode_FromString(error->description);
            if (message == NULL)
            {
                goto cleanup;
            }

            line = PyLong_FromLong(loc.line);
            if (line == NULL)
            {
                goto cleanup;
            }

            column = PyLong_FromLong(loc.column);
            if (column == NULL)
            {
                goto cleanup;
            }

            arguments = Py_BuildValue("(OOO)", message, line, column);
            if (arguments == NULL)
            {
                goto cleanup;
            }

            PyObject *instance = PyObject_CallObject(IllegalSyntaxErrorObject, arguments);
            if (instance == NULL)
            {
                goto cleanup;
            }

            PyErr_SetObject(IllegalSyntaxErrorObject, instance);
            Py_DECREF(instance);
            return;
            
        cleanup:
            Py_XDECREF(arguments);
            Py_XDECREF(message);
            Py_XDECREF(line);
            Py_XDECREF(column);
            // Fall through to catch all error handling.
        }
    }
    else if (error->code == CONF_ILLEGAL_BYTE_SEQUENCE)
    {
        PyObject *err = PyUnicodeDecodeError_Create("utf-8", source, strlen(source), error->where, error->where + 1, error->description);
        PyErr_SetObject(PyExc_UnicodeDecodeError, err);
        Py_XDECREF(err);
        return;
    }
    else if (error->code == CONF_OUT_OF_MEMORY)
    {
        PyErr_SetString(PyExc_MemoryError, error->description);
        return;
    }
    else if (error->code == CONF_MAX_DEPTH_EXCEEDED)
    {
        PyErr_SetString(PyExc_OverflowError, error->description);
        return;
    }
    
    // All potential error scenarios should already be handled.
    PyErr_SetString(PyExc_Exception, "malfunction in the Confetti bindings");
}

PyObject *confetti_parse(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"source", "c_style_comments", "expression_arguments", "punctuator_arguments", NULL};
    PyObject *py_source = NULL;
    int c_style_comments = 0;
    int expression_arguments = 0;
    PyObject *punctuator_arguments = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|ppO", kwlist, &py_source, &c_style_comments, &expression_arguments, &punctuator_arguments))
    {
        return NULL;
    }

    char *source = NULL;
    struct PunctuatorArguments punct = {0};
    conf_unit *unit = NULL;
    PyObject *top_level_directives = NULL;

    source = check_source_argument(py_source);
    if (source == NULL)
    {
        goto done;
    }
    
    // Ensure the object is a set.
    punct = check_punctuator_arguments(punctuator_arguments);
    if (!punct.ok)
    {
        goto done;
    }

    conf_extensions extensions = {
        .punctuator_arguments = (const char **)punct.argv,
        .c_style_comments = c_style_comments ? true : false,
        .expression_arguments = expression_arguments ? true : false,
    };

    conf_options options = {
        .extensions = &extensions,
        // Use Python's memory allocation routines.
        .allocator = allocator,
        // Pick a "big enough" value to avoid overflowing the C call stack.
        // If Confetti were implemented in native Python we could rely on the Python call stack.
        .max_depth = 100,
    };

    conf_error error = {0};
    unit = conf_parse(source, &options, &error);
    if (error.code != CONF_NO_ERROR)
    {
        set_error(&error, source);
        goto done;
    }

    const conf_directive *dir = conf_get_root(unit);
    const long dir_count = conf_get_directive_count(dir);

    top_level_directives = PyList_New(dir_count);
    if (top_level_directives == NULL)
    {
        PyErr_SetString(PyExc_MemoryError, "failed to allocate list");
        goto done;
    }

    for (long i = 0; i < dir_count; i++)
    {
        PyObject *top_dir = confetti_add_directive(conf_get_directive(dir, i));
        if (top_dir == NULL)
        {
            PyErr_SetString(PyExc_MemoryError, "failed to allocate directives");
            Py_CLEAR(top_level_directives);
            goto done;
        }

        // This function only errors if an index is out of bounds.
        // We know the list is big enough, so this can never happen.
        PyList_SetItem(top_level_directives, i, top_dir);
    }

done:
    conf_free(unit);
    PyMem_Free(source);
    free_punctuator_arguments(&punct);
    return top_level_directives;
}
