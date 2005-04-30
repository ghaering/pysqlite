/* prepare_protocol.c - the protocol for preparing values for SQLite
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

#include "prepare_protocol.h"

int prepare_protocol_init(SQLitePrepareProtocol* self, PyObject* args, PyObject* kwargs)
{
    return 0;
}

void prepare_protocol_dealloc(SQLitePrepareProtocol* self)
{
    self->ob_type->tp_free((PyObject*)self);
}

PyObject* prepare_protocol_adapt(SQLitePrepareProtocol* self, PyObject* args, PyObject* kwargs)
{
    PyObject* obj;
    PyObject* ret;

    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return NULL;
    }

    /* These types are supported already for the manifest typing, no need to convert them: */
    if (obj == Py_None
     || PyString_Check(obj)
     || PyUnicode_Check(obj)
     || PyInt_Check(obj)
     || PyLong_Check(obj)
     || PyFloat_Check(obj)
     || PyBuffer_Check(obj)) {
        Py_INCREF(obj);
        ret = obj;
    } else {
        Py_INCREF(Py_None);
        ret = Py_None;
    }

    return ret;
}

static PyMethodDef prepare_protocol_methods[] = {
    {"__adapt__", (PyCFunction)prepare_protocol_adapt, METH_VARARGS,
        PyDoc_STR("Adapts an object to the protocol.")},
    {NULL, NULL}
};

PyTypeObject SQLitePrepareProtocolType= {
        PyObject_HEAD_INIT(NULL)
        0,                                              /* ob_size */
        "pysqlite2.dbapi2.SQLitePrepareProtocol",       /* tp_name */
        sizeof(SQLitePrepareProtocol),                  /* tp_basicsize */
        0,                                              /* tp_itemsize */
        (destructor)prepare_protocol_dealloc,           /* tp_dealloc */
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
        0,                                              /* tp_doc */
        0,                                              /* tp_traverse */
        0,                                              /* tp_clear */
        0,                                              /* tp_richcompare */
        0,                                              /* tp_weaklistoffset */
        0,                                              /* tp_iter */
        0,                                              /* tp_iternext */
        prepare_protocol_methods,                       /* tp_methods */
        0,                                              /* tp_members */
        0,                                              /* tp_getset */
        0,                                              /* tp_base */
        0,                                              /* tp_dict */
        0,                                              /* tp_descr_get */
        0,                                              /* tp_descr_set */
        0,                                              /* tp_dictoffset */
        (initproc)prepare_protocol_init,                /* tp_init */
        0,                                              /* tp_alloc */
        0,                                              /* tp_new */
        0                                               /* tp_free */
};
