/* module.c - the module itself
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

#include "module.h"
#include "connection.h"
#include "cursor.h"
#include "cache.h"
#include "prepare_protocol.h"
#include "microprotocols.h"
#include "microprotocols_proto.h"

static PyMethodDef module_methods[] = {
    {NULL, NULL}
};

PyMODINIT_FUNC init_sqlite(void)
{
    PyObject *module, *dict;

    module = Py_InitModule("pysqlite2._sqlite", module_methods);

    ConnectionType.tp_new = PyType_GenericNew;
    CursorType.tp_new = PyType_GenericNew;
    NodeType.tp_new = PyType_GenericNew;
    CacheType.tp_new = PyType_GenericNew;
    SQLitePrepareProtocolType.tp_new = PyType_GenericNew;
    SQLitePrepareProtocolType.ob_type    = &PyType_Type;
    if (PyType_Ready(&NodeType) < 0) {
        return;
    }
    if (PyType_Ready(&CacheType) < 0) {
        return;
    }
    if (PyType_Ready(&ConnectionType) < 0) {
        return;
    }
    if (PyType_Ready(&CursorType) < 0) {
        return;
    }
    if (PyType_Ready(&SQLitePrepareProtocolType) == -1) {
        return;
    }

    Py_INCREF(&ConnectionType);
    PyModule_AddObject(module, "connect", (PyObject*) &ConnectionType);
    Py_INCREF(&CursorType);
    PyModule_AddObject(module, "Cursor", (PyObject*) &CursorType);
    Py_INCREF(&CacheType);
    PyModule_AddObject(module, "Cache", (PyObject*) &CacheType);
    Py_INCREF(&SQLitePrepareProtocolType);
    PyModule_AddObject(module, "SQLitePrepareProtocol", (PyObject*) &SQLitePrepareProtocolType);

    if (!(dict = PyModule_GetDict(module)))
    {
        goto error;
    }

    /*** Create DB-API Exception hierarchy */

    Error = PyErr_NewException("sqlite.Error", PyExc_StandardError, NULL);
    PyDict_SetItemString(dict, "Error", Error);

    Warning = PyErr_NewException("sqlite.Warning", PyExc_StandardError, NULL);
    PyDict_SetItemString(dict, "Warning", Warning);

    /* Error subclasses */

    InterfaceError = PyErr_NewException("sqlite.InterfaceError", Error, NULL);
    PyDict_SetItemString(dict, "InterfaceError", InterfaceError);

    DatabaseError = PyErr_NewException("sqlite.DatabaseError", Error, NULL);
    PyDict_SetItemString(dict, "DatabaseError", DatabaseError);

    /* DatabaseError subclasses */

    InternalError = PyErr_NewException("sqlite.InternalError", DatabaseError, NULL);
    PyDict_SetItemString(dict, "InternalError", InternalError);

    OperationalError = PyErr_NewException("sqlite.OperationalError", DatabaseError, NULL);
    PyDict_SetItemString(dict, "OperationalError", OperationalError);

    ProgrammingError = PyErr_NewException("sqlite.ProgrammingError", DatabaseError, NULL);
    PyDict_SetItemString(dict, "ProgrammingError", ProgrammingError);

    IntegrityError = PyErr_NewException("sqlite.IntegrityError", DatabaseError,NULL);
    PyDict_SetItemString(dict, "IntegrityError", IntegrityError);

    DataError = PyErr_NewException("sqlite.DataError", DatabaseError, NULL);
    PyDict_SetItemString(dict, "DataError", DataError);

    NotSupportedError = PyErr_NewException("sqlite.NotSupportedError", DatabaseError, NULL);
    PyDict_SetItemString(dict, "NotSupportedError", NotSupportedError);

    sqlite_STRING = PyInt_FromLong(1L);
    sqlite_BINARY = PyInt_FromLong(2L);
    sqlite_NUMBER = PyInt_FromLong(3L);
    sqlite_DATETIME = PyInt_FromLong(4L);
    Py_INCREF(sqlite_NUMBER);
    sqlite_ROWID = sqlite_NUMBER;

    PyDict_SetItemString(dict, "STRING", sqlite_STRING);
    PyDict_SetItemString(dict, "BINARY", sqlite_BINARY);
    PyDict_SetItemString(dict, "NUMBER", sqlite_NUMBER);
    PyDict_SetItemString(dict, "DATETIME", sqlite_DATETIME);
    PyDict_SetItemString(dict, "ROWID", sqlite_ROWID);

    PyDict_SetItemString(dict, "version", PyString_FromString(PYSQLITE_VERSION));
    PyDict_SetItemString(dict, "sqlite_version", PyString_FromString(sqlite3_libversion()));

    /* initialize microprotocols layer */
    microprotocols_init(dict);
    /* psyco_adapters_init(dict); */

  error:

    if (PyErr_Occurred())
    {
        PyErr_SetString(PyExc_ImportError, "pysqlite2._sqlite: init failed");
    }
}

