/* microprotocol_proto.c - psycopg protocols
 *
 * Copyright (C) 2003-2004 Federico Di Gregorio <fog@debian.org>
 *
 * This file is part of psycopg and was adapted for pysqlite. Federico Di
 * Gregorio gave the permission to use it within pysqlite under the following
 * license:
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

#include <Python.h>
#include <structmember.h>
#include <stringobject.h>

#include <string.h>

#include "microprotocols_proto.h"


/** void protocol implementation **/

/* getquoted - return quoted representation for object */

#define psyco_isqlquote_getquoted_doc \
"getquoted() -> return SQL-quoted representation of this object"

static PyObject *
psyco_isqlquote_getquoted(isqlquoteObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) return NULL;
    
    Py_INCREF(Py_None);
    return Py_None;
}

/* getbinary - return quoted representation for object */

#define psyco_isqlquote_getbinary_doc \
"getbinary() -> return SQL-quoted binary representation of this object"

static PyObject *
psyco_isqlquote_getbinary(isqlquoteObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) return NULL;
    
    Py_INCREF(Py_None);
    return Py_None;
}

/* getbuffer - return quoted representation for object */

#define psyco_isqlquote_getbuffer_doc \
"getbuffer() -> return this object"

static PyObject *
psyco_isqlquote_getbuffer(isqlquoteObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) return NULL;
    
    Py_INCREF(Py_None);
    return Py_None;
}



/** the ISQLQuote object **/


/* object method list */

static struct PyMethodDef isqlquoteObject_methods[] = {
    {"getquoted", (PyCFunction)psyco_isqlquote_getquoted,
     METH_VARARGS, psyco_isqlquote_getquoted_doc},
    {"getbinary", (PyCFunction)psyco_isqlquote_getbinary,
     METH_VARARGS, psyco_isqlquote_getbinary_doc},
    {"getbuffer", (PyCFunction)psyco_isqlquote_getbuffer,
     METH_VARARGS, psyco_isqlquote_getbuffer_doc},
    {NULL}
};

/* object member list */

static struct PyMemberDef isqlquoteObject_members[] = {
    /* DBAPI-2.0 extensions (exception objects) */
    {"_wrapped", T_OBJECT, offsetof(isqlquoteObject, wrapped), RO},
    {NULL}
};

/* initialization and finalization methods */

static int
isqlquote_setup(isqlquoteObject *self, PyObject *wrapped)
{
    self->wrapped = wrapped;
    Py_INCREF(wrapped);

    return 0;
}

static void
isqlquote_dealloc(PyObject* obj)
{
    isqlquoteObject *self = (isqlquoteObject *)obj;
    
    Py_XDECREF(self->wrapped);
    
    obj->ob_type->tp_free(obj);
}

static int
isqlquote_init(PyObject *obj, PyObject *args, PyObject *kwds)
{
    PyObject *wrapped = NULL;

    if (!PyArg_ParseTuple(args, "O", &wrapped))
        return -1;

    return isqlquote_setup((isqlquoteObject *)obj, wrapped);
}

static PyObject *
isqlquote_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    return type->tp_alloc(type, 0);
}

static void
isqlquote_del(PyObject* self)
{
    PyObject_Del(self);
}


/* object type */

#define isqlquoteType_doc \
"Abstract ISQLQuote protocol"

PyTypeObject isqlquoteType = {
    PyObject_HEAD_INIT(NULL)
    0,
    "psycopg.ISQLQuote",
    sizeof(isqlquoteObject),
    0,
    isqlquote_dealloc, /*tp_dealloc*/
    0,          /*tp_print*/ 
    0,          /*tp_getattr*/
    0,          /*tp_setattr*/
    0,          /*tp_compare*/
    0,          /*tp_repr*/
    0,          /*tp_as_number*/
    0,          /*tp_as_sequence*/
    0,          /*tp_as_mapping*/
    0,          /*tp_hash */

    0,          /*tp_call*/
    0,          /*tp_str*/
    0,          /*tp_getattro*/
    0,          /*tp_setattro*/
    0,          /*tp_as_buffer*/

    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /*tp_flags*/
    isqlquoteType_doc, /*tp_doc*/
    
    0,          /*tp_traverse*/
    0,          /*tp_clear*/

    0,          /*tp_richcompare*/
    0,          /*tp_weaklistoffset*/

    0,          /*tp_iter*/
    0,          /*tp_iternext*/

    /* Attribute descriptor and subclassing stuff */

    isqlquoteObject_methods, /*tp_methods*/
    isqlquoteObject_members, /*tp_members*/
    0,          /*tp_getset*/
    0,          /*tp_base*/
    0,          /*tp_dict*/
    
    0,          /*tp_descr_get*/
    0,          /*tp_descr_set*/
    0,          /*tp_dictoffset*/
    
    isqlquote_init, /*tp_init*/
#if 0
    PyType_GenericAlloc, /*tp_alloc*/
#else
    0,                   /* tp_alloc */
#endif
    isqlquote_new, /*tp_new*/
    (freefunc)isqlquote_del, /*tp_free  Low-level free-memory routine */
    0,          /*tp_is_gc For PyObject_IS_GC */
    0,          /*tp_bases*/
    0,          /*tp_mro method resolution order */
    0,          /*tp_cache*/
    0,          /*tp_subclasses*/
    0           /*tp_weaklist*/
};

extern void microprotocols_proto_init()
{
    isqlquoteType.tp_alloc = PyType_GenericAlloc;
}
