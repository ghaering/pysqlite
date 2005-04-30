/* util.c - various utility functions
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

#include "util.h"
#include "module.h"
#include "connection.h"

/*
 * it's not so trivial to write a portable sleep in C. For now, the simplest
 * solution is to just use Python's sleep().
 */
void pysqlite_sleep(float seconds)
{
    PyObject* ret;

    ret = PyObject_CallFunction(time_sleep, "f", seconds);
    Py_DECREF(ret);
}

double pysqlite_time()
{
    PyObject* ret;
    double time;

    ret = PyObject_CallFunction(time_time, "");
    time = PyFloat_AsDouble(ret);
    Py_DECREF(ret);

    return time;
}

/* TODO: find a way to avoid the circular dependency connection.h <-> util.h
 * and the stupid casting */
int _sqlite_step_with_busyhandler(sqlite3_stmt* statement, void* _connection)
{
    Connection* connection = (Connection*)_connection;
    int counter = 0;
    int rc;
    double how_long;

    Py_BEGIN_ALLOW_THREADS
    rc = sqlite3_step(statement);
    Py_END_ALLOW_THREADS

    if (rc != SQLITE_BUSY) {
        return rc;
    }

    connection->timeout_started = pysqlite_time();
    while (1) {
        Py_BEGIN_ALLOW_THREADS
        rc = sqlite3_step(statement);
        Py_END_ALLOW_THREADS
        if (rc != SQLITE_BUSY) {
            break;
        }

        if (pysqlite_time() - connection->timeout_started > connection->timeout) {
            break;
        }

        how_long = 0.01* (1 << counter);
        pysqlite_sleep(how_long);

        counter++;
    }

    return rc;
}

/**
 * Checks the SQLite error code and sets the appropriate DB-API exception.
 * Returns the error code (0 means no error occured).
 */
int _seterror(sqlite3* db)
{
    int errorcode;

    errorcode = sqlite3_errcode(db);

    switch (errorcode)
    {
        case SQLITE_OK:
            PyErr_Clear();
            break;
        case SQLITE_ERROR:
            PyErr_SetString(DatabaseError, sqlite3_errmsg(db));
            break;
        case SQLITE_INTERNAL:
            PyErr_SetString(InternalError, sqlite3_errmsg(db));
            break;
        case SQLITE_PERM:
            PyErr_SetString(OperationalError, sqlite3_errmsg(db));
            break;
        case SQLITE_ABORT:
            PyErr_SetString(OperationalError, sqlite3_errmsg(db));
            break;
        case SQLITE_BUSY:
            PyErr_SetString(OperationalError, sqlite3_errmsg(db));
            break;
        case SQLITE_LOCKED:
            PyErr_SetString(OperationalError, sqlite3_errmsg(db));
            break;
        case SQLITE_NOMEM:
            (void)PyErr_NoMemory();
            break;
        case SQLITE_READONLY:
            PyErr_SetString(DatabaseError, sqlite3_errmsg(db));
            break;
        case SQLITE_INTERRUPT:
            PyErr_SetString(OperationalError, sqlite3_errmsg(db));
            break;
        case SQLITE_IOERR:
            PyErr_SetString(OperationalError, sqlite3_errmsg(db));
            break;
        case SQLITE_CORRUPT:
            PyErr_SetString(DatabaseError, sqlite3_errmsg(db));
            break;
        case SQLITE_NOTFOUND:
            PyErr_SetString(InternalError, sqlite3_errmsg(db));
            break;
        case SQLITE_FULL:
            PyErr_SetString(DatabaseError, sqlite3_errmsg(db));
            break;
        case SQLITE_CANTOPEN:
            PyErr_SetString(DatabaseError, sqlite3_errmsg(db));
            break;
        case SQLITE_PROTOCOL:
            PyErr_SetString(OperationalError, sqlite3_errmsg(db));
            break;
        case SQLITE_EMPTY:
            PyErr_SetString(InternalError, sqlite3_errmsg(db));
            break;
        case SQLITE_SCHEMA:
            PyErr_SetString(DatabaseError, sqlite3_errmsg(db));
            break;
        case SQLITE_TOOBIG:
            PyErr_SetString(DataError, sqlite3_errmsg(db));
            break;
        case SQLITE_CONSTRAINT:
            PyErr_SetString(IntegrityError, sqlite3_errmsg(db));
            break;
        case SQLITE_MISMATCH:
            PyErr_SetString(IntegrityError, sqlite3_errmsg(db));
            break;
        case SQLITE_MISUSE:
            PyErr_SetString(ProgrammingError, sqlite3_errmsg(db));
            break;
        default:
            PyErr_SetString(DatabaseError, sqlite3_errmsg(db));
    }

    return errorcode;
}

