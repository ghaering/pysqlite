/* statement.c - the statement type
 *
 * Copyright (C) 2005 Gerhard Häring <gh@ghaering.de>
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

#include "statement.h"
#include "connection.h"

int statement_create(Statement* self, Connection* connection, unsigned char* sql)
{
    const unsigned char* tail;
    int rc;

    self->st = NULL;

    self->st = NULL;
    self->in_use = 0;

    rc = sqlite3_prepare(connection->db,
                         sql,
                         0,
                         &self->st,
                         &tail);

    return rc;
}

int pysqlite_finalize(Statement* self)
{
    int rc;

    rc = SQLITE_OK;
    if (self->st) {
        Py_BEGIN_ALLOW_THREADS
        rc = sqlite3_finalize(self->st);
        Py_END_ALLOW_THREADS
        self->st = NULL;
    }

    self->in_use = 0;

    return rc;
}

int pysqlite_reset(Statement* self)
{
    int rc;

    rc = SQLITE_OK;
    if (self->st) {
        Py_BEGIN_ALLOW_THREADS
        rc = sqlite3_reset(self->st);
        Py_END_ALLOW_THREADS
    }

    self->in_use = 0;

    return rc;
}

void statement_dealloc(Statement* self)
{
    int rc;

    if (self->st) {
        Py_BEGIN_ALLOW_THREADS
        rc = sqlite3_finalize(self->st);
        Py_END_ALLOW_THREADS
    }

    self->st = NULL;

    self->ob_type->tp_free((PyObject*)self);
}

PyTypeObject StatementType = {
        PyObject_HEAD_INIT(NULL)
        0,                                              /* ob_size */
        "pysqlite2.dbapi2.Statement",                   /* tp_name */
        sizeof(Statement),                              /* tp_basicsize */
        0,                                              /* tp_itemsize */
        (destructor)statement_dealloc,                  /* tp_dealloc */
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
        0,                                              /* tp_doc */
        0,                                              /* tp_traverse */
        0,                                              /* tp_clear */
        0,                                              /* tp_richcompare */
        0,                                              /* tp_weaklistoffset */
        0,                                              /* tp_iter */
        0,                                              /* tp_iternext */
        0,                                              /* tp_methods */
        0,                                              /* tp_members */
        0,                                              /* tp_getset */
        0,                                              /* tp_base */
        0,                                              /* tp_dict */
        0,                                              /* tp_descr_get */
        0,                                              /* tp_descr_set */
        0,                                              /* tp_dictoffset */
        (initproc)0,                                    /* tp_init */
        0,                                              /* tp_alloc */
        0,                                              /* tp_new */
        0                                               /* tp_free */
};
