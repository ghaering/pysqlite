/* connection.c - the cursor type
 *
 * Copyright (C) 2004 Gerhard Häring <gh@ghaering.de>
 *
 * This file is part of pysqlite.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "cursor.h"
#include "module.h"

static PyObject* cursor_iternext(Cursor *self);

static StatementType detect_statement_type(char* statement)
{
    char buf[20];
    char* src;
    char* dst;

    src = statement;
    /* skip over whitepace */
    while (*src == '\r' || *src == '\n' || *src == ' ' || *src == '\t') {
        src++;
    }

    if (*src == 0)
        return STATEMENT_INVALID;

    dst = buf;
    *dst = 0;
    while (isalpha(*src) && dst - buf < sizeof(buf) - 2) {
        *dst++ = tolower(*src++);
    }

    *dst = 0;

    if (!strcmp(buf, "select")) {
        return STATEMENT_SELECT;
    } else if (!strcmp(buf, "insert")) {
        return STATEMENT_INSERT;
    } else if (!strcmp(buf, "update")) {
        return STATEMENT_UPDATE;
    } else if (!strcmp(buf, "delete")) {
        return STATEMENT_DELETE;
    } else {
        return STATEMENT_OTHER;
    }
}

int cursor_init(Cursor* self, PyObject* args, PyObject* kwargs)
{
    static char *kwlist[] = {NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist))
    {
        return -1; 
    }

    return 0;
}

void cursor_dealloc(Cursor* self)
{
    int rc;

    /* Finalize the statement if the user has not closed the cursor */
    rc = sqlite3_finalize(self->statement);
    if (rc != SQLITE_OK) {
    }

    self->ob_type->tp_free((PyObject*)self);
}

PyObject* cursor_execute(Cursor* self, PyObject* args)
{
    PyObject* operation;
    char* operation_cstr;
    PyObject* parameters = NULL;
    int num_params;
    const char* tail;
    int i;
    int rc;
    PyObject* func_args;
    PyObject* result;
    int numcols;
    int coltype;
    int statement_type;
    PyObject* descriptor;

    if (!PyArg_ParseTuple(args, "S|O", &operation, &parameters))
    {
        return NULL; 
    }

    if (self->statement != NULL) {
        /* There is an active statement */
        if (sqlite3_finalize(self->statement) != SQLITE_OK) {
            PyErr_SetString(sqlite_DatabaseError, "error finalizing virtual machine");
            return NULL;
        }
    }

    operation_cstr = PyString_AsString(operation);

    /* reset description and rowcount */
    Py_DECREF(self->description);
    Py_INCREF(Py_None);
    self->description = Py_None;

    Py_DECREF(self->rowcount);
    Py_INCREF(Py_None);
    self->rowcount = Py_None;

    statement_type = detect_statement_type(operation_cstr);
    if (!self->connection->inTransaction) {
        switch (statement_type) {
            case STATEMENT_UPDATE:
            case STATEMENT_DELETE:
            case STATEMENT_INSERT:
                func_args = PyTuple_New(0);
                if (!args)
                    return NULL;
                result = connection_begin(self->connection, func_args);
                Py_DECREF(func_args);
                if (!result) {
                    return NULL;
                }
        }
    }

    rc = sqlite3_prepare(self->connection->db,
                         operation_cstr,
                         0,
                         &self->statement,
                         &tail);
    if (rc != SQLITE_OK) {
        PyErr_SetString(sqlite_DatabaseError, "error preparing statement");
        return NULL;
    }

    if (parameters != NULL) {
        num_params = PySequence_Length(parameters);
        for (i = 0; i < num_params; i++) {
            rc = sqlite3_bind_text(self->statement, i + 1, PyString_AsString(PyObject_Str(PySequence_GetItem(parameters, i))), -1, SQLITE_TRANSIENT);
        }
    }

    self->step_rc = sqlite3_step(self->statement);
    if (self->step_rc != SQLITE_DONE && self->step_rc != SQLITE_ROW) {
        PyErr_SetString(sqlite_DatabaseError, "error executing first sqlite3_step call");
        return NULL;
    }

    if (self->step_rc == SQLITE_ROW) {
        numcols = sqlite3_data_count(self->statement);

        if (self->description == Py_None) {
            Py_DECREF(self->description);
            self->description = PyTuple_New(numcols);
            for (i = 0; i < numcols; i++) {
                descriptor = PyTuple_New(7);
                coltype = sqlite3_column_type(self->statement, i);
                PyTuple_SetItem(descriptor, 0, PyString_FromString(sqlite3_column_name(self->statement, i)));
                if (coltype == SQLITE_INTEGER || coltype == SQLITE_FLOAT) {
                    Py_INCREF(sqlite_NUMBER);
                    PyTuple_SetItem(descriptor, 1, sqlite_NUMBER);
                } else if (coltype == SQLITE_TEXT) {
                    Py_INCREF(sqlite_STRING);
                    PyTuple_SetItem(descriptor, 1, sqlite_STRING);
                } else if (coltype == SQLITE_BLOB) {
                    Py_INCREF(sqlite_BINARY);
                    PyTuple_SetItem(descriptor, 1, sqlite_BINARY);
                } else {
                    /* SQLITE_NULL, cannot know type */
                    Py_INCREF(Py_None);
                    PyTuple_SetItem(descriptor, 1, Py_None);
                }

                Py_INCREF(Py_None); PyTuple_SetItem(descriptor, 2, Py_None);
                Py_INCREF(Py_None); PyTuple_SetItem(descriptor, 3, Py_None);
                Py_INCREF(Py_None); PyTuple_SetItem(descriptor, 4, Py_None);
                Py_INCREF(Py_None); PyTuple_SetItem(descriptor, 5, Py_None);
                Py_INCREF(Py_None); PyTuple_SetItem(descriptor, 6, Py_None);
                PyTuple_SetItem(self->description, i, descriptor);
            }
        }
    }

    switch (statement_type) {
        case STATEMENT_UPDATE:
        case STATEMENT_DELETE:
        case STATEMENT_INSERT:
            Py_DECREF(self->rowcount);
            self->rowcount = PyInt_FromLong((long)sqlite3_changes(self->connection->db));
    }

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* cursor_executemany(Cursor* self, PyObject* args)
{
    PyObject* operation;
    char* operation_cstr;
    PyObject* parameterParam;
    PyObject* parameterIter;
    PyObject* parameters;
    PyObject* func_args;
    int num_params;
    const char* tail;
    int i;
    int rc;
    int rowcount = 0;

    if (!PyArg_ParseTuple(args, "SO", &operation, &parameterParam))
    {
        return NULL; 
    }

    if (PyCallable_Check(parameterParam)) {
        /* generator */
        func_args = PyTuple_New(0);
        parameterIter = PyObject_CallObject(parameterParam, func_args);
        Py_DECREF(func_args);
        if (!parameterIter) {
            return NULL;
        }
    } else {
        if (PyIter_Check(parameterParam)) {
            /* iterator */
            Py_INCREF(parameterParam);
            parameterIter = parameterParam;
        } else {
            /* sequence */
            parameterIter = PyObject_GetIter(parameterParam);
            if (PyErr_Occurred())
            {
                return NULL;
            }
        }
    }

    operation_cstr = PyString_AsString(operation);

    if (self->statement != NULL) {
        /* There is an active statement */
        if (sqlite3_finalize(self->statement) != SQLITE_OK) {
            PyErr_SetString(sqlite_DatabaseError, "error finalizing virtual machine");
            return NULL;
        }
    }

    rc = sqlite3_prepare(self->connection->db,
                         operation_cstr,
                         0,
                         &self->statement,
                         &tail);
    if (rc != SQLITE_OK) {
        PyErr_SetString(sqlite_DatabaseError, "error preparing statement");
        return NULL;
    }

    while (1) {
        parameters = PyIter_Next(parameterIter);
        if (!parameters) {
            if (PyErr_Occurred()) {
                return NULL;
            } else {
                break;
            }
        }

        num_params = PySequence_Length(parameters);
        for (i = 0; i < num_params; i++) {
            rc = sqlite3_bind_text(self->statement, i + 1, PyString_AsString(PyObject_Str(PySequence_GetItem(parameters, i))), -1, SQLITE_TRANSIENT);
        }

        rc = sqlite3_step(self->statement);
        if (rc != SQLITE_DONE) {
            PyErr_SetString(sqlite_DatabaseError, "unexpected rc");
            return NULL;
        }

        rc = sqlite3_reset(self->statement);

        rowcount += sqlite3_changes(self->connection->db);
    }

    Py_DECREF(parameterIter);

    Py_DECREF(self->rowcount);
    self->rowcount = PyInt_FromLong((long)rowcount);

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* cursor_getiter(Cursor *self)
{
    Py_INCREF(self);
    return (PyObject*)self;
}

PyObject* cursor_iternext(Cursor *self)
{
    int i, numcols;
    PyObject* row;
    PyObject* item = NULL;
    int coltype;

    if (self->statement == NULL) {
        PyErr_SetString(sqlite_ProgrammingError, "no compiled statement");
        return NULL;
    }

    /* TODO: handling of step_rc here. it would be nicer if the hack with
             UNKNOWN were not necessary
    */
    if (self->step_rc == UNKNOWN) {
        self->step_rc = sqlite3_step(self->statement);
    }

    if (self->step_rc == SQLITE_DONE) {
        return NULL;
    } else if (self->step_rc != SQLITE_ROW) {
        PyErr_SetString(sqlite_DatabaseError, "wrong return code :-S");
        return NULL;
    }

    numcols = sqlite3_data_count(self->statement);

    row = PyTuple_New(numcols);

    for (i = 0; i < numcols; i++) {
        coltype = sqlite3_column_type(self->statement, i);
        if (coltype == SQLITE_NULL) {
            Py_INCREF(Py_None);
            item = Py_None;
        } else {
            /* TODO: use typecasters if available */
            if (coltype == SQLITE_INTEGER) {
                item = PyInt_FromLong((long)sqlite3_column_int(self->statement, i));
            } else if (coltype == SQLITE_FLOAT) {
                item = PyFloat_FromDouble(sqlite3_column_double(self->statement, i));
            } else if (coltype == SQLITE_TEXT) {
                /* TODO: Unicode */
                item = PyString_FromString(sqlite3_column_text(self->statement, i));
            } else {
                /* TODO: BLOB */
                assert(0);
            }
        }
        PyTuple_SetItem(row, i, item);
    }

    self->step_rc = UNKNOWN;

    /* TODO: that's wrong here, perhaps we need to have a ->typecasters and a
     * ->current_typecasters */
    Py_DECREF(self->typecasters);
    Py_INCREF(Py_None);
    self->typecasters = Py_None;

    return row;
}

PyObject* cursor_fetchone(Cursor* self, PyObject* args)
{
    PyObject* row;

    row = cursor_iternext(self);
    if (!row && !PyErr_Occurred()) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    return row;
}

PyObject* cursor_fetchmany(Cursor* self, PyObject* args)
{
    PyObject* row;
    PyObject* list;
    int maxrows = self->arraysize;
    int counter = 0;

    if (!PyArg_ParseTuple(args, "|i", &maxrows)) {
        return NULL; 
    }

    list = PyList_New(0);

    /* just make sure we enter the loop */
    row = (PyObject*)1;

    while (row) {
        row = cursor_iternext(self);
        if (row) {
            PyList_Append(list, row);
        }

        if (++counter == maxrows) {
            break;
        }
    }

    return list;
}

PyObject* cursor_fetchall(Cursor* self, PyObject* args)
{
    PyObject* row;
    PyObject* list;

    list = PyList_New(0);

    /* just make sure we enter the loop */
    row = (PyObject*)1;

    while (row) {
        row = cursor_iternext(self);
        if (row) {
            PyList_Append(list, row);
        }
    }

    return list;
}

PyObject* pysqlite_noop(Connection* self, PyObject* args)
{
    /* don't care, return None */
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* cursor_close(Cursor* self, PyObject* args)
{
    int rc;

    rc = sqlite3_finalize(self->statement);
    if (rc != SQLITE_OK) {
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef cursor_methods[] = {
    {"execute", (PyCFunction)cursor_execute, METH_VARARGS,
        PyDoc_STR("Executes a SQL statement.")},
    {"executemany", (PyCFunction)cursor_executemany, METH_VARARGS,
        PyDoc_STR("Repeatedly executes a SQL statement.")},
    {"fetchone", (PyCFunction)cursor_fetchone, METH_NOARGS,
        PyDoc_STR("Fetches several rows from the resultset.")},
    {"fetchmany", (PyCFunction)cursor_fetchmany, METH_VARARGS,
        PyDoc_STR("Fetches all rows from the resultset.")},
    {"fetchall", (PyCFunction)cursor_fetchall, METH_NOARGS,
        PyDoc_STR("Fetches one row from the resultset.")},
    {"close", (PyCFunction)cursor_close, METH_NOARGS,
        PyDoc_STR("Closes the cursor.")},
    {"setinputsizes", (PyCFunction)pysqlite_noop, METH_VARARGS,
        PyDoc_STR("Required by DB-API. Does nothing in pysqlite.")},
    {"setoutputsize", (PyCFunction)pysqlite_noop, METH_VARARGS,
        PyDoc_STR("Required by DB-API. Does nothing in pysqlite.")},
    {NULL, NULL}
};

static struct PyMemberDef cursor_members[] =
{
    {"description", T_OBJECT, offsetof(Cursor, description), RO},
    {"arraysize", T_INT, offsetof(Cursor, arraysize), 0},
    {"rowcount", T_OBJECT, offsetof(Cursor, rowcount), RO},
    {NULL}
};

PyTypeObject CursorType = {
        PyObject_HEAD_INIT(NULL)
        0,                                              /* ob_size */
        "sqlite.Cursor",                                /* tp_name */
        sizeof(Cursor),                                 /* tp_basicsize */
        0,                                              /* tp_itemsize */
        (destructor)cursor_dealloc,                     /* tp_dealloc */
        0,                                              /* tp_print */
        0,                                              /* tp_getattr */
        0,                                              /* tp_setattr */
        0,                                              /* tp_compare */
        0,                                              /* tp_repr */
        0,                                              /* tp_as_number */
        0,                                              /* tp_as_sequence */
        0,                                              /* tp_as_mapping */
        0,                                              /* tp_hash */
        0,                                              /* tp_call */
        0,                                              /* tp_str */
        PyObject_GenericGetAttr,                        /* tp_getattro */
        0,                                              /* tp_setattro */
        0,                                              /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_ITER|Py_TPFLAGS_BASETYPE, /* tp_flags */
        0,                                              /* tp_doc */
        0,                                              /* tp_traverse */
        0,                                              /* tp_clear */
        0,                                              /* tp_richcompare */
        0,                                              /* tp_weaklistoffset */
        (getiterfunc)cursor_getiter,                    /* tp_iter */
        (iternextfunc)cursor_iternext,                  /* tp_iternext */
        cursor_methods,                                 /* tp_methods */
        cursor_members,                                 /* tp_members */
        0,                                              /* tp_getset */
        0,                                              /* tp_base */
        0,                                              /* tp_dict */
        0,                                              /* tp_descr_get */
        0,                                              /* tp_descr_set */
        0,                                              /* tp_dictoffset */
        (initproc)cursor_init,                          /* tp_init */
        0,                                              /* tp_alloc */
        0,                                              /* tp_new */
        0                                               /* tp_free */
};
