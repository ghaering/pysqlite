/*
 * ===========================================================================
 * PySQLite - Bringing SQLite to Python.
 * ===========================================================================
 *
 * Copyright (C) 2004 Gerhard Häring
 *
 *   This software is provided 'as-is', without any express or implied
 *   warranty.  In no event will the authors be held liable for any damages
 *   arising from the use of this software.
 *
 *   Permission is granted to anyone to use this software for any purpose,
 *   including commercial applications, and to alter it and redistribute it
 *   freely, subject to the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must not
 *      claim that you wrote the original software. If you use this software
 *      in a product, an acknowledgment in the product documentation would be
 *      appreciated but is not required.
 *   2. Altered source versions must be plainly marked as such, and must not be
 *      misrepresented as being the original software.
 *   3. This notice may not be removed or altered from any source distribution.
 *
 * Gerhard Häring <gh@ghaering.de>
 */

#include "Python.h"
#include "structmember.h"

#include "sqlite3.h"

typedef struct
{
    PyObject_HEAD
    char* dbname;       // name of database file
    sqlite3* db;
} PySQLite_Connection;

typedef struct
{
    PyObject_HEAD
    PySQLite_Connection* connection;
    PyObject* typecasters;
    sqlite3_stmt* statement;

    /* return code of the last call to sqlite3_step */
    int step_rc;
} PySQLite_Cursor;

// XXX: possible to get rid of "statichere"?
statichere PyTypeObject PySQLite_CursorType;

static PyObject* connection_alloc(PyTypeObject* type, int aware)
{
    PyObject *self;

    self = (PyObject*)PyObject_MALLOC(sizeof(PySQLite_Connection));
    if (self == NULL)
        return (PyObject *)PyErr_NoMemory();
    PyObject_INIT(self, type);

    return self;
}

static PyObject* connection_repr(PySQLite_Connection* self)
{
    return PyString_FromString("<connection>");
}

/**
 * Exception objects
 */
static PyObject* sqlite_Error;
static PyObject* sqlite_Warning;
static PyObject* sqlite_InterfaceError;
static PyObject* sqlite_DatabaseError;
static PyObject* sqlite_InternalError;
static PyObject* sqlite_OperationalError;
static PyObject* sqlite_ProgrammingError;
static PyObject* sqlite_IntegrityError;
static PyObject* sqlite_DataError;
static PyObject* sqlite_NotSupportedError;

#define UNKNOWN (-1)

static char connection_doc[] =
PyDoc_STR("<missing docstring>");

static void connection_dealloc(PySQLite_Connection* self)
{
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject* connection_cursor(PySQLite_Connection* self, PyObject* args)
{
    PySQLite_Cursor* cursor = NULL;

    cursor = (PySQLite_Cursor*) (PySQLite_CursorType.tp_alloc(&PySQLite_CursorType, 0));
    cursor->connection = self;
    cursor->statement = NULL;
    cursor->step_rc = UNKNOWN;

    Py_INCREF(Py_None);
    cursor->typecasters = Py_None;

    return (PyObject*)cursor;
}

static PyObject* connection_close(PyObject* args)
{
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* connection_commit(PyObject* args)
{
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* connection_rollback(PyObject* args)
{
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* cursor_execute(PySQLite_Cursor* self, PyObject* args)
{
    PyObject* operation;
    char* operation_cstr;
    PyObject* parameters = NULL;
    int num_params;
    const char* tail;
    int i;
    int rc;

    if (!PyArg_ParseTuple(args, "S|O", &operation, &parameters))
    {
        return NULL; 
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

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* cursor_fetchone(PySQLite_Cursor* self, PyObject* args)
{
    int i, numcols;
    PyObject* row;
    PyObject* item;
    char* c_item;

    if (self->statement == NULL) {
        PyErr_SetString(sqlite_ProgrammingError, "no compiled statement");
        return NULL;
    }

    // TODO: handling of step_rc here. it would be nicer if the hack with
    //       UNKNOWN were not necessary
    if (self->step_rc == UNKNOWN) {
        self->step_rc = sqlite3_step(self->statement);
    }

    if (self->step_rc == SQLITE_DONE) {
        Py_INCREF(Py_None);
        return Py_None;
    } else if (self->step_rc != SQLITE_ROW) {
        PyErr_SetString(sqlite_DatabaseError, "wrong return code :-S");
        return NULL;
    }

    numcols = sqlite3_data_count(self->statement);
    row = PyTuple_New(numcols);

    for (i = 0; i < numcols; i++) {
        c_item = sqlite3_column_text(self->statement, i);
        if (c_item) {
            item = PyString_FromString(c_item);
        } else {
            Py_INCREF(Py_None);
            item = Py_None;
        }
        PyTuple_SetItem(row, i, item);
    }

    self->step_rc = UNKNOWN;

    Py_DECREF(self->typecasters);
    Py_INCREF(Py_None);
    self->typecasters = Py_None;

    return row;
}

static PyGetSetDef connection_getset[] = {
    //{"sql", (getter)connection_sql},
    {NULL}
};


static PyObject* connection_new(PyTypeObject* type, PyObject* args, PyObject* kw)
{
    PySQLite_Connection* self = NULL;

    self = (PySQLite_Connection*) (type->tp_alloc(type, 0));

    return (PyObject*)self;
}

static int connection_init(PySQLite_Connection* self, PyObject* args, PyObject* kwargs)
{
    static char *kwlist[] = {"database", NULL};
    char* database;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist,
                                     &database))
    {
        return -1; 
    }

    if (sqlite3_open(database, &self->db) != SQLITE_OK) {
        printf("error opening file\n");
        return -1;
    }


    return 0;
}

static int cursor_init(PySQLite_Cursor* self, PyObject* args, PyObject* kwargs)
{
    //static char *kwlist[] = {"connection", NULL};
    static char *kwlist[] = {NULL};

    printf("foo1\n");
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist))
    {
        printf("foo2\n");
        return -1; 
    }

    printf("foo3\n");
    return 0;
}

static PyMethodDef connection_methods[] = {
    {"cursor", (PyCFunction)connection_cursor, METH_NOARGS,
        PyDoc_STR("Return a cursor for the connection.")},
    {"close", (PyCFunction)connection_close, METH_NOARGS,
        PyDoc_STR("Closes the connection.")},
    {"commit", (PyCFunction)connection_commit, METH_NOARGS,
        PyDoc_STR("Commit the current transaction.")},
    {"rollback", (PyCFunction)connection_rollback, METH_NOARGS,
        PyDoc_STR("Roll back the current transaction.")},

    {NULL, NULL}
};

static PyMethodDef cursor_methods[] = {
    {"execute", (PyCFunction)cursor_execute, METH_VARARGS,
        PyDoc_STR("Executes a SQL statement.")},
    {"fetchone", (PyCFunction)cursor_fetchone, METH_NOARGS,
        PyDoc_STR("Fetches one row from the resultset.")},
    {NULL, NULL}
};


statichere PyTypeObject PySQLite_ConnectionType = {
        PyObject_HEAD_INIT(NULL)
        0,                                        /* ob_size */
        "sqlite.Connection",                      /* tp_name */
        sizeof(PySQLite_Connection),              /* tp_basicsize */
        0,                                        /* tp_itemsize */
        0, // (destructor)connection_dealloc,     /* tp_dealloc */
        0,                                        /* tp_print */
        0,                                        /* tp_getattr */
        0,                                        /* tp_setattr */
        0,                                        /* tp_compare */
        0, // (reprfunc)connection_repr,          /* tp_repr */
        0,                                        /* tp_as_number */
        0,                                        /* tp_as_sequence */
        0,                                        /* tp_as_mapping */
        0,                                        /* tp_hash */
        0,                                        /* tp_call */
        0,                                        /* tp_str */
        0, // PyObject_GenericGetAttr,            /* tp_getattro */
        0,                                        /* tp_setattro */
        0,                                        /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT,                       /* tp_flags */
        connection_doc,                     /* tp_doc */
        0,                                        /* tp_traverse */
        0,                                        /* tp_clear */
        0,                                        /* tp_richcompare */
        0,                                        /* tp_weaklistoffset */
        0,                                        /* tp_iter */
        0,                                        /* tp_iternext */
        connection_methods,                       /* tp_methods */
        0,                                        /* tp_members */
        0, // connection_getset,                  /* tp_getset */
        0, // &PySQLite_ConnectionType,           /* tp_base */
        0,                                        /* tp_dict */
        0,                                        /* tp_descr_get */
        0,                                        /* tp_descr_set */
        0,                                        /* tp_dictoffset */
        (initproc)connection_init,                /* tp_init */
        0, // connection_alloc,                   /* tp_alloc */
        0, // connection_new,                     /* tp_new */
        0                                         /* tp_free */
#if 0
#endif
};

statichere PyTypeObject PySQLite_CursorType = {
        PyObject_HEAD_INIT(NULL)
        0,                                        /* ob_size */
        "sqlite.Cursor",                          /* tp_name */
        sizeof(PySQLite_Cursor),                  /* tp_basicsize */
        0,                                        /* tp_itemsize */
        0, // (destructor)connection_dealloc,     /* tp_dealloc */
        0,                                        /* tp_print */
        0,                                        /* tp_getattr */
        0,                                        /* tp_setattr */
        0,                                        /* tp_compare */
        0, // (reprfunc)connection_repr,          /* tp_repr */
        0,                                        /* tp_as_number */
        0,                                        /* tp_as_sequence */
        0,                                        /* tp_as_mapping */
        0,                                        /* tp_hash */
        0,                                        /* tp_call */
        0,                                        /* tp_str */
        0, // PyObject_GenericGetAttr,            /* tp_getattro */
        0,                                        /* tp_setattro */
        0,                                        /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT,                       /* tp_flags */
        0, // connection_doc,                     /* tp_doc */
        0,                                        /* tp_traverse */
        0,                                        /* tp_clear */
        0,                                        /* tp_richcompare */
        0,                                        /* tp_weaklistoffset */
        0,                                        /* tp_iter */
        0,                                        /* tp_iternext */
        cursor_methods,                           /* tp_methods */
        0,                                        /* tp_members */
        0, // connection_getset,                  /* tp_getset */
        0, // &PySQLite_ConnectionType,           /* tp_base */
        0,                                        /* tp_dict */
        0,                                        /* tp_descr_get */
        0,                                        /* tp_descr_set */
        0,                                        /* tp_dictoffset */
        (initproc)cursor_init,                    /* tp_init */
        0, // connection_alloc,                   /* tp_alloc */
        0, // connection_new,                     /* tp_new */
        0                                         /* tp_free */
#if 0
#endif
};


static PyMethodDef module_methods[] = {
    {NULL, NULL}
};


// PyMODINIT_FUNC(initsqlite);

PyMODINIT_FUNC initsqlite(void)
{
    PyObject *module, *dict;
    //PyObject* sqlite_version;
    //PyObject* args;
    //long tc = 0L;

    // pysqlc_Type.ob_type = &PyType_Type;
    // pysqlrs_Type.ob_type = &PyType_Type;

    module = Py_InitModule("sqlite", module_methods);

    PySQLite_ConnectionType.tp_new = PyType_GenericNew;
    PySQLite_CursorType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&PySQLite_ConnectionType) < 0) {
        printf("connection not ready\n");
        return;
    }
    if (PyType_Ready(&PySQLite_CursorType) < 0) {
        printf("cursor not ready\n");
        return;
    }


    Py_INCREF(&PySQLite_ConnectionType);
    PyModule_AddObject(module, "connect", (PyObject*) &PySQLite_ConnectionType);
    Py_INCREF(&PySQLite_CursorType);
    PyModule_AddObject(module, "Cursor", (PyObject*) &PySQLite_CursorType);

    if (!(dict = PyModule_GetDict(module)))
    {
        goto error;
    }

#if 0
    requiredsqlite_version = PyTuple_New(3);
    PyTuple_SetItem(requiredsqlite_version, 0, PyInt_FromLong((long)3));
    PyTuple_SetItem(requiredsqlite_version, 1, PyInt_FromLong((long)0));
    PyTuple_SetItem(requiredsqlite_version, 2, PyInt_FromLong((long)0));

    args = PyTuple_New(0);
    sqlite_version = sqlite3_version_info(NULL, args);
    Py_DECREF(args);
    if (PyObject_Compare(sqlite_version, requiredsqlite_version) < 0)
    {
        Py_DECREF(sqlite_version);
        PyErr_SetString(PyExc_ImportError, "Need to be linked against SQLite 2.5.6 or higher.");
        return;
    }
    Py_DECREF(sqlite_version);

    /*** Initialize type codes */
    tc_INTEGER = PyInt_FromLong(tc++);
    tc_FLOAT = PyInt_FromLong(tc++);
    tc_TIMESTAMP = PyInt_FromLong(tc++);
    tc_DATE = PyInt_FromLong(tc++);
    tc_TIME = PyInt_FromLong(tc++);
    tc_INTERVAL = PyInt_FromLong(tc++);
    tc_STRING = PyInt_FromLong(tc++);
    tc_UNICODESTRING = PyInt_FromLong(tc++);
    tc_BINARY = PyInt_FromLong(tc++);

    PyDict_SetItemString(dict, "INTEGER", tc_INTEGER);
    PyDict_SetItemString(dict, "FLOAT", tc_FLOAT);
    PyDict_SetItemString(dict, "TIMESTAMP", tc_TIMESTAMP);
    PyDict_SetItemString(dict, "DATE", tc_DATE);
    PyDict_SetItemString(dict, "TIME", tc_TIME);
    PyDict_SetItemString(dict, "INTERVAL", tc_INTERVAL);
    PyDict_SetItemString(dict, "STRING", tc_STRING);
    PyDict_SetItemString(dict, "UNICODESTRING", tc_UNICODESTRING);
    PyDict_SetItemString(dict, "BINARY", tc_BINARY);
#endif

    /*** Create DB-API Exception hierarchy */

    sqlite_Error = PyErr_NewException("sqlite.Error", PyExc_StandardError, NULL);
    PyDict_SetItemString(dict, "Error", sqlite_Error);

    sqlite_Warning = PyErr_NewException("sqlite.Warning", PyExc_StandardError, NULL);
    PyDict_SetItemString(dict, "Warning", sqlite_Warning);

    /* Error subclasses */

    sqlite_InterfaceError = PyErr_NewException("sqlite.InterfaceError", sqlite_Error, NULL);
    PyDict_SetItemString(dict, "InterfaceError", sqlite_InterfaceError);

    sqlite_DatabaseError = PyErr_NewException("sqlite.DatabaseError", sqlite_Error, NULL);
    PyDict_SetItemString(dict, "DatabaseError", sqlite_DatabaseError);

    /* DatabaseError subclasses */

    sqlite_InternalError = PyErr_NewException("sqlite.InternalError", sqlite_DatabaseError, NULL);
    PyDict_SetItemString(dict, "InternalError", sqlite_InternalError);

    sqlite_OperationalError = PyErr_NewException("sqlite.OperationalError", sqlite_DatabaseError, NULL);
    PyDict_SetItemString(dict, "OperationalError", sqlite_OperationalError);

    sqlite_ProgrammingError = PyErr_NewException("sqlite.ProgrammingError", sqlite_DatabaseError, NULL);
    PyDict_SetItemString(dict, "ProgrammingError", sqlite_ProgrammingError);

    sqlite_IntegrityError = PyErr_NewException("sqlite.IntegrityError", sqlite_DatabaseError,NULL);
    PyDict_SetItemString(dict, "IntegrityError", sqlite_IntegrityError);

    sqlite_DataError = PyErr_NewException("sqlite.DataError", sqlite_DatabaseError, NULL);
    PyDict_SetItemString(dict, "DataError", sqlite_DataError);

    sqlite_NotSupportedError = PyErr_NewException("sqlite.NotSupportedError", sqlite_DatabaseError, NULL);
    PyDict_SetItemString(dict, "NotSupportedError", sqlite_NotSupportedError);

  error:

    if (PyErr_Occurred())
    {
        PyErr_SetString(PyExc_ImportError, "sqlite: init failed");
    }
}

