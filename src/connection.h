#ifndef PYSQLITE_CONNECTION_H
#define PYSQLITE_CONNECTION_H
#include "Python.h"
#include "structmember.h"

#include "sqlite3.h"

typedef struct
{
    PyObject_HEAD
    sqlite3* db;
    int inTransaction;
} Connection;

extern PyTypeObject ConnectionType;

PyObject* connection_alloc(PyTypeObject* type, int aware);
void connection_dealloc(Connection* self);
PyObject* connection_cursor(Connection* self, PyObject* args);
PyObject* connection_close(Connection* self, PyObject* args);
PyObject* connection_begin(Connection* self, PyObject* args);
PyObject* connection_commit(Connection* self, PyObject* args);
PyObject* connection_rollback(Connection* self, PyObject* args);
PyObject* connection_new(PyTypeObject* type, PyObject* args, PyObject* kw);
int connection_init(Connection* self, PyObject* args, PyObject* kwargs);
#endif
