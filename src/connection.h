/* connection.h - definitions for the connection type
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

#ifndef PYSQLITE_CONNECTION_H
#define PYSQLITE_CONNECTION_H
#include "Python.h"
#include "structmember.h"

#include "util.h"
#include "module.h"

#include "sqlite3.h"

typedef struct
{
    PyObject_HEAD
    sqlite3* db;
    int inTransaction;
    int advancedTypes;

    /* the timeout value in seconds for database locks */
    double timeout;

    /* for internal use in the timeout handler: when did the timeout handler
     * first get called with count=0? */
    double timeout_started;

    /* 0: normal mode; 1: "autocommit" mode */
    int no_implicit_begin;

    int check_same_thread;
    int thread_ident;

    PyObject* prepareProtocol;

    /* A dictionary, mapping colum types (INTEGER, VARCHAR, etc.) to converter
     * functions, that convert the SQL value to the appropriate Python value.
     * The key is uppercase.
     */
    PyObject* converters;
} Connection;

extern PyTypeObject ConnectionType;

PyObject* connection_alloc(PyTypeObject* type, int aware);
void connection_dealloc(Connection* self);
PyObject* connection_cursor(Connection* self, PyObject* args);
PyObject* connection_close(Connection* self, PyObject* args);
PyObject* _connection_begin(Connection* self, PyObject* args);
PyObject* connection_begin(Connection* self, PyObject* args);
PyObject* connection_commit(Connection* self, PyObject* args);
PyObject* connection_rollback(Connection* self, PyObject* args);
PyObject* connection_new(PyTypeObject* type, PyObject* args, PyObject* kw);
int connection_init(Connection* self, PyObject* args, PyObject* kwargs);

int check_thread(Connection* self);

#endif
