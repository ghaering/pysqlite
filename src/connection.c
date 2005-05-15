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

#include "module.h"
#include "connection.h"
#include "cursor.h"
#include "prepare_protocol.h"
#include "util.h"
#include "pythread.h"

int connection_init(Connection* self, PyObject* args, PyObject* kwargs)
{
    static char *kwlist[] = {"database", "timeout", "detect_types", "autocommit", "check_same_thread", "factory", NULL, NULL};
    char* database;
    int detect_types = 0;
    int autocommit = 0;
    PyObject* factory = NULL;
    int check_same_thread = 1;
    double timeout = 5.0;
    int rc;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|diiiO", kwlist,
                                     &database, &timeout, &detect_types, &autocommit, &check_same_thread, &factory))
    {
        return -1; 
    }

    Py_BEGIN_ALLOW_THREADS
    rc = sqlite3_open(database, &self->db);
    Py_END_ALLOW_THREADS

    if (rc != SQLITE_OK) {
        _seterror(self->db);
        return -1;
    }

    self->inTransaction = 0;
    self->detect_types = detect_types;
    self->timeout = timeout;
    self->autocommit = autocommit;
    self->thread_ident = PyThread_get_thread_ident();
    self->check_same_thread = check_same_thread;

    self->Warning = Warning;
    self->Error = Error;
    self->InterfaceError = InterfaceError;
    self->DatabaseError = DatabaseError;
    self->DataError = DataError;
    self->OperationalError = OperationalError;
    self->IntegrityError = IntegrityError;
    self->InternalError = InternalError;
    self->ProgrammingError = ProgrammingError;
    self->NotSupportedError = NotSupportedError;

    return 0;
}

void connection_dealloc(Connection* self)
{
    /* Clean up if user has not called .close() explicitly. */
    if (self->db) {
        Py_BEGIN_ALLOW_THREADS
        sqlite3_close(self->db);
        Py_END_ALLOW_THREADS
    }

    self->ob_type->tp_free((PyObject*)self);
}

PyObject* connection_cursor(Connection* self, PyObject* args, PyObject* kwargs)
{
    static char *kwlist[] = {"factory", NULL, NULL};
    PyObject* factory = NULL;
    PyObject* cursor;


    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist,
                                     &factory)) {
        return NULL;
    }

    if (!check_thread(self) || !check_connection(self)) {
        return NULL;
    }

    if (factory == NULL) {
        factory = (PyObject*)&CursorType;
    }

    cursor = PyObject_CallFunction(factory, "O", self);

    return cursor;
}

PyObject* connection_close(Connection* self, PyObject* args)
{
    int rc;

    if (!check_thread(self)) {
        return NULL;
    }

    if (self->db) {
        Py_BEGIN_ALLOW_THREADS
        rc = sqlite3_close(self->db);
        Py_END_ALLOW_THREADS

        if (rc != SQLITE_OK) {
            _seterror(self->db);
            return NULL;
        } else {
            self->db = NULL;
        }
    }

    Py_INCREF(Py_None);
    return Py_None;
}

/*
 * Checks if a connection object is usable (i. e. not closed).
 *
 * 0 => error; 1 => ok
 */
int check_connection(Connection* con)
{
    if (!con->db) {
        PyErr_SetString(ProgrammingError, "Cannot operate on a closed database.");
        return 0;
    } else {
        return 1;
    }
}

PyObject* _connection_begin(Connection* self)
{
    int rc;
    const char* tail;
    sqlite3_stmt* statement;

    Py_BEGIN_ALLOW_THREADS
    rc = sqlite3_prepare(self->db, "BEGIN", -1, &statement, &tail);
    Py_END_ALLOW_THREADS

    if (rc != SQLITE_OK) {
        _seterror(self->db);
        return NULL;
    }

    rc = _sqlite_step_with_busyhandler(statement, self);
    if (rc != SQLITE_DONE) {
        _seterror(self->db);
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    rc = sqlite3_finalize(statement);
    Py_END_ALLOW_THREADS

    if (rc != SQLITE_OK) {
        _seterror(self->db);
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

    if (!check_thread(self) || !check_connection(self)) {
        return NULL;
    }

    if (self->inTransaction) {
        Py_BEGIN_ALLOW_THREADS
        rc = sqlite3_prepare(self->db, "COMMIT", -1, &statement, &tail);
        Py_END_ALLOW_THREADS
        if (rc != SQLITE_OK) {
            _seterror(self->db);
            goto error;
        }

        rc = _sqlite_step_with_busyhandler(statement, self);

        if (rc != SQLITE_DONE) {
            _seterror(self->db);
            goto error;
        }

        Py_BEGIN_ALLOW_THREADS
        rc = sqlite3_finalize(statement);
        Py_END_ALLOW_THREADS
        if (rc != SQLITE_OK) {
            _seterror(self->db);
            goto error;
        }

        self->inTransaction = 0;
    }

error:
    if (PyErr_Occurred()) {
        return NULL;
    } else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

PyObject* connection_rollback(Connection* self, PyObject* args)
{
    int rc;
    const char* tail;
    sqlite3_stmt* statement;

    if (!check_thread(self) || !check_connection(self)) {
        return NULL;
    }

    if (self->inTransaction) {
        Py_BEGIN_ALLOW_THREADS
        rc = sqlite3_prepare(self->db, "ROLLBACK", -1, &statement, &tail);
        Py_END_ALLOW_THREADS
        if (rc != SQLITE_OK) {
            _seterror(self->db);
            return NULL;
        }

        rc = _sqlite_step_with_busyhandler(statement, self);
        if (rc != SQLITE_DONE) {
            _seterror(self->db);
            return NULL;
        }

        Py_BEGIN_ALLOW_THREADS
        rc = sqlite3_finalize(statement);
        Py_END_ALLOW_THREADS
        if (rc != SQLITE_OK) {
            _seterror(self->db);
            return NULL;
        }

        self->inTransaction = 0;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

void _set_result(sqlite3_context* context, PyObject* py_val)
{
    long longval;
    const char* buffer;
    int buflen;
    PyObject* stringval;

    if (PyErr_Occurred()) {
        /* Errors in callbacks are ignored, and we return NULL */
        PyErr_Clear();
        sqlite3_result_null(context);
    } else if (py_val == Py_None) {
        sqlite3_result_null(context);
    } else if (PyInt_Check(py_val)) {
        longval = PyInt_AsLong(py_val);
        /* TODO: investigate what to do with range overflows - long vs. long long */
        sqlite3_result_int64(context, (long long int)longval);
    } else if (PyFloat_Check(py_val)) {
        sqlite3_result_double(context, PyFloat_AsDouble(py_val));
    } else if (PyBuffer_Check(py_val)) {
        if (PyObject_AsCharBuffer(py_val, &buffer, &buflen) != 0) {
            PyErr_SetString(PyExc_ValueError, "could not convert BLOB to buffer");
        }
        sqlite3_result_blob(context, buffer, buflen, SQLITE_TRANSIENT);
    } else if (PyString_Check(py_val)) {
        sqlite3_result_text(context, PyString_AsString(py_val), -1, SQLITE_TRANSIENT);
    } else if (PyUnicode_Check(py_val)) {
        stringval = PyUnicode_AsUTF8String(py_val);
        sqlite3_result_text(context, PyString_AsString(stringval), -1, SQLITE_TRANSIENT);
        Py_DECREF(stringval);
    } else {
        /* TODO: raise error */
    }
}

PyObject* _build_py_params(sqlite3_context *context, int argc, sqlite3_value** argv)
{
    PyObject* args;
    int i;
    sqlite3_value* cur_value;
    PyObject* cur_py_value;
    const char* val_str;
    long long int val_int;
    int buflen;
    void* raw_buffer;

    args = PyTuple_New(argc);

    for (i = 0; i < argc; i++) {
        cur_value = argv[i];
        switch (sqlite3_value_type(argv[i])) {
            case SQLITE_INTEGER:
                val_int = sqlite3_value_int64(cur_value);
                cur_py_value = PyInt_FromLong((long)val_int);
                break;
            case SQLITE_FLOAT:
                cur_py_value = PyFloat_FromDouble(sqlite3_value_double(cur_value));
                break;
            case SQLITE_TEXT:
                val_str = sqlite3_value_text(cur_value);
                cur_py_value = PyUnicode_DecodeUTF8(val_str, strlen(val_str), NULL);
                /* TODO: have a way to show errors here */
                if (!cur_py_value) {
                    Py_INCREF(Py_None);
                    cur_py_value = Py_None;
                }
                break;
            case SQLITE_BLOB:
                buflen = sqlite3_value_bytes(cur_value);
                cur_py_value = PyBuffer_New(buflen);
                if (!cur_py_value) {
                    /* TODO: error */
                }
                if (PyObject_AsWriteBuffer(cur_py_value, &raw_buffer, &buflen)) {
                    /* TODO: error */
                }
                memcpy(raw_buffer, sqlite3_value_blob(cur_value), buflen);
                break;
            case SQLITE_NULL:
            default:
                Py_INCREF(Py_None);
                cur_py_value = Py_None;
        }
        PyTuple_SetItem(args, i, cur_py_value);

    }

    return args;
}

void _func_callback(sqlite3_context* context, int argc, sqlite3_value** argv)
{
    PyObject* args;
    PyObject* py_func;
    PyObject* py_retval;


    PyGILState_STATE threadstate;

    threadstate = PyGILState_Ensure();

    py_func = (PyObject*)sqlite3_user_data(context);

    args = _build_py_params(context, argc, argv);

    py_retval = PyObject_CallObject(py_func, args);
    Py_DECREF(args);

    _set_result(context, py_retval);
    Py_XDECREF(py_retval);

    PyGILState_Release(threadstate);
}

static void _step_callback(sqlite3_context *context, int argc, sqlite3_value** params)
{
    PyObject* args;
    PyObject* function_result;
    PyObject* aggregate_class;
    PyObject** aggregate_instance;
    PyObject* stepmethod;

    PyGILState_STATE threadstate;

    threadstate = PyGILState_Ensure();

    aggregate_class = (PyObject*)sqlite3_user_data(context);

    aggregate_instance = (PyObject**)sqlite3_aggregate_context(context, sizeof(PyObject*));

    if (*aggregate_instance == 0) {
        *aggregate_instance = PyObject_CallFunction(aggregate_class, "");

        if (PyErr_Occurred())
        {
            PyErr_Clear();
            *aggregate_instance = 0;
            PyGILState_Release(threadstate);
            return;
        }
    }

    stepmethod = PyObject_GetAttrString(*aggregate_instance, "step");
    if (!stepmethod)
    {
        PyGILState_Release(threadstate);
        return;
    }

    args = _build_py_params(context, argc, params);

    function_result = PyObject_CallObject(stepmethod, args);
    Py_DECREF(args);
    Py_DECREF(stepmethod);

    if (function_result == NULL) {
        PyErr_Clear();
    } else {
        Py_DECREF(function_result);
    }

    PyGILState_Release(threadstate);
}

void _final_callback(sqlite3_context* context)
{
    PyObject* args;
    PyObject* function_result;
    PyObject** aggregate_instance;
    PyObject* aggregate_class;
    PyObject* finalizemethod;

    PyGILState_STATE threadstate;

    threadstate = PyGILState_Ensure();

    aggregate_class = (PyObject*)sqlite3_user_data(context);

    aggregate_instance = (PyObject**)sqlite3_aggregate_context(context, sizeof(PyObject*));
    if (!*aggregate_instance) {
        /* this branch is executed if there was an exception in the aggregate's
         * __init__ */

        PyGILState_Release(threadstate);
        return;
    }

    finalizemethod = PyObject_GetAttrString(*aggregate_instance, "finalize");

    if (!finalizemethod) {
        /*
        PyErr_SetString(ProgrammingError, "finalize method missing");
        goto error;
        */
        Py_INCREF(Py_None);
        function_result = Py_None;
    } else {
        args = PyTuple_New(0);
        function_result = PyObject_CallObject(finalizemethod, args);
        Py_DECREF(args);
        Py_DECREF(finalizemethod);
    }

    _set_result(context, function_result);
    Py_XDECREF(*aggregate_instance);
    Py_XDECREF(function_result);

    PyGILState_Release(threadstate);
}


PyObject* connection_create_function(Connection* self, PyObject* args, PyObject* kwargs)
{
    static char *kwlist[] = {"name", "narg", "func", NULL, NULL};

    PyObject* func;
    char* name;
    int narg;
    int rc;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "siO", kwlist,
                                     &name, &narg, &func))
    {
        return NULL;
    }

    rc = sqlite3_create_function(self->db, name, narg, SQLITE_UTF8, (void*)func, _func_callback, NULL, NULL);
;
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* connection_create_aggregate(Connection* self, PyObject* args, PyObject* kwargs)
{
    PyObject* aggregate_class;

    int n_arg;
    char* name;
    static char *kwlist[] = { "name", "n_arg", "aggregate_class", NULL };
    int rc;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "siO:create_aggregate",
                                      kwlist, &name, &n_arg, &aggregate_class)) {
        return NULL;
    }

    rc = sqlite3_create_function(self->db, name, n_arg, SQLITE_UTF8, (void*)aggregate_class, 0, &_step_callback, &_final_callback);
    if (rc != SQLITE_OK) {
        _seterror(self->db);
        return NULL;
    } else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

int check_thread(Connection* self)
{
    if (self->check_same_thread) {
        if (PyThread_get_thread_ident() != self->thread_ident) {
            PyErr_Format(ProgrammingError,
                        "SQLite objects created in a thread can only be used in that same thread."
                        "The object was created in thread id %ld and this is thread id %ld",
                        self->thread_ident, PyThread_get_thread_ident());
            return 0;
        }

    }

    return 1;
}

static PyObject* connection_get_autocommit(Connection* self, void* unused)
{
    return PyBool_FromLong(self->autocommit);
}

static int connection_set_autocommit(Connection* self, PyObject* value)
{
    PyObject* empty;
    PyObject* res;

    if (PyObject_IsTrue(value)) {
        empty = PyTuple_New(0);
        res = connection_commit(self, empty);
        Py_DECREF(empty);
        Py_DECREF(res);

        self->inTransaction = 0;
        self->autocommit = 1;
    } else {
        self->autocommit = 0;
    }

    return 0;
}


static char connection_doc[] =
PyDoc_STR("<missing docstring>");

static PyGetSetDef connection_getset[] = {
    {"autocommit",  (getter)connection_get_autocommit, (setter)connection_set_autocommit},
    {NULL}
};

static PyMethodDef connection_methods[] = {
    {"cursor", (PyCFunction)connection_cursor, METH_VARARGS|METH_KEYWORDS,
        PyDoc_STR("Return a cursor for the connection.")},
    {"close", (PyCFunction)connection_close, METH_NOARGS,
        PyDoc_STR("Closes the connection.")},
    {"commit", (PyCFunction)connection_commit, METH_NOARGS,
        PyDoc_STR("Commit the current transaction.")},
    {"rollback", (PyCFunction)connection_rollback, METH_NOARGS,
        PyDoc_STR("Roll back the current transaction.")},
    {"create_function", (PyCFunction)connection_create_function, METH_VARARGS|METH_KEYWORDS,
        PyDoc_STR("Creates a new function.")},
    {"create_aggregate", (PyCFunction)connection_create_aggregate, METH_VARARGS|METH_KEYWORDS,
        PyDoc_STR("Creates a new aggregate.")},
    {NULL, NULL}
};

static struct PyMemberDef connection_members[] =
{
    {"Warning", T_OBJECT, offsetof(Connection, Warning), RO},
    {"Error", T_OBJECT, offsetof(Connection, Error), RO},
    {"InterfaceError", T_OBJECT, offsetof(Connection, InterfaceError), RO},
    {"DatabaseError", T_OBJECT, offsetof(Connection, DatabaseError), RO},
    {"DataError", T_OBJECT, offsetof(Connection, DataError), RO},
    {"OperationalError", T_OBJECT, offsetof(Connection, OperationalError), RO},
    {"IntegrityError", T_OBJECT, offsetof(Connection, IntegrityError), RO},
    {"InternalError", T_OBJECT, offsetof(Connection, InternalError), RO},
    {"ProgrammingError", T_OBJECT, offsetof(Connection, ProgrammingError), RO},
    {"NotSupportedError", T_OBJECT, offsetof(Connection, NotSupportedError), RO},
    {NULL}
};

PyTypeObject ConnectionType = {
        PyObject_HEAD_INIT(NULL)
        0,                                              /* ob_size */
        "pysqlite2.dbapi2.Connection",                  /* tp_name */
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
        Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,         /* tp_flags */
        connection_doc,                                 /* tp_doc */
        0,                                              /* tp_traverse */
        0,                                              /* tp_clear */
        0,                                              /* tp_richcompare */
        0,                                              /* tp_weaklistoffset */
        0,                                              /* tp_iter */
        0,                                              /* tp_iternext */
        connection_methods,                             /* tp_methods */
        connection_members,                             /* tp_members */
        connection_getset,                              /* tp_getset */
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
 
