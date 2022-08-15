#include "blob.h"
#include "util.h"


int pysqlite_blob_init(pysqlite_Blob *self, pysqlite_Connection* connection, sqlite3_blob *blob)
{
    Py_INCREF(connection);
    self->connection = connection;
    self->offset=0;
    self->blob = blob;
    self->in_weakreflist = NULL;

    if (!pysqlite_check_thread(self->connection)){
        return -1;
    }
    return 0;
}

static void remove_blob_from_connection_blob_list(pysqlite_Blob* self)
{
    Py_ssize_t i;

    for(i=0;i<PyList_GET_SIZE(self->connection->blobs);i++)
    {
      if(PyWeakref_GetObject(PyList_GET_ITEM(self->connection->blobs, i))==(PyObject *)self)
        {
          PyList_SetSlice(self->connection->blobs, i, i+1, NULL);
          break;
        }
    }
}



static void pysqlite_blob_dealloc(pysqlite_Blob* self)
{
    /* close the blob */
    if (self->blob) {
        Py_BEGIN_ALLOW_THREADS
        sqlite3_blob_close(self->blob);
        Py_END_ALLOW_THREADS
    }

    // remove from connection weaklist
    remove_blob_from_connection_blob_list(self);
    if (self->in_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject*)self);
    }

    Py_XDECREF(self->connection);

    self->blob = NULL;

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
    } else if (!pysqlite_check_connection(blob->connection) || !pysqlite_check_thread(blob->connection)) {
        return 0;
    } else {
        return 1;
    }
}


PyObject* pysqlite_blob_close(pysqlite_Blob *self){
    if (!pysqlite_check_blob(self)){
        return NULL;
    }
    /* close the blob */
    if (self->blob) {
        Py_BEGIN_ALLOW_THREADS
        sqlite3_blob_close(self->blob);
        Py_END_ALLOW_THREADS
    }

    self->blob = NULL;

    // remove from connection weaklist
    remove_blob_from_connection_blob_list(self);
    if (self->in_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject*)self);
    }

    Py_RETURN_NONE;
};


PyObject* pysqlite_blob_length(pysqlite_Blob *self){
    int blob_length;
    if (!pysqlite_check_blob(self)){
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    blob_length = sqlite3_blob_bytes(self->blob);
    Py_END_ALLOW_THREADS

    return PyInt_FromLong(blob_length);
};


PyObject* pysqlite_blob_read(pysqlite_Blob *self, PyObject *args){
    int read_length = -1;
    int blob_length = 0;
    PyObject* buffer;
    char* raw_buffer;
    int rc;

    if (!PyArg_ParseTuple(args, "|i", &read_length)) {
        return NULL;
    }

    if (!pysqlite_check_blob(self)){
        return NULL;
    }


    //TODO: make this multithreaded and safe!
    Py_BEGIN_ALLOW_THREADS
    blob_length = sqlite3_blob_bytes(self->blob);
    Py_END_ALLOW_THREADS

    if (read_length < 0) {
        // same as file read.
        read_length = blob_length;
    }
    
    // making sure we don't read more then blob size
    if (self->offset + read_length > blob_length){
        read_length = blob_length - self->offset;
    }

    buffer = PyBytes_FromStringAndSize(NULL, read_length);
    if (!buffer) {
        return NULL;
    }
    raw_buffer = PyBytes_AS_STRING(buffer);

    Py_BEGIN_ALLOW_THREADS
    rc = sqlite3_blob_read(self->blob, raw_buffer, read_length, self->offset);
    Py_END_ALLOW_THREADS

    if (rc != SQLITE_OK){
        Py_DECREF(buffer);
        // For some reasone after modifieng blob the error is not set on the connection db.
        if (rc == SQLITE_ABORT){
            PyErr_SetString(pysqlite_OperationalError, "Cannot operate on modified blob");
        } else {
            _pysqlite_seterror(self->connection->db, NULL);
        }
        return NULL;
    }

    // update offset.
    self->offset += read_length;
    
    return buffer;
};


PyObject* pysqlite_blob_write(pysqlite_Blob *self, PyObject *data){
    Py_ssize_t data_size;
    char *data_buffer;
    int rc;

    if (PyBytes_AsStringAndSize(data, &data_buffer, &data_size)){
        return NULL;
    }

    if (!pysqlite_check_blob(self)){
        return NULL;
    }

    //TODO: throw better error on data bigger then blob.

    Py_BEGIN_ALLOW_THREADS
    rc = sqlite3_blob_write(self->blob, data_buffer, data_size, self->offset);
    Py_END_ALLOW_THREADS
    if (rc != SQLITE_OK) {
        // For some reasone after modifieng blob the error is not set on the connection db.
        if (rc == SQLITE_ABORT){
            PyErr_SetString(pysqlite_OperationalError, "Cannot operate on modified blob");
        } else {
            _pysqlite_seterror(self->connection->db, NULL);
        }
        return NULL;
    }

    self->offset += (int)data_size;
    Py_RETURN_NONE;
}


PyObject* pysqlite_blob_seek(pysqlite_Blob *self, PyObject *args){
    int blob_length, offset, from_what=0;

    if(!PyArg_ParseTuple(args, "i|i", &offset, &from_what)){
        return NULL;
    }


    if (!pysqlite_check_blob(self)){
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    blob_length = sqlite3_blob_bytes(self->blob);
    Py_END_ALLOW_THREADS

    switch(from_what){
        case 0: //realtive to blob begin
            break;
        case 1: //realtive to current position
            offset = self->offset + offset;
            break;
        case 2: //realtive to blob end
            offset = blob_length + offset;
            break;
        default:
            return PyErr_Format(PyExc_ValueError, "from_what should be 0, 1 or 2");
    }

    if (offset < 0 || offset > blob_length){
        return PyErr_Format(PyExc_ValueError, "offset out of blob range");
    }

    self->offset = offset;
    Py_RETURN_NONE;
};


PyObject* pysqlite_blob_tell(pysqlite_Blob *self){
    if (!pysqlite_check_blob(self)){
        return NULL;
    }

    return PyInt_FromLong(self->offset);
}


PyObject* pysqlite_blob_enter(pysqlite_Blob *self){
    if (!pysqlite_check_blob(self)){
        return NULL;
    }

    Py_INCREF(self);
    return (PyObject *)self;
}


PyObject* pysqlite_blob_exit(pysqlite_Blob *self, PyObject *args){
    PyObject *res;
    if (!pysqlite_check_blob(self)){
        return NULL;
    }

    res = pysqlite_blob_close(self);
    Py_XDECREF(res);
    if (!res) {
        return NULL;
    }

    Py_RETURN_FALSE;
}


static PyMethodDef blob_methods[] = {
    {"length", (PyCFunction)pysqlite_blob_length, METH_NOARGS,
        PyDoc_STR("return blob length")},
    {"read", (PyCFunction)pysqlite_blob_read, METH_VARARGS,
        PyDoc_STR("read data from blob")},
    {"write", (PyCFunction)pysqlite_blob_write, METH_O,
        PyDoc_STR("write data to blob")},
    {"close", (PyCFunction)pysqlite_blob_close, METH_NOARGS,
        PyDoc_STR("close blob")},
    {"seek", (PyCFunction)pysqlite_blob_seek, METH_VARARGS,
        PyDoc_STR("change blob current offset")},
    {"tell", (PyCFunction)pysqlite_blob_tell, METH_NOARGS,
        PyDoc_STR("return blob current offset")},
    {"__enter__", (PyCFunction)pysqlite_blob_enter, METH_NOARGS,
        PyDoc_STR("blob context manager enter")},
    {"__exit__", (PyCFunction)pysqlite_blob_exit, METH_VARARGS,
        PyDoc_STR("blob context manager exit")},
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

