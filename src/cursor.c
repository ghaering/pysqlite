/* cursor.c - the cursor type
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
#include "util.h"
#include "microprotocols.h"
#include "microprotocols_proto.h"
#include "prepare_protocol.h"

/* used to decide wether to call PyInt_FromLong or PyLong_FromLongLong */
#define INT32_MIN (-2147483647 - 1)
#define INT32_MAX 2147483647

PyObject* cursor_iternext(Cursor *self);

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

    printf("cursor init\n");
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist))
    {
        return -1; 
    }

    printf("initializing\n");
    self->row_cast_map = PyList_New(0);

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

void build_row_cast_map(Cursor* self)
{
    int i;
    const unsigned char* colname;
    const unsigned char* decltype;
    PyObject* converter = NULL;
    PyObject* typename;

    Py_DECREF(self->row_cast_map);
    self->row_cast_map = PyList_New(0);

    for (i = 0; i < sqlite3_column_count(self->statement); i++) {
        colname = sqlite3_column_name(self->statement, i);
        /* TODO: toupper() */
        typename = PyDict_GetItemString(self->coltypes, colname);
        if (typename) {
            converter = PyDict_GetItem(self->connection->converters, typename);
        } else {
            decltype = sqlite3_column_decltype(self->statement, i);
            if (decltype) {
                converter = PyDict_GetItemString(self->connection->converters, decltype);
            }
        }

        if (converter) {
            Py_INCREF(converter);
            PyList_Append(self->row_cast_map, converter);
        } else {
            Py_INCREF(Py_None);
            PyList_Append(self->row_cast_map, Py_None);
        }
    }
}

int _bind_parameter(Cursor* self, int pos, PyObject* parameter)
{
    int rc = SQLITE_OK;
    long longval;
    const char* buffer;
    int buflen;
    PyObject* stringval;

    if (parameter == Py_None) {
        rc = sqlite3_bind_null(self->statement, pos);
    } else if (PyInt_Check(parameter)) {
        longval = PyInt_AsLong(parameter);
        /* TODO: investigate what to do with range overflows - long vs. long long */
        rc = sqlite3_bind_int64(self->statement, pos, (long long)longval);
    } else if (PyFloat_Check(parameter)) {
        rc = sqlite3_bind_double(self->statement, pos, PyFloat_AsDouble(parameter));
    } else if (PyBuffer_Check(parameter)) {
        if (PyObject_AsCharBuffer(parameter, &buffer, &buflen) != 0) {
            /* TODO: decide what error to raise */
            PyErr_SetString(PyExc_ValueError, "could not convert BLOB to buffer");
            return -1;
        }
        rc = sqlite3_bind_blob(self->statement, pos, buffer, buflen, SQLITE_TRANSIENT);
    } else {
        if (!PyString_Check(parameter)) {
            PyErr_SetString(InternalError, "Could not convert parameter value to string.");
        } else {
            stringval = PyObject_Str(parameter);
            /*
            if (PyUnicode_Check(parameter)) {
                stringval = PyUnicode_AsUTF8String(parameter);
            } else {
                stringval = PyObject_Str(parameter);
            }
            */

            if (stringval)  {
                rc = sqlite3_bind_text(self->statement, pos, PyString_AsString(stringval), -1, SQLITE_TRANSIENT);
            } else {
                PyErr_SetString(InternalError, "Could not convert parameter value to string.");
            }
        }
    }

    return rc;
}

PyObject* _query_execute(Cursor* self, int multiple, PyObject* args)
{
    PyObject* operation;
    char* operation_cstr;
    PyObject* parameters_list;
    PyObject* parameters_iter;
    PyObject* parameters;
    int num_params;
    const char* tail;
    int i;
    int rc;
    PyObject* func_args;
    PyObject* result;
    int numcols;
    /*int coltype;*/
    int statement_type;
    PyObject* descriptor;
    PyObject* current_param;
    PyObject* adapted;
    PyObject* second_argument = NULL;
    int num_params_needed;
    const char* binding_name;

    if (!check_thread(self->connection)) {
        return NULL;
    }

    if (multiple) {
        /* executemany() */
        if (!PyArg_ParseTuple(args, "SO", &operation, &second_argument)) {
            return NULL; 
        }

        if (PyIter_Check(second_argument)) {
            /* iterator */
            Py_INCREF(second_argument);
            parameters_iter = second_argument;
        } else {
            /* sequence */
            parameters_iter = PyObject_GetIter(second_argument);
            if (PyErr_Occurred())
            {
                return NULL;
            }
        }
    } else {
        /* execute() */
        if (!PyArg_ParseTuple(args, "S|O", &operation, &second_argument)) {
            return NULL; 
        }

        parameters_list = PyList_New(0);
        if (!parameters_list) {
            return NULL;
        }

        if (second_argument == NULL) {
            Py_INCREF(Py_None);
            second_argument = Py_None;
        }
        PyList_Append(parameters_list, second_argument);

        parameters_iter = PyObject_GetIter(parameters_list);
    }

    if (self->statement != NULL) {
        /* There is an active statement */
        if (sqlite3_finalize(self->statement) != SQLITE_OK) {
            _seterror(self->connection->db);
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

    Py_DECREF(self->coltypes);
    self->coltypes = self->next_coltypes;
    Py_INCREF(self->coltypes);
    Py_DECREF(self->next_coltypes);
    Py_INCREF(Py_None);
    self->next_coltypes = Py_None;

    statement_type = detect_statement_type(operation_cstr);
    if (!self->connection->inTransaction) {
        switch (statement_type) {
            case STATEMENT_UPDATE:
            case STATEMENT_DELETE:
            case STATEMENT_INSERT:
                func_args = PyTuple_New(0);
                if (!args)
                    return NULL;
                result = _connection_begin(self->connection, func_args);
                Py_DECREF(func_args);
                if (!result) {
                    return NULL;
                }
            case STATEMENT_SELECT:
                /* TODO: raise error in executemany() case */
                break;
        }
    }

    rc = sqlite3_prepare(self->connection->db,
                         operation_cstr,
                         0,
                         &self->statement,
                         &tail);
    if (rc != SQLITE_OK) {
        _seterror(self->connection->db);
        return NULL;
    }

    num_params_needed = sqlite3_bind_parameter_count(self->statement);

    while (1) {
        parameters = PyIter_Next(parameters_iter);
        if (!parameters) {
            break;
        }

        if (parameters == Py_None && num_params_needed > 0) {
            PyErr_Format(ProgrammingError, "Incorrect number of bindings supplied.  The current statement uses %d, but there are none supplied.",
                         num_params_needed);
            return NULL;
        }

        if (parameters != Py_None) {
            if (PyDict_Check(parameters)) {
                /* parameters passed as dictionary */
                for (i = 1; i <= num_params_needed; i++) {
                    binding_name = sqlite3_bind_parameter_name(self->statement, i);
                    if (!binding_name) {
                        PyErr_Format(ProgrammingError, "Binding %d has no name, but you supplied a dictionary (which has only names).", i);
                        return NULL;
                    }

                    binding_name++; /* skip first char (the colon) */
                    current_param = PyDict_GetItemString(parameters, binding_name);
                    if (!current_param) {
                        PyErr_Format(ProgrammingError, "You did not supply a value for binding %d.", i);
                        return NULL;
                    }


                    Py_INCREF(current_param);
                    adapted = microprotocols_adapt(current_param, (PyObject*)self->connection->prepareProtocol, NULL);
                    Py_DECREF(current_param);
                    if (adapted) {
                    } else {
                        PyErr_Clear();
                        adapted = current_param;
                        Py_INCREF(adapted);
                    }

                    rc = _bind_parameter(self, i, adapted);
                    if (rc != SQLITE_OK) {
                        /* TODO: fail */
                    }

                    Py_DECREF(adapted);
                }
            } else {
                /* parameters passed as sequence */
                num_params = PySequence_Length(parameters);
                if (num_params != num_params_needed) {
                    PyErr_Format(ProgrammingError, "Incorrect number of bindings supplied.  The current statement uses %d, and there are %d supplied.",
                                 num_params_needed, num_params);
                    return NULL;
                }
                for (i = 0; i < num_params; i++) {
                    current_param = PySequence_GetItem(parameters, i);

                    Py_INCREF(current_param);
                    adapted = microprotocols_adapt(current_param, (PyObject*)self->connection->prepareProtocol, NULL);
                    Py_DECREF(current_param);
                    if (adapted) {
                    } else {
                        PyErr_Clear();
                        adapted = current_param;
                        Py_INCREF(adapted);
                    }

                    rc = _bind_parameter(self, i + 1, adapted);
                    if (rc != SQLITE_OK) {
                        /* TODO: fail */
                    }

                    Py_DECREF(adapted);
                }
            }
        }

        if (PyErr_Occurred()) {
            return NULL;
        }

        build_row_cast_map(self);

        self->step_rc = _sqlite_step_with_busyhandler(self->statement, self->connection);
        if (self->step_rc != SQLITE_DONE && self->step_rc != SQLITE_ROW) {
            _seterror(self->connection->db);
            return NULL;
        }

        if (self->step_rc == SQLITE_ROW) {
            if (multiple) {
                /* TODO: raise error */
            }

            numcols = sqlite3_data_count(self->statement);

            if (self->description == Py_None) {
                Py_DECREF(self->description);
                self->description = PyTuple_New(numcols);
                for (i = 0; i < numcols; i++) {
                    descriptor = PyTuple_New(7);
                    PyTuple_SetItem(descriptor, 0, PyString_FromString(sqlite3_column_name(self->statement, i)));
                    Py_INCREF(Py_None); PyTuple_SetItem(descriptor, 1, Py_None);
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

        Py_DECREF(self->lastrowid);
        if (statement_type == STATEMENT_INSERT) {
            self->lastrowid = PyInt_FromLong((long)sqlite3_last_insert_rowid(self->connection->db));
        } else {
            Py_INCREF(Py_None);
            self->lastrowid = Py_None;
        }

        if (multiple) {
            rc = sqlite3_reset(self->statement);
        }
    }

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* cursor_execute(Cursor* self, PyObject* args)
{
    return _query_execute(self, 0, args);
}

PyObject* cursor_executemany(Cursor* self, PyObject* args)
{
    return _query_execute(self, 1, args);
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
    long long intval;
    PyObject* converter;
    PyObject* converted;
    int nbytes;
    PyObject* buffer;
    void* raw_buffer;

    if (!check_thread(self->connection)) {
        return NULL;
    }

    if (self->statement == NULL) {
        PyErr_SetString(ProgrammingError, "no compiled statement - you need to execute() before you can fetch data");
        return NULL;
    }

    /* TODO: handling of step_rc here. it would be nicer if the hack with
             UNKNOWN were not necessary
    */
    if (self->step_rc == UNKNOWN) {
        self->step_rc = _sqlite_step_with_busyhandler(self->statement, self->connection);
    }

    if (self->step_rc == SQLITE_DONE) {
        return NULL;
    } else if (self->step_rc != SQLITE_ROW) {
        PyErr_SetString(DatabaseError, "wrong return code :-S");
        return NULL;
    }

    numcols = sqlite3_data_count(self->statement);

    row = PyTuple_New(numcols);

    for (i = 0; i < numcols; i++) {
        if (!self->connection->advancedTypes) {
            /* Use SQLite's primitive type system (default) */
            coltype = sqlite3_column_type(self->statement, i);
            if (coltype == SQLITE_NULL) {
                Py_INCREF(Py_None);
                converted = Py_None;
            } else if (coltype == SQLITE_INTEGER) {
                intval = sqlite3_column_int64(self->statement, i);
                if (intval < INT32_MIN || intval > INT32_MAX) {
                    converted = PyLong_FromLongLong(intval);
                } else {
                    converted = PyInt_FromLong((long)intval);
                }
            } else if (coltype == SQLITE_FLOAT) {
                converted = PyFloat_FromDouble(sqlite3_column_double(self->statement, i));
            } else if (coltype == SQLITE_TEXT) {
                converted = PyString_FromString(sqlite3_column_text(self->statement, i));
            } else if (coltype == SQLITE_BLOB) {
                nbytes = sqlite3_column_bytes(self->statement, i);
                buffer = PyBuffer_New(nbytes);
                if (!buffer) {
                    /* TODO: error */
                }
                if (PyObject_AsWriteBuffer(buffer, &raw_buffer, &nbytes)) {
                    /* TODO: error */
                }
                memcpy(raw_buffer, sqlite3_column_blob(self->statement, i), nbytes);
                converted = buffer;
            } else {
                /* TODO: BLOB */
                Py_INCREF(Py_None);
                converted = Py_None;
            }
        } else {
            /* advanced type system */
            coltype = sqlite3_column_type(self->statement, i);
            if (coltype == SQLITE_NULL) {
                Py_INCREF(Py_None);
                converted = Py_None;
            } else {
                converter = PyList_GetItem(self->row_cast_map, i);
                if (!converter) {
                    PyErr_SetString(InternalError, "no converter found");
                    return NULL;
                }

                if (converter == Py_None) {
                    converted = PyString_FromString(sqlite3_column_text(self->statement, i));
                } else {
                    item = PyString_FromString(sqlite3_column_text(self->statement, i));
                    converted = PyObject_CallFunction(converter, "O", item);
                    if (!converted) {
                        /* TODO: have a way to log these errors */
                        Py_INCREF(Py_None);
                        converted = Py_None;
                        PyErr_Clear();
                    }
                    Py_DECREF(item);
                }
            }
        }

        PyTuple_SetItem(row, i, converted);
    }

    self->step_rc = UNKNOWN;

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
        } else {
            break;
        }

        if (++counter == maxrows) {
            break;
        }
    }

    if (PyErr_Occurred()) {
        Py_DECREF(list);
        return NULL;
    } else {
        return list;
    }
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

    if (PyErr_Occurred()) {
        Py_DECREF(list);
        return NULL;
    } else {
        return list;
    }
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

    if (!check_thread(self->connection)) {
        return NULL;
    }

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
    {"lastrowid", T_OBJECT, offsetof(Cursor, lastrowid), RO},
    {"rowcount", T_OBJECT, offsetof(Cursor, rowcount), RO},
    {"coltypes", T_OBJECT, offsetof(Cursor, next_coltypes), 0},
    {NULL}
};

PyTypeObject CursorType = {
        PyObject_HEAD_INIT(NULL)
        0,                                              /* ob_size */
        "pysqlite2.dbapi2.Cursor",                      /* tp_name */
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
        0,                                              /* tp_getattro */
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
