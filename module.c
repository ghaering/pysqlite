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
    sqlite3* db;
    int inTransaction;
} Connection;

typedef struct
{
    PyObject_HEAD
    Connection* connection;
    PyObject* typecasters;
    PyObject* description;
    int arraysize;
    PyObject* rowcount;
    sqlite3_stmt* statement;

    /* return code of the last call to sqlite3_step */
    int step_rc;
} Cursor;

typedef enum {STATEMENT_INVALID, STATEMENT_INSERT, STATEMENT_DELETE,
    STATEMENT_UPDATE, STATEMENT_SELECT, STATEMENT_OTHER} StatementType;

// XXX: possible to get rid of "statichere"?
statichere PyTypeObject CursorType;

static PyObject* connection_alloc(PyTypeObject* type, int aware)
{
    PyObject *self;

    self = (PyObject*)PyObject_MALLOC(sizeof(Connection));
    if (self == NULL)
        return (PyObject *)PyErr_NoMemory();
    PyObject_INIT(self, type);

    return self;
}

static PyObject* connection_repr(Connection* self)
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

static void connection_dealloc(Connection* self)
{
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject* connection_cursor(Connection* self, PyObject* args)
{
    Cursor* cursor = NULL;

    cursor = (Cursor*) (CursorType.tp_alloc(&CursorType, 0));
    cursor->connection = self;
    cursor->statement = NULL;
    cursor->step_rc = UNKNOWN;

    Py_INCREF(Py_None);
    cursor->typecasters = Py_None;

    Py_INCREF(Py_None);
    cursor->description = Py_None;

    cursor->arraysize = 1;
    Py_INCREF(Py_None);
    cursor->arraysize = Py_None;

    Py_INCREF(Py_None);
    cursor->rowcount = Py_None;

    return (PyObject*)cursor;
}

static PyObject* connection_close(Connection* self, PyObject* args)
{
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* connection_begin(Connection* self, PyObject* args)
{
    int rc;
    const char* tail;
    sqlite3_stmt* statement;

    rc = sqlite3_prepare(self->db, "BEGIN", -1, &statement, &tail);
    if (rc != SQLITE_OK) {
        PyErr_SetString(sqlite_DatabaseError, sqlite3_errmsg(self->db));
        return NULL;
    }

    rc = sqlite3_step(statement);
    if (rc != SQLITE_DONE) {
        PyErr_SetString(sqlite_DatabaseError, sqlite3_errmsg(self->db));
        return NULL;
    }

    rc = sqlite3_finalize(statement);
    if (rc != SQLITE_OK) {
        PyErr_SetString(sqlite_DatabaseError, sqlite3_errmsg(self->db));
        return NULL;
    }

    self->inTransaction = 1;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* connection_commit(Connection* self, PyObject* args)
{
    int rc;
    const char* tail;
    sqlite3_stmt* statement;

    if (self->inTransaction) {
        rc = sqlite3_prepare(self->db, "COMMIT", -1, &statement, &tail);
        if (rc != SQLITE_OK) {
            PyErr_SetString(sqlite_DatabaseError, sqlite3_errmsg(self->db));
            return NULL;
        }

        rc = sqlite3_step(statement);
        if (rc != SQLITE_DONE) {
            PyErr_SetString(sqlite_DatabaseError, sqlite3_errmsg(self->db));
            return NULL;
        }

        rc = sqlite3_finalize(statement);
        if (rc != SQLITE_OK) {
            PyErr_SetString(sqlite_DatabaseError, sqlite3_errmsg(self->db));
            return NULL;
        }

        self->inTransaction = 0;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* connection_rollback(Connection* self, PyObject* args)
{
    int rc;
    const char* tail;
    sqlite3_stmt* statement;

    if (self->inTransaction) {
        rc = sqlite3_prepare(self->db, "ROLLBACK", -1, &statement, &tail);
        if (rc != SQLITE_OK) {
            PyErr_SetString(sqlite_DatabaseError, sqlite3_errmsg(self->db));
            return NULL;
        }

        rc = sqlite3_step(statement);
        if (rc != SQLITE_DONE) {
            PyErr_SetString(sqlite_DatabaseError, sqlite3_errmsg(self->db));
            return NULL;
        }

        rc = sqlite3_finalize(statement);
        if (rc != SQLITE_OK) {
            PyErr_SetString(sqlite_DatabaseError, sqlite3_errmsg(self->db));
            return NULL;
        }

        self->inTransaction = 0;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

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
static struct PyMemberDef cursor_members[];
static PyMethodDef cursor_methods[];

static PyObject* cursor_getattr(Cursor* self, char* attr)
{
    PyObject* item;

    item = Py_FindMethod(cursor_methods, (PyObject*)self, attr);
    if (item) {
        return item;
    } else {
        PyErr_Clear();
        return PyMember_Get((char*)self, cursor_members, attr);
    }
}

static PyObject* cursor_execute(Cursor* self, PyObject* args)
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
                PyTuple_SetItem(descriptor, 0, PyString_FromString(sqlite3_column_name(self->statement, i)));
                PyTuple_SetItem(descriptor, 1, PyInt_FromLong(-1));
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

static PyObject* cursor_executemany(Cursor* self, PyObject* args)
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

static PyObject* cursor_fetchone(Cursor* self, PyObject* args)
{
    PyObject* row;

    row = cursor_iternext(self);
    if (!row && !PyErr_Occurred()) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    return row;
}

static PyObject* cursor_fetchmany(Cursor* self, PyObject* args)
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

static PyObject* cursor_fetchall(Cursor* self, PyObject* args)
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

static PyObject* pysqlite_noop(Connection* self, PyObject* args)
{
    /* don't care, return None */
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* cursor_close(Connection* self, PyObject* args)
{
    Py_INCREF(Py_None);
    return Py_None;
}

static PyGetSetDef connection_getset[] = {
    //{"sql", (getter)connection_sql},
    {NULL}
};


static PyObject* connection_new(PyTypeObject* type, PyObject* args, PyObject* kw)
{
    Connection* self = NULL;

    self = (Connection*) (type->tp_alloc(type, 0));

    return (PyObject*)self;
}

static int connection_init(Connection* self, PyObject* args, PyObject* kwargs)
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

    self->inTransaction = 0;

    return 0;
}

static int cursor_init(Cursor* self, PyObject* args, PyObject* kwargs)
{
    //static char *kwlist[] = {"connection", NULL};
    static char *kwlist[] = {NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist))
    {
        return -1; 
    }

    return 0;
}

static PyObject* cursor_getiter(Cursor *self)
{
    Py_INCREF(self);
    return (PyObject*)self;
}

static PyObject* cursor_iternext(Cursor *self)
{
    int i, numcols;
    PyObject* row;
    PyObject* item = NULL;
    int coltype;

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
    {"executemany", (PyCFunction)cursor_executemany, METH_VARARGS,
        PyDoc_STR("Repeatedly executes a SQL statement.")},
    {"fetchone", (PyCFunction)cursor_fetchone, METH_NOARGS,
        PyDoc_STR("Fetches several rows from the resultset.")},
    {"fetchmany", (PyCFunction)cursor_fetchmany, METH_VARARGS,
        PyDoc_STR("Fetches all rows from the resultset.")},
    {"fetchall", (PyCFunction)cursor_fetchall, METH_NOARGS,
        PyDoc_STR("Fetches one row from the resultset.")},
    {"close", (PyCFunction)pysqlite_noop, METH_NOARGS,
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

statichere PyTypeObject ConnectionType = {
        PyObject_HEAD_INIT(NULL)
        0,                                        /* ob_size */
        "sqlite.Connection",                      /* tp_name */
        sizeof(Connection),              /* tp_basicsize */
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
        0, // &ConnectionType,           /* tp_base */
        0,                                        /* tp_dict */
        0,                                        /* tp_descr_get */
        0,                                        /* tp_descr_set */
        0,                                        /* tp_dictoffset */
        (initproc)connection_init,                /* tp_init */
        0, // connection_alloc,                   /* tp_alloc */
        0, // connection_new,                     /* tp_new */
        0                                         /* tp_free */
};

statichere PyTypeObject CursorType = {
        PyObject_HEAD_INIT(NULL)
        0,                                        /* ob_size */
        "sqlite.Cursor",                          /* tp_name */
        sizeof(Cursor),                  /* tp_basicsize */
        0,                                        /* tp_itemsize */
        0, // (destructor)connection_dealloc,     /* tp_dealloc */
        0,                                        /* tp_print */
        0,                                      /* tp_getattr */
        0,                                        /* tp_setattr */
        0,                                        /* tp_compare */
        0, // (reprfunc)connection_repr,          /* tp_repr */
        0,                                        /* tp_as_number */
        0,                                        /* tp_as_sequence */
        0,                                        /* tp_as_mapping */
        0,                                        /* tp_hash */
        0,                                        /* tp_call */
        0,                                        /* tp_str */
        PyObject_GenericGetAttr,                  /* tp_getattro */
        0,                                        /* tp_setattro */
        0,                                        /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_ITER|Py_TPFLAGS_BASETYPE,                       /* tp_flags */
        0, // connection_doc,                     /* tp_doc */
        0,                                        /* tp_traverse */
        0,                                        /* tp_clear */
        0,                                        /* tp_richcompare */
        0,                                        /* tp_weaklistoffset */
        (getiterfunc)cursor_getiter,              /* tp_iter */
        (iternextfunc)cursor_iternext,            /* tp_iternext */
        cursor_methods,                           /* tp_methods */
        cursor_members,                           /* tp_members */
        0, // connection_getset,                  /* tp_getset */
        0, // &ConnectionType,           /* tp_base */
        0,                                        /* tp_dict */
        0,                                        /* tp_descr_get */
        0,                                        /* tp_descr_set */
        0,                                        /* tp_dictoffset */
        (initproc)cursor_init,                    /* tp_init */
        0, // connection_alloc,                   /* tp_alloc */
        0, // connection_new,                     /* tp_new */
        0                                         /* tp_free */
};

static PyMethodDef module_methods[] = {
    {NULL, NULL}
};


// PyMODINIT_FUNC(initsqlite);

PyMODINIT_FUNC init_sqlite(void)
{
    PyObject *module, *dict;
    //PyObject* sqlite_version;
    //PyObject* args;
    //long tc = 0L;

    // pysqlc_Type.ob_type = &PyType_Type;
    // pysqlrs_Type.ob_type = &PyType_Type;

    module = Py_InitModule("_sqlite", module_methods);

    ConnectionType.tp_new = PyType_GenericNew;
    CursorType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&ConnectionType) < 0) {
        printf("connection not ready\n");
        return;
    }
    if (PyType_Ready(&CursorType) < 0) {
        printf("cursor not ready\n");
        return;
    }

    Py_INCREF(&ConnectionType);
    PyModule_AddObject(module, "connect", (PyObject*) &ConnectionType);
    Py_INCREF(&CursorType);
    PyModule_AddObject(module, "Cursor", (PyObject*) &CursorType);

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

