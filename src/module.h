#ifndef PYSQLITE_MODULE_H
#define PYSQLITE_MODULE_H
#include "Python.h"

/**
 * Exception objects
 */
PyObject* sqlite_Error;
PyObject* sqlite_Warning;
PyObject* sqlite_InterfaceError;
PyObject* sqlite_DatabaseError;
PyObject* sqlite_InternalError;
PyObject* sqlite_OperationalError;
PyObject* sqlite_ProgrammingError;
PyObject* sqlite_IntegrityError;
PyObject* sqlite_DataError;
PyObject* sqlite_NotSupportedError;

/**
 * Type objects
 */
PyObject* sqlite_STRING;
PyObject* sqlite_BINARY;
PyObject* sqlite_NUMBER;
PyObject* sqlite_DATETIME;
PyObject* sqlite_ROWID;
#endif
