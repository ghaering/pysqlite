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
#include "connection.h"

/*
 * it's not so trivial to write a portable sleep in C. For now, the simplest
 * solution is to just use Python's sleep().
 */
void pysqlite_sleep(float seconds)
{
    PyObject* timeModule;
    PyObject* sleepFunc;
    PyObject* ret;

    timeModule = PyImport_AddModule("time");
    sleepFunc = PyObject_GetAttrString(timeModule, "sleep");
    ret = PyObject_CallFunction(sleepFunc, "f", seconds);
    Py_DECREF(ret);
}

double pysqlite_time()
{
    PyObject* timeModule;
    PyObject* timeFunc;
    PyObject* ret;
    double time;

    timeModule = PyImport_AddModule("time");
    timeFunc = PyObject_GetAttrString(timeModule, "time");
    ret = PyObject_CallFunction(timeFunc, "");
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

    connection->timeout_started = pysqlite_time();
    while (1) {
        rc = sqlite3_step(statement);
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

