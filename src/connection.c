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

    self->ob_type->tp_free((PyObject*)self);
}

PyObject* connection_cursor(Connection* self, PyObject* args)
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
    cursor->rowcount = Py_None;

    return (PyObject*)cursor;
}

PyObject* connection_close(Connection* self, PyObject* args)
{
    int rc;

    if (self->db) {
        rc = sqlite3_close(self->db);
        if (rc != SQLITE_OK) {
            PyErr_SetString(sqlite_DatabaseError, sqlite3_errmsg(self->db));
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

PyObject* connection_commit(Connection* self, PyObject* args)
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

PyObject* connection_rollback(Connection* self, PyObject* args)
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
 
