/* connection.c - the connection type
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

#include "connection.h"
#include "cursor.h"

PyObject* connection_alloc(PyTypeObject* type, int aware)
{
    PyObject *self;

    self = (PyObject*)PyObject_MALLOC(sizeof(Connection));
    if (self == NULL)
        return (PyObject *)PyErr_NoMemory();
    PyObject_INIT(self, type);

    return self;
}

int connection_init(Connection* self, PyObject* args, PyObject* kwargs)
{
    static char *kwlist[] = {"database", "more_types", NULL, NULL};
    char* database;
    int more_types = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|i", kwlist,
                                     &database, &more_types))
    {
        return -1; 
    }

    if (sqlite3_open(database, &self->db) != SQLITE_OK) {
        PyErr_SetString(DatabaseError, sqlite3_errmsg(self->db));
        return -1;
    }

    self->inTransaction = 0;
    self->advancedTypes = more_types;
    self->converters = PyDict_New();

    return 0;
}

PyObject* connection_new(PyTypeObject* type, PyObject* args, PyObject* kw)
{
    Connection* self = NULL;

    self = (Connection*) (type->tp_alloc(type, 0));

    return (PyObject*)self;
}

void connection_dealloc(Connection* self)
{
    /* Clean up if user has not called .close() explicitly. */
    if (self->db) {
        sqlite3_close(self->db);
    }

    Py_XDECREF(self->converters);

    self->ob_type->tp_free((PyObject*)self);
}

PyObject* connection_cursor(Connection* self, PyObject* args)
{
    Cursor* cursor = NULL;

    cursor = (Cursor*) (CursorType.tp_alloc(&CursorType, 0));
    cursor->connection = self;
    cursor->statement = NULL;
    cursor->step_rc = UNKNOWN;
    cursor->row_cast_map = PyList_New(0);

    Py_INCREF(Py_None);
    cursor->description = Py_None;

    cursor->arraysize = 1;

    Py_INCREF(Py_None);
    cursor->rowcount = Py_None;

    Py_INCREF(Py_None);
    cursor->coltypes = Py_None;
    Py_INCREF(Py_None);
    cursor->next_coltypes = Py_None;

    return (PyObject*)cursor;
}

PyObject* connection_close(Connection* self, PyObject* args)
{
    int rc;

    if (self->db) {
        rc = sqlite3_close(self->db);
        if (rc != SQLITE_OK) {
            PyErr_SetString(DatabaseError, sqlite3_errmsg(self->db));
            return NULL;
        } else {
            self->db = NULL;
        }
    }

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* connection_begin(Connection* self, PyObject* args)
{
    int rc;
    const char* tail;
    sqlite3_stmt* statement;

    rc = sqlite3_prepare(self->db, "BEGIN", -1, &statement, &tail);
    if (rc != SQLITE_OK) {
        PyErr_SetString(DatabaseError, sqlite3_errmsg(self->db));
        return NULL;
    }

    rc = sqlite3_step(statement);
    if (rc != SQLITE_DONE) {
        PyErr_SetString(DatabaseError, sqlite3_errmsg(self->db));
        return NULL;
    }

    rc = sqlite3_finalize(statement);
    if (rc != SQLITE_OK) {
        PyErr_SetString(DatabaseError, sqlite3_errmsg(self->db));
        return NULL;
    }

    self->inTransaction = 1;

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* connection_commit(Connection* self, PyObject* args)
{
    int rc;
    const char* tail;
    sqlite3_stmt* statement;

    if (self->inTransaction) {
        rc = sqlite3_prepare(self->db, "COMMIT", -1, &statement, &tail);
        if (rc != SQLITE_OK) {
            PyErr_SetString(DatabaseError, sqlite3_errmsg(self->db));
            return NULL;
        }

        rc = sqlite3_step(statement);
        if (rc != SQLITE_DONE) {
            PyErr_SetString(DatabaseError, sqlite3_errmsg(self->db));
            return NULL;
        }

        rc = sqlite3_finalize(statement);
        if (rc != SQLITE_OK) {
            PyErr_SetString(DatabaseError, sqlite3_errmsg(self->db));
            return NULL;
        }

        self->inTransaction = 0;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* connection_rollback(Connection* self, PyObject* args)
{
    int rc;
    const char* tail;
    sqlite3_stmt* statement;

    if (self->inTransaction) {
        rc = sqlite3_prepare(self->db, "ROLLBACK", -1, &statement, &tail);
        if (rc != SQLITE_OK) {
            PyErr_SetString(DatabaseError, sqlite3_errmsg(self->db));
            return NULL;
        }

        rc = sqlite3_step(statement);
        if (rc != SQLITE_DONE) {
            PyErr_SetString(DatabaseError, sqlite3_errmsg(self->db));
            return NULL;
        }

        rc = sqlite3_finalize(statement);
        if (rc != SQLITE_OK) {
            PyErr_SetString(DatabaseError, sqlite3_errmsg(self->db));
            return NULL;
        }

        self->inTransaction = 0;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* connection_register_converter(Connection* self, PyObject* args)
{
    PyObject* name;
    PyObject* func;
    PyObject* typecode = NULL;
    PyObject* value;

    if (!PyArg_ParseTuple(args, "OO|O", &name, &func, &typecode)) {
        return NULL;
    }

    if (!typecode) {
        Py_INCREF(Py_None);
        typecode = Py_None;
    }

    Py_INCREF(name);
    Py_INCREF(func);
    Py_INCREF(typecode);
    value = PyTuple_New(2);
    PyTuple_SetItem(value, 0, func);
    PyTuple_SetItem(value, 1, typecode);
    PyDict_SetItem(self->converters, name, value);

    Py_INCREF(Py_None);
    return Py_None;
}

static char connection_doc[] =
PyDoc_STR("<missing docstring>");

static PyMethodDef connection_methods[] = {
    {"cursor", (PyCFunction)connection_cursor, METH_NOARGS,
        PyDoc_STR("Return a cursor for the connection.")},
    {"close", (PyCFunction)connection_close, METH_NOARGS,
        PyDoc_STR("Closes the connection.")},
    {"commit", (PyCFunction)connection_commit, METH_NOARGS,
        PyDoc_STR("Commit the current transaction.")},
    {"rollback", (PyCFunction)connection_rollback, METH_NOARGS,
        PyDoc_STR("Roll back the current transaction.")},
    {"register_converter", (PyCFunction)connection_register_converter, METH_VARARGS,
        PyDoc_STR("Registers a new type converter.")},

    {NULL, NULL}
};

PyTypeObject ConnectionType = {
        PyObject_HEAD_INIT(NULL)
        0,                                              /* ob_size */
        "sqlite.Connection",                            /* tp_name */
        sizeof(Connection),                             /* tp_basicsize */
        0,                                              /* tp_itemsize */
        (destructor)connection_dealloc,                 /* tp_dealloc */
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
        0,                                              /* tp_getattro */
        0,                                              /* tp_setattro */
        0,                                              /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT,                             /* tp_flags */
        connection_doc,                                 /* tp_doc */
        0,                                              /* tp_traverse */
        0,                                              /* tp_clear */
        0,                                              /* tp_richcompare */
        0,                                              /* tp_weaklistoffset */
        0,                                              /* tp_iter */
        0,                                              /* tp_iternext */
        connection_methods,                             /* tp_methods */
        0,                                              /* tp_members */
        0,                                              /* tp_getset */
        0,                                              /* tp_base */
        0,                                              /* tp_dict */
        0,                                              /* tp_descr_get */
        0,                                              /* tp_descr_set */
        0,                                              /* tp_dictoffset */
        (initproc)connection_init,                      /* tp_init */
        0,                                              /* tp_alloc */
        0,                                              /* tp_new */
        0                                               /* tp_free */
};
 
