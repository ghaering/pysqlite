#ifndef PYSQLITE_CURSOR_H
#define PYSQLITE_CURSOR_H
#include "Python.h"

#include "connection.h"
#include "module.h"

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

extern PyTypeObject CursorType;

int cursor_init(Cursor* self, PyObject* args, PyObject* kwargs);
void cursor_dealloc(Cursor* self);
PyObject* cursor_execute(Cursor* self, PyObject* args);
PyObject* cursor_executemany(Cursor* self, PyObject* args);
PyObject* cursor_getiter(Cursor *self);
PyObject* cursor_iternext(Cursor *self);
PyObject* cursor_fetchone(Cursor* self, PyObject* args);
PyObject* cursor_fetchmany(Cursor* self, PyObject* args);
PyObject* cursor_fetchall(Cursor* self, PyObject* args);
PyObject* pysqlite_noop(Connection* self, PyObject* args);
PyObject* cursor_close(Cursor* self, PyObject* args);

#define UNKNOWN (-1)
#endif
