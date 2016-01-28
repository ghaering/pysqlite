#ifndef PYSQLITE_BLOB_H
#define PYSQLITE_BLOB_H
#include "Python.h"
#include "sqlite3.h"
#include "connection.h"

typedef struct
{
    PyObject_HEAD
    pysqlite_Connection* connection;
    sqlite3_blob *blob;
    unsigned int offset;
} pysqlite_Blob;

extern PyTypeObject pysqlite_BlobType;

int pysqlite_blob_init(pysqlite_Blob* self, pysqlite_Connection* connection, sqlite3_blob *blob);

int pysqlite_blob_setup_types(void);

#endif
