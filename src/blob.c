#include "blob.h"


int pysqlite_blob_init(pysqlite_Blob *self, pysqlite_Connection* connection, sqlite3_blob *blob)
{
    Py_INCREF(connection);
    self->connection = connection;
    self->offset=0;
    self->in_weakreflist = NULL;
    self->blob = blob;

    // TODO: register blob
    //if (!pysqlite_connection_register_cursor(connection, (PyObject*)self)) {
    //    return -1;
    //}
    return 0;
}


static void pysqlite_blob_dealloc(pysqlite_Blob* self)
{
    /* close the blob */
    if (self->blob) {
        Py_BEGIN_ALLOW_THREADS
        sqlite3_blob_close(self->blob);
        Py_END_ALLOW_THREADS
    }

    self->blob = NULL;

    if (self->in_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject*)self);
    }
    //TODO: remove from connection blob list

    self->ob_type->tp_free((PyObject*)self);
}


/*
 * Checks if a blob object is usable (i. e. not closed).
 *
 * 0 => error; 1 => ok
 */
int pysqlite_check_blob(pysqlite_Blob* blob)
{
    if (!blob->blob) {
        PyErr_SetString(pysqlite_ProgrammingError, "Cannot operate on a closed blob.");
        return 0;
    } else {
        return 1;
    }
}


PyObject* pysqlite_blob_test(pysqlite_Blob *self){
    printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n");
    Py_INCREF(Py_None);
    return Py_None;
}


PyObject* pysqlite_blob_length(pysqlite_Blob *self){
    if (!pysqlite_check_blob(self))
        return NULL;

    //TODO: consider using PyLong
    return PyInt_FromLong(sqlite3_blob_bytes(self->blob));
};


static PyMethodDef blob_methods[] = {
    {"test", (PyCFunction)pysqlite_blob_test, METH_NOARGS,
        PyDoc_STR("test")},
    {"length", (PyCFunction)pysqlite_blob_length, METH_NOARGS,
        PyDoc_STR("return blob length")},
    {NULL, NULL}
};


PyTypeObject pysqlite_BlobType = {
        PyVarObject_HEAD_INIT(NULL, 0)
        MODULE_NAME ".Blob",                            /* tp_name */
        sizeof(pysqlite_Blob),                          /* tp_basicsize */
        0,                                              /* tp_itemsize */
        (destructor)pysqlite_blob_dealloc,              /* tp_dealloc */
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
        Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_WEAKREFS,    /* tp_flags */
        0,                                              /* tp_doc */
        0,                                              /* tp_traverse */
        0,                                              /* tp_clear */
        0,                                              /* tp_richcompare */
        offsetof(pysqlite_Blob, in_weakreflist),        /* tp_weaklistoffset */
        0,                                              /* tp_iter */
        0,                                              /* tp_iternext */
        blob_methods,                                   /* tp_methods */
        0,                                              /* tp_members */
        0,                                              /* tp_getset */
        0,                                              /* tp_base */
        0,                                              /* tp_dict */
        0,                                              /* tp_descr_get */
        0,                                              /* tp_descr_set */
        0,                                              /* tp_dictoffset */
        (initproc)pysqlite_blob_init,                   /* tp_init */
        0,                                              /* tp_alloc */
        0,                                              /* tp_new */
        0                                               /* tp_free */
};

extern int pysqlite_blob_setup_types(void)
{
    pysqlite_BlobType.tp_new = PyType_GenericNew;
    return PyType_Ready(&pysqlite_BlobType);
}

